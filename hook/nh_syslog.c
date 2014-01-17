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

#define klog_ap(fmt, ap) do { \
	KLOG_INNER_VAR_DEF(); \
	if (__kl_ver_get > __kl_ver_sav) { \
		__kl_ver_sav = __kl_ver_get; \
		KLOG_SETUP_NAME_AND_ID(); \
		__kl_mask = klog_calc_mask(__kl_prog_name_id_, __kl_modu_name_id_, __kl_file_name_id_, __kl_func_name_id, __LINE__, (int)spl_process_current()); \
		if (!(__kl_mask & KLOG_LOG)) \
			__kl_mask = 0; \
	} \
	if (__kl_mask) \
		klog_f('L', __kl_mask, __kl_prog_name_, KMODU_NAME, __kl_file_name_, __FUNCTION__, __LINE__, fmt, ap); \
} while (0)

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
		klog_ap(__fmt, __ap);

		realfunc(__pri, __fmt, __ap);
	}
}
#endif

