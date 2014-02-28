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

#define KMODU_NAME "NHSYSLOG"
#include <hilda/klog.h>

#define NH_SYSLOG

/*-----------------------------------------------------------------------
 * KLOG
 *
 * Direct to server
 */
#ifdef NH_SYSLOG

#include "log_monitor.h"

void vsyslog(int __pri, __const char *__fmt, __gnuc_va_list __ap)
{
	static int call_realfunc = 0;
	static int (*realfunc)(int, __const char*, __gnuc_va_list) = NULL;

	if (!realfunc)
		realfunc = dlsym(RTLD_NEXT, "vsyslog");

	if (call_realfunc == 0) {
		if (getenv("NH_SYSLOG_NOREAL"))
			call_realfunc = 'n';
		else
			call_realfunc = 'y';
	}

	if (call_realfunc == 'y') {
		klogmon_init();
		KLOG_CHK_AND_CALL_AP(KLOG_INFO, 'L', KMODU_NAME, __FILE__, __func__, __LINE__, fmt, ap);

		realfunc(__pri, __fmt, __ap);
	}
}
#endif

