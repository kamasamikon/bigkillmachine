/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <helper.h>

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
#include <dalog/dalog.h>
#include <dalog_setup.h>

static int __g_rlog_serv_skt = -1;

static char __g_klog_serv[128];
static unsigned short __g_klog_serv_port;

static pid_t __g_pid;
static char *__g_prog;

static int _output(const char *fmt, ...)
{
	va_list arg;
	int done;
	char buf[2048], cmd[2048];

	va_start(arg, fmt);
	done = vsnprintf(buf, sizeof(buf), fmt, arg);
	va_end(arg);

	printf("<%s@%d> %s", __g_prog, __g_pid, buf);

	sprintf(cmd, "echo -n '<%s@%d> %s' >> '%s'", __g_prog, __g_pid, buf, "/tmp/lm.out");
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

		_output("load_cfg_file: %s", buf);
		klog_rule_add(buf);
		lines_done++;
	}

	fclose(fp);
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

	_output("connect_rlog_serv: server:<%s>, port:%d\n", server, port);

	if ((he = gethostbyname(server)) == NULL) {
		_output("connect_rlog_serv: gethostbyname error: %s.\n", strerror(errno));
		return -1;
	}
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		_output("connect_rlog_serv: socket error: %s.\n", strerror(errno));
		return -1;
	}

	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(port);
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(their_addr.sin_zero, '\0', sizeof their_addr.sin_zero);
	if (connect(sockfd, (struct sockaddr *)&their_addr,
				sizeof their_addr) == -1) {
		_output("connect_rlog_serv: connect error: %s.\n", strerror(errno));
		close(sockfd);
		return -1;
	}

	config_socket(sockfd);

	*retfd = sockfd;
	_output("connect_rlog_serv: retfd: %d\n", sockfd);
	return 0;
}

static void logger_remote(const char *content, int len)
{
	if (__g_rlog_serv_skt == -1)
		connect_rlog_serv(__g_klog_serv, __g_klog_serv_port, &__g_rlog_serv_skt);

	if (len != send(__g_rlog_serv_skt, content, len, 0)) {
		_output("logger_wlogf: send error: %s, %d\n", strerror(errno), __g_rlog_serv_skt);
		if (__g_rlog_serv_skt != -1)
			close(__g_rlog_serv_skt);
		__g_rlog_serv_skt = -1;
	} else if (content[len - 1] != '\n')
		send(__g_rlog_serv_skt, "\n", 1, 0);
}
static void logger_file(const char *content, int len)
{
	static FILE *fp = NULL;

	if (!fp)
		fp = fopen(getenv("KLOG_TO_LOCAL"), "a+");

	if (fp) {
		if (content[len - 1] == '\n')
			fprintf(fp, "%s", content);
		else
			fprintf(fp, "%s\n", content);
		fflush(fp);
	}
}
static void logger_syslog(const char *content, int len)
{
	syslog(LOG_INFO, "%s", content);
}

static char *get_prog_name()
{
	FILE *fp;
	char path[256], buf[2048];

	sprintf(path, "/proc/%d/cmdline", getpid());
	fp = fopen(path, "rt");
	if (!fp)
		return strdup("?");

	fgets(buf, sizeof(buf), fp);
	fclose(fp);
	return strdup(basename(buf));
}

void dalog_setup()
{
	static int inited = 0;
	char *env;

	if (dagou_unlikely(inited))
		return;

	inited = 1;
	__g_pid = getpid();
	__g_prog = get_prog_name();

	_output("dalog_setup\n");

	env = getenv("KLOG_TO_LOCAL");
	if (env) {
		_output("KLog: KLOG_TO_LOCAL opened <%s>\n", env);
		klog_add_logger(logger_file);
	}
	env = getenv("KLOG_TO_SYSLOG");
	if (env) {
		_output("KLog: KLOG_TO_SYSLOG opened <%s>\n", env);
		klog_add_logger(logger_syslog);
	}
	env = getenv("KLOG_TO_REMOTE");
	if (env) {
		_output("KLog: KLOG_TO_REMOTE opened <%s>\n", env);
		klog_add_logger(logger_remote);

		if (!rlog_serv_from_kernel_cmdline(env, __g_klog_serv, &__g_klog_serv_port))
			connect_rlog_serv(__g_klog_serv, __g_klog_serv_port, &__g_rlog_serv_skt);
	}
}
