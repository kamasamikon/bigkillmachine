/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __HN_HELPER_H__
#define __HN_HELPER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* XXX: define before include dlfcn.h */
#define _GNU_SOURCE
#include <dlfcn.h>

#ifndef likely
#define likely(x)      __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x)    __builtin_expect(!!(x), 0)
#endif

#ifdef __cplusplus
}
#endif
#endif /* __HN_HELPER_H__ */

