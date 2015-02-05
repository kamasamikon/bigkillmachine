/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __FBUTTON_H__
#define __FBUTTON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <fbox.h>

/*-----------------------------------------------------------------------
 * Button
 */

typedef struct _fbutton_t fbutton_t;
struct _fbutton_t {
	fbox_t box;
};

#define fbox_button_new(self, parent) \
	(fbox_t*)fbox_button_new_((fbox_t*)self, (fbox_t*)parent)
fbutton_t *fbox_button_new_(fbox_t *self, fbox_t *parent);

#ifdef __cplusplus
}
#endif
#endif /* __FBUTTON_H__ */
