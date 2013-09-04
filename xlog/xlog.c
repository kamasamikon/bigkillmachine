/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>

#include <helper.h>
#include <kmem.h>

/*-----------------------------------------------------------------------
 * xlog.h part
 */

inline int klog_ver();

/*
 * FILE_NAME
 * MODU_NAME
 * PROC_ID
 */

/* __FILE__ 的名字都比较奇怪，应该去掉路径部分，只保留文件名 */
static int __file_name_id = -1;
static char *__file_name = NULL;

/* 进程的名字可能不总是需要，所以没有必要总是算这个 */
static int __prog_name_id = -1;
static char *__prog_name = NULL;

/* 模块的名字是需要的 */
static int __modu_name_id = -1;
#if (defined(O_LOG_COMPNAME))
    #define COMP_NAME  O_LOG_COMPNAME
#elif (defined(PACKAGE_NAME))
    #define COMP_NAME  PACKAGE_NAME
#else
    #define COMP_NAME  "?"
#endif

#define MODU_NAME COMP_NAME

static int file_nr, modu_nr, prog_nr;

#define INNER_VAR_DEF() \
	static int ver_sav = -1; \
	static int func_name_id = -1; \
	int ver_get = klog_ver()

#define SETUP_NAME_AND_ID() do { \
	if (__file_name_id != -1) { \
		__file_name = only_name(__FILE__); \
		__file_name_id = file_name_add(__file_name); \
	} \
	if (__prog_name_id != -1) { \
		__prog_name = prog_name(); \
		__prog_name_id = prog_name_add(__prog_name); \
	} \
	if (__modu_name_id != -1) { \
		__modu_name = MODU_NAME; \
		__modu_name_id = modu_name_add(__modu_name); \
	} \
	if (func_name_id != -1) { \
		func_name_id = func_name_add(__func__); \
	} \
} while (0)

int rlogf(unsigned char type, unsigned int flg, const char *modu, const char *file, int ln, const char *fmt, ...);

#define klog(fmt, ...) do { \
	INNER_VAR_DEF(); \
	int flg; \
	if (ver_get < ver_sav) { \
		ver_sav = ver_get; \
		SETUP_NAME_AND_ID(); \
		flg = klog_calc_flg(__prog_name_id, __modu_name_id, __file_name_id, func_name_id, getpid(), __LINE__); \
		showme = ???; \
	} \
	if (flg & LOG_LOG) \
		klogf('L', flg, __prog_name, __modu_name, __file_name, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
} while (0)

char *only_name(char *name)
{
	char *dup = strdup(name);
	char *bn = basename(dup);

	free(dup);
	if (bn)
		bn = "";
	return strdup(bn);
}

char *prog_name()
{
	static char *prog_name = NULL;
	char buf[256];
	FILE *fp;

	if (!prog_name) {
		sprintf(buf, "/proc/%d/cmdline", getpid());
		fp = fopen(buf, "rt");
		if (fp) {
			fread(buf, sizeof(char), sizeof(buf), fp);
			fclose(fp);
			prog_name = only_name(buf);
		}
	}
	return prog_name;
}

inline int klog_ver()
{
	return 2;
}

struct _name_arr_t {
	char *name;
	int nr;
};

typedef struct _rule_t rule_t;
struct _rule_t {
	/* XXX: Don't filter ThreadID, it need the getflg called every log */

	/* 0 is know care */
	/* -1 is all */

	int program_name;
	int module_name;
	int file_name;
	int function_name;

	int line_number;

	int pid;
	int tid;

	/* After parse =leftXXX */
	unsigned int flag;
};


typedef struct _strarr_t strarr_t;
struct _strarr_t {
	int size;
	int cnt;
	char **arr;
};

/*-----------------------------------------------------------------------
 * xlog.c part
 */
static strarr_t sa_file_name = { 0 };
static strarr_t sa_modu_name = { 0 };
static strarr_t sa_prog_name = { 0 };
static strarr_t sa_func_name = { 0 };

static int strarr_find(strarr_t *sa, const char *str)
{
	int i;

	for (i = 0; i < sa->cnt; i++)
		if (!strcmp(sa->arr[i], str))
			return i;
	return -1;
}

static int strarr_add(strarr_t *sa, const char *str)
{
	int pos = strarr_find(sa, str);

	if (pos != -1)
		return pos;

	if (sa->cnt >= sa->size)
		ARR_INC(10, sa->arr, sa->size, char*);

	sa->arr[sa->cnt] = strdup(str);
	sa->cnt++;

	/* Return the position been inserted */
	return sa->cnt - 1;
}

int file_name_add(const char *name)
{
	return strarr_add(&sa_file_name, name);
}
int file_name_find(const char *name)
{
	return strarr_find(&sa_file_name, name);
}
int modu_name_add(const char *name)
{
	return strarr_add(&sa_modu_name, name);
}
int modu_name_find(const char *name)
{
	return strarr_find(&sa_modu_name, name);
}
int prog_name_add(const char *name)
{
	return strarr_add(&sa_prog_name, name);
}
int prog_name_find(const char *name)
{
	return strarr_find(&sa_prog_name, name);
}
int func_name_add(const char *name)
{
	return strarr_add(&sa_func_name, name);
}
int func_name_find(const char *name)
{
	return strarr_find(&sa_func_name, name);
}


typedef struct _rulearr_t rulearr_t;
struct _rulearr_t {
	int size;
	int cnt;
	rule_t *arr;
};

static rulearr_t rulearr = { 0 };

static int rulearr_add(rulearr_t *ra, int program_name, int module_name,
		int file_name, int function_name, int line_number, int pid)
{
	if (ra->cnt >= ra->size)
		ARR_INC(10, ra->arr, ra->size, rule_t*);

	rule_t *rule = &ra->arr[ra->cnt];

	rule->program_name = program_name;
	rule->module_name = module_name;
	rule->file_name = file_name;
	rule->function_name = function_name;
	rule->line_number = line_number;
	rule->pid = pid;

	ra->cnt++;

	/* Return the position been inserted */
	return ra->cnt - 1;
}

void rule_add(const char *rule)
{
	int program_name;
	int module_name;
	int file_name;
	int function_name;
	int line_number;
	int pid;

	const char *tmp = rule;

	tmp = strchr(tmp, '|');
	program_name = atoi(tmp);

	tmp = strchr(tmp, '|') + 1;
	module_name = atoi(tmp);

	tmp = strchr(tmp, '|') + 1;
	file_name = atoi(tmp);

	tmp = strchr(tmp, '|') + 1;
	function_name = atoi(tmp);

	tmp = strchr(tmp, '|') + 1;
	line_number = atoi(tmp);

	tmp = strchr(tmp, '|') + 1;
	pid = atoi(tmp);

	tmp = strchr(tmp, '=') + 1;

	/* OK, parse the flag into int */
	unsigned int set = 0, clr = 0;
	parse_flg(tmp + 1, &set, &clr);

	rulearr_add(rulearr, program_name, module_name, file_name, function_name, line_number, pid);
}

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
#define LOG_FUNC        0x00080000 /* H: Function Name, 'HanShu' */

typedef struct _flgmap_t flgmap_t;
struct _flgmap_t {
	char code;
	unsigned int bit;
};

flgmap_t flgmap[] = {
	{ 'l', LOG_LOG },
	{ 'e', LOG_ERR },
	{ 'f', LOG_FAT },

	{ 't', LOG_RTM },
	{ 'T', LOG_ATM },

	{ 'p', LOG_PID },
	{ 'P', LOG_TID },

	{ 'N', LOG_LINE },
	{ 'F', LOG_FILE },
	{ 'M', LOG_MODU },
	{ 'H', LOG_FUNC },
};

unsigned int get_flg(char c)
{
	int i;

	for (i = 0; i < sizeof(flgmap) / sizeof(flgmap[0]); i++)
		if (flgmap[i].code == c)
			return flgmap[i].bit;
	return 0;
}

void parse_flg(const char *flg, unsigned int *set, unsigned int *clr)
{
	int i = 0, len = strlen(flg);
	char c;

	*set = 0;
	*clr = 0;

	for (i = 0; i < len; i++) {
		c = flg[i];
		if (c == '-') {
			c = flg[++i];
			*clr |= get_flg(c);
		} else
			*set |= get_flg(c);
	}
}

int klog_calc_flg(int program_name, int module_name, int file_name, int function_name, int line_number, int pid, int tid)
{
	int i;

	unsigned int all, set, clr;

	/* 0 means ignore */

	// PID, TID, 命令行

	// |程序|模块|PID|TID|file|func|line=left
	// /usr/bin/shit|shit.c|SHIT|3423|45234|455=leftAM

	/* shit.c 的第100行 */
	// NULL|shit.c|NULL|3423|0|100=leftAM
	//

	// 初始化的时候，命令行就已经拿到了
	// 文件名应该格式化一下，

	for (i = 0; i < MAX; i++) {
		if (fname)
			if (fname == rule[i].name)
				shit;
	}
}

/*-----------------------------------------------------------------------
 * main.c part
 */

