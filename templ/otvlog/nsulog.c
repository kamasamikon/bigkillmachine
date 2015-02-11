/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include "nsulog.h"

int nsulog_add_logger(NSU_NLOGGER logger)
{
	return 0;
}

int nsulog_del_logger(NSU_NLOGGER logger)
{
	return 0;
}

int nsulog_add_rlogger(NSU_RLOGGER logger)
{
	return 0;
}

int nsulog_del_rlogger(NSU_RLOGGER logger)
{
	return 0;
}

void *nsulog_attach(void *logcc)
{
	return NULL;
}

void nsulog_touch(void)
{
}

int nsulog_touches(void)
{
	return 0;
}

void nsulog_set_default_mask(unsigned int mask)
{
}

void *nsulog_init(int argc, char **argv)
{
	return NULL;
}

int nsulog_vf(unsigned char type, unsigned int mask, char *prog, char *modu,
		char *file, char *func, int ln, const char *fmt, va_list ap)
{
	return 0;
}

int nsulog_f(unsigned char type, unsigned int mask, char *prog, char *modu,
		char *file, char *func, int ln, const char *fmt, ...)
{
	return 0;
}

char *nsulog_file_name_add(char *name)
{
	return NULL;
}
char *nsulog_modu_name_add(char *name)
{
	return NULL;
}
char *nsulog_prog_name_add(char *name)
{
	return NULL;
}
char *nsulog_func_name_add(char *name)
{
	return NULL;
}

void nsulog_rule_add(char *rule)
{
}
void nsulog_rule_del(unsigned int idx)
{
}
void nsulog_rule_clr()
{
}

unsigned int nsulog_calc_mask(char *prog, char *modu, char *file, char *func, int line)
{
	return 0;
}

