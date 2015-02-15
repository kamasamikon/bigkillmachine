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
#define EPOLL_MAX 50

static void config_socket(int s);
static void ignore_pipe();

static int __save_log = 0;

static int _output(const char *fmt, ...)
{
	va_list arg;
	int done;
	char buf[2048], cmd[2048];
	int ret;

	va_start(arg, fmt);
	done = vsnprintf(buf, sizeof(buf), fmt, arg);
	va_end(arg);

	printf("<%s> %s", "LOGSEW", buf);

	if (__save_log) {
		sprintf(cmd, "echo -n '<%s> %s' >> '%s'", "LOGSEW", buf, "/tmp/dalogsewer.log");
		ret = system(cmd);
	}

	return done;
}

/*-----------------------------------------------------------------------
 * Server
 */
static int process_dalog_data(int s, char *buf, int len, FILE *fp)
{
	if (len != fwrite(buf, sizeof(char), len, fp))
		_output("fwrite error: %d\n", errno);
	fflush(fp);
	return 0;
}

static void close_connect(int s)
{
	close(s);
}

static void *worker_thread_or_server(unsigned short port, const char *file)
{
	int ready, i, n, bufsize = 256 * 1024;
	void *buf;

	int s_listen, new_fd;
	struct sockaddr_in their_addr;
	struct sockaddr_in my_addr;
	socklen_t sin_size;

	int epoll_fd = -1;
	struct epoll_event epoll_events[EPOLL_MAX];
	struct epoll_event ev, *e;

	FILE *fp = NULL;

	ignore_pipe();

	if ((s_listen = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		_output("c:%s, e:%s\n", "socket", strerror(errno));
		return NULL;
	}

	config_socket(s_listen);

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(my_addr.sin_zero, '\0', sizeof(my_addr.sin_zero));
	if (bind(s_listen, (struct sockaddr *) &my_addr, sizeof(my_addr)) == -1) {
		_output("c:%s, e:%s\n", "bind", strerror(errno));
		return NULL;
	}

	if (listen(s_listen, BACK_LOG) == -1) {
		_output("c:%s, e:%s\n", "listen", strerror(errno));
		return NULL;
	}

	epoll_fd = epoll_create(EPOLL_MAX);
	memset(&ev, 0, sizeof(ev));
	ev.data.fd = s_listen;
	ev.events = EPOLLIN;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, s_listen, &ev);

	buf = malloc(bufsize);
	for (;;) {
		do
			ready = epoll_wait(epoll_fd, epoll_events, EPOLL_MAX, -1);
		while ((ready == -1) && (errno == EINTR));

		for (i = 0; i < ready; i++) {
			e = epoll_events + i;

			if (e->data.fd == s_listen) {
				sin_size = sizeof(their_addr);
				if ((new_fd = accept(s_listen, (struct sockaddr *)&their_addr, &sin_size)) == -1) {
					_output("c:%s, e:%s\n", "accept", strerror(errno));
					continue;
				}

				unsigned char addr[4];
				memcpy(addr, &their_addr.sin_addr.s_addr, 4);
				_output("New connect, addr:%d.%d.%d.%d, port:%d, fd:%d\n",
						addr[0], addr[1], addr[2], addr[3],
						ntohs(their_addr.sin_port),
						new_fd);

				struct epoll_event ev;
				memset(&ev, 0, sizeof(ev));
				ev.data.fd = new_fd;
				ev.events = EPOLLIN;
				epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd, &ev);

				continue;
			} else if (!(e->events & EPOLLIN)) {
				_output("!!! Not EPOLLIN: event is %08x, fd:%d\n", e->events, e->data.fd);
				continue;
			}

			if ((n = recv(e->data.fd, buf, bufsize, 0)) > 0) {
				if (!fp)
					fp = fopen(file, "a+");

				if (!fp) {
					_output("Open '%s' NG\n", file);
					continue;
				}

				if (process_dalog_data(e->data.fd, buf, n, fp))
					close_connect(e->data.fd);
			} else {
				_output("Remote close socket: %d\n", e->data.fd);
				close_connect(e->data.fd);

				/* XXX: Should not put it here */
				fflush(fp);
			}
		}
	}
	free(buf);

	close(epoll_fd);
	epoll_fd = -1;

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
	printf("       environ: LOGSEW_SKIP_LOG\n");

	if (die)
		exit(0);
}

int main(int argc, char *argv[])
{
	unsigned short port;
	const char *file;
	char *env;

	if (getenv("LOGSEW_SKIP_LOG"))
		__save_log = 0;
	else
		__save_log = 1;

	if (argc < 3) {
		env = getenv("LOGSEW_PORT");
		if (!env)
			help(1);
		port = (unsigned short)atoi(env);

		env = getenv("LOGSEW_FILE");
		if (!env)
			help(1);
		file = strdup(env);
	} else {
		port = (unsigned short)atoi(argv[1]);
		file = strdup(argv[2]);
	}

	worker_thread_or_server(port, file);

	free((void*)file);
	return 0;
}

