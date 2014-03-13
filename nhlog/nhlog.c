/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include "nhlog.h"

int nhlog_touches(void)
{
	return 0;
}

int nhlog_vf(unsigned char type, unsigned int mask, const char *prog, const char *modu, const char *file, const char *func, int ln, const char *fmt, va_list ap)
{
	return 0;
}

int nhlog_f(unsigned char type, unsigned int mask, const char *prog, const char *modu, const char *file, const char *func, int ln, const char *fmt, ...)
{
	return 0;
}

char* nhlog_get_name_part(char *name)
{
	return 0;
}

char* nhlog_get_prog_name()
{
	return 0;
}

int nhlog_file_name_add(const char *name)
{
	return 0;
}

int nhlog_modu_name_add(const char *name)
{
	return 0;
}

int nhlog_prog_name_add(const char *name)
{
	return 0;
}

int nhlog_func_name_add(const char *name)
{
	return 0;
}

unsigned int nhlog_calc_mask(int prog, int modu, int file, int func, int line, int pid)
{
	return 0;
}

