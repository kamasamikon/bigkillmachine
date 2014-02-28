/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <hilda/klog.h>

/*-----------------------------------------------------------------------
 * klog-logger
 */
int klog_add_logger(KNLOGGER logger)
{
	return 0;
}

int klog_del_logger(KNLOGGER logger)
{
	return 0;
}

int klog_add_rlogger(KRLOGGER logger)
{
	return 0;
}

int klog_del_rlogger(KRLOGGER logger)
{
	return 0;
}

void* klog_attach(void *logcc)
{
	return 0;
}

void* klog_cc(void)
{
	return 0;
}

void klog_touch(void)
{
}

int klog_touches(void)
{
	return 0;
}

void* klog_init(unsigned int mask, int argc, char **argv)
{
	return 0;
}

int klog_vf(unsigned char type, unsigned int mask, const char *prog, const char *modu, const char *file, const char *func, int ln, const char *fmt, va_list ap)
{
	return 0;
}

int klog_f(unsigned char type, unsigned int mask, const char *prog, const char *modu, const char *file, const char *func, int ln, const char *fmt, ...)
{
	return 0;
}

char* klog_get_name_part(char *name)
{
	return 0;
}

char* klog_get_prog_name()
{
	return 0;
}

int klog_file_name_add(const char *name)
{
	return 0;
}

int klog_modu_name_add(const char *name)
{
	return 0;
}

int klog_prog_name_add(const char *name)
{
	return 0;
}

int klog_func_name_add(const char *name)
{
	return 0;
}

void klog_rule_add(const char *rule)
{
}

void klog_rule_del(int index)
{
}

void klog_rule_clr()
{
}

unsigned int klog_calc_mask(int prog, int modu, int file, int func, int line, int pid)
{
	return 0;
}

