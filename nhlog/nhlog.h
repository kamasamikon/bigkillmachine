/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_NHLOG_H__
#define __K_NHLOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdarg.h>

#include <hilda/xtcool.h>

#define VAR_UNUSED __attribute__ ((unused))

/*-----------------------------------------------------------------------
 * Embedded variable used by ktrace, nhlog, kerror, kfatal etc
 */
static int VAR_UNUSED __kl_file_name_id_ = -1;
static char VAR_UNUSED *__kl_file_name_ = NULL;

static int VAR_UNUSED __kl_prog_name_id_ = -1;
static char VAR_UNUSED *__kl_prog_name_ = NULL;

static int VAR_UNUSED __kl_modu_name_id_ = -1;


/*-----------------------------------------------------------------------
 * Normal and Raw Logger
 */
typedef void (*KNLOGGER)(const char *content, int len);
typedef void (*KRLOGGER)(unsigned char type, unsigned int mask, const char *prog, const char *modu, const char *file, const char *func, int ln, const char *fmt, va_list ap);


/*-----------------------------------------------------------------------
 * Define KMODU_NAME if someone forgot it.
 */
#ifndef KMODU_NAME
#define KMODU_NAME  "?"
#endif


/*-----------------------------------------------------------------------
 * nhlog switch mask
 */
/* Copied from syslog */
#define nhlog_FATAL      0x00000001 /* 0:f(fatal): system is unusable */
#define nhlog_ALERT      0x00000002 /* 1:a: action must be taken immediately */
#define nhlog_CRIT       0x00000004 /* 2:c: critical conditions */
#define nhlog_ERR        0x00000008 /* 3:e: error conditions */
#define nhlog_WARNING    0x00000010 /* 4:w: warning conditions */
#define nhlog_NOTICE     0x00000020 /* 5:n: normal but significant condition */
#define nhlog_INFO       0x00000040 /* 6:i:l(log): informational */
#define nhlog_DEBUG      0x00000080 /* 7:d:t(trace): debug-level messages */
#define nhlog_TYPE_ALL   0x000000ff

#define nhlog_LOG        nhlog_INFO
#define nhlog_TRC        nhlog_DEBUG

#define nhlog_RTM        0x00000100 /* s: Relative Time, in MS, 'ShiJian' */
#define nhlog_ATM        0x00000200 /* S: ABS Time, in MS, 'ShiJian' */

#define nhlog_PID        0x00001000 /* j: Process ID, 'JinCheng' */
#define nhlog_TID        0x00002000 /* x: Thread ID, 'XianCheng' */

#define nhlog_PROG       0x00010000 /* P: Process Name */
#define nhlog_MODU       0x00020000 /* M: Module Name */
#define nhlog_FILE       0x00040000 /* F: File Name */
#define nhlog_FUNC       0x00080000 /* H: Function Name, 'HanShu' */
#define nhlog_LINE       0x00100000 /* N: Line Number */

#define nhlog_ALL        0xffffffff
#define nhlog_DFT        (nhlog_FATAL | nhlog_ALERT | nhlog_CRIT | nhlog_ERR | nhlog_WARNING | nhlog_NOTICE | nhlog_ATM | nhlog_PROG | nhlog_MODU | nhlog_FILE | nhlog_LINE)


/*-----------------------------------------------------------------------
 * Embedded variable used by ktrace, nhlog, kerror, kfatal etc
 */
#define nhlog_INNER_VAR_DEF() \
	static int VAR_UNUSED __kl_ver_sav = -1; \
	static int VAR_UNUSED __kl_func_name_id = -1; \
	static int VAR_UNUSED __kl_mask = 0; \
	int VAR_UNUSED __kl_ver_get = nhlog_touches();

#define nhlog_SETUP_NAME_AND_ID(modu, file, func) do { \
	if (__kl_file_name_id_ == -1) { \
		__kl_file_name_ = nhlog_get_name_part(file); \
		__kl_file_name_id_ = nhlog_file_name_add(__kl_file_name_); \
	} \
	if (__kl_prog_name_id_ == -1) { \
		__kl_prog_name_ = nhlog_get_prog_name(); \
		__kl_prog_name_id_ = nhlog_prog_name_add(__kl_prog_name_); \
	} \
	if (__kl_modu_name_id_ == -1) \
		__kl_modu_name_id_ = nhlog_modu_name_add(modu); \
	if (__kl_func_name_id == -1) \
		__kl_func_name_id = nhlog_func_name_add(func); \
} while (0)

#define nhlog_CHK_AND_CALL(mask, indi, modu, file, func, line, fmt, ...) do { \
	nhlog_INNER_VAR_DEF(); \
	if (__kl_ver_get > __kl_ver_sav) { \
		__kl_ver_sav = __kl_ver_get; \
		nhlog_SETUP_NAME_AND_ID(modu, file, func); \
		__kl_mask = nhlog_calc_mask(__kl_prog_name_id_, __kl_modu_name_id_, __kl_file_name_id_, __kl_func_name_id, line, (int)spl_process_current()); \
		if (!(__kl_mask & (mask))) \
			__kl_mask = 0; \
	} \
	if (__kl_mask) \
		nhlog_f((indi), __kl_mask, __kl_prog_name_, modu, __kl_file_name_, func, line, fmt, ##__VA_ARGS__); \
} while (0)

#define nhlog_CHK_AND_CALL_AP(mask, indi, modu, file, func, line, fmt, ap) do { \
	nhlog_INNER_VAR_DEF(); \
	if (__kl_ver_get > __kl_ver_sav) { \
		__kl_ver_sav = __kl_ver_get; \
		nhlog_SETUP_NAME_AND_ID(modu, file, func); \
		__kl_mask = nhlog_calc_mask(__kl_prog_name_id_, __kl_modu_name_id_, __kl_file_name_id_, __kl_func_name_id, line, (int)spl_process_current()); \
		if (!(__kl_mask & (mask))) \
			__kl_mask = 0; \
	} \
	if (__kl_mask) \
		nhlog_vf((indi), __kl_mask, __kl_prog_name_, modu, __kl_file_name_, func, line, fmt, ap); \
} while (0)

/*-----------------------------------------------------------------------
 * nhlog, kerror, kfatal etc
 */
#define nhlogs(fmt, ...) do { \
	nhlog_f(0, 0, NULL, NULL, NULL, NULL, 0, fmt, ##__VA_ARGS__); \
} while (0)

#define kfatal(fmt, ...)        nhlog_CHK_AND_CALL(nhlog_FATAL,   'F', KMODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define kalert(fmt, ...)        nhlog_CHK_AND_CALL(nhlog_ALERT,   'A', KMODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define kcritical(fmt, ...)     nhlog_CHK_AND_CALL(nhlog_CRIT,    'C', KMODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define kerror(fmt, ...)        nhlog_CHK_AND_CALL(nhlog_ERR,     'E', KMODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define kwarning(fmt, ...)      nhlog_CHK_AND_CALL(nhlog_WARNING, 'W', KMODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define knotice(fmt, ...)       nhlog_CHK_AND_CALL(nhlog_NOTICE,  'N', KMODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define kinfo(fmt, ...)         nhlog_CHK_AND_CALL(nhlog_INFO,    'I', KMODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define nhlog(fmt, ...)          nhlog_CHK_AND_CALL(nhlog_INFO,    'L', KMODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define kdebug(fmt, ...)        nhlog_CHK_AND_CALL(nhlog_DEBUG,   'D', KMODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define ktrace(fmt, ...)        nhlog_CHK_AND_CALL(nhlog_DEBUG,   'T', KMODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)

#define kassert(_x_) do { \
	if (!(_x_)) { \
		nhlog_INNER_VAR_DEF(); \
		if (__kl_ver_get > __kl_ver_sav) { \
			__kl_ver_sav = __kl_ver_get; \
			nhlog_SETUP_NAME_AND_ID(KMODU_NAME, __FILE__, __func__); \
		} \
		nhlog_f('!', nhlog_ALL, __kl_prog_name_, KMODU_NAME, __kl_file_name_, __FUNCTION__, __LINE__, \
				"\n\tASSERT NG: \"%s\"\n\n", #_x_); \
	} \
} while (0)

/*-----------------------------------------------------------------------
 * Functions:
 */
int nhlog_touches(void) VAR_UNUSED;

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

