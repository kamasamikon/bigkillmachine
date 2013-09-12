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

#define NH_KLOG

/*-----------------------------------------------------------------------
 * KLOG
 *
 * Direct to server
 */
#ifdef NH_KLOG

#include <klog.h>
#include <karg.h>
#include <kopt.h>

#define EVENT_MASK (IN_MODIFY)

static int __g_rlog_serv_skt = -1;

static char __g_klog_serv[128];
static unsigned short __g_klog_serv_port;

static void load_cfg_file(const char *path)
{
	char buf[8092];
	FILE *fp;

	fp = fopen(path, "rt");
	if (!fp)
		return;

	while (fgets(buf, sizeof(buf), fp))
		klog_rule_add(buf);

	fclose(fp);
}

static void* thread_monitor_cfgfile(const char *path)
{
    int fd, wd, i, len, tmp_len;
    char buffer[4096], *offset = NULL;
    struct inotify_event *event;

    fd = inotify_init();
    if (fd < 0) {
        printf("<%d> thread_monitor_cfgfile: inotify_init failed.\n", getpid());
        exit(-1);
    }

    if (!path)
	    path = "/tmp/klog.cfg";

    wd = inotify_add_watch(fd, path, EVENT_MASK);
    if (wd < 0) {
        printf("<%d> thread_monitor_cfgfile: inotify_add_watch failed.\n", getpid());
        return NULL;
    }

    while (len = read(fd, buffer, sizeof(buffer))) {
        offset = buffer;
        event = (struct inotify_event*)buffer;

        while (((char*)event - buffer) < len) {
            if (event->wd == wd) {
                if (!(EVENT_MASK& event->mask)) {
                    printf("<%d> thread_monitor_cfgfile: Opt: Configure changed\n", getpid());
		    klog_rule_clr();
		    load_cfg_file(path);
                }
                break;
            }

            tmp_len = sizeof(struct inotify_event) + event->len;
            event = (struct inotify_event*)(offset + tmp_len);
            offset += tmp_len;
        }
    }
    return NULL;
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

static void rlog_serv_from_kernel_cmdline(char *serv, unsigned short *port)
{
	int i, j;
	char *url = getenv("KLOG_SEWER_URL");

	if (url && strchr(url, ':')) {
		for (i = 0; url[i] != ':'; i++)
			serv[i] = url[i];
		serv[i] = '\0';
		*port = atoi((const char*)&url[i + 1]);
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

	printf("<%d> connect_rlog_serv: server:'%s', port:%d\n", getpid(), server, port);

	if ((he = gethostbyname(server)) == NULL) {
		printf("<%d> connect_rlog_serv: gethostbyname error: %s.\n", getpid(), strerror(errno));
		return -1;
	}
	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		printf("<%d> connect_rlog_serv: socket error: %s.\n", getpid(), strerror(errno));
		return -1;
	}

	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(port);
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(their_addr.sin_zero, '\0', sizeof their_addr.sin_zero);
	if (connect(sockfd, (struct sockaddr *)&their_addr,
				sizeof their_addr) == -1) {
		printf("<%d> connect_rlog_serv: connect error: %s.\n", getpid(), strerror(errno));
		close(sockfd);
		return -1;
	}

	config_socket(sockfd);

	*retfd = sockfd;
	printf("<%d> connect_rlog_serv: retfd: %d\n", getpid(), sockfd);
	return 0;
}

static void logger_remote(const char *content, int len)
{
	if (__g_rlog_serv_skt == -1)
		connect_rlog_serv(__g_klog_serv, __g_klog_serv_port, &__g_rlog_serv_skt);

	if (len != send(__g_rlog_serv_skt, content, len, 0)) {
		printf("<%d> logger_wlogf: send error: %s, %d\n", getpid(), strerror(errno), __g_rlog_serv_skt);
		if (__g_rlog_serv_skt != -1)
			close(__g_rlog_serv_skt);
		__g_rlog_serv_skt = -1;
	}
}
static void logger_printf(const char *content, int len)
{
	printf("%s", content);
}
static void logger_syslog(const char *content, int len)
{
	syslog(LOG_INFO, "%s", content);
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

	if (getenv("KLOG_TO_REMOTE")) {
		printf("<%d> KLog: KLOG_TO_REMOTE opened\n", getpid());
		klog_add_logger(logger_remote);
	}
	if (getenv("KLOG_TO_PRINTF")) {
		printf("<%d> KLog: KLOG_TO_PRINTF opened\n", getpid());
		klog_add_logger(logger_printf);
	}
	if (getenv("KLOG_TO_SYSLOG")) {
		printf("<%d> KLog: KLOG_TO_SYSLOG opened\n", getpid());
		klog_add_logger(logger_syslog);
	}

	rlog_serv_from_kernel_cmdline(__g_klog_serv, &__g_klog_serv_port);
	connect_rlog_serv(__g_klog_serv, __g_klog_serv_port, &__g_rlog_serv_skt);

	spl_thread_create(thread_monitor_cfgfile, (void*)getenv("KLOG_RTCFG"), 0);
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

