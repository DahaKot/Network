#include "net.h"

//====================================================
//starter part										||
//====================================================
int call_workers() {
	errno = 0;
	int udp_sk = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_sk == -1) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}

	int val = 1;
	errno = 0;
	CHECK_ERROR(setsockopt(udp_sk, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val)));
	
	int val1 = 1;
	errno = 0;
	CHECK_ERROR(setsockopt(udp_sk, SOL_SOCKET, SO_REUSEADDR, &val1, sizeof(val1)));

	struct sockaddr_in broadcast_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(s_udp_port),
		.sin_addr = INADDR_BROADCAST
	};

	errno = 0;
	CHECK_ERROR(bind(udp_sk, &broadcast_addr, sizeof(broadcast_addr)));

	struct sockaddr_in worker_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(w_udp_port),
		.sin_addr = INADDR_BROADCAST
	};

	char buf = SIG;

	errno = 0;
	CHECK_ERROR(sendto(udp_sk, &buf, sizeof(buf), 0, &worker_addr, sizeof(worker_addr)));
	
	return 0;

error:
	close(udp_sk);
	return -1;
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
	CHECK_ERROR(setsockopt(tcp_sk, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)));

	errno = 0;
	CHECK_ERROR(keep_alive_enable(tcp_sk));

	struct sockaddr_in tcp_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(s_tcp_port),
		.sin_addr =  INADDR_ANY
	};

	errno = 0;
	CHECK_ERROR(bind(tcp_sk, &tcp_addr, sizeof(tcp_addr)));

	errno = 0;
	CHECK_ERROR(listen(tcp_sk, 256));

	return tcp_sk;

error:
	close(tcp_sk);
	return -1;
}

//====================================================
//worker part										||
//====================================================

int get_starter_addr(struct sockaddr_in *starter_addr) {
	int udp_sk = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_sk == -1) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}

	int val = 1;
	errno = 0;
	CHECK_ERROR(setsockopt(udp_sk, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val)));
	
	int val1 = 1;
	errno = 0;
	CHECK_ERROR(setsockopt(udp_sk, SOL_SOCKET, SO_REUSEADDR, &val1, sizeof(val1)));

	struct sockaddr_in udp_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(w_udp_port),
		.sin_addr = INADDR_BROADCAST
	};

	errno = 0;
	CHECK_ERROR(bind(udp_sk, &udp_addr, sizeof(udp_addr)));
	
	char buf = 0;

	// errno = 0;
	// if(fcntl(udp_sk, F_SETFL, O_NONBLOCK) < 0) {
	// 	printf("error on %d: %s\n", __LINE__, strerror(errno));
	// 	return -1;
	// }

	socklen_t starter_addr_size = sizeof(*starter_addr);

	printf("waiting for udp msg...\n");
	
	CHECK_ERROR(recvfrom(udp_sk, &buf, sizeof(SIG), 0, starter_addr, &starter_addr_size));

	if (buf != SIG) {
		printf("got wrong message\n");
		return -1;
	}
	
	return 0;

error:
	close(udp_sk);
	return -1;
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
	CHECK_ERROR(setsockopt(tcp_sk, SOL_SOCKET, SO_REUSEADDR, &val1, sizeof(val1)));

	errno = 0;
	CHECK_ERROR(fcntl(tcp_sk, F_SETFL, O_NONBLOCK));

	errno = 0;
	CHECK_ERROR(keep_alive_enable(tcp_sk));

	fd_set set;
	FD_ZERO(&set);
	FD_SET(tcp_sk, &set);

	struct timeval tv = {
		.tv_sec     = 0,
		.tv_usec    = INET_TIMEOUT
	};

	errno = 0;
	int connect_ret = connect(tcp_sk, starter_addr, sizeof(*starter_addr));
	if (connect_ret < 0 && errno == EINPROGRESS) {
		errno = 0;
		if(select(tcp_sk + 1, NULL, &set, NULL, &tv) < 0) {
			if(errno != 0) {
				printf("error on %d: %s\n", __LINE__, strerror(errno));
			} else {
				printf("select time out\n");
			}
			
			close(tcp_sk);
			return -1;
		}

		int errval = 0;
		socklen_t len = sizeof(errval);
		CHECK_ERROR(getsockopt(tcp_sk, SOL_SOCKET, SO_ERROR, &errval, &len));

		if(errval != 0) {
			printf("error on %d: %s\n", __LINE__, strerror(errno));
			
			close(tcp_sk);
			return -1;
		}
	}


	CHECK_ERROR(fcntl(tcp_sk, F_SETFL, 0));

	CHECK_ERROR(set_own(tcp_sk));

	return tcp_sk;

error:
	close(tcp_sk);
	return -1;
}

//====================================================
//common part										||
//====================================================

int keep_alive_enable(int tcp_sk)
{
	int val_ka = KA_VAL;
	CHECK_ERROR(setsockopt(tcp_sk, SOL_SOCKET, SO_KEEPALIVE, &val_ka, sizeof(val_ka)));

	int val_idle = KA_IDLE;
	CHECK_ERROR(setsockopt(tcp_sk, IPPROTO_TCP, TCP_KEEPIDLE, &val_idle, sizeof(val_idle)));

	int val_intvl = KA_INTVL;
	CHECK_ERROR(setsockopt(tcp_sk, IPPROTO_TCP, TCP_KEEPINTVL, &val_intvl, sizeof(val_intvl)));

	int val_cnt = KA_CNT;
	CHECK_ERROR(setsockopt(tcp_sk, IPPROTO_TCP, TCP_KEEPCNT, &val_cnt, sizeof(val_cnt)));

	return 0;

error:
	return -1;
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

int tcp_write(int fd, void *buf, size_t size) {
	errno = 0;
	int ret = write(fd, buf, size);
	if (ret <= 0) {
		printf("write failed errno: %s\n", strerror(errno));
		return -1;
	}

	if (ret != size) {
		printf("should write more but cant");
		return -1;
	}

	return ret;
}

int tcp_read(int fd, void *buf, size_t size) {
	errno = 0;
	int ret = read(fd, buf, size);
	if (ret <= 0) {
		printf("read failed errno: %s\n", strerror(errno));
		return -1;
	}

	if (ret != size) {
		printf("should read more but cant");
		return -1;
	}

	return ret;
}

int init_sig_actions() {
	struct sigaction ignore_action = {};
	ignore_action.sa_handler = SIG_IGN;

	sigaction (SIGPIPE, &ignore_action, NULL);
	sigaction (SIGURG, &ignore_action, NULL);

	struct sigaction exit_action = {};
	exit_action.sa_handler = sigIO_handler;

	sigaction (SIGIO, &exit_action, 0);

	return 0;
}

void sigIO_handler(int signal) {
	printf("sigIO caught\n");
	exit(1);
}