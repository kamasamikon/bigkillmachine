/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */


/**
 * @file     nbuf.h
 * @author   Yu Nemo Wenbin
 * @version  1.0
 * @date     Sep, 2014
 */

#ifndef __N_LIB_ARG_H__
#define __N_LIB_ARG_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

int narg_free(int argc, char **argv);
int narg_build(const char *input, int *arg_c, char ***arg_v);
int narg_find(int argc, char **argv, const char *opt, int fullmatch);
char *narg_get(int argc, char **argv, const char *opt, int len);
char **narg_copy(char **argv, int argc_fr, int argc_to);
char **narg_build_nul(const char *ibuf, int ilen, int *arg_c, char ***arg_v);

#endif /* __N_LIB_ARG_H__ */

