/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_NHLOG_H__
#define __K_NHLOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#define VAR_UNUSED __attribute__ ((unused))

/*-----------------------------------------------------------------------
 * Embedded variable used by nhtrace, nhlog, nherror, nhfatal etc
 */
static int VAR_UNUSED __nhl_file_name_id_ = -1;
static char VAR_UNUSED *__nhl_file_name_ = NULL;

static int VAR_UNUSED __nhl_prog_name_id_ = -1;
static char VAR_UNUSED *__nhl_prog_name_ = NULL;

static int VAR_UNUSED __nhl_modu_name_id_ = -1;

/*-----------------------------------------------------------------------
 * Define NHL_MODU_NAME if someone forgot it.
 */
#ifndef NHL_MODU_NAME
#define NHL_MODU_NAME  "?"
#endif

/*-----------------------------------------------------------------------
 * nhlog switch mask
 */
/* Copied from syslog */
#define NHLOG_FATAL      0x00000001 /* 0:f(fatal): system is unusable */
#define NHLOG_ALERT      0x00000002 /* 1:a: action must be taken immediately */
#define NHLOG_CRIT       0x00000004 /* 2:c: critical conditions */
#define NHLOG_ERR        0x00000008 /* 3:e: error conditions */
#define NHLOG_WARNING    0x00000010 /* 4:w: warning conditions */
#define NHLOG_NOTICE     0x00000020 /* 5:n: normal but significant condition */
#define NHLOG_INFO       0x00000040 /* 6:i:l(log): informational */
#define NHLOG_DEBUG      0x00000080 /* 7:d:t(trace): debug-level messages */
#define NHLOG_TYPE_ALL   0x000000ff

#define NHLOG_LOG        NHLOG_INFO
#define NHLOG_TRC        NHLOG_DEBUG

#define NHLOG_RTM        0x00000100 /* s: Relative Time, in MS, 'ShiJian' */
#define NHLOG_ATM        0x00000200 /* S: ABS Time, in MS, 'ShiJian' */

#define NHLOG_PID        0x00001000 /* j: Process ID, 'JinCheng' */
#define NHLOG_TID        0x00002000 /* x: Thread ID, 'XianCheng' */

#define NHLOG_PROG       0x00010000 /* P: Process Name */
#define NHLOG_MODU       0x00020000 /* M: Module Name */
#define NHLOG_FILE       0x00040000 /* F: File Name */
#define NHLOG_FUNC       0x00080000 /* H: Function Name, 'HanShu' */
#define NHLOG_LINE       0x00100000 /* N: Line Number */

#define NHLOG_ALL        0xffffffff

/*-----------------------------------------------------------------------
 * Embedded variable used by nhtrace, nhlog, nherror, nhfatal etc
 */
#define NHLOG_INNER_VAR_DEF() \
	static int VAR_UNUSED __nhl_ver_sav = -1; \
	static int VAR_UNUSED __nhl_func_name_id = -1; \
	static int VAR_UNUSED __nhl_mask = 0; \
	int VAR_UNUSED __nhl_ver_get = nhlog_touches();

#define NHLOG_SETUP_NAME_AND_ID(modu, file, func) do { \
	if (__nhl_file_name_id_ == -1) { \
		__nhl_file_name_ = nhlog_get_name_part(file); \
		__nhl_file_name_id_ = nhlog_file_name_add(__nhl_file_name_); \
	} \
	if (__nhl_prog_name_id_ == -1) { \
		__nhl_prog_name_ = nhlog_get_prog_name(); \
		__nhl_prog_name_id_ = nhlog_prog_name_add(__nhl_prog_name_); \
	} \
	if (__nhl_modu_name_id_ == -1) { \
		__nhl_modu_name_id_ = nhlog_modu_name_add(modu); \
	} \
	if (__nhl_func_name_id == -1) { \
		__nhl_func_name_id = nhlog_func_name_add(func); \
	} \
} while (0)

#define NHLOG_CHK_AND_CALL(mask, indi, modu, file, func, line, fmt, ...) do { \
	NHLOG_INNER_VAR_DEF(); \
	if (__nhl_ver_get > __nhl_ver_sav) { \
		__nhl_ver_sav = __nhl_ver_get; \
		NHLOG_SETUP_NAME_AND_ID(modu, file, func); \
		__nhl_mask = nhlog_calc_mask(__nhl_prog_name_id_, __nhl_modu_name_id_, __nhl_file_name_id_, __nhl_func_name_id, line, (int)getpid()); \
		if (!(__nhl_mask & (mask))) { \
			__nhl_mask = 0; \
		} \
	} \
	if (__nhl_mask) { \
		nhlog_f(indi, __nhl_mask, __nhl_prog_name_, modu, __nhl_file_name_, func, line, fmt, ##__VA_ARGS__); \
	} \
} while (0)

#define NHLOG_CHK_AND_CALL_AP(mask, indi, modu, file, func, line, fmt, ap) do { \
	NHLOG_INNER_VAR_DEF(); \
	if (__nhl_ver_get > __nhl_ver_sav) { \
		__nhl_ver_sav = __nhl_ver_get; \
		NHLOG_SETUP_NAME_AND_ID(modu, file, func); \
		__nhl_mask = nhlog_calc_mask(__nhl_prog_name_id_, __nhl_modu_name_id_, __nhl_file_name_id_, __nhl_func_name_id, line, (int)getpid()); \
		if (!(__nhl_mask & (mask))) { \
			__nhl_mask = 0; \
		} \
	} \
	if (__nhl_mask) { \
		nhlog_vf(indi, __nhl_mask, __nhl_prog_name_, modu, __nhl_file_name_, func, line, fmt, ap); \
	} \
} while (0)

/*-----------------------------------------------------------------------
 * nhlog, nherror, nhfatal etc
 */
#define nhlogs(fmt, ...) do { \
	nhlog_f(0, 0, NULL, NULL, NULL, NULL, 0, fmt, ##__VA_ARGS__); \
} while (0)

#define nhfatal(fmt, ...)       NHLOG_CHK_AND_CALL(NHLOG_FATAL,   'F', NHL_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define nhalert(fmt, ...)       NHLOG_CHK_AND_CALL(NHLOG_ALERT,   'A', NHL_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define nhcritical(fmt, ...)    NHLOG_CHK_AND_CALL(NHLOG_CRIT,    'C', NHL_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define nherror(fmt, ...)       NHLOG_CHK_AND_CALL(NHLOG_ERR,     'E', NHL_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define nhwarning(fmt, ...)     NHLOG_CHK_AND_CALL(NHLOG_WARNING, 'W', NHL_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define nhnotice(fmt, ...)      NHLOG_CHK_AND_CALL(NHLOG_NOTICE,  'N', NHL_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define nhinfo(fmt, ...)        NHLOG_CHK_AND_CALL(NHLOG_INFO,    'I', NHL_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define nhlog(fmt, ...)         NHLOG_CHK_AND_CALL(NHLOG_INFO,    'L', NHL_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define nhdebug(fmt, ...)       NHLOG_CHK_AND_CALL(NHLOG_DEBUG,   'D', NHL_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define nhtrace(fmt, ...)       NHLOG_CHK_AND_CALL(NHLOG_DEBUG,   'T', NHL_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)

#define nhassert(_x_) do { \
	if (!(_x_)) { \
		NHLOG_INNER_VAR_DEF(); \
		if (__nhl_ver_get > __nhl_ver_sav) { \
			__nhl_ver_sav = __nhl_ver_get; \
			NHLOG_SETUP_NAME_AND_ID(NHL_MODU_NAME, __FILE__, __func__); \
		} \
		nhlog_f('!', NHLOG_ALL, __nhl_prog_name_, NHL_MODU_NAME, __nhl_file_name_, __FUNCTION__, __LINE__, \
				"\n\tASSERT NG: \"%s\"\n\n", #_x_); \
	} \
} while (0)

/*-----------------------------------------------------------------------
 * Functions:
 */
int nhlog_touches(void);

char *nhlog_get_name_part(char *name);
char *nhlog_get_prog_name();

int nhlog_file_name_add(const char *name);
int nhlog_modu_name_add(const char *name);
int nhlog_prog_name_add(const char *name);
int nhlog_func_name_add(const char *name);

unsigned int nhlog_calc_mask(int prog, int modu, int file, int func, int line, int pid);

int nhlog_f(unsigned char type, unsigned int mask, const char *prog, const char *modu, const char *file, const char *func, int ln, const char *fmt, ...);
int nhlog_vf(unsigned char type, unsigned int mask, const char *prog, const char *modu, const char *file, const char *func, int ln, const char *fmt, va_list ap);

#ifdef __cplusplus
}
#endif
#endif /* __K_NHLOG_H__ */

