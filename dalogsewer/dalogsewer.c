/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#include <stdarg.h>
#include <assert.h>

#define BACK_LOG 50
#define __epoll_max 50

static void config_socket(int s);
static void ignore_pipe();

static int __epoll_fd = -1;
static struct epoll_event __epoll_events[__epoll_max];

static FILE *__fp_out = NULL;

/*-----------------------------------------------------------------------
 * Server
 */
static int process_dalog_data(int s, char *buf, int len)
{
	if (len != fwrite(buf, sizeof(char), len, __fp_out))
		printf("fwrite error: %d\n", errno);
	return 0;
}

static void close_connect(int s)
{
	close(s);
}

static void *worker_thread_or_server(unsigned short port)
{
	int ready, i, n, bufsize = 64 * 1024;
	void *buf;
	struct epoll_event ev, *e;

	int s_listen, new_fd;
	struct sockaddr_in their_addr;
	struct sockaddr_in my_addr;
	socklen_t sin_size;

	ignore_pipe();

	if ((s_listen = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("c:%s, e:%s\n", "socket", strerror(errno));
		return NULL;
	}

	config_socket(s_listen);

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(my_addr.sin_zero, '\0', sizeof(my_addr.sin_zero));
	if (bind(s_listen, (struct sockaddr *) &my_addr, sizeof(my_addr)) == -1) {
		printf("c:%s, e:%s\n", "bind", strerror(errno));
		return NULL;
	}

	if (listen(s_listen, BACK_LOG) == -1) {
		printf("c:%s, e:%s\n", "listen", strerror(errno));
		return NULL;
	}

	__epoll_fd = epoll_create(__epoll_max);
	memset(&ev, 0, sizeof(ev));
	ev.data.fd = s_listen;
	ev.events = EPOLLIN;
	epoll_ctl(__epoll_fd, EPOLL_CTL_ADD, s_listen, &ev);

	buf = malloc(bufsize);
	for (;;) {
		do
			ready = epoll_wait(__epoll_fd, __epoll_events, __epoll_max, -1);
		while ((ready == -1) && (errno == EINTR));

		for (i = 0; i < ready; i++) {
			e = __epoll_events + i;

			if (e->data.fd == s_listen) {
				sin_size = sizeof(their_addr);
				if ((new_fd = accept(s_listen, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
					printf("c:%s, e:%s\n", "accept", strerror(errno));
					continue;
				}

				unsigned char addr[4];
				memcpy(addr, &their_addr.sin_addr.s_addr, 4);
				printf("New connect, addr:%d.%d.%d.%d, port:%d, fd:%d\n",
						addr[0], addr[1], addr[2], addr[3],
						ntohs(their_addr.sin_port),
						new_fd);

				struct epoll_event ev;
				memset(&ev, 0, sizeof(ev));
				ev.data.fd = new_fd;
				ev.events = EPOLLIN;
				epoll_ctl(__epoll_fd, EPOLL_CTL_ADD, new_fd, &ev);

				continue;
			} else if (!(e->events & EPOLLIN)) {
				printf("!!! Not EPOLLIN: event is %08x, fd:%d\n", e->events, e->data.fd);
				continue;
			}

			if ((n = recv(e->data.fd, buf, bufsize, 0)) > 0) {
				if (process_dalog_data(e->data.fd, buf, n))
					close_connect(e->data.fd);
			} else {
				printf("Remote close socket: %d\n", e->data.fd);
				close_connect(e->data.fd);

				/* XXX: Should not put it here */
				fflush(__fp_out);
			}
		}
	}
	free(buf);

	close(__epoll_fd);
	__epoll_fd = -1;

	return NULL;
}

static void config_socket(int s)
{
	int yes = 1;
	struct linger lin;

	lin.l_onoff = 0;
	lin.l_linger = 0;
	setsockopt(s, SOL_SOCKET, SO_LINGER, (const char *) &lin, sizeof(lin));
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
}

static void ignore_pipe()
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, 0);
}

static void help(int die)
{
	printf("usage: dalogsewer [PORT] [TOFILE]\n");
	printf("       environ: LOGSEW_PORT LOGSEW_FILE\n");

	if (die)
		exit(0);
}

int main(int argc, char *argv[])
{
	unsigned short port;
	char *env;

	if (argc < 3) {
		env = getenv("LOGSEW_PORT");
		if (!env)
			help(1);
		port = (unsigned short)atoi(env);

		env = getenv("LOGSEW_FILE");
		if (!env)
			help(1);
		__fp_out = fopen(env, "wt+");
	} else {
		port = (unsigned short)atoi(argv[1]);
		__fp_out = fopen(argv[2], "wt+");
	}

	if (!__fp_out) {
		printf("open '%s' failed, error: '%s'\n", argv[2], strerror(errno));
		exit(0);
	}

	worker_thread_or_server(port);

	return 0;
}

