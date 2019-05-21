#ifndef NET
#define NET

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

#define KA_VAL		1
#define KA_IDLE		1
#define KA_INTVL	1
#define KA_CNT		1

static const char SIG = 246;
static const int INET_TIMEOUT = 1000;
static const size_t BUF_SIZE = 1024;
static const size_t s_udp_port = 4001;
static const size_t w_udp_port = 4002;
static const size_t s_tcp_port = 4242;
static const size_t w_tcp_port = 4243;

int call_workers();
int create_tcp_socket();

int get_starter_addr(struct sockaddr_in *starter_addr);
int tcp_connect(struct sockaddr_in *starter_addr);

int keep_alive_enable(int sk);
int set_own(int sk);
int tcp_write(int fd, void *buf, size_t size);
int tcp_read(int fd, void *buf, size_t size);

#endif