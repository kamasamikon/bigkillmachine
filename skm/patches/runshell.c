/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>

/* runproc(1, argv[0], ..., argv[Last], NULL, envp[0], ..., envp[Last], NULL */
static void runproc(int wait, const char *arg, ...)
{
	pid_t pid;

	int argc = 0, envc = 0;
	const char *argv[1024], *envv[1024], *tmp;

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
		envv[envc++] = tmp;
		tmp = va_arg(ap, char *);
	}
	envv[envc] = NULL;

	va_end(ap);

	if ((pid = vfork()) < 0) {
		printf("fork error.\n");
		_exit(0);
	} else if (pid == 0) {
		printf("Will run '%s'\n", argv[0]);

		execvpe(argv[0], argv, envv);
		_exit(0);
	}

	if (wait)
		if (waitpid(pid, NULL, 0) != pid) {
			printf("waitpid error.\n");
		}
}

int main(int argc, char *argv[])
{
	// runproc(1, "/bin/sh", NULL, "PS1=SKM> ", NULL);
	// runproc(1, "/bin/sh", NULL, NULL);
    runproc(1, "/bin/cat", "/BUILDINFO", NULL, NULL);
    runproc(1, "/bin/sh", NULL, "PS1=SKM>  ", NULL);
	runproc(1, "/bin/sh", NULL);
}

