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

#define DALOG_MODU_NAME "NHSYSLOG"
#include <dalog.h>

#define DAGOU_SYSLOG

/*-----------------------------------------------------------------------
 * KLOG
 *
 * Direct to server
 */
#ifdef DAGOU_SYSLOG

#include <dalog_setup.h>

static int skip_dalog = -1;

void syslog(int __pri, __const char *__fmt, ...)
{
	va_list ap;

	if (dagou_unlikely(skip_dalog == -1)) {
		if (getenv("DAGOU_SYSLOG_SKIP"))
			skip_dalog = 1;
		else
			skip_dalog = 0;
	}
	if (dagou_unlikely(skip_dalog))
		return;

	va_start(ap, __fmt);
	dalog_setup();
	switch (__pri) {
	case LOG_EMERG:
		DALOG_CHK_AND_CALL_AP(DALOG_FATAL, 'F', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
		break;
	case LOG_ALERT:
		DALOG_CHK_AND_CALL_AP(DALOG_ALERT, 'A', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
		break;
	case LOG_CRIT:
		DALOG_CHK_AND_CALL_AP(DALOG_CRIT, 'C', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
		break;
	case LOG_ERR:
		DALOG_CHK_AND_CALL_AP(DALOG_ERR, 'E', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
		break;
	case LOG_WARNING:
		DALOG_CHK_AND_CALL_AP(DALOG_WARNING, 'W', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
		break;
	case LOG_NOTICE:
		DALOG_CHK_AND_CALL_AP(DALOG_NOTICE, 'N', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
		break;
	case LOG_INFO:
		DALOG_CHK_AND_CALL_AP(DALOG_INFO, 'L', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
		break;
	case LOG_DEBUG:
		DALOG_CHK_AND_CALL_AP(DALOG_DEBUG, 'T', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
		break;
	default:
		DALOG_CHK_AND_CALL_AP(DALOG_INFO, 'L', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
	}
	va_end(ap);
}

void vsyslog(int __pri, const char *__fmt, va_list ap)
{
	if (dagou_unlikely(skip_dalog == -1)) {
		if (getenv("DAGOU_SYSLOG_SKIP"))
			skip_dalog = 1;
		else
			skip_dalog = 0;
	}
	if (dagou_unlikely(skip_dalog))
		return;

	dalog_setup();
	switch (__pri) {
	case LOG_EMERG:
		DALOG_CHK_AND_CALL_AP(DALOG_FATAL, 'F', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
		break;
	case LOG_ALERT:
		DALOG_CHK_AND_CALL_AP(DALOG_ALERT, 'A', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
		break;
	case LOG_CRIT:
		DALOG_CHK_AND_CALL_AP(DALOG_CRIT, 'C', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
		break;
	case LOG_ERR:
		DALOG_CHK_AND_CALL_AP(DALOG_ERR, 'E', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
		break;
	case LOG_WARNING:
		DALOG_CHK_AND_CALL_AP(DALOG_WARNING, 'W', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
		break;
	case LOG_NOTICE:
		DALOG_CHK_AND_CALL_AP(DALOG_NOTICE, 'N', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
		break;
	case LOG_INFO:
		DALOG_CHK_AND_CALL_AP(DALOG_INFO, 'L', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
		break;
	case LOG_DEBUG:
		DALOG_CHK_AND_CALL_AP(DALOG_DEBUG, 'T', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
		break;
	default:
		DALOG_CHK_AND_CALL_AP(DALOG_INFO, 'L', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, __fmt, ap);
	}
}

#endif

