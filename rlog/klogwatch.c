/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/select.h>

#include <kstr.h>
#include <karg.h>
#include <opt.h>
#include <opt-rpc-server.h>

#include <rlog.h>

static int __g_boot_argc;
static char **__g_boot_argv;

static int __g_rlog_serv_skt;

static void setup_env(int argc, char *argv[])
{
	void *logcc, *optcc;
	unsigned int flg = LOG_LOG | LOG_ERR | LOG_FAT | LOG_ATM | LOG_MODU | LOG_FILE | LOG_LINE;

	logcc = rlog_init(flg, argc, argv);
	optcc = opt_init(argc, argv);

	opt_add_s("p:/env/log/cc", OA_GET, NULL, NULL);
	opt_setptr("p:/env/log/cc", logcc);

	opt_add_s("p:/env/opt/cc", OA_GET, NULL, NULL);
	opt_setptr("p:/env/opt/cc", optcc);
}

static int os_rlog_argv(int ses, void *opt, void *pa, void *pb)
{
	rlog_setflg(opt_get_new_str(opt));
	return 0;
}

static int load_boot_args(int *argc, char ***argv)
{
	FILE *fp = fopen("/proc/cmdline", "rt");
	char buffer[4096];
	int bytes;

	if (fp) {
		bytes = fread(buffer, sizeof(char), sizeof(buffer), fp);
		fclose(fp);

		if (bytes <= 0)
			return -1;

		buffer[bytes] = '\0';
		kstr_trim(buffer);

		build_argv(buffer, argc, argv);
		return 0;
	}
	return -1;
}

int rlog_s(const char *content, int len)
{
	if (len != send(__g_rlog_serv_skt, content, len, 0))
		printf("logger_wlogf: send error: %d\n", errno);

	return 0;
}

int rlog_v(unsigned char type, unsigned int flg, const char *modu, const char *file, int ln, const char *fmt, va_list ap)
{
	return 0;
}


static void get_rlog_server(char *serv, kushort *port)
{
	int i, j;

	/* rlog-server=172.22.7.144:2123 */
	i = arg_find(__g_boot_argc, __g_boot_argv, "rlog-server=", 0);
	if (i >= 0) {
		for (j = 0; __g_boot_argv[i][j + 12] != ':'; j++)
			serv[j] = __g_boot_argv[i][j + 12];
		serv[j] = '\0';
		*port = atoi((const char*)&__g_boot_argv[i][j + 1 + 12]);
	} else {
		strcpy(serv, "127.0.0.1");
		*port = 9000;
	}
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

	if ((he = gethostbyname(server)) == NULL)
		return -1;
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
		return -1;

	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(port);
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(their_addr.sin_zero, '\0', sizeof their_addr.sin_zero);
	if (connect(sockfd, (struct sockaddr *)&their_addr,
				sizeof their_addr) == -1) {
		kerror("c:%s, e:%s\n", "connect", strerror(errno));
		close(sockfd);
		return -1;
	}

	config_socket(sockfd);

	*retfd = sockfd;
	return 0;
}

int setup_rlog(int argc, char *argv[])
{
	char serv[128];
	kushort port;

	get_rlog_server(serv, &port);
	rlog("serv:%s\n", serv);
	rlog("port:%u\n", port);

	connect_rlog_serv(serv, port, &__g_rlog_serv_skt);

	return 0;
}

static void init_log_monitor()
{
	static int inited = 0;

	if (inited)
		return;

	inited = 1;
	load_boot_args(&__g_boot_argc, &__g_boot_argv);

	if (1) {
		int i;

		for (i = 0; i < __g_boot_argc; i++)
			printf("argv[%d], <%s>\n", i, __g_boot_argv[i]);
	}

	setup_env(__g_boot_argc, __g_boot_argv);

	setup_rlog(__g_boot_argc, __g_boot_argv);

	opt_add_s("s:/rlog/argv", OA_DFT, os_rlog_argv, NULL);

	opt_rpc_server_init(9000 + getpid(), __g_boot_argc, __g_boot_argv);
}

#if 0
rlog()
{
	/* Should LOG PID and TID */

	init_log_monitor();
	rlog(...);
}
#endif

#if 0
#define GET_LOG_LEVEL() do { \
	int touches = rlog_touches(); \
	if (logger_not_set) \
		rlog_add_logger(buitin_logger); \
	if (__g_rlog_touches < touches) { \
		__g_rlog_touches = touches; \
		__gc_rlog_level = rlog_getflg((const kchar*)__FILE__); \
	} \
} while (0)

#define rlog(fmt, ...) do { \
	GET_LOG_LEVEL(); \
	if (__gc_rlog_level & LOG_LOG) { \
		rlogf('L', __gc_rlog_level, RLOG_MODU, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
	} \
} while (0)
#endif

int main(int argc, char *argv[])
{
	unsigned long i, tick, cost;
	unsigned int count;

	init_log_monitor();

	rlog("sdlfasdflf\n");

	if (argv[1])
		count = strtoul(argv[1], NULL, 10);
	else
		count = 100000;

	tick = spl_get_ticks();

	printf("usage: rlog count rlog-server=172.22.1.144:9000\n");

	for (i = 0; i < count; i++) {
		rlog("remote rlog test. puppy FANG is a bad egg. done<%d>\n", i);
	}

	cost = spl_get_ticks() - tick;

	if (cost == 0)
		cost = 1;

	printf("time cost: %lu\n", cost);
	printf("count: %u\n", count);
	printf("count / ms = %lu\n", count / cost);

	return 0;
}
