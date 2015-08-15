/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>

#include <hilda/klog.h>

static FILE* __out_fp = NULL;

static void logger_file(const char *content, int len)
{
	fprintf(__out_fp, content);
}

int main(int argc, char *argv[])
{
	int i, count, dirty;
	unsigned long long int s, u;

	count = atoi(argv[1]);
	dirty = atoi(argv[2]);

	__out_fp = fopen(argv[3], "wt");

	klog_add_logger(logger_file);

	s = spl_time_get_usec();
	if (dirty)
		for (i = 0; i < count; i++) {
			klog_touch();
			klog("This is loop: %d\n", i);
		}
	else
		for (i = 0; i < count; i++)
			klog("This is loop: %d\n", i);

	u = spl_time_get_usec() - s;
	printf("Cost: %lluus or %llums, %llu/ms, klog touches %d\n", u, u / 1000, count * 1000 / u, klog_touches());

	fclose(__out_fp);
}

