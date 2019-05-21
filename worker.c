#include <signal.h>
#include "calculate.h"
#include "net.h"

int main(int argc, char **argv) {
	if (argc != 2) {
		printf("Ayayay!\n");
		return -1;
	}

	init_sig_actions();

	int n_threads = strtol(argv[1], NULL, 10);

	/*
	1. get from starter anything UDP (and its address) #
	2. Send to the starter sk and number of cores #
	3. get number in a chain of workers from starter #
	4. calculate own part #
	5. send results to starter #
	*/

	printf("%d %d %d %d\n", s_udp_port, w_udp_port, s_tcp_port, w_tcp_port);

	struct sockaddr_in starter_addr;
	
	printf("get starter addr %d\n", get_starter_addr(&starter_addr));
	starter_addr.sin_port = htons(s_tcp_port);

	int tcp_fd = tcp_connect(&starter_addr);
	printf("tcp connect %d\n", tcp_fd);

	int n_proc = get_nprocs();
	int *real_procs = (int *) calloc(n_proc, sizeof(int));
	int n_real_procs = get_real_procs(n_proc, real_procs);

	int n_usefull_threads = min(n_real_procs, n_threads);

	if (tcp_write(tcp_fd, &n_usefull_threads, sizeof(int)) < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}
	printf("real procs: %d\n", n_real_procs);

	struct task this_task;

	if (tcp_read(tcp_fd, &this_task, sizeof(struct task)) < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}
	
	printf("this worker has task from %lg to %lg\n", this_task.start, this_task.end);

	errno = 0;
	if (fcntl(tcp_fd, F_SETFL, O_ASYNC) < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}
 
	double result = calculate(n_threads, n_real_procs, real_procs, 
					this_task.start, this_task.end);

	errno = 0;
	if (fcntl(tcp_fd, F_SETFL, 0) < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}

	printf("got result %lg\n", result);

	if (tcp_write(tcp_fd, &result, sizeof(double)) < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}

	return 0;
}