/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

/* rlog: Remote logger */

#ifndef __K_LOG_H__
#define __K_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sysdeps.h>

#include <stdio.h>

#include <ktypes.h>
#include <xtcool.h>

/*----------------------------------------------------------------------------
 * debug message mask
 */
#define LOG_ALL         0xffffffff

#define LOG_LOG         0x00000001 /* l: Log */
#define LOG_ERR         0x00000002 /* e: Error */
#define LOG_FAT         0x00000004 /* f: Fatal Error */
#define LOG_TYPE_ALL    0x0000000f

#define LOG_RTM         0x00000100 /* t: Relative Time, in MS */
#define LOG_ATM         0x00000200 /* T: ABS Time, in MS */

#define LOG_PID         0x00001000 /* p: Process ID */
#define LOG_TID         0x00002000 /* P: Thread ID */

#define LOG_LINE        0x00010000 /* N: Line Number */
#define LOG_FILE        0x00020000 /* F: File Name */
#define LOG_MODU        0x00040000 /* M: Module Name */

/*
 * String format or VA format
 */
/* content ends with NUL, len should be equal to strlen(content) */
extern int rlog_s(const char *content, int len);
/* modu = MODULE, file = FileName, ln = Line */
extern int rlog_v(unsigned char type, unsigned int flg, const char *modu, const char *file, int ln, const char *fmt, va_list ap);

#if defined(CFG_RLOG_DO_NOTHING)
#define rlog_init(deflev, argc, argv) do {} while (0)
#define rlog_attach(logcc) do {} while (0)
#define rlog_getflg(file) 0

#define LOG_HIDE_LOG    1
#define LOG_HIDE_ERR    1
#define LOG_HIDE_FAT    1
#define LOG_HIDE_ASSERT 1

#else
void *rlog_init(unsigned int deflev, int argc, char **argv);
kinline void *rlog_cc(void);
void *rlog_attach(void *logcc);

unsigned int rlog_getflg(const char *file);
void rlog_setflg(const char *cmd);
kinline int rlog_touches(void);

static int VAR_UNUSED __g_rlog_touches = -1;
static unsigned int VAR_UNUSED __gc_rlog_level;

/* RLOG Full */
/* modu = MODULE, file = FileName, ln = Line */
int rlogf(unsigned char type, unsigned int flg, const char *modu, const char *file, int ln, const char *fmt, ...);

/* RLOG Short */
#define rlogs(fmt, ...) do { \
	rlogf(0, 0, NULL, NULL, 0, fmt, ##__VA_ARGS__); \
} while (0)

#define GET_LOG_LEVEL() do { \
	int touches = rlog_touches(); \
	if (__g_rlog_touches < touches) { \
		__g_rlog_touches = touches; \
		__gc_rlog_level = rlog_getflg((const char*)__FILE__); \
	} \
} while (0)

#if defined(LOG_HIDE_LOG) || defined(LOG_HIDE_CUR_LOG)
#define rlog(fmt, ...) do {} while (0)
#else
#ifdef RLOG_MODU
#define rlog(fmt, ...) do { \
	GET_LOG_LEVEL(); \
	if (__gc_rlog_level & LOG_LOG) { \
		rlogf('L', __gc_rlog_level, RLOG_MODU, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
	} \
} while (0)
#else
#define rlog(fmt, ...) do { \
	GET_LOG_LEVEL(); \
	if (__gc_rlog_level & LOG_LOG) { \
		rlogf('L', __gc_rlog_level, NULL, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
	} \
} while (0)
#endif
#endif

#if defined(LOG_HIDE_ERR) || defined(LOG_HIDE_CUR_ERR)
#define kerror(fmt, ...) do {} while (0)
#else
#ifdef RLOG_MODU
#define kerror(fmt, ...) do { \
	GET_LOG_LEVEL(); \
	if (__gc_rlog_level & LOG_ERR) { \
		rlogf('E', __gc_rlog_level, RLOG_MODU, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
	} \
} while (0)
#else
#define kerror(fmt, ...) do { \
	GET_LOG_LEVEL(); \
	if (__gc_rlog_level & LOG_ERR) { \
		rlogf('E', __gc_rlog_level, NULL, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
	} \
} while (0)
#endif
#endif

#if defined(LOG_HIDE_FAT) || defined(LOG_HIDE_CUR_FAT)
#define kfatal(fmt, ...) do {} while (0)
#else
#ifdef RLOG_MODU
#define kfatal(fmt, ...) do { \
	GET_LOG_LEVEL(); \
	if (__gc_rlog_level & LOG_FAT) { \
		rlogf('F', __gc_rlog_level, RLOG_MODU, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
	} \
} while (0)
#else
#define kfatal(fmt, ...) do { \
	GET_LOG_LEVEL(); \
	if (__gc_rlog_level & LOG_FAT) { \
		rlogf('F', __gc_rlog_level, NULL, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
	} \
} while (0)
#endif
#endif

#if defined(LOG_HIDE_ASSERT) || defined(LOG_HIDE_ASSERT)
#define kassert(x) do {} while (0)
#else
#ifdef RLOG_MODU
#define kassert(_x_) \
	do { \
		if (!(_x_)) { \
			rlogf(0, 0, RLOG_MODU, __FILE__, __LINE__, "\n\tKASSERT FAILED: [%s]\n\n", #_x_); \
			/* rlogbrk(); kbacktrace(); */ \
		} \
	} while (0)
#else
#define kassert(_x_) \
	do { \
		if (!(_x_)) { \
			rlogf(0, 0, NULL, __FILE__, __LINE__, "\n\tKASSERT FAILED: [%s]\n\n", #_x_); \
			/* rlogbrk(); kbacktrace(); */ \
		} \
	} while (0)
#endif
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif /* __K_LOG_H__ */

