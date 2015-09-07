/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <glib-object.h>
#include <glib.h>

#include <stdarg.h>

static int __debug = 0;

/*-----------------------------------------------------------------------
 * nmem_xxx
 */
#define nmem_alloc(cnt, type) (type*)malloc((cnt) * sizeof(type))
#define nmem_alloz(cnt, type) (type*)calloc((cnt), sizeof(type))
#define nmem_realloc(p, sz) realloc((p), (sz))
#define nmem_free(p) do { free(p); } while (0)
#define nmem_free_s(p) do { if (p) free(p); } while (0)
#define nmem_free_z(p) do { free(p); p = NULL; } while (0)
#define nmem_free_sz(p) do { if (p) { free(p); p = NULL; } } while (0)

/*-----------------------------------------------------------------------
 * narg_xxx
 */
#define BLANK(c) ((c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n')
#define INIT_MAX_ARGC 16
#define ISNUL(c) ((c) == '\0')

static int narg_free(char **argv)
{
	int i;
	char *v;

	if (argv == NULL)
		return -1;

	for (i = 0; v = argv[i]; i++)
		nmem_free_s(v);
	nmem_free(argv);

	return 0;
}

/* XXX: Normal command line, NUL as end, SP as delimiters */
static int narg_build(const char *input, int *arg_c, char ***arg_v)
{
	int squote = 0, dquote = 0, bsquote = 0, argc = 0, maxargc = 0;
	char c, *arg, *copybuf, **argv = NULL, **nargv;

	*arg_c = argc;
	if (input == NULL)
		return -1;

	copybuf = nmem_alloc((strlen(input) + 1), char);

	do {
		while (BLANK(*input))
			input++;

		if ((maxargc == 0) || (argc >= (maxargc - 1))) {
			if (argv == NULL)
				maxargc = INIT_MAX_ARGC;
			else
				maxargc *= 2;

			nargv = (char **)nmem_realloc(argv,
					maxargc * sizeof(char*));
			if (nargv == NULL) {
				if (argv != NULL) {
					nmem_free(argv);
					argv = NULL;
				}
				break;
			}
			argv = nargv;
			argv[argc] = NULL;
		}

		arg = copybuf;
		while ((c = *input) != '\0') {
			if (BLANK(c) && !squote && !dquote && !bsquote)
				break;

			if (bsquote) {
				bsquote = 0;
				*arg++ = c;
			} else if (c == '\\') {
				bsquote = 1;
			} else if (squote) {
				if (c == '\'')
					squote = 0;
				else
					*arg++ = c;
			} else if (dquote) {
				if (c == '"')
					dquote = 0;
				else
					*arg++ = c;
			} else {
				if (c == '\'')
					squote = 1;
				else if (c == '"')
					dquote = 1;
				else
					*arg++ = c;
			}
			input++;
		}

		*arg = '\0';
		argv[argc] = strdup(copybuf);
		if (argv[argc] == NULL) {
			nmem_free(argv);
			argv = NULL;
			break;
		}
		argc++;
		argv[argc] = NULL;

		while (BLANK(*input))
			input++;
	} while (*input != '\0');

	nmem_free_s(copybuf);

	*arg_c = argc;
	*arg_v = argv;

	return 0;
}

static int narg_find(int argc, char **argv, const char *opt, int fullmatch)
{
	int i;

	if (fullmatch) {
		for (i = 0; i < argc; i++)
			if (argv[i] && !strcmp(argv[i], opt))
				return i;
	} else {
		int slen = strlen(opt);
		for (i = 0; i < argc; i++)
			if (argv[i] && !strncmp(argv[i], opt, slen))
				return i;
	}
	return -1;
}

/*-----------------------------------------------------------------------
 * Log
 */
static int showlog(const char *fmt, ...)
{
	va_list ap;
	int ret;

	if (!__debug)
		return 0;

	va_start(ap, fmt);
	ret = vprintf(fmt, ap);
	va_end(ap);

	return ret;
}

/*-----------------------------------------------------------------------
 * muzei
 */
static pid_t run_command(char *const argv[], char *const envp[])
{
	pid_t pid;

	pid = vfork();
	if (0 == pid) {
		/* child process, execute the command */
		execve(argv[0], argv, envp);
		exit(EXIT_FAILURE);
	}

	return pid;
}

static void help()
{
	printf("Tou(1) Liang(2) Huan(4) Zhu(4)\n");
	printf("\n");
	printf("muzei load another process by muzeirc file\n");
	printf("Load order: \n");
	printf("    1. /etc/muzeirc\n");
	printf("    2. ~/.muzeirc\n");
	printf("    3. ./.muzeirc\n");
	printf("    4. env[\"muzeiRC\"]\n");
	printf("    5. /tmp/.muzeirc\n");
	printf("\n");
	printf("Environment:\n");
	printf("    MUZEI_HELP : Show this and quit\n");
	printf("\n");
	printf("muzei_OPT:\n");
	printf("    -d : Show debug message\n");
}

static void load_muzei_opt()
{
	int argc;
	char **argv;
	char *env = getenv("MUZEI_OPT");

	if (narg_build(env, &argc, &argv))
		return;

	if (narg_find(argc, argv, "-h", 1) > 0) {
		help();
		narg_free(argv);
		exit(0);
	}

	if (narg_find(argc, argv, "-d", 1) > 0)
		__debug = 1;

	narg_free(argv);
}

static void load_env(const char *path, GHashTable *exe_map)
{
	FILE *fp;

	char *line = NULL;
	size_t len = 0;
	ssize_t bytes;

	int exe_c;
	char **exe_v;

	fp = fopen(path, "rt");
	if (!fp) {
		showlog("Open '%s' failed, errno is %d\n", path, errno);
		return;
	} else
		showlog("Open '%s' success\n", path);

	while ((bytes = getline(&line, &len, fp)) != -1) {
		showlog("load_env: file='%s', line=%s\n", path, line);
		if (narg_build(line, &exe_c, &exe_v))
			continue;
		g_hash_table_insert(exe_map, strdup(exe_v[0]), exe_v);
	}

	fclose(fp);
	free(line);
}

static void load_all_rc(GHashTable *exe_map)
{
	load_env("/etc/muzeirc", exe_map);
	load_env("~/.muzeirc", exe_map);
	load_env("./.muzeirc", exe_map);
	load_env(getenv("MUZEIRC"), exe_map);
	load_env("/tmp/.muzeirc", exe_map);
}

static void free_argv(void *data)
{
	narg_free((char**)argv);
}

//
// muzeirc
// verbose
// map map file path
//

static void load_bkm_opt()
{
	// Load env from /tmp/xxx
	//
	GHashTable *env_map;
	load_rc("/tmp/xxx", env_map)
}

// mz exe arg arg arg
int main(int argc, char *argv[])
{
	GHashTable *opt_map;
	GHashTable *exe_map;
	int i;

	if (getenv("MUZEI_HELP")) {
		help();
		return 0;
	}

	opt_map = g_hash_table_new_full(g_str_hash, g_str_equal, free, free_argv);
	exe_map = g_hash_table_new_full(g_str_hash, g_str_equal, free, free_argv);

	load_bkm_opt();
	load_muzei_opt();

	load_all_rc(exe_map);

	exe_list = g_hash_table_lookup(exe_map, argv[0]);
	for (i = 0; exe = exe_list[i]; i++) {
		argv[0] = exe;
		run_command(argv);
	}

	return 0;
}

int main(int argc, char *argv[], char **env[])
{

}

