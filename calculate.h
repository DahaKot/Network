#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>

#define min(a, b) a < b ? a : b
#define max(a, b) a > b ? a : b

static const double DX = 5*1e-9;

struct thread_param {
	double start;
	double end;
	double dx;
	double res;
};

struct task {
	double start;
	double end;
};

int spread_jobs (int n, int n_real_procs, int *real_procs, double start, double end);
int get_real_procs(int n_proc, int *real_procs);
int in_array(int x, int *arr, int len);
void *useful_routine(void *arg);
void *useless_routine(void *arg);
void *empty_routine(void *arg);
void init(struct thread_param *arr, int n, double start, double end);