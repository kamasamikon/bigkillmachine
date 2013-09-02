/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __HN_HELPER_H__
#define __HN_HELPER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* XXX: define before include dlfcn.h */
#define _GNU_SOURCE

#include <dlfcn.h>

int klogf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif
#endif /* __HN_HELPER_H__ */

