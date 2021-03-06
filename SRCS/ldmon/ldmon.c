/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#include <sys/sysinfo.h>

#define FSHIFT		16			/* nr of bits of precision */
#define FIXED_1		(1 << FSHIFT)		/* 1.0 as fixed-point */
#define LOAD_INT(x)	(unsigned)((x) >> FSHIFT)
#define LOAD_FRAC(x)	LOAD_INT(((x) & (FIXED_1 - 1)) * 100)

#define CACHE_DEPT	50000

static char *__g_cache[CACHE_DEPT];
static int __g_cache_used = 0;

int main(int argc, char *argv[])
{
	struct sysinfo info;
	struct tm *current_time;
	time_t current_secs;
	char tm_buf[32], cache_line[2048];
	FILE *fp = NULL;
	int loops = -1;

	char *output_file;
	int pause_time;

	printf("Usage: ldmon output-file pause-time(ms)\n");
	printf("\n");

	output_file = argc > 1 ? argv[1] : "/dev/stdout";
	if (!strcmp(output_file, "--help"))
		return 0;

	pause_time = argc > 2 ? atoi(argv[2]) : 500;

	if (pause_time < 100)
		pause_time = 100;

	printf("Current settings:\n");
	printf("    output-file : %s\n", output_file);
	printf("     pause-time : %d\n", pause_time);
	printf("\n");

	while (1) {
		loops++;

		if (!fp) {
			fp = fopen(output_file, "wt");
			if (!fp)
				printf("Loop:%d, Open file '%s' failed.\n", loops, output_file);
			else
				printf("   CNT!     TIME!   UPTIME!   PS!  LD1!  LD5! LD15!     totalram!      freeram!    sharedram!    bufferram!    totalswap!     freeswap!    totalhigh!     freehigh!\n");
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

			fprintf(fp,
					"%06u! %s! %8lu! %4u! %u.%02u! %u.%02u! %u.%02u! %12lu! %12lu! %12lu! %12lu! %12lu! %12lu! %12lu! %12lu!\n",
					loops, tm_buf, info.uptime, info.procs,
					LOAD_INT(info.loads[0]), LOAD_FRAC(info.loads[0]),
					LOAD_INT(info.loads[1]), LOAD_FRAC(info.loads[1]),
					LOAD_INT(info.loads[2]), LOAD_FRAC(info.loads[2]),
					info.totalram, info.freeram, info.sharedram, info.bufferram, info.totalswap, info.freeswap, info.totalhigh, info.freehigh
			       );
			fflush(fp);
		} else {
			/* The output file is not ready, save the information to cache */
			snprintf(cache_line, sizeof(cache_line),
					"%06u! %s! %8lu! %4u! %u.%02u! %u.%02u! %u.%02u! %12lu! %12lu! %12lu! %12lu! %12lu! %12lu! %12lu! %12lu!\n",
					loops, tm_buf, info.uptime, info.procs,
					LOAD_INT(info.loads[0]), LOAD_FRAC(info.loads[0]),
					LOAD_INT(info.loads[1]), LOAD_FRAC(info.loads[1]),
					LOAD_INT(info.loads[2]), LOAD_FRAC(info.loads[2]),
					info.totalram, info.freeram, info.sharedram, info.bufferram, info.totalswap, info.freeswap, info.totalhigh, info.freehigh
			       );

			__g_cache[__g_cache_used % CACHE_DEPT] = strdup(cache_line);
			__g_cache_used++;
		}

		usleep(1000 * pause_time);
	}

	return 0;
}

