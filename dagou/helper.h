/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __HN_HELPER_H__
#define __HN_HELPER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* XXX: define before include dlfcn.h */
#define _GNU_SOURCE
#include <dlfcn.h>

#include <stdio.h>

#ifndef dagou_likely
#define dagou_likely(x)      __builtin_expect(!!(x), 1)
#endif
#ifndef dagou_unlikely
#define dagou_unlikely(x)    __builtin_expect(!!(x), 0)
#endif


/*-----------------------------------------------------------------------
 * ARRary INCrease
 */
#define ARR_INC(STEP, ARR, CUR_ARR_LEN, ITEM_TYPE) \
	do { \
		void *__arr; \
		(CUR_ARR_LEN) += (STEP); \
		\
		__arr = calloc((CUR_ARR_LEN), sizeof(ITEM_TYPE)); \
		if (ARR) { \
			memcpy(__arr, (ARR), ((CUR_ARR_LEN) - (STEP)) * sizeof(ITEM_TYPE)); \
			free((void*)ARR); \
		} \
		ARR = (ITEM_TYPE*)__arr; \
	} while (0)


/*-----------------------------------------------------------------------
 * kmem
 */
#define nmem_alloc(cnt, type) (type*)malloc((cnt) * sizeof(type))
#define nmem_alloz(cnt, type) (type*)calloc((cnt), sizeof(type))
#define nmem_realloc(p, sz) realloc((p), (sz))
#define nmem_free(p) do { free(p); } while (0)
#define nmem_free_s(p) do { if (p) free(p); } while (0)
#define nmem_free_z(p) do { free(p); p = NULL; } while (0)
#define nmem_free_sz(p) do { if (p) { free(p); p = NULL; } } while (0)

#define nmem_dup(addr, len, out) do { \
	(out) = malloc(len); \
	memcpy((out), (addr), (len)); \
} while (0)

#ifdef __cplusplus
}
#endif
#endif /* __HN_HELPER_H__ */

