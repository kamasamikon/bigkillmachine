/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */


/**
 * @file     dalog.h
 * @author   Yu Nemo Wenbin
 * @version  1.0
 * @date     Sep, 2014
 */

#ifndef __N_LIB_LOG_H__
#define __N_LIB_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#ifndef dalog_likely
#define dalog_likely(x)      __builtin_expect(!!(x), 1)
#endif
#ifndef dalog_unlikely
#define dalog_unlikely(x)    __builtin_expect(!!(x), 0)
#endif

/*-----------------------------------------------------------------------
 * Embedded variable used by dalog_xxx
 */
static char __attribute__((unused)) *__dal_file_name = NULL;
static char __attribute__((unused)) *__dal_prog_name = NULL;

/*-----------------------------------------------------------------------
 * Normal and Raw Logger
 */
typedef void (*DAL_NLOGGER)(char *content, int len);
typedef void (*DAL_RLOGGER)(unsigned char type, unsigned int mask, char *prog, char *modu, char *file, char *func, int ln, const char *fmt, va_list ap);

/*-----------------------------------------------------------------------
 * Define DALOG_MODU_NAME if someone forgot it.
 */
#ifndef DALOG_MODU_NAME
#define DALOG_MODU_NAME  "?"
#endif

/*-----------------------------------------------------------------------
 * dalog switch mask
 */
/* Copied from syslog */
#define DALOG_FATAL      0x00000001 /* 0:f(fatal): system is unusable */
#define DALOG_ALERT      0x00000002 /* 1:a: action must be taken immediately */
#define DALOG_CRIT       0x00000004 /* 2:c: critical conditions */
#define DALOG_ERR        0x00000008 /* 3:e: error conditions */
#define DALOG_WARNING    0x00000010 /* 4:w: warning conditions */
#define DALOG_NOTICE     0x00000020 /* 5:n: normal but significant condition */
#define DALOG_INFO       0x00000040 /* 6:i:l(log): informational */
#define DALOG_DEBUG      0x00000080 /* 7:d:t(trace): debug-level messages */
#define DALOG_TYPE_ALL   0x000000ff

#define DALOG_LOG        DALOG_INFO
#define DALOG_TRC        DALOG_DEBUG

#define DALOG_RTM        0x00000100 /* s: Relative Time, in MS, 'ShiJian' */
#define DALOG_ATM        0x00000200 /* S: ABS Time, in MS, 'ShiJian' */

#define DALOG_PID        0x00001000 /* j: Process ID, 'JinCheng' */
#define DALOG_TID        0x00002000 /* x: Thread ID, 'XianCheng' */

#define DALOG_PROG       0x00010000 /* P: Process Name */
#define DALOG_MODU       0x00020000 /* M: Module Name */
#define DALOG_FILE       0x00040000 /* F: File Name */
#define DALOG_FUNC       0x00080000 /* H: Function Name, 'HanShu' */
#define DALOG_LINE       0x00100000 /* N: Line Number */

#define DALOG_ALL        0xffffffff
#define DALOG_DFT        (DALOG_FATAL | DALOG_ALERT | DALOG_CRIT | DALOG_ERR | DALOG_WARNING | DALOG_NOTICE | DALOG_ATM | DALOG_PROG | DALOG_MODU | DALOG_FILE | DALOG_LINE)

/*-----------------------------------------------------------------------
 * Embedded variable used by dalog_xxx
 */
#define DALOG_INNER_VAR_DEF() \
	static int __attribute__((unused)) __dal_ver_sav = -1; \
	static char __attribute__((unused)) *__dal_modu_name = NULL; \
	static char __attribute__((unused)) *__dal_func_name = NULL; \
	static int __attribute__((unused)) __dal_mask = 0; \
	int __attribute__((unused)) __dal_ver_get = dalog_touches()

#define DALOG_SETUP_NAME(modu, file, func) do { \
	if (dalog_unlikely(__dal_file_name == NULL)) { \
		__dal_file_name = dalog_file_name_add((char*)file); \
	} \
	if (dalog_unlikely(__dal_prog_name == NULL)) { \
		__dal_prog_name = dalog_prog_name_add(NULL); \
	} \
	if (dalog_unlikely(__dal_modu_name == NULL)) { \
		__dal_modu_name = dalog_modu_name_add((char*)modu); \
	} \
	if (dalog_unlikely(__dal_func_name == NULL)) { \
		__dal_func_name = dalog_func_name_add((char*)func); \
	} \
} while (0)

#define DALOG_CHK_AND_CALL(mask, indi, modu, file, func, line, fmt, ...) do { \
	DALOG_INNER_VAR_DEF(); \
	if (dalog_unlikely(__dal_ver_get > __dal_ver_sav)) { \
		__dal_ver_sav = __dal_ver_get; \
		DALOG_SETUP_NAME(modu, file, func); \
		__dal_mask = dalog_calc_mask(__dal_prog_name, __dal_modu_name, __dal_file_name, __dal_func_name, line); \
		if (!(__dal_mask & (mask))) { \
			__dal_mask = 0; \
		} \
	} \
	if (__dal_mask) { \
		dalog_f(indi, __dal_mask, __dal_prog_name, modu, __dal_file_name, (char*)func, line, fmt, ##__VA_ARGS__); \
	} \
} while (0)

#define DALOG_CHK_AND_CALL_AP(mask, indi, modu, file, func, line, fmt, ap) do { \
	DALOG_INNER_VAR_DEF(); \
	if (dalog_unlikely(__dal_ver_get > __dal_ver_sav)) { \
		__dal_ver_sav = __dal_ver_get; \
		DALOG_SETUP_NAME(modu, file, func); \
		__dal_mask = dalog_calc_mask(__dal_prog_name, __dal_modu_name, __dal_file_name, __dal_func_name, line); \
		if (!(__dal_mask & (mask))) { \
			__dal_mask = 0; \
		} \
	} \
	if (__dal_mask) { \
		dalog_vf(indi, __dal_mask, __dal_prog_name, modu, __dal_file_name, (char*)func, line, fmt, ap); \
	} \
} while (0)

/*-----------------------------------------------------------------------
 * dalog_xxx
 */
#define dalogs(fmt, ...) do { \
	dalog_f(0, 0, NULL, NULL, NULL, NULL, 0, fmt, ##__VA_ARGS__); \
} while (0)

#define dalog_fatal(fmt, ...)       DALOG_CHK_AND_CALL(DALOG_FATAL,   'F', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define dalog_alert(fmt, ...)       DALOG_CHK_AND_CALL(DALOG_ALERT,   'A', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define dalog_critical(fmt, ...)    DALOG_CHK_AND_CALL(DALOG_CRIT,    'C', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define dalog_error(fmt, ...)       DALOG_CHK_AND_CALL(DALOG_ERR,     'E', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define dalog_warning(fmt, ...)     DALOG_CHK_AND_CALL(DALOG_WARNING, 'W', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define dalog_notice(fmt, ...)      DALOG_CHK_AND_CALL(DALOG_NOTICE,  'N', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define dalog_info(fmt, ...)        DALOG_CHK_AND_CALL(DALOG_INFO,    'I', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#define dalog_debug(fmt, ...)       DALOG_CHK_AND_CALL(DALOG_DEBUG,   'D', DALOG_MODU_NAME, __FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)

#define dalog_assert(_x_) do { \
	if (!(_x_)) { \
		DALOG_INNER_VAR_DEF(); \
		if (__dal_ver_get > __dal_ver_sav) { \
			__dal_ver_sav = __dal_ver_get; \
			DALOG_SETUP_NAME(DALOG_MODU_NAME, __FILE__, __func__); \
		} \
		dalog_f('!', DALOG_ALL, __dal_prog_name_, DALOG_MODU_NAME, __dal_file_name_, __FUNCTION__, __LINE__, \
				"\n\tASSERT NG: \"%s\"\n\n", #_x_); \
	} \
} while (0)

/*-----------------------------------------------------------------------
 * Functions:
 */
int dalog_touches(void);

char *dalog_file_name_add(char *name);
char *dalog_modu_name_add(char *name);
char *dalog_prog_name_add(char *name);
char *dalog_func_name_add(char *name);

unsigned int dalog_calc_mask(char *prog, char *modu, char *file, char *func, int line);

int dalog_f(unsigned char type, unsigned int mask, char *prog, char *modu, char *file, char *func, int ln, const char *fmt, ...) __attribute__ ((format (printf, 8, 9)));
int dalog_vf(unsigned char type, unsigned int mask, char *prog, char *modu, char *file, char *func, int ln, const char *fmt, va_list ap);

int dalog_add_logger(DAL_NLOGGER logger);
int dalog_del_logger(DAL_NLOGGER logger);

int dalog_add_rlogger(DAL_RLOGGER logger);
int dalog_del_rlogger(DAL_RLOGGER logger);

void *dalog_attach(void *logcc);
void dalog_touch(void);

void dalog_set_default_mask(unsigned int mask);
void *dalog_init(int argc, char **argv);

void dalog_rule_add(char *rule);
void dalog_rule_del(unsigned int idx);
void dalog_rule_clr(void);

#ifdef __cplusplus
}
#endif
#endif /* __N_LIB_LOG_H__ */

