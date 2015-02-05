/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */


/**
 * @file     nbuf.h
 * @author   Yu Nemo Wenbin
 * @version  1.0
 * @date     Sep, 2014
 */

#ifndef __N_LIB_BUF_H__
#define __N_LIB_BUF_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

typedef struct _nbuf_s nbuf_s;
struct _nbuf_s {
	size_t alloc;
	size_t len;
	char *buf;
};

#define alloc_nr(x) (((x) + 16) * 3 / 2)

#define MEM_INC(x, nr, alloc) \
	do { \
		if ((nr) > alloc) { \
			if (alloc_nr(alloc) < (nr)) \
				alloc = (nr); \
			else \
				alloc = alloc_nr(alloc); \
			x = realloc((x), alloc * sizeof(*(x))); \
		} \
	} while (0)

void nbuf_setlen(nbuf_s *nb, size_t len);

void nbuf_init(nbuf_s *nb, size_t hint);
void nbuf_release(nbuf_s *nb);

char *nbuf_detach(nbuf_s *nb, size_t *size);
void nbuf_attach(nbuf_s *nb, void *buf, size_t len, size_t alloc);

void nbuf_grow(nbuf_s *nb, size_t extra);

void nbuf_add(nbuf_s *nb, const void *data, size_t len);
void nbuf_addf(nbuf_s *nb, const char *fmt, ...);
void nbuf_vaddf(nbuf_s *nb, const char *fmt, va_list ap);

size_t nbuf_fread(nbuf_s *nb, size_t size, FILE *fp, int *err);
void nbuf_dump(nbuf_s *nb, const char *banner, char *dat, int len, int width);

#endif /* __N_LIB_BUF_H__ */

