/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include "nh_helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>

/* LD_PRELOAD=/home/auv/nemohook.so ./prog1 */
/*-----------------------------------------------------------------------
 * helper
 */
int klogf(const char *fmt, ...);
int klogf(const char *fmt, ...)
{
	char buffer[4096], *bufptr = buffer;
	static FILE *fp = NULL;
	int ret, ofs, bufsize = sizeof(buffer);

	va_list ap;

	char tmbuf[128];
	time_t t;
	struct tm *tmp;

	t = time(NULL);
	tmp = localtime(&t);
	strftime(tmbuf, sizeof(tmbuf), "%Y/%m/%d %H:%M:%S", tmp);
	if (!fp)
		fp = fopen("/tmp/nemohook.out", "wt");

	va_start(ap, fmt);
	ofs = 0;
	ofs += sprintf(bufptr + ofs, "%d|", (int)getpid());
	ofs += sprintf(bufptr + ofs, "%s|", tmbuf);
	ret = vsnprintf(bufptr + ofs, bufsize - ofs, fmt, ap);

	if (ret > bufsize - ofs - 1) {
		bufsize = ret + ofs + 1;
		if (bufptr != buffer)
			free(bufptr);
		bufptr = (char*)malloc(bufsize);

		ofs = 0;
		ofs += sprintf(bufptr + ofs, "%d|", (int)getpid());
		ofs += sprintf(bufptr + ofs, "%s|", tmbuf);
		ret = vsnprintf(bufptr + ofs, bufsize - ofs, fmt, ap);
	}
	va_end(ap);

	ret += ofs;
	va_start(ap, fmt);
	fwrite(bufptr, 1, ret, fp);
	fflush(fp);
	va_end(ap);
	if (bufptr != buffer)
		free(bufptr);

	return 0;
}

