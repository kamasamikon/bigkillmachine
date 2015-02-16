/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <dalog.h>

int nsulog_touches(void)
{
	/* This is the first function called in macro */
	dalog_setup();

	return dalog_touches();
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

unsigned int nsulog_calc_mask(char *prog, char *modu, char *file, char *func, int line)
{
	return dalog_calc_mask(prog, modu, file, func, line);
}

