#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <unistd.h>

const char SIG = 246;
const size_t BUF_SIZE = 1024;
const size_t s_udp_port = 4001;
const size_t w_udp_port = 4002;
const size_t s_tcp_port = 4242;
const size_t w_tcp_port = 4243;

int call_workers();

int main(int argc, char **argv) {
	if (argc != 2) {
		printf("Ayayay!\n");
		return -1;
	}

	int n_workers = strtol(argv[1], NULL, 10);

	/*
	0. get and set tcp socket
	1. send to workers anything UDP #
	2. get from workers their addrs TCP
	3. spread work on workers:
		3.1. send to certain worker its number
	4. get results
	*/	

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
	//now tcp_sk is in listening state

	printf("call_workers %d\n", call_workers());

	fd_set set;
    FD_ZERO(&set);
    FD_SET(tcp_sk, &set);

    int ret = -1;
    printf("before select\n");
    ret = select(tcp_sk + 1, &set, NULL, NULL, NULL);
    printf("after select\n");
    fflush(stdout);

	struct sockaddr_in workers_addr;
	socklen_t workers_addr_size = sizeof(workers_addr);

	int tcp_fd = accept(tcp_sk, &workers_addr, &workers_addr_size);
	if (tcp_fd == -1) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}

	char buf = 1;

	write(tcp_fd, &buf, 1);

	// errno = 0;
	// err = recvfrom(sk, &worker_addr, sizeof(worker_addr), 0, 
	// 				&_addr, &host_addr_size);
	// printf("recvfrom returned %d %s\n", err, strerror(errno));

	return 0;
}

int call_workers() {
	//create udp_socket
	errno = 0;
	int udp_sk = socket(AF_INET, SOCK_DGRAM, 0);
	if (udp_sk == -1) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}
	
	//set udp_socket to broadcast regime
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