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

#include <ktypes.h>

#include <karg.h>
#include <klog.h>
#include <kmem.h>
#include <kstr.h>

#include <helper.h>

#include <xtcool.h>

#include <opt-rpc-common.h>
#include <opt-rpc-server.h>

#define BACKLOG 50
#define __g_epoll_max 50

static void config_socket(int s);
static void ignore_pipe();

static int __g_epoll_fd = -1;
static struct epoll_event __g_epoll_events[__g_epoll_max];

static FILE *__g_fp_out = NULL;

/*-----------------------------------------------------------------------
 * Server
 */
static int process_klog_data(int s, char *buf, int len)
{
	fwrite(buf, sizeof(char), len, __g_fp_out);
	fflush(__g_fp_out);
	return 0;
}

static void close_connect(int s)
{
	close(s);
}

static void *worker_thread_or_server(void *userdata)
{
	int ready, i, n, bufsize = 128 * 1024;
	void *buf;
	struct epoll_event ev, *e;

	unsigned short port = (unsigned short)(int)userdata;

	int s_listen, new_fd;
	struct sockaddr_in their_addr;
	struct sockaddr_in my_addr;
	socklen_t sin_size;

	ignore_pipe();

	if ((s_listen = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		kerror("c:%s, e:%s\n", "socket", strerror(errno));
		return NULL;
	}

	config_socket(s_listen);

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(my_addr.sin_zero, '\0', sizeof(my_addr.sin_zero));
	if (bind(s_listen, (struct sockaddr *) &my_addr, sizeof(my_addr)) == -1) {
		kerror("c:%s, e:%s\n", "bind", strerror(errno));
		return NULL;
	}

	if (listen(s_listen, BACKLOG) == -1) {
		kerror("c:%s, e:%s\n", "listen", strerror(errno));
		return NULL;
	}

	__g_epoll_fd = epoll_create(__g_epoll_max);
	memset(&ev, 0, sizeof(ev));
	ev.data.fd = s_listen;
	ev.events = EPOLLIN;
	epoll_ctl(__g_epoll_fd, EPOLL_CTL_ADD, s_listen, &ev);

	buf = kmem_alloc(bufsize, char);
	for (;;) {
		do
			ready = epoll_wait(__g_epoll_fd, __g_epoll_events, __g_epoll_max, -1);
		while ((ready == -1) && (errno == EINTR));

		for (i = 0; i < ready; i++) {
			e = __g_epoll_events + i;

			if (e->data.fd == s_listen) {
				sin_size = sizeof(their_addr);
				if ((new_fd = accept(s_listen, (struct sockaddr *) &their_addr, &sin_size)) == -1) {
					kerror("c:%s, e:%s\n", "accept", strerror(errno));
					continue;
				}

				klog("new_fd: %d\n", new_fd);

				struct epoll_event ev;
				ev.data.fd = new_fd;
				ev.events = EPOLLIN;
				epoll_ctl(__g_epoll_fd, EPOLL_CTL_ADD, new_fd, &ev);

				continue;
			} else if (!(e->events & EPOLLIN)) {
				kerror("!!! Not EPOLLIN: event is %08x, fd:%d\n", e->events, e->data.fd);
				continue;
			}

			if ((n = recv(e->data.fd, buf, bufsize, 0)) > 0) {
				if (process_klog_data(e->data.fd, buf, n))
					close_connect(e->data.fd);
			} else {
				klog("Remote close socket: %d\n", e->data.fd);
				close_connect(e->data.fd);
			}
		}
	}
	kmem_free(buf);

	close(__g_epoll_fd);
	__g_epoll_fd = -1;

	return NULL;
}

int opt_rpc_server_init(unsigned short port, int argc, char *argv[])
{
	int i;

	if (port == 0)
		port = 9000;

	i = arg_find(argc, argv, "--or-port", 1);
	if (i > 0 && (i + 1) < argc) {
		int tmp;
		if (!kstr_toint((const kchar*)argv[i + 1], &tmp))
			port = tmp;
	}

	klog("port: %d\n", port);

	ignore_pipe();
	spl_thread_create(worker_thread_or_server, (void*)(int)port, 0);
	return 0;
}

int opt_rpc_server_final()
{
	return 0;
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

static int logger_wlogf(const char *content, int len)
{
	return printf(content);
}

int main(int argc, char *argv[])
{
	kushort port;

	klog_init(-1, argc, argv);
	klog_add_logger(logger_wlogf);

	klogs("usage: klogserv PORT TOFILE\n");

	port = (kushort)atoi(argv[1]);
	__g_fp_out = fopen(argv[2], "wt+");

	worker_thread_or_server((void*)(int)port);

	return 0;
}
