/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

#include <karg.h>
#include <kstr.h>
#include <klog.h>

static int __g_boot_argc;
static char **__g_boot_argv;

static int __g_klog_serv_skt = -1;

static int load_boot_args()
{
	// FILE *fp = fopen("/proc/cmdline", "rt");
	FILE *fp = fopen("/tmp/cmdline", "rt");

	char buffer[4096];
	int ret = -1, bytes;

	if (fp) {
		bytes = fread(buffer, sizeof(char), sizeof(buffer), fp);
		fclose(fp);

		if (bytes <= 0)
			return -1;

		buffer[bytes] = '\0';
		kstr_trim(buffer);

		build_argv(buffer, &__g_boot_argc, &__g_boot_argv);
	}
	return ret;
}

static int logger_wlogf(const char *content, int len)
{
	send(__g_klog_serv_skt, content, len, 0);
	return 0;
}

static void get_klog_server(char *serv, kushort *port)
{
	int i, j;

	load_boot_args();

	/* klog-server=172.22.7.144:2123 */
	i = arg_find(__g_boot_argc, __g_boot_argv, "klog-server=", 0);
	if (i >= 0) {
		for (j = 0; __g_boot_argv[i][j + 12] != ':'; j++)
			serv[j] = __g_boot_argv[i][j + 12];
		serv[j] = '\0';
		*port = atoi((const char*)&__g_boot_argv[i][j + 1 + 12]);
	} else {
		strcpy(serv, "172.22.7.144");
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

static int connect_klog_serv(const char *server, unsigned short port, int *retfd)
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

#define O_LOG_SHIT klog

#define xioctl(fd, req...) do { \
	klog("shit\n"); \
	ioctl(fd, req); \
} while (0)

int main(int argc, char *argv[])
{
	char serv[128];
	kushort port;

	klog_init(-1, argc, argv);
	klog_add_logger(logger_wlogf);

	get_klog_server(serv, &port);
	klog("serv:%s\n", serv);
	klog("port:%u\n", port);

	connect_klog_serv(serv, port, &__g_klog_serv_skt);

	xioctl(1, 222, 333, 444);

	while (1) {
		O_LOG_SHIT("ssssssssssssssssssssssssssssssssssssssss\n");
		klog("port:%u\n", port);
		sleep(1);
	}

	return 0;
}