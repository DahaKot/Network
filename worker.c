#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <setjmp.h>
#include <sys/select.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#include <signal.h>

#define KA_VAL          1
#define KA_IDLE         1
#define KA_INTVL        1
#define KA_CNT          1

const char SIG = 246;
const int INET_TIMEOUT = 1000;
const size_t BUF_SIZE = 1024;
const size_t s_udp_port = 4001;
const size_t w_udp_port = 4002;
const size_t s_tcp_port = 4242;
const size_t w_tcp_port = 4243;

int get_starter_addr(struct sockaddr_in *starter_addr);
int tcp_connect(struct sockaddr_in *starter_addr);
int tcpkaenable(int sk);
int tcpsetown(int sk);

int main(int argc, char **argv) {
	if (argc != 2) {
		printf("Ayayay!\n");
		return -1;
	}

	int n = strtol(argv[1], NULL, 10);

	/*
	1. get from starter anything UDP (and its address) #
	2. Send to the starter own address
	3. get number in a chain of workers from starter
	4. calculate own part
	5. send results to starter
	*/

	struct sockaddr_in starter_addr;
	
	printf("get starter addr %d\n", get_starter_addr(&starter_addr));
	starter_addr.sin_port = s_tcp_port;

	printf("tcp connect %d\n", tcp_connect(&starter_addr));
	
	// struct sockaddr_in tcp_addr = {
	// 	.sin_family = AF_INET,
	// 	.sin_port = htons(w_tcp_port),
	// 	.sin_addr = INADDR_BROADCAST
	// };
	// socklen_t starter_addr_size = sizeof(starter_addr);
	
	// E;
	// if (listen(tcp_sk, 256) < 0) {
	// 	printf("error on %d: %s\n", __LINE__, strerror(errno));
	// 	return -1;
	// }

	// E;
	// if (bind(tcp_sk, &tcp_addr, sizeof(tcp_addr)) < 0) {
	// 	printf("error on %d: %s\n", __LINE__, strerror(errno));
	// 	return -1;
	// }

	// int tcp_fd = accept(tcp_sk, &starter_addr, &starter_addr_size);
	// if (tcp_fd == -1) {
	// 	printf("error on %d: %s\n", __LINE__, strerror(errno));
	// 	return -1;
	// }

	// char buf = 0;
	// read(tcp_fd, &buf, 1);
	// printf("%d\n", buf);

	// errno = 0;
	// sendto(sk, &addr, sizeof(addr), 0, 
	// 		&starter_addr, sizeof(starter_addr));
	// printf("sendto() returned %s\n", strerror(errno));
	
	return 0;
}

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
	if (bind(udp_sk, &udp_addr, sizeof(udp_addr)) < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}
	
	char buf = 0;

	socklen_t starter_addr_size = sizeof(*starter_addr);
	printf("before recvfrom\n");
	errno = 0;
	if (recvfrom(udp_sk, &buf, sizeof(SIG), 0, 
					starter_addr, &starter_addr_size) < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
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
		printf("connect returned %d %s\n", connect_ret, strerror(errno));

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
    if(tcpsetown(tcp_sk) < 0) {
        printf("error on %d: %s\n", __LINE__, strerror(errno));
            
		close(tcp_sk);
       	return -1;
    }

    return tcp_sk;
}

int tcpkaenable(int tcp_sk)
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

int tcpsetown(int sk)
{
    struct f_owner_ex self = {
        .type = F_OWNER_PID,
        .pid  = gettid()
    };
    if(0 > fcntl(sk, F_SETOWN_EX, &self)) {
        printf("fcntl setown failed\n");
        return -1;
    }
    return 0;
}