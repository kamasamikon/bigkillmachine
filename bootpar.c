/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <string.h>

#include <karg.h>
#include <kstr.h>

/* return the REAL length of the param, -1: error, -2: not found */
/* XXX: for bmem= alike parameter, the opt is "bmem=" */
int get_boot_param(const char *opt, char *buf, int blen)
{
	FILE *fp = fopen("/proc/cmdline", "rt");

	char buffer[4096], **argv, **arg_v;
	int ret = -1, i, bytes, arg_c;

	if (fp) {
		bytes = fread(buffer, sizeof(char), sizeof(buffer), fp);
		fclose(fp);

		if (bytes <= 0)
			return -1;

		buffer[bytes] = '\0';
		kstr_trim(buffer);

		argv = build_argv(buffer, &arg_c, &arg_v);
		for (i = 0; i < arg_c; i++)
			printf("argv[%d] = \"%s\"\n", i, arg_v[i]);

		i = arg_find(arg_c, arg_v, opt, 0);
		if (i >= 0) {
			strncpy(buf, arg_v[i], blen);
			buf[blen - 1] = '\0';
			ret = strlen(argv[i]);
		}

		free_argv(argv);
	}
	return ret;
}

int main(int argc, char *argv[])
{
	char buf[41];
	int ret;

	ret = get_boot_param("BOOT_IMAGE", buf, sizeof(buf));
	printf("ret:%d, opt:\"%s\"\n", ret, buf);

	return 0;
}

