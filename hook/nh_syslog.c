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

void syslog(int __pri, __const char *__fmt, ...)
{
	va_list ap;

	va_start(ap, __fmt);
	klogmon_init();
	switch (__pri) {
	case LOG_EMERG:
		KLOG_CHK_AND_CALL_AP(KLOG_FATAL, 'F', KMODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
		break;
	case LOG_ALERT:
		KLOG_CHK_AND_CALL_AP(KLOG_ALERT, 'A', KMODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
		break;
	case LOG_CRIT:
		KLOG_CHK_AND_CALL_AP(KLOG_CRIT, 'C', KMODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
		break;
	case LOG_ERR:
		KLOG_CHK_AND_CALL_AP(KLOG_ERR, 'E', KMODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
		break;
	case LOG_WARNING:
		KLOG_CHK_AND_CALL_AP(KLOG_WARNING, 'W', KMODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
		break;
	case LOG_NOTICE:
		KLOG_CHK_AND_CALL_AP(KLOG_NOTICE, 'N', KMODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
		break;
	case LOG_INFO:
		KLOG_CHK_AND_CALL_AP(KLOG_INFO, 'L', KMODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
		break;
	case LOG_DEBUG:
		KLOG_CHK_AND_CALL_AP(KLOG_DEBUG, 'T', KMODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
		break;
	default:
		KLOG_CHK_AND_CALL_AP(KLOG_INFO, 'L', KMODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
	}
	va_end(ap);
}
#endif

