/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <helper.h>

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
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
#include <arpa/inet.h>
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

static int __log_to_file = 1;
static int __log_to_stdout = 1;

static void printlog(const char *fmt, ...)
{
	va_list arg;
	int done;
	char buf[2048], cmd[2048];
	int ret;

	va_start(arg, fmt);
	done = vsnprintf(buf, sizeof(buf), fmt, arg);
	va_end(arg);

	if (__log_to_stdout)
		printf("<%s@%d> %s", __prg, __pid, buf);

	if (__log_to_file) {
		sprintf(cmd, "echo -n '<%s@%d> %s' >> '%s'",
				__prg, __pid, buf, "/tmp/dalog_setup.log");
		ret = system(cmd);
	}
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

		printlog("load_cfg_file: %s", buf);
		dalog_rule_add(buf);
		lines_done++;
	}

	fclose(fp);
}

static int dalog_serv_from_kernel_cmdline(const char *url, char *serv, unsigned short *port)
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

static int connect_dalog_serv(const char *server, unsigned short port, int *retfd)
{
	int sockfd;
	struct hostent *he;
	struct sockaddr_in their_addr;

	printlog("connect_dalog_serv: server:<%s>, port:%d\n", server, port);

	if ((he = gethostbyname(server)) == NULL) {
		printlog("connect_dalog_serv: gethostbyname error: %s.\n", strerror(errno));
		return -1;
	}
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		printlog("connect_dalog_serv: socket error: %s.\n", strerror(errno));
		return -1;
	}

	memset((char*)&their_addr, 0, sizeof(their_addr));
	their_addr.sin_family = he->h_addrtype;
	memcpy((char*)&their_addr.sin_addr, he->h_addr, he->h_length);
	their_addr.sin_port = htons(port);

	if (connect(sockfd, (struct sockaddr *)&their_addr,
				sizeof their_addr) == -1) {
		printlog("connect_dalog_serv: connect error: %s.\n", strerror(errno));
		close(sockfd);
		return -1;
	}

	config_socket(sockfd);

	*retfd = sockfd;
	printlog("connect_dalog_serv: retfd: %d\n", sockfd);
	return 0;
}

static void logger_network(char *content, int len)
{
	if (__serv_sock == -1)
		connect_dalog_serv(__serv_addr, __serv_port, &__serv_sock);
	if (__serv_sock == -1)
		return;

	if (len != send(__serv_sock, content, len, 0)) {
		printlog("logger_wlogf: send error: %s, %d\n", strerror(errno), __serv_sock);
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

static void load_dalogrc(char *path)
{
	char *line = NULL;
	size_t len = 0;
	ssize_t i, bytes;

	FILE *fp;

	fp = fopen(path, "rt");
	if (!fp) {
		printf("load_dalogrc: fopen '%s' failed, e:%d\n", path, errno);
		return;
	}

	while ((bytes = getline(&line, &len, fp)) != -1) {
		if (line[bytes] == '\n')
			line[bytes] = '\0';

		for (i = bytes - 1; i >= 0; i--) {
			if (isspace(line[i]))
				continue;
			line[i + 1] = '\0';
			break;
		}

		printf("line: '%s'\n", line);

		if (!strncmp(line, "DALOG_NO_LOG_TO_FILE=", 21)) {
			printf("func:load_dalogrc:%d\n", __LINE__);
			setenv("DALOG_NO_LOG_TO_FILE", line + 21, 1);
		} else if (!strncmp(line, "DALOG_NO_LOG_TO_STDOUT=", 22)) {
			printf("func:load_dalogrc:%d\n", __LINE__);
			setenv("DALOG_NO_LOG_TO_STDOUT", line + 22, 1);
		} else if (!strncmp(line, "DALOG_TO_LOCAL=", 15)) {
			printf("func:load_dalogrc:%d\n", __LINE__);
			setenv("DALOG_TO_LOCAL", line + 15, 1);
		} else if (!strncmp(line, "DALOG_TO_SYSLOG=", 16)) {
			printf("func:load_dalogrc:%d\n", __LINE__);
			setenv("DALOG_TO_SYSLOG", line + 16, 1);
		} else if (!strncmp(line, "DALOG_TO_NETWORK=", 17)) {
			printf("func:load_dalogrc:%d\n", __LINE__);
			setenv("DALOG_TO_NETWORK", line + 17, 1);
		}
	}

	if (line)
		free(line);
	fclose(fp);
}

void dalog_setup()
{
	static int inited = 0;
	char *env;

	if (dagou_likely(inited))
		return;

	inited = 1;
	__pid = getpid();
	__prg = get_prog_name();

	load_dalogrc("/etc/dalogrc");
	load_dalogrc("/home/ntvroot/dalogrc");
	load_dalogrc("/tmp/dalogrc");

	env = getenv("DALOG_NO_LOG_TO_FILE");
	if (env && !strcmp(env, "YES"))
		__log_to_file = 0;

	env = getenv("DALOG_NO_LOG_TO_STDOUT");
	if (env && !strcmp(env, "YES"))
		__log_to_stdout = 0;

	printlog("dalog_setup\n");

	env = getenv("DALOG_TO_LOCAL");
	printlog("env('DALOG_TO_LOCAL') = '%s'\n", env);
	if (env) {
		printlog("daLog: DALOG_TO_LOCAL opened <%s>\n", env);
		dalog_add_logger(logger_file);
	} else {
		printlog("daLog: force DALOG_TO_LOCAL\n");
		setenv("DALOG_TO_LOCAL", "/tmp/dalog.output", 1);
		dalog_add_logger(logger_file);
	}

	env = getenv("DALOG_TO_SYSLOG");
	printlog("env('DALOG_TO_SYSLOG') = '%s'\n", env);
	if (env && !strcmp(env, "YES")) {
		printlog("daLog: DALOG_TO_SYSLOG opened <%s>\n", env);
		dalog_add_logger(logger_syslog);
	} else {
		printlog("daLog: force DALOG_TO_SYSLOG\n");
		dalog_add_logger(logger_syslog);
	}

	env = getenv("DALOG_TO_NETWORK");
	printlog("env('DALOG_TO_NETWORK') = '%s'\n", env);
	if (env) {
		printlog("daLog: DALOG_TO_NETWORK opened <%s>\n", env);
		dalog_add_logger(logger_network);

		if (!dalog_serv_from_kernel_cmdline(env, __serv_addr, &__serv_port))
			connect_dalog_serv(__serv_addr, __serv_port, &__serv_sock);
	}
}

