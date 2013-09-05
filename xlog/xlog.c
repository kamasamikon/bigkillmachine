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
void parse_flg(const char *flg, unsigned int *set, unsigned int *clr);

#define KLOG_LOG         0x00000001 /* l: Log */
#define KLOG_ERR         0x00000002 /* e: Error */
#define KLOG_FAT         0x00000004 /* f: Fatal Error */
#define KLOG_TYPE_ALL    0x0000000f

#define KLOG_RTM         0x00000100 /* t: Relative Time, in MS */
#define KLOG_ATM         0x00000200 /* T: ABS Time, in MS */

#define KLOG_PID         0x00001000 /* p: Process ID */
#define KLOG_TID         0x00002000 /* P: Thread ID */

#define KLOG_LINE        0x00010000 /* N: Line Number */
#define KLOG_FILE        0x00020000 /* F: File Name */
#define KLOG_MODU        0x00040000 /* M: Module Name */
#define KLOG_FUNC        0x00080000 /* H: Function Name, 'HanShu' */

/* __FILE__ 的名字都比较奇怪，应该去掉路径部分，只保留文件名 */
static int __file_name_id = -1;
static char *__file_name = NULL;

/* 进程的名字可能不总是需要，所以没有必要总是算这个 */
static int __prog_name_id = -1;
static char *__prog_name = NULL;

/* 模块的名字是需要的 */
static int __modu_name_id = -1;

#define O_LOG_COMPNAME "XLOG"

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
	static int flg = 0; \
	int ver_get = klog_ver()

#define SETUP_NAME_AND_ID() do { \
	if (__file_name_id == -1) { \
		__file_name = only_name(__FILE__); \
		__file_name_id = file_name_add(__file_name); \
	} \
	if (__prog_name_id == -1) { \
		__prog_name = get_prog_name(); \
		__prog_name_id = prog_name_add(__prog_name); \
	} \
	if (__modu_name_id == -1) { \
		__modu_name_id = modu_name_add(MODU_NAME); \
	} \
	if (func_name_id == -1) { \
		func_name_id = func_name_add(__func__); \
	} \
} while (0)

int rlogf(unsigned char type, unsigned int flg, const char *prog, const char *modu, const char *file, const char *func, int ln, const char *fmt, ...)
{
	printf("type: %c\n", type);
	printf("flag: %x\n", flg);
	printf("prog: %s\n", prog);
	printf("modu: %s\n", modu);
	printf("file: %s\n", file);
	printf("func: %s\n", func);
	printf("line: %d\n", ln);
	printf("fmt: %s\n", fmt);
	return 0;
}

#define klog(fmt, ...) do { \
	INNER_VAR_DEF(); \
	if (ver_get > ver_sav) { \
		ver_sav = ver_get; \
		SETUP_NAME_AND_ID(); \
		flg = klog_calc_flg(__prog_name_id, __modu_name_id, __file_name_id, func_name_id, __LINE__, getpid()); \
		if (!(flg & KLOG_LOG)) \
			flg = 0; \
	} \
	if (flg) \
		rlogf('L', flg, __prog_name, MODU_NAME, __file_name, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__); \
} while (0)

char *only_name(char *name)
{
	char *dup = strdup(name);
	char *bn = basename(dup);

	bn = bn ? strdup(bn) : strdup("");

	free(dup);

	return bn;
}

char *get_prog_name()
{
	static char *pname = NULL;
	char buf[256];
	FILE *fp;

	if (!pname) {
		sprintf(buf, "/proc/%d/cmdline", getpid());
		fp = fopen(buf, "rt");
		if (fp) {
			fread(buf, sizeof(char), sizeof(buf), fp);
			fclose(fp);
			pname = only_name(buf);
		}
	}
	return pname;
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

	int prog;	/* Program command line */
	int modu;	/* Module name */
	int file;	/* File name */
	int func;	/* Function name */

	int line;	/* Line number */

	int pid;	/* Process ID */

	/* After parse =leftXXX */
	unsigned int set, clr;
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

static int rulearr_add(rulearr_t *ra, int prog, int modu,
		int file, int func, int line, int pid,
		unsigned int fset, unsigned int fclr)
{
	if (ra->cnt >= ra->size)
		ARR_INC(10, ra->arr, ra->size, rule_t*);

	rule_t *rule = &ra->arr[ra->cnt];

	rule->prog = prog;
	rule->modu = modu;
	rule->file = file;
	rule->func = func;
	rule->line = line;
	rule->pid = pid;

	rule->set = fset;
	rule->clr = fclr;

	ra->cnt++;

	/* Return the position been inserted */
	return ra->cnt - 1;
}

/*
 * rule =
 * prog | modu | file | func | line | pid = left
 */
void rule_add(const char *rule)
{
	int i_prog, i_modu, i_file, i_func, i_line, i_pid;
	char *s_prog, *s_modu, *s_file, *s_func, *s_line, *s_pid;
	const char *tmp = rule;
	char buf[4096];

	/* TODO: split the rule first */
	/* strtok or strsplit */

	strcpy(buf, rule);

	s_prog = strtok(buf, " |=");
	s_modu = strtok(NULL, " |=");
	s_file = strtok(NULL, " |=");
	s_func = strtok(NULL, " |=");
	s_line = strtok(NULL, " |=");
	s_pid = strtok(NULL, " |=");


	i_prog = prog_name_add(s_prog);
	i_modu = modu_name_add(s_modu);
	i_file = file_name_add(s_file);
	i_func = func_name_add(s_func);

	i_line = atoi(s_line);
	i_pid = atoi(s_pid);

	tmp = strchr(s_line, '=') + 1;

	/* OK, parse the flag into int */
	unsigned int set = 0, clr = 0;
	parse_flg(tmp + 1, &set, &clr);

	rulearr_add(&rulearr, prog, modu, file, func, line, pid, set, clr);
}


typedef struct _flgmap_t flgmap_t;
struct _flgmap_t {
	char code;
	unsigned int bit;
};

flgmap_t flgmap[] = {
	{ 'l', KLOG_LOG },
	{ 'e', KLOG_ERR },
	{ 'f', KLOG_FAT },

	{ 't', KLOG_RTM },
	{ 'T', KLOG_ATM },

	{ 'p', KLOG_PID },
	{ 'P', KLOG_TID },

	{ 'N', KLOG_LINE },
	{ 'F', KLOG_FILE },
	{ 'M', KLOG_MODU },
	{ 'H', KLOG_FUNC },
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

int klog_calc_flg(int prog, int modu, int file, int func, int line, int pid)
{
	int i;

	unsigned int all = 0;

	/* 0 means ignore */

	// |程序|模块|PID|TID|file|func|line=left
	// /usr/bin/shit|shit.c|SHIT|3423|45234|455=leftAM
	// NULL|shit.c|NULL|3423|0|100=leftAM
	// NULL|NULL|NULL|3423|0|100=leftAM
	//

	for (i = 0; i < rulearr.cnt; i++) {
		rule_t *rule = &rulearr.arr[i];

		if (rule->prog && rule->prog != prog)
			continue;
		if (rule->modu && rule->modu != modu)
			continue;
		if (rule->file && rule->file != file)
			continue;
		if (rule->func && rule->func != func)
			continue;
		if (rule->line && rule->line != line)
			continue;
		if (rule->pid && rule->pid != pid)
			continue;

		all &= ^rule->clr;
		all |= rule->set;
	}

	return 0;
}

/*-----------------------------------------------------------------------
 * main.c part
 */
int main(int argc, char *argv[])
{
	char *rule = "0|0|0|0|0|0|=leftRMHN";
	char *rule = "0|0|0|0|0|0|=leftRMHN";

	rule_add(rule);

	klog("shit\n");

	return 0;
}
