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

	struct sockaddr_in starter_addr;
	
	errno = 0;
	if (get_starter_addr(&starter_addr) < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}
	starter_addr.sin_port = htons(s_tcp_port);

	int tcp_fd = tcp_connect(&starter_addr);
	if (tcp_fd < 0) {
		printf("error on %d: %s\n", __LINE__, strerror(errno));
		return -1;
	}

	int n_proc = get_nprocs();
	int *real_procs = (int *) calloc(n_proc, sizeof(int));
	int n_real_procs = get_real_procs(n_proc, real_procs);

	int n_usefull_threads = min(n_real_procs, n_threads);

	CHECK_ERROR(tcp_write(tcp_fd, &n_usefull_threads, sizeof(int)));

	printf("real procs: %d\n", n_real_procs);

	struct task this_task;

	CHECK_ERROR(tcp_read(tcp_fd, &this_task, sizeof(struct task)));
	
	printf("this worker has task from: %lg to: %lg\n", this_task.start, this_task.end);

	errno = 0;
	CHECK_ERROR(fcntl(tcp_fd, F_SETFL, O_ASYNC));
 
	double result = calculate(n_threads, n_real_procs, real_procs, 
					this_task.start, this_task.end);

	errno = 0;
	CHECK_ERROR(fcntl(tcp_fd, F_SETFL, 0));

	printf("got result: %lg\n", result);

	CHECK_ERROR(tcp_write(tcp_fd, &result, sizeof(double)));

	close(tcp_fd);
	return 0;

error:
	close(tcp_fd);
	return -1;
}