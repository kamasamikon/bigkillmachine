/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __FEDIT_H__
#define __FEDIT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <fbox.h>
/*-----------------------------------------------------------------------
 * edit
 */
typedef struct _fedit_t fedit_t;
struct _fedit_t {
	fbox_t box;

	struct {
		int l, r, t, b;
	} margin;

	int shadow;			/* use for password etc, when not NIL show org */
	int max_len;
};

#define fbox_edit_new(self, parent) \
	(fbox_t*)fbox_edit_new_((fbox_t*)self, (fbox_t*)parent)
fedit_t *fbox_edit_new_(fbox_t *self, fbox_t *parent);

void fbox_edit_set_margin(fbox_t *box, int l, int r, int t, int b);

#ifdef __cplusplus
}
#endif
#endif /* __FEDIT_H__ */
