/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>

/*
 * runproc(1, argv[0], ..., argv[Last], NULL, envk[0], envv[0], ..., NULL
 * runproc(1, "/bin/sh", NULL, "PS1", "SKM>  ", NULL);
 * runproc(1, "/bin/cat", "/BUILDINFO", NULL, NULL);
 */
static void runproc(int wait, const char *arg, ...)
{
	pid_t pid;

	/* env's key value */
	int i, argc = 0, ekvc = 0;
	const char *argv[1024], *ekvv[1024], *tmp;

	va_list ap;

	va_start(ap, arg);

	argv[argc++] = arg;
	tmp = va_arg(ap, char *);
	while (tmp) {
		argv[argc++] = tmp;
		tmp = va_arg(ap, char *);
	}
	argv[argc] = NULL;

	tmp = va_arg(ap, char *);
	while (tmp) {
		ekvv[ekvc++] = tmp;
		tmp = va_arg(ap, char *);
	}
	ekvv[ekvc] = NULL;

	va_end(ap);

	if ((pid = vfork()) < 0) {
		printf("fork error.\n");
		_exit(0);
	} else if (pid == 0) {
		printf("Will run '%s'\n", argv[0]);

		for (i = 0; i < ekvc - 1; i += 2)
			setenv(ekvv[i], ekvv[i + 1], 1);

		execvp(argv[0], (char * const *)argv);
		_exit(0);
	}

	if (wait)
		if (waitpid(pid, NULL, 0) != pid) {
			printf("waitpid error.\n");
		}
}

// start pause resume stop
// kill a tree all at once
//
// How to move on to next (by Parser)
//
