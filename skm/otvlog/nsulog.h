/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __NSU_LOG_H__
#define __NSU_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#ifndef nsulog_likely
#define nsulog_likely(x)      __builtin_expect(!!(x), 1)
#endif
#ifndef nsulog_unlikely
#define nsulog_unlikely(x)    __builtin_expect(!!(x), 0)
#endif

/*-----------------------------------------------------------------------
 * Embedded variable used by nsulog_xxx
 */
static char __attribute__((unused)) *__nsul_file_name = NULL;
static char __attribute__((unused)) *__nsul_prog_name = NULL;

/*-----------------------------------------------------------------------
 * Define NSULOG_MODU_NAME if someone forgot it.
 */
#ifndef NSULOG_MODU_NAME
#define NSULOG_MODU_NAME  "?"
#endif

/*-----------------------------------------------------------------------
 * nsulog switch mask
 */
/* Copied from syslog */
#define NSULOG_FATAL      0x00000001 /* 0:f(fatal): system is unusable */
#define NSULOG_ALERT      0x00000002 /* 1:a: action must be taken immediately */
#define NSULOG_CRIT       0x00000004 /* 2:c: critical conditions */
#define NSULOG_ERR        0x00000008 /* 3:e: error conditions */
#define NSULOG_WARNING    0x00000010 /* 4:w: warning conditions */
#define NSULOG_NOTICE     0x00000020 /* 5:n: normal but significant condition */
#define NSULOG_INFO       0x00000040 /* 6:i:l(log): informational */
#define NSULOG_DEBUG      0x00000080 /* 7:d:t(trace): debug-level messages */
#define NSULOG_TYPE_ALL   0x000000ff

#define NSULOG_LOG        NSULOG_INFO
#define NSULOG_TRC        NSULOG_DEBUG

#define NSULOG_RTM        0x00000100 /* s: Relative Time, in MS, 'ShiJian' */
#define NSULOG_ATM        0x00000200 /* S: ABS Time, in MS, 'ShiJian' */

#define NSULOG_PID        0x00001000 /* j: Process ID, 'JinCheng' */
#define NSULOG_TID        0x00002000 /* x: Thread ID, 'XianCheng' */

#define NSULOG_PROG       0x00010000 /* P: Process Name */
#define NSULOG_MODU       0x00020000 /* M: Module Name */
#define NSULOG_FILE       0x00040000 /* F: File Name */
#define NSULOG_FUNC       0x00080000 /* H: Function Name, 'HanShu' */
#define NSULOG_LINE       0x00100000 /* N: Line Number */

#define NSULOG_ALL        0xffffffff
#define NSULOG_DFT        (NSULOG_FATAL | NSULOG_ALERT | NSULOG_CRIT | NSULOG_ERR | NSULOG_WARNING | NSULOG_NOTICE | NSULOG_ATM | NSULOG_PROG | NSULOG_MODU | NSULOG_FILE | NSULOG_LINE)

/*-----------------------------------------------------------------------
 * Embedded variable used by nsulog_xxx
 */
#define NSULOG_INNER_VAR_DEF() \
	static int __attribute__((unused)) __nsul_ver_sav = -1; \
	static char __attribute__((unused)) *__nsul_modu_name = NULL; \
	static char __attribute__((unused)) *__nsul_func_name = NULL; \
	static int __attribute__((unused)) __nsul_mask = 0; \
	int __attribute__((unused)) __nsul_ver_get = nsulog_touches()

#define NSULOG_SETUP_NAME(modu, file, func) do { \
	if (nsulog_unlikely(__nsul_file_name == NULL)) { \
		__nsul_file_name = nsulog_file_name_add((char*)file); \
	} \
	if (nsulog_unlikely(__nsul_prog_name == NULL)) { \
		__nsul_prog_name = nsulog_prog_name_add(NULL); \
	} \
	if (nsulog_unlikely(__nsul_modu_name == NULL)) { \
		__nsul_modu_name = nsulog_modu_name_add((char*)modu); \
	} \
	if (nsulog_unlikely(__nsul_func_name == NULL)) { \
		__nsul_func_name = nsulog_func_name_add((char*)func); \
	} \
} while (0)

#define NSULOG_CHK_AND_CALL(mask, indi, modu, file, func, line, fmt, ...) do { \
	NSULOG_INNER_VAR_DEF(); \
	if (nsulog_unlikely(__nsul_ver_get > __nsul_ver_sav)) { \
		__nsul_ver_sav = __nsul_ver_get; \
		NSULOG_SETUP_NAME(modu, file, func); \
		__nsul_mask = nsulog_calc_mask(__nsul_prog_name, __nsul_modu_name, __nsul_file_name, __nsul_func_name, line); \
		if (!(__nsul_mask & (mask))) { \
			__nsul_mask = 0; \
		} \
	} \
	if (__nsul_mask) { \
		nsulog_f(indi, __nsul_mask, __nsul_prog_name, modu, __nsul_file_name, (char*)func, line, fmt, ##__VA_ARGS__); \
	} \
} while (0)

#define NSULOG_CHK_AND_CALL_AP(mask, indi, modu, file, func, line, fmt, ap) do { \
	NSULOG_INNER_VAR_DEF(); \
	if (nsulog_unlikely(__nsul_ver_get > __nsul_ver_sav)) { \
		__nsul_ver_sav = __nsul_ver_get; \
		NSULOG_SETUP_NAME(modu, file, func); \
		__nsul_mask = nsulog_calc_mask(__nsul_prog_name, __nsul_modu_name, __nsul_file_name, __nsul_func_name, line); \
		if (!(__nsul_mask & (mask))) { \
			__nsul_mask = 0; \
		} \
	} \
	if (__nsul_mask) { \
		nsulog_vf(indi, __nsul_mask, __nsul_prog_name, modu, __nsul_file_name, (char*)func, line, fmt, ap); \
	} \
} while (0)

/*-----------------------------------------------------------------------
 * nsulog_xxx
 */
#define nsulogs(fmt, ...) do { \
	nsulog_f(0, 0, NULL, NULL, NULL, NULL, 0, fmt, ##__VA_ARGS__); \
} while (0)

#define nsulog_fatal(fmt, ...)       NSULOG_CHK_AND_CALL(NSULOG_FATAL,   'F', NSULOG_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define nsulog_alert(fmt, ...)       NSULOG_CHK_AND_CALL(NSULOG_ALERT,   'A', NSULOG_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define nsulog_critical(fmt, ...)    NSULOG_CHK_AND_CALL(NSULOG_CRIT,    'C', NSULOG_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define nsulog_error(fmt, ...)       NSULOG_CHK_AND_CALL(NSULOG_ERR,     'E', NSULOG_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define nsulog_warning(fmt, ...)     NSULOG_CHK_AND_CALL(NSULOG_WARNING, 'W', NSULOG_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define nsulog_notice(fmt, ...)      NSULOG_CHK_AND_CALL(NSULOG_NOTICE,  'N', NSULOG_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define nsulog_info(fmt, ...)        NSULOG_CHK_AND_CALL(NSULOG_INFO,    'I', NSULOG_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define nsulog_debug(fmt, ...)       NSULOG_CHK_AND_CALL(NSULOG_DEBUG,   'D', NSULOG_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)

#define nsulog_assert(_x_) do { \
	if (!(_x_)) { \
		NSULOG_INNER_VAR_DEF(); \
		if (__nsul_ver_get > __nsul_ver_sav) { \
			__nsul_ver_sav = __nsul_ver_get; \
			NSULOG_SETUP_NAME(NSULOG_MODU_NAME, __FILE__, __func__); \
		} \
		nsulog_f('!', NSULOG_ALL, __nsul_prog_name, NSULOG_MODU_NAME, __nsul_file_name, (char*)__FUNCTION__, __LINE__, \
				"\n\tASSERT NG: \"%s\"\n\n", #_x_); \
	} \
} while (0)

/*-----------------------------------------------------------------------
 * Functions:
 */
int nsulog_touches(void);

char *nsulog_file_name_add(char *name);
char *nsulog_modu_name_add(char *name);
char *nsulog_prog_name_add(char *name);
char *nsulog_func_name_add(char *name);

unsigned int nsulog_calc_mask(char *prog, char *modu, char *file, char *func, int line);

int nsulog_f(unsigned char type, unsigned int mask, char *prog, char *modu, char *file, char *func, int ln, const char *fmt, ...) __attribute__ ((format (printf, 8, 9)));
int nsulog_vf(unsigned char type, unsigned int mask, char *prog, char *modu, char *file, char *func, int ln, const char *fmt, va_list ap);


#ifdef __cplusplus
}
#endif
#endif /* __NSU_LOG_H__ */

