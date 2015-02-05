/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <hilda/kmem.h>

#include <fedit.h>
#include <fdesktop.h>

#include "pat.h"

static int edit_on_evt(fbox_t *box, DFBWindowEvent *evt)
{
	if (evt->type == DWET_KEYDOWN && box->text.cursor_pos > -1) {
		int oldpos, textlen = strlen(box->text.buf);
		char text[1024];

		switch (evt->key_symbol) {
		case DIKS_CURSOR_RIGHT:
			oldpos = box->text.cursor_pos++;
			if (box->text.cursor_pos >= textlen)
				box->text.cursor_pos = textlen;

			if (oldpos != box->text.cursor_pos) {
				box->attr.dirty = 1;
			}
			return 0;

		case DIKS_CURSOR_LEFT:
			oldpos = box->text.cursor_pos--;
			if (box->text.cursor_pos < 0)
				box->text.cursor_pos = 0;

			if (oldpos != box->text.cursor_pos) {
				box->attr.dirty = 1;
			}
			return 0;

		case DIKS_BACKSPACE:
			if (box->text.cursor_pos > 0) {
				memcpy(text, box->text.buf, box->text.cursor_pos - 1);
				memcpy(text + box->text.cursor_pos - 1, box->text.buf + box->text.cursor_pos,
						strlen(box->text.buf) - box->text.cursor_pos + 1);
				box->text.cursor_pos--;
				strcpy(box->text.buf, text);

				box->attr.dirty = 1;
			}
			return 0;

		default:
			if (evt->key_symbol >= DIKS_SPACE && evt->key_symbol <= DIKS_SMALL_Z) {
				snprintf(text, box->text.cursor_pos + 1, "%s", box->text.buf);
				sprintf(text + box->text.cursor_pos, "%c", evt->key_symbol);
				sprintf(text + box->text.cursor_pos + 1, "%s", box->text.buf + box->text.cursor_pos);
				box->text.cursor_pos++;
				strcpy(box->text.buf, text);

				box->attr.dirty = 1;
			}
			return 0;
		}
	}

	return 0;
}

#define fbox_edit_new(self, parent) \
	(fbox_t*)fbox_edit_new_((fbox_t*)self, (fbox_t*)parent)

fedit_t *fbox_edit_new_(fbox_t *self, fbox_t *parent)
{
	fedit_t *edit = (fedit_t*)self;
	if (!edit)
		edit = (fedit_t*)kmem_alloz(1, fedit_t);
	fbox_cons(&edit->box, "edit", parent);

	edit->box.type = BT_EDIT;
	edit->box.attr.can_focus = 1;

	fbox_set_fg_color(&edit->box, BS_NORMAL, 0xff, 0xff, 0xff);
	fbox_set_fg_color(&edit->box, BS_FOCUSED, 0xff, 0xff, 0xff);

	fbox_set_bg_color(&edit->box, BS_NORMAL, 0x70, 0x49, 0x49);
	fbox_set_bg_color(&edit->box, BS_FOCUSED, 0x1f, 0x00, 0xff);
	fbox_set_bg_color(&edit->box, BS_FOCUSED, 0xd6, 0xc7, 0x31);

	edit->box.on_evt_default = edit_on_evt;

	self->text.len = 256;
	self->text.buf = (char*)kmem_alloz(self->text.len, char);
	self->text.cursor_pos = 0;
	self->text.align = TL_CENTER;

	fbox_set_font_size(self,BS_FOCUSED, 'z');

	return edit;
}

void fbox_edit_set_margin(fbox_t *box, int l, int r, int t, int b)
{
	fedit_t *edit = (fedit_t*)box;
	edit->margin.l = l;
	edit->margin.r = r;
	edit->margin.t = t;
	edit->margin.b = b;
}


