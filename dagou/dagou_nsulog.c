/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include "dalog/dalog.h"

int nsulog_add_logger(NLOGGER logger)
{
	/* All the logger provided by BKM */
	return 0;
}

int nsulog_del_logger(NLOGGER logger)
{
	/* All the logger provided by BKM */
	return 0;
}

int nsulog_add_rlogger(RLOGGER logger)
{
	/* All the logger provided by BKM */
	return 0;
}

int nsulog_del_rlogger(RLOGGER logger)
{
	/* All the logger provided by BKM */
	return 0;
}

void *nsulog_attach(void *logcc)
{
	return dalog_attach(logcc);
}

void nsulog_touch(void)
{
	dalog_touch();
}

int nsulog_touches(void)
{
	dalogmon_init();
	return dalog_touches();
}

void nsulog_set_default_mask(unsigned int mask)
{
	dalog_set_default_mask(mask);
}

void *nsulog_init(int argc, char **argv)
{
	return dalog_init(argc, argv);
}

int nsulog_vf(unsigned char type, unsigned int mask, char *prog, char *modu,
		char *file, char *func, int ln, const char *fmt, va_list ap)
{
	return dalog_vf(type, mask, prog, modu, file, func, ln, fmt, ap);
}

int nsulog_f(unsigned char type, unsigned int mask, char *prog, char *modu,
		char *file, char *func, int ln, const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = dalog_vf(type, mask, prog, modu, file, func, ln, fmt, ap);
	va_end(ap);

	return ret;
}

char *nsulog_file_name_add(char *name)
{
	return dalog_file_name_add(name);
}
char *nsulog_modu_name_add(char *name)
{
	return dalog_modu_name_add(name);
}
char *nsulog_prog_name_add(char *name)
{
	return dalog_prog_name_add(name);
}
char *nsulog_func_name_add(char *name)
{
	return dalog_func_name_add(name);
}

void nsulog_rule_add(char *rule)
{
	dalog_rule_add(rule);
}
void nsulog_rule_del(unsigned int idx)
{
	dalog_rule_del(idx);
}
void nsulog_rule_clr()
{
	dalog_rule_clr();
}

unsigned int nsulog_calc_mask(char *prog, char *modu, char *file, char *func, int line)
{
	return dalog_calc_mask(prog, modu, file, func, line);
}

