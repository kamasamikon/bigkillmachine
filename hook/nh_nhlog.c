/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include "nh_helper.h"

#include <hilda/klog.h>

int nhlog_touches(void)
{
	klogmon_init();
	return klog_touches();
}

int nhlog_vf(unsigned char type, unsigned int mask, const char *prog, const char *modu, const char *file, const char *func, int ln, const char *fmt, va_list ap)
{
	return klog_vf(type, mask, prog, modu, file, func, ln, fmt, ap);
}

int nhlog_f(unsigned char type, unsigned int mask, const char *prog, const char *modu, const char *file, const char *func, int ln, const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = klog_vf(type, mask, prog, modu, file, func, ln, fmt, ap);
	va_end(ap);

	return ret;
}

char* nhlog_get_name_part(char *name)
{
	return klog_get_name_part(name);
}

char* nhlog_get_prog_name()
{
	return klog_get_prog_name();
}

int nhlog_file_name_add(const char *name)
{
	return klog_file_name_add(name);
}

int nhlog_modu_name_add(const char *name)
{
	return klog_modu_name_add(name);
}

int nhlog_prog_name_add(const char *name)
{
	return klog_prog_name_add(name);
}

int nhlog_func_name_add(const char *name)
{
	return klog_func_name_add(name);
}

unsigned int nhlog_calc_mask(int prog, int modu, int file, int func, int line, int pid)
{
	return klog_calc_mask(prog, modu, file, func, line, pid);
}

