/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <time.h>

#include <helper.h>
#include <kmem.h>
#include <kflg.h>
#include <xtcool.h>

/*-----------------------------------------------------------------------
 * xlog.h part
 */

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

/* __FILE__ 的名字都比较奇怪，应该去掉路径部分，只保留文件名 */
static int __file_name_id = -1;
static char *__file_name = NULL;

/* 进程的名字可能不总是需要，所以没有必要总是算这个 */
static int __prog_name_id = -1;
static char *__prog_name = NULL;

/* 模块的名字是需要的 */
static int __modu_name_id = -1;

#define O_LOG_COMPNAME "MODU_XLOG"

#if (defined(O_LOG_COMPNAME))
#define COMP_NAME  O_LOG_COMPNAME
#elif (defined(PACKAGE_NAME))
#define COMP_NAME  PACKAGE_NAME
#else
#define COMP_NAME  "?"
#endif

#define MODU_NAME COMP_NAME

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
#if 0
	printf("type: %c\n", type);
	printf("flag: %x\n", flg);
	printf("prog: %s\n", prog);
	printf("modu: %s\n", modu);
	printf("file: %s\n", file);
	printf("func: %s\n", func);
	printf("line: %d\n", ln);
	printf("fmt: %s\n", fmt);
	return 0;
#endif

	// klogcc_t *cc = (klogcc_t*)klog_cc();
	va_list ap;
	char buffer[2048], *bufptr = buffer;
	int ret, ofs, bufsize = sizeof(buffer);

	char tmbuf[128];
	time_t t;
	struct tm *tmp;

	unsigned long tick;

	va_start(ap, fmt);

#if 0
	for (i = 0; i < cc->rlogger_cnt; i++)
		if (cc->rloggers[i])
			cc->rloggers[i](type, flg, modu, file, ln, fmt, ap);
#endif

	if (flg & (KLOG_RTM | KLOG_ATM))
		tick = spl_get_ticks();

	ofs = 0;

	/* Type */
	if (type)
		ofs += sprintf(bufptr, "|%c|", type);

	/* Time */
	if (flg & KLOG_RTM)
		ofs += sprintf(bufptr + ofs, "s:%lu|", tick);
	if (flg & KLOG_ATM) {
		t = time(NULL);
		tmp = localtime(&t);
		strftime(tmbuf, sizeof(tmbuf), "%Y/%m/%d %H:%M:%S", tmp);
		ofs += sprintf(bufptr + ofs, "S:%s.%03d|", tmbuf, (kuint)(tick % 1000));
	}

	/* ID */
	if (flg & KLOG_PID)
		ofs += sprintf(bufptr + ofs, "j:%d|", (int)spl_process_currrent());
	if (flg & KLOG_TID)
		ofs += sprintf(bufptr + ofs, "x:%x|", (int)spl_thread_current());

	/* Name and LINE */
	if ((flg & KLOG_PROG) && prog)
		ofs += sprintf(bufptr + ofs, "P:%s|", prog);

	if ((flg & KLOG_MODU) && modu)
		ofs += sprintf(bufptr + ofs, "M:%s|", modu);

	if ((flg & KLOG_FILE) && file)
		ofs += sprintf(bufptr + ofs, "F:%s|", file);

	if ((flg & KLOG_FUNC) && func)
		ofs += sprintf(bufptr + ofs, "H:%s|", func);

	if (flg & KLOG_LINE)
		ofs += sprintf(bufptr + ofs, "L:%d|", ln);

	ofs += sprintf(bufptr + ofs, " ");

	ret = vsnprintf(bufptr + ofs, bufsize - ofs, fmt, ap);
	while (ret > bufsize - ofs - 1) {
		bufsize = ret + ofs + 1;
		if (bufptr != buffer)
			kmem_free(bufptr);
		bufptr = kmem_get(bufsize);


		ofs = 0;

		/* Type */
		if (type)
			ofs += sprintf(bufptr, "|%c|", type);

		/* Time */
		if (flg & KLOG_RTM)
			ofs += sprintf(bufptr + ofs, "s:%lu|", tick);
		if (flg & KLOG_ATM)
			ofs += sprintf(bufptr + ofs, "%s.%03d|", tmbuf, (kuint)(tick % 1000));

		/* ID */
		if (flg & KLOG_PID)
			ofs += sprintf(bufptr + ofs, "j:%d|", (int)spl_process_currrent());
		if (flg & KLOG_TID)
			ofs += sprintf(bufptr + ofs, "x:%x|", (int)spl_thread_current());

		/* Name and LINE */
		if ((flg & KLOG_PROG) && prog)
			ofs += sprintf(bufptr + ofs, "P:%s|", prog);

		if ((flg & KLOG_MODU) && modu)
			ofs += sprintf(bufptr + ofs, "M:%s|", modu);

		if ((flg & KLOG_FILE) && file)
			ofs += sprintf(bufptr + ofs, "F:%s|", file);

		if ((flg & KLOG_FUNC) && func)
			ofs += sprintf(bufptr + ofs, "H:%s|", func);

		if (flg & KLOG_LINE)
			ofs += sprintf(bufptr + ofs, "L:%d|", ln);

		ofs += sprintf(bufptr + ofs, " ");

		ret = vsnprintf(bufptr + ofs, bufsize - ofs, fmt, ap);
	}
	va_end(ap);

	ret += ofs;
#if 0
	for (i = 0; i < cc->nlogger_cnt; i++)
		if (cc->nloggers[i])
			cc->nloggers[i](bufptr, ret);
#endif
	printf("%s", bufptr);

	if (bufptr != buffer)
		kmem_free(bufptr);
	return ret;

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
		ARR_INC(1, ra->arr, ra->size, rule_t);

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

	tmp = strchr(rule, '=') + 1;

	/* OK, parse the flag into int */
	unsigned int set = 0, clr = 0;
	parse_flg(tmp, &set, &clr);

	rulearr_add(&rulearr, i_prog, i_modu, i_file, i_func, i_line, i_pid, set, clr);
}


typedef struct _flgmap_t flgmap_t;
struct _flgmap_t {
	char code;
	unsigned int bit;
};

static flgmap_t flgmap[] = {
	{ 't', KLOG_TRC },
	{ 'l', KLOG_LOG },
	{ 'e', KLOG_ERR },
	{ 'f', KLOG_FAT },

	{ 's', KLOG_RTM },
	{ 'S', KLOG_ATM },

	{ 'j', KLOG_PID },
	{ 'x', KLOG_TID },

	{ 'N', KLOG_LINE },
	{ 'F', KLOG_FILE },
	{ 'M', KLOG_MODU },
	{ 'H', KLOG_FUNC },
	{ 'P', KLOG_PROG },
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

		kflg_clr(all, rule->clr);
		kflg_set(all, rule->set);
	}

	return all;
}

/*-----------------------------------------------------------------------
 * main.c part
 */
void show_help()
{
	printf("Usage: xlog count fmt\n\n");

	printf("\tt: Trace\n");
	printf("\tl: Log\n");
	printf("\te: Error\n");
	printf("\tf: Fatal Error\n");

	printf("\ts: Relative Time, in MS, 'ShiJian'\n");
	printf("\tS: ABS Time, in MS, 'ShiJian'\n");

	printf("\tj: Process ID, 'JinCheng'\n");
	printf("\tx: Thread ID, 'XianCheng'\n");

	printf("\tP: Process Name\n");
	printf("\tM: Module Name\n");
	printf("\tF: File Name\n");
	printf("\tH: Function Name, 'HanShu'\n");
	printf("\tN: Line Number\n");
}

/*
 * s_prog = strtok(buf, " |=");
 * s_modu = strtok(NULL, " |=");
 * s_file = strtok(NULL, " |=");
 * s_func = strtok(NULL, " |=");
 * s_line = strtok(NULL, " |=");
 * s_pid = strtok(NULL, " |=");
 */

int main(int argc, char *argv[])
{
	unsigned long i, tick, cost;
	unsigned int count;

	char fmt[1024];

	char *rule0 = "0|0|0|0|0|0|=";
	char *rule1 = "0|0|0|0|594|0|=-l";

	show_help();

	sprintf(fmt, "%s%s", rule0, argv[2]);
	rule_add(fmt);
	rule_add(rule1);

	count = strtoul(argv[1], NULL, 10);
	tick = spl_get_ticks();

	klog("Shit\n");

	for (i = 0; i < count; i++) {
		klog("remote rlog test. puppy FANG is a bad egg. done<%d>\n", i);
	}

	cost = spl_get_ticks() - tick;

	if (cost == 0)
		cost = 1;

	fprintf(stderr, "time cost: %lu\n", cost);
	fprintf(stderr, "count: %u\n", count);
	fprintf(stderr, "count / ms = %lu\n", count / cost);

	return 0;
}
