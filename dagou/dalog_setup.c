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
 * DALOG
 *
 * Direct to server
 */
#include <dalog.h>
#include <dalog_setup.h>

static int __serv_sock = -1;

static char __serv_addr[128];
static unsigned short __serv_port;

static pid_t __pid;
static char *__prg;

static int _output(const char *fmt, ...)
{
	va_list arg;
	int done;
	char buf[2048], cmd[2048];
	int ret;

	va_start(arg, fmt);
	done = vsnprintf(buf, sizeof(buf), fmt, arg);
	va_end(arg);

	printf("<%s@%d> %s", __prg, __pid, buf);

	sprintf(cmd, "echo -n '<%s@%d> %s' >> '%s'", __prg, __pid, buf, "/tmp/lm.out");
	ret = system(cmd);

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
		log_rule_add(buf);
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

static void logger_network(char *content, int len)
{
	if (__serv_sock == -1)
		connect_rlog_serv(__serv_addr, __serv_port, &__serv_sock);

	if (len != send(__serv_sock, content, len, 0)) {
		_output("logger_wlogf: send error: %s, %d\n", strerror(errno), __serv_sock);
		if (__serv_sock != -1)
			close(__serv_sock);
		__serv_sock = -1;
	} else if (content[len - 1] != '\n')
		send(__serv_sock, "\n", 1, 0);
}
static void logger_file(char *content, int len)
{
	static FILE *fp = NULL;

	if (!fp)
		fp = fopen(getenv("DALOG_TO_LOCAL"), "a+");

	if (fp) {
		if (content[len - 1] == '\n')
			fprintf(fp, "%s", content);
		else
			fprintf(fp, "%s\n", content);
		fflush(fp);
	}
}
static void logger_syslog(char *content, int len)
{
	syslog(LOG_INFO, "%s", content);
}

static char *get_prog_name()
{
	FILE *fp;
	char path[256], buf[2048];
	char *ret;

	sprintf(path, "/proc/%d/cmdline", getpid());
	fp = fopen(path, "rt");
	if (!fp)
		return strdup("?");

	ret = fgets(buf, sizeof(buf), fp);
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
	__pid = getpid();
	__prg = get_prog_name();

	_output("dalog_setup\n");

	env = getenv("DALOG_TO_LOCAL");
	if (env) {
		_output("daLog: DALOG_TO_LOCAL opened <%s>\n", env);
		dalog_add_logger(logger_file);
	}
	env = getenv("DALOG_TO_SYSLOG");
	if (env) {
		_output("daLog: DALOG_TO_SYSLOG opened <%s>\n", env);
		dalog_add_logger(logger_syslog);
	}
	env = getenv("DALOG_TO_NETWORK");
	if (env) {
		_output("daLog: DALOG_TO_NETWORK opened <%s>\n", env);
		dalog_add_logger(logger_network);

		if (!rlog_serv_from_kernel_cmdline(env, __serv_addr, &__serv_port))
			connect_rlog_serv(__serv_addr, __serv_port, &__serv_sock);
	}
}

