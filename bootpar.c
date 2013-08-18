/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <karg.h>
#include <kstr.h>

static int __g_boot_argc;
static char **__g_boot_argv;

static int load_boot_args(int *argc, char ***argv)
{
	FILE *fp = fopen("/proc/cmdline", "rt");
	char buffer[4096];
	int bytes;

	if (fp) {
		bytes = fread(buffer, sizeof(char), sizeof(buffer), fp);
		fclose(fp);

		if (bytes <= 0)
			return -1;

		buffer[bytes] = '\0';
		kstr_trim(buffer);

		build_argv(buffer, argc, argv);
		return 0;
	}
	return -1;
}

int main(int argc, char *argv[])
{
	int i;

	printf("usage: dsq OPTNAME FULLMATCH\n\n");

	load_boot_args(&__g_boot_argc, &__g_boot_argv);

	for (i = 0; i < __g_boot_argc; i++)
		printf("arg[%02d] = '%s'\n", i, __g_boot_argv[i]);

	printf("\n\nWhat found?\n");
	i = arg_find(__g_boot_argc, __g_boot_argv, argv[1], atoi(argv[2]));
	if (i >= 0)
		printf("arg[%02d] = '%s'\n", i, __g_boot_argv[i]);
	else
		printf("Nothing\n");

	return 0;
}

