/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __FIMAGE_H__
#define __FIMAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <fbox.h>

typedef struct _fimage_t fimage_t;
struct _fimage_t {
	fbox_t box;
};

#define fbox_image_new(self, parent) \
	(fbox_t*)fbox_image_new_((fbox_t*)self, (fbox_t*)parent)
fimage_t *fbox_image_new_(fbox_t *self, fbox_t *parent);

#ifdef __cplusplus
}
#endif
#endif /* __FIMAGE_H__ */
