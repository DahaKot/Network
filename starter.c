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
#include "calculate.h"
#include "net.h"

struct worker {
	double start;
	double end;
	int n_usefull_threads;
	int fd;
};

const double low = 0;
const double high = 100;

int spread_tasks(struct worker *workers, int n_workers, int total_threads);

int main(int argc, char **argv) {
	if (argc != 2) {
		printf("Ayayay!\n");
		return -1;
	}

	init_sig_actions();

	int n_workers = strtol(argv[1], NULL, 10);

	printf("%d %d %d %d\n", s_udp_port, w_udp_port, s_tcp_port, w_tcp_port);

	/*
	0. get and set tcp socket #
	1. send to workers anything UDP #
	2. get from workers their sks and number of cores TCP #
	3. spread work on workers: #
		3.1. send to certain worker its number #
	4. get results #
	*/	

	int tcp_sk = create_tcp_socket();
	if (tcp_sk < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}
	printf("create_tcp_socket returned %d\n", tcp_sk);

	if (call_workers() < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}

	struct worker *workers = calloc(n_workers, sizeof(struct worker));

	int total_threads = 0;

	for (int i = 0; i < n_workers; i++) {
		fd_set set;
		FD_ZERO(&set);
		FD_SET(tcp_sk, &set);

		int ret = -1;
		struct timeval tv = {
			.tv_usec    = INET_TIMEOUT
		};
		ret = select(tcp_sk + 1, &set, NULL, NULL, &tv);
		if (ret < 0) {
			printf("select gave an error\n");
			return -1;
		}
		if (ret == 0) {
			printf("selec time out\n");
			return -1;
		}
		
		struct sockaddr_in workers_addr;
		socklen_t workers_addr_size = sizeof(workers_addr);

		int tcp_fd = accept(tcp_sk, &workers_addr, &workers_addr_size);
		if (tcp_fd == -1) {
			printf("error on %d: %s\n", __LINE__, strerror(errno));
			return -1;
		}

		if(keep_alive_enable(tcp_fd) < 0) {
			printf("error on %d: %s\n", __LINE__, strerror(errno));

			close(tcp_fd);
			return -1;
		}
		if(set_own(tcp_fd) < 0) {
			printf("error on %d: %s\n", __LINE__, strerror(errno));
				
			close(tcp_fd);
			return -1;
		}

		workers[i].fd = tcp_fd;

		if (tcp_read(tcp_fd, &(workers[i].n_usefull_threads), 
					sizeof(workers[i].n_usefull_threads)) < 0) {
			printf("error on %d: %s\n", __LINE__, strerror(errno));
			return -1;
		}
		printf("%dth has %d cores(or threads)\n", i, workers[i].n_usefull_threads);

		total_threads += workers[i].n_usefull_threads;
	}

	if (spread_tasks(workers, n_workers, total_threads) < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}

	for (int i = 0; i < n_workers; i++) {
		struct task this_task;
		this_task.start = workers[i].start;
		this_task.end = workers[i].end;

		if (tcp_write(workers[i].fd, &this_task, sizeof(struct task)) < 0) {
			printf("error on %d: %s\n", __LINE__, strerror(errno));
			return -1;
		}
	}

	double result = 0;
	double w_result = 0;
	for (int i = 0; i < n_workers; i++) {
		w_result = 0;
		if (tcp_read(workers[i].fd, &w_result, sizeof(double)) < 0) {
			printf("error on %d: %s\n", __LINE__, strerror(errno));
			return -1;
		}

		result += w_result;
	}

	for (int i = 0; i < n_workers; i++) {
		close(workers[i].fd);
	}

	printf("got result %lg\n", result);

	return 0;
}

int spread_tasks(struct worker *workers, int n_workers, int total_threads) {
	double step = (high - low) / total_threads;

	int i = 0;
	workers[i].start = low;
	workers[i].end = low + workers[i].n_usefull_threads*step;

	for (int i = 1; i < n_workers; i++) {
		workers[i].start = workers[i-1].end;
		workers[i].end = workers[i].start + workers[i].n_usefull_threads*step;
	}
}