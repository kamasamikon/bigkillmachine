/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */


/**
 * @file     narg.c
 * @author   Yu Nemo Wenbin
 * @version  1.0
 * @date     Sep, 2014
 */


#include <stdlib.h>
#include <string.h>

#include <helper.h>
#include <narg.h>

/*-----------------------------------------------------------------------
 * narg_xxx
 */
#define BLANK(c) ((c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n')
#define INIT_MAX_ARGC 16
#define ISNUL(c) ((c) == '\0')

int narg_free(int argc, char **argv)
{
	int i;

	if (argv == NULL)
		return -1;

	for (i = 0; i < argc; i++)
		nmem_free_s(argv[i]);
	nmem_free(argv);

	return 0;
}

/* XXX: Normal command line, NUL as end, SP as delimiters */
int narg_build(const char *input, int *arg_c, char ***arg_v)
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

int narg_find(int argc, char **argv, const char *opt, int fullmatch)
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

/* Get --opt=xxx, len=strlen("--opt="), return strdup("xxx") */
char *narg_get(int argc, char **argv, const char *opt, int len)
{
	int i;

	for (i = 0; i < argc; i++)
		if (argv[i] && !strncmp(argv[i], opt, len))
			return strdup(argv[i] + len);
	return NULL;
}

char **narg_copy(char **argv, int argc_fr, int argc_to)
{
	int i, len;
	char **new_argv;

	len = argc_to - argc_fr;

	if (len <= 0)
		return NULL;

	new_argv = (char**)nmem_alloc(len + 1, char*);

	memcpy(new_argv, argv + argc_fr, len * sizeof(char*));

	for (i = 0; i < len; i++)
		new_argv[i] = strdup(argv[argc_fr + i]);
	new_argv[i] = NULL;

	return new_argv;
}

/* XXX: NUL as delimiters */
char **narg_build_nul(const char *ibuf, int ilen, int *arg_c, char ***arg_v)
{
	int argc = 0, maxargc = 0;
	char c, *arg, *copybuf, **argv = NULL, **nargv;
	const char *input = ibuf;

	*arg_c = argc;
	if (input == NULL)
		return NULL;

	copybuf = (char *)nmem_alloc(ilen, char);

	do {
		while (input - ibuf < ilen) {
			c = *input;
			if (ISNUL(c))
				input++;
			else
				break;
		}

		if ((maxargc == 0) || (argc >= (maxargc - 1))) {
			if (argv == NULL)
				maxargc = 16;
			else
				maxargc *= 2;

			nargv = (char **)nmem_realloc(argv,
					maxargc * sizeof(char*));
			if (nargv == NULL) {
				if (argv != NULL) {
					narg_free(argc, argv);
					argv = NULL;
				}
				break;
			}
			argv = nargv;
			argv[argc] = NULL;
		}

		arg = copybuf;
		while (input - ibuf < ilen) {
			c = *input;
			if (ISNUL(c))
				break;

			*arg++ = c;
			input++;
		}

		*arg = '\0';
		argv[argc] = strdup(copybuf);
		if (argv[argc] == NULL) {
			narg_free(argc, argv);
			argv = NULL;
			break;
		}
		argc++;
		argv[argc] = NULL;

		while (input - ibuf < ilen) {
			c = *input;
			if (ISNUL(c))
				input++;
			else
				break;
		}
	} while (input - ibuf < ilen);

	nmem_free_s(copybuf);

	*arg_c = argc;
	*arg_v = argv;

	return argv;
}


