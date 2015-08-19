/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include "nsulog.h"

int nsulog_touches(void)
{
	return -1;
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

unsigned int nsulog_calc_mask(char *prog, char *modu, char *file, char *func, int line)
{
	return 0;
}

