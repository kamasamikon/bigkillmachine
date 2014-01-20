/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include "nh_helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sys/inotify.h>

#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <syslog.h>

/*-----------------------------------------------------------------------
 * KLOG
 *
 * Direct to server
 */
#include <hilda/klog.h>
#include <hilda/karg.h>

#include "log_monitor.h"

static int __g_rlog_serv_skt = -1;

static char __g_klog_serv[128];
static unsigned short __g_klog_serv_port;

static int _output(const char *fmt, ...)
{
	va_list arg;
	int done;
	char buf[2048], cmd[2048];

	va_start(arg, fmt);
	done = vsnprintf(buf, sizeof(buf), fmt, arg);
	va_end(arg);

	fprintf(stderr, "%s", buf);

	sprintf(cmd, "echo -n '%s' >> '%s'", buf, "/tmp/lm.out");
	system(cmd);

	return done;
}

static void load_cfg_file(const char *path)
{
	static int lines_done = 0;
	int line = 0;
	char buf[8092];
	FILE *fp;

	fp = fopen(path, "rt");
	if (!fp)
		return;

	while (fgets(buf, sizeof(buf), fp)) {
		if (line++ < lines_done)
			continue;

		_output("<%d> load_cfg_file: %s", getpid(), buf);
		klog_rule_add(buf);
		lines_done++;
	}

	fclose(fp);
}

static void* thread_monitor_cfgfile(void *user_data)
{
	const char *path = (const char*)user_data;
	int fd, wd, len, tmp_len;
	char buffer[4096], *offset = NULL;
	struct inotify_event *event;

	fd = inotify_init();
	if (fd < 0) {
		_output("<%d> monitor_cfgfile: inotify_init failed: %s.\n", getpid(), strerror(errno));
		exit(-1);
	}

	if (!path)
		path = "/tmp/klog.rt.cfg";

	_output("monitor_cfgfile: path: <%s>\n", path);
	wd = inotify_add_watch(fd, path, EVENT_MASK);
	if (wd < 0) {
		_output("<%d> monitor_cfgfile: inotify_add_watch failed: %s.\n", getpid(), strerror(errno));
		return NULL;
	}

	while (len = read(fd, buffer, sizeof(buffer))) {
		offset = buffer;
		event = (struct inotify_event*)buffer;

		while (((char*)event - buffer) < len) {
			if (event->wd == wd) {
				if (EVENT_MASK & event->mask) {
					_output("<%d> monitor_cfgfile: Opt: Configure changed\n", getpid());
					load_cfg_file(path);
				}
				break;
			}

			tmp_len = sizeof(struct inotify_event) + event->len;
			event = (struct inotify_event*)(offset + tmp_len);
			offset += tmp_len;
		}
	}
	_output("monitor_cfgfile: bye\n");
	return NULL;
}

static int load_boot_args(int *argc, char ***argv)
{
	FILE *fp = fopen("/proc/cmdline", "rt");
	char buffer[4096];
	int bytes;

	printf("load_boot_args: fp:%p\n", fp);
	if (fp) {
		bytes = fread(buffer, sizeof(char), sizeof(buffer), fp);
		fclose(fp);

		printf("load_boot_args: bytes:%d\n", bytes);

		if (bytes <= 0)
			return -1;

		buffer[bytes] = '\0';
		printf("load_boot_args:'%s'\n", buffer);
		karg_build(buffer, argc, argv);
		printf("load_boot_args: argc:%d\n", argc);
		return 0;
	}
	return -1;
}

static int rlog_serv_from_kernel_cmdline(const char *url, char *serv, unsigned short *port)
{
	int i;

	if (url && strchr(url, ':')) {
		for (i = 0; url[i] != ':'; i++)
			serv[i] = url[i];
		serv[i] = '\0';
		*port = atoi((const char*)&url[i + 1]);
		return 0;
	}
	return -1;
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

static int connect_rlog_serv(const char *server, unsigned short port, int *retfd)
{
	int sockfd;
	struct hostent *he;
	struct sockaddr_in their_addr;

	_output("<%d> connect_rlog_serv: server:<%s>, port:%d\n", getpid(), server, port);

	if ((he = gethostbyname(server)) == NULL) {
		_output("<%d> connect_rlog_serv: gethostbyname error: %s.\n", getpid(), strerror(errno));
		return -1;
	}
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		_output("<%d> connect_rlog_serv: socket error: %s.\n", getpid(), strerror(errno));
		return -1;
	}

	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(port);
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(their_addr.sin_zero, '\0', sizeof their_addr.sin_zero);
	if (connect(sockfd, (struct sockaddr *)&their_addr,
				sizeof their_addr) == -1) {
		_output("<%d> connect_rlog_serv: connect error: %s.\n", getpid(), strerror(errno));
		close(sockfd);
		return -1;
	}

	config_socket(sockfd);

	*retfd = sockfd;
	_output("<%d> connect_rlog_serv: retfd: %d\n", getpid(), sockfd);
	return 0;
}

static void logger_remote(const char *content, int len)
{
	if (__g_rlog_serv_skt == -1)
		connect_rlog_serv(__g_klog_serv, __g_klog_serv_port, &__g_rlog_serv_skt);

	if (len != send(__g_rlog_serv_skt, content, len, 0)) {
		_output("<%d> logger_wlogf: send error: %s, %d\n", getpid(), strerror(errno), __g_rlog_serv_skt);
		if (__g_rlog_serv_skt != -1)
			close(__g_rlog_serv_skt);
		__g_rlog_serv_skt = -1;
	}
}
static void logger_file(const char *content, int len)
{
	static FILE *fp = NULL;

	if (!fp)
		fp = fopen(getenv("KLOG_TO_LOCAL"), "a+");

	if (fp) {
		fprintf(fp, "%s", content);
		fflush(fp);
	}
}
static void logger_syslog(const char *content, int len)
{
	syslog(LOG_INFO, "%s", content);
}

void klogmon_init()
{
	static int inited = 0;
	int argc;
	char **argv, *env;

	if (inited)
		return;

	inited = 1;
	_output("klogmon_init: pid=%d\n", getpid());

	printf("Will call load_boot_args\n");
	load_boot_args(&argc, &argv);
	printf("after call load_boot_args\n");

	env = getenv("KLOG_TO_LOCAL");
	if (env) {
		_output("<%d> KLog: KLOG_TO_LOCAL opened, <%s>\n", getpid(), env);
		klog_add_logger(logger_file);
	}
	env = getenv("KLOG_TO_SYSLOG");
	if (env) {
		_output("<%d> KLog: KLOG_TO_SYSLOG opened, <%s>\n", getpid(), env);
		klog_add_logger(logger_syslog);
	}
	env = getenv("KLOG_TO_REMOTE");
	if (env) {
		_output("<%d> KLog: KLOG_TO_REMOTE opened, <%s>\n", getpid(), env);
		klog_add_logger(logger_remote);

		if (!rlog_serv_from_kernel_cmdline(env, __g_klog_serv, &__g_klog_serv_port))
			connect_rlog_serv(__g_klog_serv, __g_klog_serv_port, &__g_rlog_serv_skt);
	}

	spl_thread_create(thread_monitor_cfgfile, (void*)getenv("KLOG_RTCFG"), 0);
}

