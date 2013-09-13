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

#include "log_monitor.h"

int klog_vf(unsigned char type, unsigned int mask, const char *prog,
		const char *modu, const char *file, const char *func,
		int ln, const char *fmt, va_list ap)
{
	static int (*realfunc)(unsigned char, unsigned int, const char*,
			const char*, const char*, const char*, int,
			const char*, va_list) = NULL;
	if (!realfunc)
		realfunc = dlsym(RTLD_NEXT, "klog_vf");

	klogmon_init();
	return realfunc(type, mask, prog, modu, file, func, ln, fmt, ap);
}
#endif

