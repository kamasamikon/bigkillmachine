/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <hilda/kmem.h>

#include <fimage.h>
#include <fdesktop.h>

#define fbox_image_new(self, parent) \
	(fbox_t*)fbox_image_new_((fbox_t*)self, (fbox_t*)parent)

fimage_t *fbox_image_new_(fbox_t *self, fbox_t *parent)
{
	fimage_t *img = (fimage_t*)self;
	if (!img)
		img = (fimage_t*)kmem_alloz(1, fimage_t);
	fbox_cons(&img->box, "image", parent);

	img->box.type = BT_IMAGE;

	return img;
}


