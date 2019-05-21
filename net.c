#include "net.h"

//starter part
int call_workers() {
	errno = 0;
	int udp_sk = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_sk == -1) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}

	int val = 1;
	errno = 0;
	if (setsockopt(udp_sk, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val)) < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}
	
	int val1 = 1;
	errno = 0;
	if (setsockopt(udp_sk, SOL_SOCKET, SO_REUSEADDR, &val1, sizeof(val1)) < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}

	struct sockaddr_in broadcast_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(s_udp_port),
		.sin_addr = INADDR_BROADCAST
	};

	errno = 0;
	if (bind(udp_sk, &broadcast_addr, sizeof(broadcast_addr)) < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}

	struct sockaddr_in worker_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(w_udp_port),
		.sin_addr = INADDR_BROADCAST
	};

	char buf = SIG;

	errno = 0;
	if (sendto(udp_sk, &buf, sizeof(buf), 0, &worker_addr, sizeof(worker_addr)) < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}

	return 0;
}

int create_tcp_socket() {
	errno = 0;
	int tcp_sk = socket(AF_INET, SOCK_STREAM, 0);
	if (tcp_sk == -1) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}

	int val = 1;
	errno = 0;
	if (setsockopt(tcp_sk, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}

	struct sockaddr_in tcp_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(s_tcp_port),
		.sin_addr =  INADDR_ANY
	};

	errno = 0;
	if (bind(tcp_sk, &tcp_addr, sizeof(tcp_addr)) < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}

	errno = 0;
	if (listen(tcp_sk, 256) < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}

	return tcp_sk;
}

//================================================================
//worker part													||
//================================================================

int get_starter_addr(struct sockaddr_in *starter_addr) {
	int udp_sk = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_sk == -1) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}

	int val = 1;
	errno = 0;
	if (setsockopt(udp_sk, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val)) < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}
	
	int val1 = 1;
	errno = 0;
	if (setsockopt(udp_sk, SOL_SOCKET, SO_REUSEADDR, &val1, sizeof(val1)) < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}

	struct sockaddr_in udp_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(w_udp_port),
		.sin_addr = INADDR_BROADCAST
	};

	errno = 0;
	if (bind(udp_sk, &udp_addr, sizeof(udp_addr)) < 0) {//
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}
	
	char buf = 0;

	socklen_t starter_addr_size = sizeof(*starter_addr);
	printf("before recvfrom\n");
	errno = 0;
	if (recvfrom(udp_sk, &buf, sizeof(SIG), 0, 
					starter_addr, &starter_addr_size) < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));//
		return -1;
	}

	if (buf != SIG) {
		printf("got wrong message\n");
		return -1;
	}
	
	return 0;
}

int tcp_connect(struct sockaddr_in *starter_addr) {
	errno = 0;
	int tcp_sk = socket(AF_INET, SOCK_STREAM, 0);
	if (tcp_sk == -1) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));		
		return -1;
	}
	
	int val1 = 1;
	errno = 0;
	if (setsockopt(tcp_sk, SOL_SOCKET, SO_REUSEADDR, &val1, sizeof(val1)) < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}

	errno = 0;
	if(fcntl(tcp_sk, F_SETFL, O_NONBLOCK) < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}

	errno = 0;
	if(keep_alive_enable(tcp_sk) < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}

	fd_set set;
	FD_ZERO(&set);
	FD_SET(tcp_sk, &set);

	struct timeval tv = {
		.tv_sec     = 0,
		.tv_usec    = INET_TIMEOUT
	};

	errno = 0;
	int connect_ret = connect(tcp_sk, starter_addr, sizeof(*starter_addr));
	if (connect_ret < 0 && errno == EINPROGRESS) {//
		errno = 0;
		if(select(tcp_sk + 1, NULL, &set, NULL, &tv) < 0) {
			if(errno != 0) {
				printf("error on %d: %s\n", __LINE__, strerror(errno));
			} else {
				printf("error on %d: %s\n", __LINE__, strerror(errno));
			}
			
			close(tcp_sk);
			return -1;
		}

		int errval = 0;
		socklen_t len = sizeof(errval);
		if(getsockopt(tcp_sk, SOL_SOCKET, SO_ERROR, &errval, &len) < 0) {
			printf("error on %d: %s\n", __LINE__, strerror(errno));

			close(tcp_sk);
			return -1;
		}

		if(errval != 0) {
			printf("error on %d: %s\n", __LINE__, strerror(errno));
			
			close(tcp_sk);
			return -1;
		}
	}

	if(fcntl(tcp_sk, F_SETFL, 0) < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
			
		close(tcp_sk);
		return -1;
	}
	if(set_own(tcp_sk) < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
			
		close(tcp_sk);
		return -1;
	}

	return tcp_sk;
}

int keep_alive_enable(int tcp_sk)
{
	int val_ka = KA_VAL;
	if(0 > setsockopt(tcp_sk, SOL_SOCKET, SO_KEEPALIVE, &val_ka, sizeof(val_ka))) {
		printf("setsockopt SO_KEEPALIVE failed\n");
		return -1;
	}

	int val_idle = KA_IDLE;
	if(0 > setsockopt(tcp_sk, IPPROTO_TCP, TCP_KEEPIDLE, &val_idle, sizeof(val_idle))){
		printf("setsockopt TCP_KEEPIDLE failed\n");
		return -1;
	}

	int val_intvl = KA_INTVL;
	if(0 > setsockopt(tcp_sk, IPPROTO_TCP, TCP_KEEPINTVL, &val_intvl, sizeof(val_intvl))){
		printf("setsockopt TCP_KEEPINTVL failed\n");
		return -1;
	}

	int val_cnt = KA_CNT;
		if(0 > setsockopt(tcp_sk, IPPROTO_TCP, TCP_KEEPCNT, &val_cnt, sizeof(val_cnt))){
		printf("setsockopt TCP_KEEPCNT failed\n");
		return -1;
	}

	return 0;
}

int set_own(int sk)
{
	struct f_owner_ex self = {
		.type = F_OWNER_PID,
		.pid  = gettid()
	};
	if(fcntl(sk, F_SETOWN_EX, &self) < 0) {
		printf("fcntl setown failed\n");
		return -1;
	}
	return 0;
}

//common part
int tcp_write(int fd, void *buf, size_t size) {
	int ret = write(fd, buf, size);
	if (ret <= 0) {
		printf("write failed\n");
		return -1;
	}

	if (ret != size) {
		printf("should write more but cant");
		return -1;
	}

	return ret;
}

int tcp_read(int fd, void *buf, size_t size) {
	int ret = read(fd, buf, size);
	if (ret <= 0) {
		printf("read failed\n");
		return -1;
	}

	if (ret != size) {
		printf("should read more but cant");
		return -1;
	}

	return ret;
}