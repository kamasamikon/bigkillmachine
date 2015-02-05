/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */


/**
 * @file     nbuf.c
 * @author   Yu Nemo Wenbin
 * @version  1.0
 * @date     Sep, 2014
 */

#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>

#include <nbuf.h>

char nbuf_slopbuf[1];

static size_t nbuf_avail(const nbuf_s *nb)
{
	return nb->alloc ? nb->alloc - nb->len - 1 : 0;
}

void nbuf_setlen(nbuf_s *nb, size_t len)
{
	if (len > (nb->alloc ? nb->alloc - 1 : 0)) {
		fprintf(stderr, "fatal! nbuf_setlen: len = %lu\n", (unsigned long)len);
		exit(-1);
	}
	nb->len = len;
	nb->buf[len] = '\0';
}

void nbuf_init(nbuf_s *nb, size_t hint)
{
	nb->alloc = nb->len = 0;
	nb->buf = nbuf_slopbuf;
	if (hint)
		nbuf_grow(nb, hint);
}

void nbuf_release(nbuf_s *nb)
{
	if (nb->alloc) {
		free(nb->buf);
		nbuf_init(nb, 0);
	}
}

char *nbuf_detach(nbuf_s *nb, size_t *size)
{
	char *res = nb->alloc ? nb->buf : NULL;

	if (size)
		*size = nb->len;
	nbuf_init(nb, 0);
	return res;
}

void nbuf_attach(nbuf_s *nb, void *buf, size_t len, size_t alloc)
{
	nbuf_release(nb);
	nb->buf = buf;
	nb->len = len;
	nb->alloc = alloc;
	nbuf_grow(nb, 0);
	nb->buf[nb->len] = '\0';
}

void nbuf_grow(nbuf_s *nb, size_t extra)
{
	int new_buf = !nb->alloc;

	if (new_buf)
		nb->buf = NULL;
	MEM_INC(nb->buf, nb->len + extra + 1, nb->alloc);
	if (new_buf)
		nb->buf[0] = '\0';
}

void nbuf_add(nbuf_s *nb, const void *data, size_t len)
{
	nbuf_grow(nb, len);
	memcpy(nb->buf + nb->len, data, len);
	nbuf_setlen(nb, nb->len + len);
}

void nbuf_addf(nbuf_s *nb, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	nbuf_vaddf(nb, fmt, ap);
	va_end(ap);
}

void nbuf_vaddf(nbuf_s *nb, const char *fmt, va_list ap)
{
	int len;
	va_list cp;

	if (!nbuf_avail(nb))
		nbuf_grow(nb, 64);
	va_copy(cp, ap);
	len = vsnprintf(nb->buf + nb->len, nb->alloc - nb->len, fmt, cp);
	va_end(cp);
	if (len <= 0) {
		fprintf(stderr, "fatal when vsnprintf\n");
		exit(-1);
	}
	if ((size_t)len > nbuf_avail(nb)) {
		nbuf_grow(nb, len);
		len = vsnprintf(nb->buf + nb->len, nb->alloc - nb->len, fmt, ap);
		if ((size_t)len > nbuf_avail(nb)) {
			fprintf(stderr, "fatal when vsnprintf\n");
			exit(-1);
		}
	}
	nbuf_setlen(nb, nb->len + len);
}

size_t nbuf_fread(nbuf_s *nb, size_t size, FILE *fp, int *err)
{
	size_t bytes;

	*err = 0;

	nbuf_grow(nb, size);

	bytes = fread(nb->buf + nb->len, 1, size, fp);
	if (bytes > 0)
		nbuf_setlen(nb, nb->len + bytes);
	if (bytes < size) {
		if (ferror(fp))
			*err = 1;
	}

	return bytes;
}

void nbuf_dump(nbuf_s *nb, const char *banner, char *dat, int len, int width)
{
	int i, line, offset = 0, blen;
	char cache[2048], *pbuf, *p;

	if (width <= 0)
		width = 16;

	/* 6 => "%04x  "; 3 => "%02x ", 1 => "%c", 2 => " |", 2 => "|\n" */
	blen = 6 + (3 + 1) * width + 2 + 2;
	if ((size_t)blen > sizeof(cache))
		pbuf = (char*)malloc(blen * sizeof(char));
	else
		pbuf = cache;

	nbuf_addf(nb, "\n%s\n", banner);
	nbuf_addf(nb, "Data:%p, Length:%d\n", dat, len);

	while (offset < len) {
		p = pbuf;

		p += sprintf(p, "%04x  ", offset);
		line = len - offset;

		if (line > width)
			line = width;

		for (i = 0; i < line; i++)
			p += sprintf(p, "%02x ", (unsigned char)dat[i]);
		for (; i < width; i++)
			p += sprintf(p, "   ");

		p += sprintf(p, " |");
		for (i = 0; i < line; i++)
			if ((unsigned char)dat[i] >= 0x20 && (unsigned char)dat[i] < 0x7f)
				p += sprintf(p, "%c",  (unsigned char)dat[i]);
			else
				p += sprintf(p, ".");
		p += sprintf(p, "|\n");
		nbuf_addf(nb, "%s", pbuf);

		offset += line;
		dat += width;
	}

	if (pbuf != cache)
		free(pbuf);
}

