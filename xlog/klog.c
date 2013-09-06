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
#include <karg.h>

#include "xlog.h"

/*-----------------------------------------------------------------------
 * Local definition:
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


/* How many logger slot */
#define MAX_NLOGGER 8
#define MAX_RLOGGER 8

/* Control Center for klog */
typedef struct _klogcc_t klogcc_t;
struct _klogcc_t {
	/** version is a ref count user change klog arg */
	int touches;

	strarr_t arr_file_name;
	strarr_t arr_modu_name;
	strarr_t arr_prog_name;
	strarr_t arr_func_name;

	rulearr_t arr_rule;

	unsigned char nlogger_cnt, rlogger_cnt;
	KNLOGGER nloggers[MAX_NLOGGER];
	KRLOGGER rloggers[MAX_RLOGGER];
};

static klogcc_t *__g_klogcc = NULL;

/*-----------------------------------------------------------------------
 * klog-logger
 */
int klog_add_logger(KNLOGGER logger)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();
	int i;

	for (i = 0; i < MAX_NLOGGER; i++)
		if (cc->nloggers[i] == logger)
			return 0;

	if (cc->nlogger_cnt >= MAX_NLOGGER) {
		wlogf("klog_add_logger: Only up to %d logger supported.\n", MAX_NLOGGER);
		return -1;
	}

	cc->nloggers[cc->nlogger_cnt++] = logger;
	return 0;
}

int klog_del_logger(KNLOGGER logger)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();
	int i;

	for (i = 0; i < cc->nlogger_cnt; i++)
		if (cc->nloggers[i] == logger) {
			cc->nlogger_cnt--;
			cc->nloggers[i] = cc->nloggers[cc->nlogger_cnt];
			cc->nloggers[cc->nlogger_cnt] = NULL;
			return 0;
		}

	return -1;
}

int klog_add_rlogger(KRLOGGER logger)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();
	int i;

	for (i = 0; i < MAX_RLOGGER; i++)
		if (cc->rloggers[i] == logger)
			return 0;

	if (cc->rlogger_cnt >= MAX_RLOGGER) {
		wlogf("klog_add_logger: Only up to %d logger supported.\n", MAX_RLOGGER);
		return -1;
	}

	cc->rloggers[cc->rlogger_cnt++] = logger;
	return 0;
}

int klog_del_rlogger(KRLOGGER logger)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();
	int i;

	for (i = 0; i < cc->rlogger_cnt; i++)
		if (cc->rloggers[i] == logger) {
			cc->rlogger_cnt--;
			cc->rloggers[i] = cc->rloggers[cc->rlogger_cnt];
			cc->rloggers[cc->rlogger_cnt] = NULL;
			return 0;
		}

	return -1;
}


/*-----------------------------------------------------------------------
 * implementation
 */
static void klog_parse_mask(const char *mask, unsigned int *set, unsigned int *clr);

/**
 * \brief Other module call this to use already inited CC
 *
 * It should be called by other so or dll.
 */
void *klog_attach(void *logcc)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();

	return (void*)cc;
}

kinline void *klog_cc(void)
{
	int argc, cl_size = 0;
	char **argv, *cl_buf;
	void *cc;

	if (__g_klogcc)
		return (void*)__g_klogcc;

	/* klog_init not called, load args by myself */
	argc = 0;
	argv = NULL;

	cl_buf = spl_get_cmdline(&cl_size);
	build_argv_nul(cl_buf, cl_size, &argc, &argv);
	kmem_free(cl_buf);

	cc = klog_init(KLOG_ALL, argc, argv);
	free_argv(argv);

	return cc;

}

kinline void klog_touch(void)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();

	cc->touches++;
}
kinline int klog_touches(void)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();

	return cc->touches;
}

static void load_cfg_file(const char *path)
{
	char buf[4096];
	FILE *fp;

	fp = fopen(path, "rt");
	if (!fp)
		return;

	while (fgets(buf, sizeof(buf), fp))
		klog_rule_add(buf);

	fclose(fp);
}

static void load_cfg(int argc, char *argv[])
{
	char *cfgpath;
	int i;

	/* Load configure from env */
	cfgpath = getenv("KLOG_CFGFILE");
	if (cfgpath)
		load_cfg_file(cfgpath);

	/* Load configure from command line */
	i = arg_find(argc, argv, "--klog-cfgfile", 1);
	if (i > 1)
		load_cfg_file(argv[i + 1]);
}

static void rule_add_from_mask(unsigned int mask)
{
	char rule[256];
	int i;

	strcpy(rule, "0|0|0|0|0|0|=");
	i = 13;

	if (mask & KLOG_TRC)
		rule[i++] = 't';
	if (mask & KLOG_LOG)
		rule[i++] = 'l';
	if (mask & KLOG_ERR)
		rule[i++] = 'e';
	if (mask & KLOG_FAT)
		rule[i++] = 'f';

	if (mask & KLOG_RTM)
		rule[i++] = 's';
	if (mask & KLOG_ATM)
		rule[i++] = 'S';

	if (mask & KLOG_PID)
		rule[i++] = 'j';
	if (mask & KLOG_TID)
		rule[i++] = 'x';

	if (mask & KLOG_LINE)
		rule[i++] = 'N';
	if (mask & KLOG_FILE)
		rule[i++] = 'F';
	if (mask & KLOG_MODU)
		rule[i++] = 'M';
	if (mask & KLOG_FUNC)
		rule[i++] = 'H';
	if (mask & KLOG_PROG)
		rule[i++] = 'P';

	klog_rule_add(rule);
}

void *klog_init(unsigned int mask, int argc, char **argv)
{
	klogcc_t *cc;

	if (__g_klogcc)
		return (void*)__g_klogcc;

	cc = (klogcc_t*)kmem_alloz(1, klogcc_t);
	__g_klogcc = cc;

	/* Set default before configure file */
	if (mask)
		rule_add_from_mask(mask);

	load_cfg(argc, argv);

	klog_touch();

	return (void*)__g_klogcc;
}

int rvlogf(unsigned char type, unsigned int mask,
		const char *prog, const char *modu,
		const char *file, const char *func, int ln,
		const char *fmt, va_list ap)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();

	char buffer[2048], *bufptr = buffer;
	int i, ret, ofs, bufsize = sizeof(buffer);

	char tmbuf[128];
	time_t t;
	struct tm *tmp;

	unsigned long tick;

	for (i = 0; i < cc->rlogger_cnt; i++)
		if (cc->rloggers[i])
			cc->rloggers[i](type, mask, prog, modu, file, func, ln, fmt, ap);

	if (mask & (KLOG_RTM | KLOG_ATM))
		tick = spl_get_ticks();

	ofs = 0;

	/* Type */
	if (type)
		ofs += sprintf(bufptr, "|%c|", type);

	/* Time */
	if (mask & KLOG_RTM)
		ofs += sprintf(bufptr + ofs, "s:%lu|", tick);
	if (mask & KLOG_ATM) {
		t = time(NULL);
		tmp = localtime(&t);
		strftime(tmbuf, sizeof(tmbuf), "%Y/%m/%d %H:%M:%S", tmp);
		ofs += sprintf(bufptr + ofs, "S:%s.%03d|", tmbuf, (unsigned int)(tick % 1000));
	}

	/* ID */
	if (mask & KLOG_PID)
		ofs += sprintf(bufptr + ofs, "j:%d|", (int)spl_process_currrent());
	if (mask & KLOG_TID)
		ofs += sprintf(bufptr + ofs, "x:%x|", (int)spl_thread_current());

	/* Name and LINE */
	if ((mask & KLOG_PROG) && prog)
		ofs += sprintf(bufptr + ofs, "P:%s|", prog);

	if ((mask & KLOG_MODU) && modu)
		ofs += sprintf(bufptr + ofs, "M:%s|", modu);

	if ((mask & KLOG_FILE) && file)
		ofs += sprintf(bufptr + ofs, "F:%s|", file);

	if ((mask & KLOG_FUNC) && func)
		ofs += sprintf(bufptr + ofs, "H:%s|", func);

	if (mask & KLOG_LINE)
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
		if (mask & KLOG_RTM)
			ofs += sprintf(bufptr + ofs, "s:%lu|", tick);
		if (mask & KLOG_ATM)
			ofs += sprintf(bufptr + ofs, "%s.%03d|", tmbuf, (unsigned int)(tick % 1000));

		/* ID */
		if (mask & KLOG_PID)
			ofs += sprintf(bufptr + ofs, "j:%d|", (int)spl_process_currrent());
		if (mask & KLOG_TID)
			ofs += sprintf(bufptr + ofs, "x:%x|", (int)spl_thread_current());

		/* Name and LINE */
		if ((mask & KLOG_PROG) && prog)
			ofs += sprintf(bufptr + ofs, "P:%s|", prog);

		if ((mask & KLOG_MODU) && modu)
			ofs += sprintf(bufptr + ofs, "M:%s|", modu);

		if ((mask & KLOG_FILE) && file)
			ofs += sprintf(bufptr + ofs, "F:%s|", file);

		if ((mask & KLOG_FUNC) && func)
			ofs += sprintf(bufptr + ofs, "H:%s|", func);

		if (mask & KLOG_LINE)
			ofs += sprintf(bufptr + ofs, "L:%d|", ln);

		if (ofs)
			ofs += sprintf(bufptr + ofs, " ");

		ret = vsnprintf(bufptr + ofs, bufsize - ofs, fmt, ap);
	}

	ret += ofs;

	for (i = 0; i < cc->nlogger_cnt; i++)
		if (cc->nloggers[i])
			cc->nloggers[i](bufptr, ret);

	if (bufptr != buffer)
		kmem_free(bufptr);
	return ret;

}

int rlogf(unsigned char type, unsigned int mask,
		const char *prog, const char *modu,
		const char *file, const char *func, int ln,
		const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = rvlogf(type, mask, prog, modu, file, func, ln, fmt, ap);
	va_end(ap);

	return ret;

}

char *klog_get_name_part(char *name)
{
	char *dup = strdup(name);
	char *bn = basename(dup);

	bn = bn ? strdup(bn) : strdup("");

	free(dup);

	return bn;
}

char *klog_get_prog_name()
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
			pname = klog_get_name_part(buf);
		}
	}
	return pname;
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

int klog_file_name_add(const char *name)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();

	return strarr_add(&cc->arr_file_name, name);
}
int klog_modu_name_add(const char *name)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();

	return strarr_add(&cc->arr_modu_name, name);
}
int klog_prog_name_add(const char *name)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();

	return strarr_add(&cc->arr_prog_name, name);
}
int klog_func_name_add(const char *name)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();

	return strarr_add(&cc->arr_func_name, name);
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
	klogcc_t *cc = (klogcc_t*)klog_cc();
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


	i_prog = klog_prog_name_add(s_prog);
	i_modu = klog_modu_name_add(s_modu);
	i_file = klog_file_name_add(s_file);
	i_func = klog_func_name_add(s_func);

	i_line = atoi(s_line);
	i_pid = atoi(s_pid);

	tmp = strchr(rule, '=') + 1;

	/* OK, parse the flag into int */
	unsigned int set = 0, clr = 0;
	klog_parse_mask(tmp, &set, &clr);

	if (set || clr)
		rulearr_add(&cc->arr_rule, i_prog, i_modu, i_file, i_func, i_line, i_pid, set, clr);
}

static unsigned int get_mask(char c)
{
	int i;

	static struct {
		char code;
		unsigned int bit;
	} flagmap[] = {
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

	for (i = 0; i < sizeof(flagmap) / sizeof(flagmap[0]); i++)
		if (flagmap[i].code == c)
			return flagmap[i].bit;
	return 0;
}

static void klog_parse_mask(const char *mask, unsigned int *set, unsigned int *clr)
{
	int i = 0, len = strlen(mask);
	char c;

	*set = 0;
	*clr = 0;

	for (i = 0; i < len; i++) {
		c = mask[i];
		if (c == '-') {
			c = mask[++i];
			*clr |= get_mask(c);
		} else
			*set |= get_mask(c);
	}
}

unsigned int klog_calc_mask(int prog, int modu, int file, int func, int line, int pid)
{
	klogcc_t *cc = (klogcc_t*)klog_cc();
	unsigned int i, all = 0;

	for (i = 0; i < cc->arr_rule.cnt; i++) {
		rule_t *rule = &cc->arr_rule.arr[i];

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
