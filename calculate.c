#include "calculate.h"

double calculate (int n, int n_real_procs, int *real_procs, double start, double end) {
	int max_n = max(n, n_real_procs);
	int min_n = min(n, n_real_procs);

	pthread_t *threads = (pthread_t *) calloc(n, sizeof(pthread_t));
	struct thread_param *thread_parameters = (struct thread_param *) calloc(n, sizeof(struct thread_param));
	
	//distribute mini-sections between threads
	init(thread_parameters, min_n, start, end);
	
	pthread_attr_t *thread_attributes = (pthread_attr_t *) calloc(n, sizeof(pthread_attr_t));
	cpu_set_t *cpusets = calloc(n, sizeof(cpu_set_t));

	void *(*routine) (void *) = useful_routine;

	// if number of threads is less than number of procs
	// 		lets run other procs with useless_routine = while(1) to fight turbobust
	// else
	// 		lets run other threads with empty_routine = return not to 
	// 		waste calculation resources

	int i = 0;
	int cur_proc = 0;
	for (; i < max_n; i++, cur_proc++) {
		if (i == min_n) {
			routine = useless_routine;
		}
		if (i >= n_real_procs) {
			routine = empty_routine;
		}
		pthread_attr_init(thread_attributes + i);

		CPU_ZERO(cpusets + i);
		CPU_SET(real_procs[i % n_real_procs], cpusets + i);
		pthread_attr_setaffinity_np(thread_attributes + i, sizeof(cpu_set_t), cpusets + i);

		pthread_create(threads + i, thread_attributes + i, routine, thread_parameters + i);
	}

	i = 0;
	for (; i < n; i++) {
		pthread_join(threads[i], NULL);
	}

	double sum = 0;
	i = 0;
	for (; i < n; i++) {
		sum += thread_parameters[i].res;
	}

	return sum;
}

void *useful_routine(void *arg) {
	struct thread_param *these_param = (struct thread_param *) arg;

	double a = these_param->start;
	double b = these_param->end;
	double dx = these_param->dx;

	double res = these_param->res;

	double x = a;
	for (; x < b; x += dx) {
		res += x * dx;
	}

	these_param->res = res;

	return NULL;
}

void *useless_routine(void *arg) {
	while (1);
	pthread_exit (0);
}

void *empty_routine(void *arg) {
	return NULL;
}

void init(struct thread_param *arr, int n, double start, double end) {
	double fraq = (end - start) / (double) n;

	arr[0].start = start;
	arr[0].end = start + fraq;
	arr[0].dx = DX;

	int i = 1;
	for (; i < n; i++) {
		arr[i].start = arr[i-1].end;
		arr[i].end = arr[i].start + fraq;
		arr[i].dx = DX;
	}
}

int get_real_procs(int n_proc, int *real_procs) {
	char path[] = "/sys/bus/cpu/devices/cpu0/topology/core_id";

	//parse cpuinfo searching for unique cores and their numbers
	int n_real_procs = 0;
	int i = 0;
	for (; i < n_proc; i++) {
		FILE *info = fopen(path, "r");

		int core_id = -1;
		fscanf(info, "%d", &core_id);

		if (!in_array(core_id, real_procs, n_real_procs)) {
			real_procs[n_real_procs] = core_id;
			n_real_procs++;
		}

		fclose(info);
		path[24]++;
	}

	return n_real_procs;
}

int in_array(int x, int *arr, int len) {
	int i = 0;
	for (; i < len; i++) {
		if (x == arr[i]) {
			return 1;
		}
	}

	return 0;
}
