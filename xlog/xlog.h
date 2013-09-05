/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __K_LOG_H__
#define __K_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sysdeps.h>

inline int klog_ver();
void parse_flg(const char *flg, unsigned int *set, unsigned int *clr);

#define KLOG_TRC         0x00000001 /* t: Trace */
#define KLOG_LOG         0x00000002 /* l: Log */
#define KLOG_ERR         0x00000004 /* e: Error */
#define KLOG_FAT         0x00000008 /* f: Fatal Error */
#define KLOG_TYPE_ALL    0x0000000f

#define KLOG_RTM         0x00000100 /* s: Relative Time, in MS, 'ShiJian' */
#define KLOG_ATM         0x00000200 /* S: ABS Time, in MS, 'ShiJian' */

#define KLOG_PID         0x00001000 /* j: Process ID, 'JinCheng' */
#define KLOG_TID         0x00002000 /* x: Thread ID, 'XianCheng' */

#define KLOG_PROG        0x00010000 /* P: Process Name */
#define KLOG_MODU        0x00020000 /* M: Module Name */
#define KLOG_FILE        0x00040000 /* F: File Name */
#define KLOG_FUNC        0x00080000 /* H: Function Name, 'HanShu' */
#define KLOG_LINE        0x00100000 /* N: Line Number */

static int VAR_UNUSED __file_name_id = -1;
static char VAR_UNUSED *__file_name = NULL;

static int VAR_UNUSED __prog_name_id = -1;
static char VAR_UNUSED *__prog_name = NULL;

static int VAR_UNUSED __modu_name_id = -1;

#ifndef MODU_NAME
#define MODU_NAME  "?"
#endif


#define klogs(fmt, ...) do { \
	rlogf(0, 0, NULL, NULL, NULL, NULL, NULL, fmt, ##__VA_ARGS__); \
} while (0)

#define KLOG_INNER_VAR_DEF() \
	static int ver_sav = -1; \
	static int func_name_id = -1; \
	static int flg = 0; \
	int ver_get = klog_ver()

#define KLOG_SETUP_NAME_AND_ID() do { \
	if (__file_name_id == -1) { \
		__file_name = klog_get_name_part(__FILE__); \
		__file_name_id = klog_file_name_add(__file_name); \
	} \
	if (__prog_name_id == -1) { \
		__prog_name = klog_get_prog_name(); \
		__prog_name_id = klog_prog_name_add(__prog_name); \
	} \
	if (__modu_name_id == -1) { \
		__modu_name_id = klog_modu_name_add(MODU_NAME); \
	} \
	if (func_name_id == -1) { \
		func_name_id = klog_func_name_add(__func__); \
	} \
} while (0)

#define klog(fmt, ...) do { \
	KLOG_INNER_VAR_DEF(); \
	if (ver_get > ver_sav) { \
		ver_sav = ver_get; \
		KLOG_SETUP_NAME_AND_ID(); \
		flg = klog_calc_flg(__prog_name_id, __modu_name_id, __file_name_id, func_name_id, __LINE__, getpid()); \
		if (!(flg & KLOG_LOG)) \
			flg = 0; \
	} \
	if (flg) \
		rlogf('L', flg, __prog_name, MODU_NAME, __file_name, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
} while (0)

char *klog_get_name_part(char *name);
char *klog_get_prog_name();

int klog_file_name_add(const char *name);
int klog_modu_name_add(const char *name);
int klog_prog_name_add(const char *name);
int klog_func_name_add(const char *name);

void klog_rule_add(const char *rule);

void klog_parse_flg(const char *flg, unsigned int *set, unsigned int *clr);
unsigned int klog_calc_flg(int prog, int modu, int file, int func, int line, int pid);

int rlogf(unsigned char type, unsigned int flg, const char *prog, const char *modu, const char *file, const char *func, int ln, const char *fmt, ...);

#ifdef __cplusplus
}
#endif
#endif /* __K_LOG_H__ */

