/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <hilda/kmem.h>

#include <fbutton.h>
#include <fdesktop.h>

#include "pat.h"

#define fbox_button_new(self, parent) \
	(fbox_t*)fbox_button_new_((fbox_t*)self, (fbox_t*)parent)

fbutton_t *fbox_button_new_(fbox_t *self, fbox_t *parent)
{
	fbutton_t *but = (fbutton_t*)self;
	if (!self)
		but = (fbutton_t*)kmem_alloz(1, fbutton_t);
	fbox_cons(&but->box, "button", parent);

	but->box.type = BT_BUTTON;
	but->box.attr.can_focus = 1;

	// fbox_set_palette((fbox_t*)but, BS_NORMAL, __g_pat_button_n);
	// fbox_set_palette((fbox_t*)but, BS_FOCUSED, __g_pat_button_f);
	// fbox_set_palette((fbox_t*)but, BS_FOCUSED_CHECK, __g_pat_button_f);

	return but;
}

