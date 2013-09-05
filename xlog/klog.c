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

#include "xlog.h"

/*-----------------------------------------------------------------------
 * xlog.h part
 */

/* STRing ARRay to save programe name, module name, file name etc */
typedef struct _strarr_t strarr_t;
struct _strarr_t {
	int size;
	int cnt;
	char **arr;
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

	/* Which flag to be set or clear */
	unsigned int set, clr;
};

typedef struct _rulearr_t rulearr_t;
struct _rulearr_t {
	int size;
	int cnt;
	rule_t *arr;
};

typedef struct _flgmap_t flgmap_t;
struct _flgmap_t {
	char code;
	unsigned int bit;
};

static flgmap_t __g_flgmap[] = {
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

/*-----------------------------------------------------------------------
 * xlog.c part
 */
static strarr_t __g_sa_file_name = { 0 };
static strarr_t __g_sa_modu_name = { 0 };
static strarr_t __g_sa_prog_name = { 0 };
static strarr_t __g_sa_func_name = { 0 };

static rulearr_t __g_rulearr = { 0 };

inline int klog_ver();
void klog_parse_flg(const char *flg, unsigned int *set, unsigned int *clr);

int rlogf(unsigned char type, unsigned int flg,
		const char *prog, const char *modu,
		const char *file, const char *func, int ln,
		const char *fmt, ...)
{
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

	if (ofs)
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

		if (ofs)
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
	return strarr_add(&__g_sa_file_name, name);
}
int file_name_find(const char *name)
{
	return strarr_find(&__g_sa_file_name, name);
}
int modu_name_add(const char *name)
{
	return strarr_add(&__g_sa_modu_name, name);
}
int modu_name_find(const char *name)
{
	return strarr_find(&__g_sa_modu_name, name);
}
int prog_name_add(const char *name)
{
	return strarr_add(&__g_sa_prog_name, name);
}
int prog_name_find(const char *name)
{
	return strarr_find(&__g_sa_prog_name, name);
}
int func_name_add(const char *name)
{
	return strarr_add(&__g_sa_func_name, name);
}
int func_name_find(const char *name)
{
	return strarr_find(&__g_sa_func_name, name);
}



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
void klog_rule_add(const char *rule)
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
	klog_parse_flg(tmp, &set, &clr);

	rulearr_add(&__g_rulearr, i_prog, i_modu, i_file, i_func, i_line, i_pid, set, clr);
}

static unsigned int get_flg(char c)
{
	int i;

	for (i = 0; i < sizeof(__g_flgmap) / sizeof(__g_flgmap[0]); i++)
		if (__g_flgmap[i].code == c)
			return __g_flgmap[i].bit;
	return 0;
}

void klog_parse_flg(const char *flg, unsigned int *set, unsigned int *clr)
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

unsigned int klog_calc_flg(int prog, int modu, int file, int func, int line, int pid)
{
	unsigned int i, all = 0;

	for (i = 0; i < __g_rulearr.cnt; i++) {
		rule_t *rule = &__g_rulearr.arr[i];

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

void shit()
{
	klog("Swming in shit\n");
}

int main(int argc, char *argv[])
{
	unsigned long i, tick, cost;
	unsigned int count;

	char fmt[1024];

	char *rule0 = "0|0|0|0|0|0|=";
	char *rule1 = "0|0|0|0|505|0|=-l";
	char *rule2 = "0|0|0|shit|0|0|=l-P-x-j";

	show_help();

	sprintf(fmt, "%s%s", rule0, argv[2]);
	klog_rule_add(fmt);
	klog_rule_add(rule1);
	klog_rule_add(rule2);

	count = strtoul(argv[1], NULL, 10);
	tick = spl_get_ticks();

	shit();
	klog("Shit\n");

	for (i = 0; i < count; i++) {
		klog("remote rlog test. puppy FANG is a bad egg. done<%d>\n", i);
	}

	cost = spl_get_ticks() - tick;
	fprintf(stderr, "count: %u\n", count);
	fprintf(stderr, "time cost: %lu\n", cost);
	fprintf(stderr, "count / ms = %lu\n", count / (cost ? cost : 1));

	return 0;
}
