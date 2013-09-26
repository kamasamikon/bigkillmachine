/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <time.h>

#include <hilda/helper.h>
#include <hilda/kmem.h>
#include <hilda/kflg.h>
#include <hilda/xtcool.h>

#define KMODU_NAME "NHKLOG"
#include <hilda/klog.h>

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

static void mylogger(const char *content, int len)
{
	printf("%s", content);
}

int main_klog(int argc, char *argv[])
{
	unsigned long i, tick, cost;
	unsigned int count;

	show_help();

	// klog_add_logger(mylogger);

	count = strtoul(argv[1], NULL, 10);
	tick = spl_get_ticks();

	shit();
	klog("Shit\n");

	for (i = 0; i < count; i++) {
		klog("remote klog test. puppy FANG is a bad egg. done<%d>\n", i);
		shit();
		kerror("This is an error\n");
		spl_sleep(1000);
	}

	cost = spl_get_ticks() - tick;
	fprintf(stderr, "count: %u\n", count);
	fprintf(stderr, "time cost: %lu\n", cost);
	fprintf(stderr, "count / ms = %lu\n", count / (cost ? cost : 1));

	return 0;
}
