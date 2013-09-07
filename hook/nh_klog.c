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
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>

#define NH_KLOG

/*-----------------------------------------------------------------------
 * KLOG
 *
 * Direct to server
 */
#ifdef NH_KLOG

#include <klog.h>
#include <karg.h>
#include <opt.h>
#include <opt-rpc-server.h>

static int __g_rlog_serv_skt = -1;

static int os_rlog_rule(int ses, void *opt, void *pa, void *pb)
{
	klog_rule_add(opt_get_new_str(opt));
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
		build_argv(buffer, argc, argv);
		return 0;
	}
	return -1;
}


static void rlog_serv_from_kernel_cmdline(int argc, char **argv, char *serv, unsigned short *port)
{
	int i, j;

	/* rlog-server=172.22.7.144:2123 */
	i = arg_find(argc, argv, "rlog-server=", 0);
	if (i >= 0) {
		for (j = 0; argv[i][j + 12] != ':'; j++)
			serv[j] = argv[i][j + 12];
		serv[j] = '\0';
		*port = atoi((const char*)&argv[i][j + 1 + 12]);
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

static void setup_rlog(int argc, char *argv[])
{
	char serv[128];
	unsigned short port;

	rlog_serv_from_kernel_cmdline(argc, argv, serv, &port);
	printf("serv:%s\n", serv);
	printf("port:%u\n", port);

	connect_rlog_serv(serv, port, &__g_rlog_serv_skt);
}

static void logger_remote(const char *content, int len)
{
	if (len != send(__g_rlog_serv_skt, content, len, 0))
		printf("logger_wlogf: send error: %s, %d\n", strerror(errno), __g_rlog_serv_skt);
}

static void init_log_monitor()
{
	static int inited = 0;
	int argc;
	char **argv;

	if (inited)
		return;

	inited = 1;

	load_boot_args(&argc, &argv);

	/* os_rlog_rule to update the klog rule */
	opt_init(argc, argv);
	opt_add_s("s:/rlog/rule", OA_DFT, os_rlog_rule, NULL);

	klog_add_logger(logger_remote);

	setup_rlog(argc, argv);

	opt_rpc_server_init(9000 + getpid(), argc, argv);
	klogs("NH_KLOG: OptServer established, port: %d\n", 9000 + getpid());
}

int klog_vf(unsigned char type, unsigned int mask, const char *prog,
		const char *modu, const char *file, const char *func,
		int ln, const char *fmt, va_list ap)
{
	static int (*realfunc)(unsigned char, unsigned int, const char*,
			const char*, const char*, const char*, int,
			const char*, va_list) = NULL;
	if (!realfunc)
		realfunc = dlsym(RTLD_NEXT, "klog_vf");

	init_log_monitor();
	return realfunc(type, mask, prog, modu, file, func, ln, fmt, ap);
}
#endif

