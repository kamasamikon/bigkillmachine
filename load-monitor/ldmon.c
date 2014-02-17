/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#include <sys/sysinfo.h>

#define FSHIFT          16                      /* nr of bits of precision */
#define FIXED_1         (1 << FSHIFT)           /* 1.0 as fixed-point */
#define LOAD_INT(x)     (unsigned)((x) >> FSHIFT)
#define LOAD_FRAC(x)    LOAD_INT(((x) & (FIXED_1 - 1)) * 100)

#define CACHE_DEPT      50000
static char *__g_cache[CACHE_DEPT];
static int __g_cache_used = 0;

int main(int argc, char *argv[])
{
	struct sysinfo info;
	struct tm *current_time;
	time_t current_secs;
	char tm_buf[32], cache_line[64];
	FILE *fp = NULL;
	int loops = -1;

	char *output_file;
	int pause_time;

	printf("Usage: ldmon output-file pause-time\n");

	output_file = argc > 1 ?  argv[1] : "/dev/stdout";
	pause_time = argc > 2 ?  atoi(argv[2]) : 500;

	if (pause_time < 100)
		pause_time = 100;

	while (1) {
		loops++;

		if (!fp) {
			fp = fopen(output_file, "wt");
			if (!fp)
				printf("Loop:%d, Open file '%s' failed.\n", loops, output_file);
		}

		sysinfo(&info);

		time(&current_secs);
		current_time = localtime(&current_secs);
		strftime(tm_buf, sizeof(tm_buf), "%H:%M:%S", current_time);

		if (fp) {
			/* Flush all cached line */
			if (__g_cache_used) {
				int i;
				for (i = 0; i < __g_cache_used; i++)
					fprintf(fp, "%s", __g_cache[i]);
				fflush(fp);
				__g_cache_used = 0;
			}

			fprintf(fp, "%04u %s %8lu %4u %u.%02u %u.%02u %u.%02u\n",
					loops, tm_buf, info.uptime, info.procs,
					LOAD_INT(info.loads[0]), LOAD_FRAC(info.loads[0]),
					LOAD_INT(info.loads[1]), LOAD_FRAC(info.loads[1]),
					LOAD_INT(info.loads[2]), LOAD_FRAC(info.loads[2]));
			fflush(fp);
		} else {
			/* The output file is not ready, save the information to cache */
			snprintf(cache_line, sizeof(cache_line), "%04u %s %8lu %4u %u.%02u %u.%02u %u.%02u\n",
					loops, tm_buf, info.uptime, info.procs,
					LOAD_INT(info.loads[0]), LOAD_FRAC(info.loads[0]),
					LOAD_INT(info.loads[1]), LOAD_FRAC(info.loads[1]),
					LOAD_INT(info.loads[2]), LOAD_FRAC(info.loads[2]));

			__g_cache[__g_cache_used % CACHE_DEPT] = strdup(cache_line);
			__g_cache_used++;
		}

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

		usleep(1000 * pause_time);
	}

	return 0;
}

