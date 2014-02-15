/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#include <sys/sysinfo.h>

#define FSHIFT 16              /* nr of bits of precision */
#define FIXED_1      (1 << FSHIFT)     /* 1.0 as fixed-point */
#define LOAD_INT(x)  (unsigned)((x) >> FSHIFT)
#define LOAD_FRAC(x) LOAD_INT(((x) & (FIXED_1 - 1)) * 100)

int main(int argc, char *argv[])
{
	struct sysinfo info;
	struct tm *current_time;
	time_t current_secs;
	char tm_buf[1024];
	FILE *fp = NULL;
	int loops = -1;

	if (argc < 2) {
		printf("Usage: ldmon output-file\n");
		return 0;
	}

	while (1) {
		loops++;

		if (!fp) {
			fp = fopen(argv[1], "wt");
			if (!fp) {
				printf("Loop:%d, Open file '%s' failed.\n", loops, argv[1]);
				sleep(1);
				continue;
			}
		}

		sysinfo(&info);

		time(&current_secs);
		current_time = localtime(&current_secs);
		strftime(tm_buf, sizeof(tm_buf), "%H:%M:%S", current_time);

		fprintf(fp, "%4u %s %8lu %4u %u.%02u %u.%02u %u.%02u\n",
				loops, tm_buf, info.uptime, info.procs,
				LOAD_INT(info.loads[0]), LOAD_FRAC(info.loads[0]),
				LOAD_INT(info.loads[1]), LOAD_FRAC(info.loads[1]),
				LOAD_INT(info.loads[2]), LOAD_FRAC(info.loads[2]));
		fflush(fp);

#if 0
		printf("totalram:  %lu\n", info.totalram);
		printf("freeram:   %lu\n", info.freeram);
		printf("sharedram: %lu\n", info.sharedram);
		printf("bufferram: %lu\n", info.bufferram);
		printf("totalswap: %lu\n", info.totalswap);
		printf("freeswap:  %lu\n", info.freeswap);
		printf("totalhigh: %lu\n", info.totalhigh);
		printf("freehigh:  %lu\n", info.freehigh);
#endif

		sleep(1);
	}

	return 0;
}
