/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <hilda/kmem.h>
#include <hilda/klog.h>
#include <hilda/kstr.h>
#include <hilda/kmque.h>
#include <hilda/karg.h>

#include <fbox.h>
#include <fdesktop.h>

#include <fbutton.h>

#include <fedit.h>

#include <math.h>

#ifndef MIN
#define MIN(x,y) ((x) > (y) ? (y) : (x))
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif

#define REAL_IS_FLOAT

typedef struct _waitcall_t waitcall_t;
struct _waitcall_t {
	void (*mqw)(void *ua, void *ub);
	void *ua;
	void *ub;
};

static kbool volatile __g_running = kfalse;
static kbool volatile __g_quited = kfalse;
static kbool __g_paint_pending = kfalse;
static kbool __g_drop_key = kfalse;

static SPL_HANDLE __g_fbcall_sema;

kmque_t *__g_dfb_mque = NULL;

static void erase_screen();

static fgui_t *__g_gui = NULL;

fgui_t *gui()
{
	return __g_gui;
}

/*-----------------------------------------------------------------------
 * C part
 */
fbox_t *fbox_cons(fbox_t *box, char *name, fbox_t *parent)
{
	int i, will_free = !box;

	if (!box && !(box = kmem_alloz(1, fbox_t)))
		return NULL;

	box->name = name ? kstr_dup(name) : NULL;
	kdlist_init_head(&box->sub_box_hdr);
	kdlist_init_head(&box->box_ent);

	box->attr.will_free = will_free;
	box->attr.dirty = 1;

	box->laf.state = -1;
	for (i = 0; i < BS_MAX; i++) {
		box->laf.fg_cr_r[i] = -1;
		box->laf.fg_cr_g[i] = -1;
		box->laf.fg_cr_b[i] = -1;

		box->laf.bg_cr_r[i] = -1;
		box->laf.bg_cr_g[i] = -1;
		box->laf.bg_cr_b[i] = -1;

		box->laf.palette[i] = NULL;
	}

	box->text.align = TL_HCENTER | TL_VCENTER;

	box->parent = parent;
	if (parent)
		kdlist_insert_tail_entry(&parent->sub_box_hdr, &box->box_ent);
	else
		kdlist_insert_tail_entry(&desktop()->fbox_hdr, &box->box_ent);

	box->text.cursor_pos = -1;
	return box;
}

void fbox_set_parent(fbox_t *box, fbox_t *parent)
{
	kdlist_remove_entry(&box->box_ent);

	box->parent = parent;
	if (parent)
		kdlist_insert_tail_entry(&parent->sub_box_hdr, &box->box_ent);
	else
		kdlist_insert_tail_entry(&desktop()->fbox_hdr, &box->box_ent);
}

fbox_t *fbox_get_top_parent_(fbox_t *box)
{
	fbox_t *current = box, *parent = current->parent;

	while (parent) {
		current = parent;
		parent = current->parent;
	}

	return current;
}

fbox_t *fbox_get_top_visable()
{
	K_dlist_entry *entry, *qhdr;
	fbox_t *subbox;

	qhdr = &desktop()->fbox_hdr;
	entry = qhdr->next;
	while (entry != qhdr) {
		subbox = FIELD_TO_STRUCTURE(entry, fbox_t, box_ent);
		if (subbox->attr.shown)
			return subbox;
		entry = entry->next;
	}

	return NULL;
}

fbox_t *fbox_from_pt(fbox_t *box, int x, int y)
{
	fbox_t *tmpbox, *last, *tmpbox2;
	K_dlist_entry *entry, *qhdr;

	if (box) {
		if (!box->attr.shown)
			return NULL;
		if (box->rect_scr.x > x ||
				box->rect_scr.y > y ||
				box->rect_scr.x + box->rect_scr.w <= x ||
				box->rect_scr.y + box->rect_scr.h <= y)
			return NULL;
		qhdr = &box->sub_box_hdr;
	}
	else
		qhdr = &desktop()->fbox_hdr;

	entry = qhdr->prev;
	while (entry != qhdr) {
		tmpbox = FIELD_TO_STRUCTURE(entry, fbox_t, box_ent);
		entry = entry->prev;

		if (!tmpbox->attr.shown)
			continue;
		if (tmpbox->rect_scr.x <= x &&
				tmpbox->rect_scr.y <= y &&
				tmpbox->rect_scr.x + tmpbox->rect_scr.w > x &&
				tmpbox->rect_scr.y + tmpbox->rect_scr.h > y) {
			tmpbox2 = fbox_from_pt(tmpbox, x, y);
			if (tmpbox2)
				return tmpbox2;
			else
				return tmpbox;
		}
	}

	return box;
}

void fbox_to_top(fbox_t *box)
{
	K_dlist_entry *hdr;

	if (box->parent)
		hdr = &box->parent->sub_box_hdr;
	else
		hdr = &desktop()->fbox_hdr;

	/* printf("fbox_to_top: box:%p, %s\n", box, box->name); */
	kdlist_remove_entry(&box->box_ent);
	kdlist_insert_tail_entry(hdr, &box->box_ent);
}

void fbox_to_bottom(fbox_t *box)
{
	K_dlist_entry *hdr;

	if (box->parent)
		hdr = &box->parent->sub_box_hdr;
	else
		hdr = &desktop()->fbox_hdr;

	kdlist_remove_entry(&box->box_ent);
	kdlist_insert_head_entry(hdr, &box->box_ent);
}

void fbox_set_posit_(fbox_t *box, kuint align, kushort x, kushort y)
{
	if (!box)
		return;

	box->attr.align = align;
	box->posit.x = x;
	box->posit.y = y;
}

void fbox_set_size_(fbox_t *box, kushort w, kushort h)
{
	if (!box)
		return;

	box->size.w = w;
	box->size.h = h;
}

void fbox_set_name_(fbox_t *box, const char *name)
{
	kmem_free_sz(box->name);
	box->name = kstr_dup(name);
}

static kuint fbox_get_parent_rect(fbox_t *box, krect_t *rect)
{
	if (!box->parent) {
		rect->x = 0;
		rect->y = 0;
		rect->w = desktop()->width;
		rect->h = desktop()->height;
	} else {
		rect->x = box->parent->rect.x;
		rect->y = box->parent->rect.y;
		rect->w = box->parent->rect.w;
		rect->h = box->parent->rect.h;
	}
	return 0;
}

static void fbox_get_win_rect(fbox_t *box, krect_t *rect)
{
	*rect = box->rect;
}

void fbox_get_scr_rect(fbox_t *box, krect_t *rect)
{
	short offx, offy;
	fbox_t *tmpbox;

	fbox_get_win_rect(box, rect);
	offx = rect->x;
	offy = rect->y;

	tmpbox = box->parent;
	while (tmpbox) {
		offx += tmpbox->rect.x;
		offy += tmpbox->rect.y;
		tmpbox = tmpbox->parent;
	}
	rect->x = offx;
	rect->y = offy;
}

static void rect_move(krect_t *dest, kushort x, kushort y)
{
	dest->x = x;
	dest->y = y;
}

#define FBOX_MIN(x, y) ((x) > (y) ? (y) : (x))
#define FBOX_MAX(x, y) ((x) > (y) ? (x) : (y))

static int rect_intersect(krect_t *intersection,
		const krect_t *i1, const krect_t *i2)
{
	krect_t out;
	out.x = FBOX_MAX(i1->x, i2->x);
	out.y = FBOX_MAX(i1->y, i2->y);
	out.w = /*right*/FBOX_MIN(i1->x + i1->w, i2->x + i2->w)/**/ - out.x;
	out.h = /*bottom*/FBOX_MIN(i1->y + i1->h, i2->y + i2->h)/**/ - out.y;

	if ((out.w < 0) || (out.h < 0))
		return 0;

	if (intersection != NULL)
		*intersection = out;

	return 1;
}

/*
 * a_minus_b = a - b
 * minus_type : b's type
 */
static int rect_subtract(krect_t *a_minus_b,
		const krect_t *a, const krect_t *b, kuint minus_type)
{
	krect_t t;
	krect_t a_jian_b;
	krect_t la = *a;

	if (0 == rect_intersect(&t, a, b))
		return 0;

	switch (minus_type) {
	case PS_OCP_N:
		a_jian_b = *a;
		break;
	case PS_OCP_L:
		a_jian_b.x = t.x + t.w;
		a_jian_b.y = la.y;
		a_jian_b.w = la.w - (t.x - la.x) - t.w;
		a_jian_b.h = la.h;
		break;
	case PS_OCP_T:
		a_jian_b.x = a->x;
		a_jian_b.y = t.y + t.h;
		a_jian_b.w = a->w;
		a_jian_b.h = a->h - (t.y - a->y) - t.h;
		break;
	case PS_OCP_R:
		a_jian_b.x = a->x;
		a_jian_b.y = a->y;
		a_jian_b.w = t.x - a->x;
		a_jian_b.h = a->h;
		break;
	case PS_OCP_B:
		a_jian_b.x = a->x;
		a_jian_b.y = a->y;
		a_jian_b.w = a->w;
		a_jian_b.h = t.y - a->y;
		break;
	default:
		return 0;
	}
	*a_minus_b = a_jian_b;
	return 1;
}

#if 0
static int rect_subtract3(krect_t *a_minus_b,
		const krect_t *a, const krect_t *b)
{
	(*a_minus_b) = (*a);
	if (b->x <= a->x && b->x + b->w >= a->x + a->w) {
		/* intersect in x direction */
		if (b->y <= a->y && b->y + b->h >= a->y + a->h) {
			/* cut off top */
			a_minus_b->y = b->y + b->h;
			return 1;
		}
		if (b->y + b->h >= a->y + a->h && b->y <= a->y + a->h) {
			/* cut off bottom */
			a_minus_b->h = b->y - a_minus_b->y;
			return 1;
		}
	}
	if (b->y <= a->y && b->y + b->h >= a->y + a->h) {
		/* intersect in y direction */
		if (b->x <= a->x && b->x + b->w >= a->x) {
			/* cut off left */
			a_minus_b->x = b->x + b->w;
			return 1;
		}
		if (b->x + b->w >= a->x + a->w && b->x <= a->x + a->w) {
			/* cut off right */
			a_minus_b->w = b->x - a_minus_b->x;
			return 1;
		}
	}
	return 0;
}
#endif

static kuint fbox_get_parent_client_rect(fbox_t *box, krect_t *rect)
{
	fbox_t *tmpbox;
	K_dlist_entry *entry;

	krect_t prc/*parent*/, trc/*temp*/;

	if (!box->parent) {
		rect->x = 0;
		rect->y = 0;
		rect->w = desktop()->width;
		rect->h = desktop()->height;
	} else {
		fbox_get_parent_rect(box, &prc);
		rect_move(&prc, 0, 0);

		entry = box->parent->sub_box_hdr.next;
		while (entry != &box->box_ent) {
			tmpbox = FIELD_TO_STRUCTURE(entry, fbox_t, box_ent);
			fbox_get_win_rect(tmpbox, &trc);
			rect_subtract(&prc, &prc, &trc, tmpbox->attr.align);
			entry = entry->next;
		}
		*rect = prc;
	}
	return 0;
}

kuint fbox_layout(fbox_t *box)
{
	K_dlist_entry *entry;
	krect_t prc, crc;
	fbox_t *subbox;

	if (!box) {
		entry = desktop()->fbox_hdr.next;
		while (entry != &desktop()->fbox_hdr) {
			subbox = FIELD_TO_STRUCTURE(entry, fbox_t, box_ent);
			fbox_layout(subbox);
			entry = entry->next;
		}
		return 0;
	}

	/*
	 * layout self
	 */
#if 0
	klog("layout: %s: size(%x, %x, %x, %x)\n",
			box->name, box->posit.x, box->posit.y,
			box->size.w, box->size.h);
#endif
	fbox_get_parent_rect(box, &prc);
	fbox_get_parent_client_rect(box, &crc);
	rect_move(&prc, 0, 0);

	/*
	 * 1. size
	 */

	/*
	 * 1.1 size.w
	 */
	if (SZ_JD == (box->size.w & SZ_MSC_JDXD))
		box->rect.w = (box->size.w & SZ_MSC_CTX);
	else
		if (SZ_KH == (box->size.w & SZ_MSC_FU_KEHU))
			box->rect.w = (box->size.w & SZ_MSC_CTX) * crc.w / 1000;
		else
			box->rect.w = (box->size.w & SZ_MSC_CTX) * prc.w / 1000;

	/*
	 * 1.2 size.h
	 */
	if (SZ_JD == (box->size.h & SZ_MSC_JDXD))
		box->rect.h = (box->size.h & SZ_MSC_CTX);
	else
		if (SZ_KH == (box->size.h & SZ_MSC_FU_KEHU))
			box->rect.h = (box->size.h & SZ_MSC_CTX) * crc.h / 1000;
		else
			box->rect.h = (box->size.h & SZ_MSC_CTX) * prc.h / 1000;

	/*
	 * 2. posit
	 */

	/*
	 * 2.1 posit.x
	 */
	if (PS_JD == (box->posit.x & PS_MSC_JDXD))
		if (PS_KH == (box->posit.x & PS_MSC_FU_KEHU))
			box->rect.x = crc.x + (box->posit.x & PS_MSC_CTX);
		else
			box->rect.x = (box->posit.x & PS_MSC_CTX);
	else
		if (PS_KH == (box->posit.x & PS_MSC_FU_KEHU))
			if (PS_R == (box->posit.x & PS_MSC_BASE_EDGE))
				box->rect.x = crc.x + crc.w - (1000 - (box->posit.x & PS_MSC_CTX)) * crc.w / 1000 - box->rect.w;
			else if (PS_L == (box->posit.x & PS_MSC_BASE_EDGE))
				box->rect.x = crc.x + (box->posit.x & PS_MSC_CTX) * crc.w / 1000;
			else if (PS_C == (box->posit.x & PS_MSC_BASE_EDGE))
				box->rect.x = crc.x + (box->posit.x & PS_MSC_CTX) * crc.w / 1000 - box->rect.w / 2;
			else
				printf("bad:X:PS_KH\n");
		else
			if (PS_R == (box->posit.x & PS_MSC_BASE_EDGE))
				box->rect.x = prc.w - (1000 - (box->posit.x & PS_MSC_CTX)) * prc.w / 1000 - box->rect.w;
			else if (PS_L == (box->posit.x & PS_MSC_BASE_EDGE))
				box->rect.x = (box->posit.x & PS_MSC_CTX) * prc.w / 1000;
			else if (PS_C == (box->posit.x & PS_MSC_BASE_EDGE))
				box->rect.x = (box->posit.x & PS_MSC_CTX) * prc.w / 1000 - box->rect.w / 2;
			else
				printf("bad:X:PS_FU\n");

	/*
	 * 2.2 posit.y
	 */
	if (PS_JD == (box->posit.y & PS_MSC_JDXD))
		if (PS_KH == (box->posit.y & PS_MSC_FU_KEHU))
			box->rect.y = crc.y + (box->posit.y & PS_MSC_CTX);
		else
			box->rect.y = (box->posit.y & PS_MSC_CTX);
	else
		if (PS_KH == (box->posit.y & PS_MSC_FU_KEHU))
			if (PS_R == (box->posit.y & PS_MSC_BASE_EDGE))
				box->rect.y = crc.y + crc.h - (1000 - (box->posit.y & PS_MSC_CTX)) * crc.h / 1000 - box->rect.h;
			else if (PS_L == (box->posit.y & PS_MSC_BASE_EDGE))
				box->rect.y = crc.y + (box->posit.y & PS_MSC_CTX) * crc.h / 1000;
			else if (PS_C == (box->posit.y & PS_MSC_BASE_EDGE))
				box->rect.y = crc.y + (box->posit.y & PS_MSC_CTX) * crc.h / 1000 - box->rect.h / 2;
			else
				printf("bad:Y:PS_KH\n");
		else
			if (PS_B == (box->posit.y & PS_MSC_BASE_EDGE))
				box->rect.y = prc.h - (box->posit.y & PS_MSC_CTX) * prc.h / 1000 - box->rect.h;
			else if (PS_T == (box->posit.y & PS_MSC_BASE_EDGE))
				box->rect.y = (box->posit.y & PS_MSC_CTX) * prc.h / 1000;
			else if (PS_C == (box->posit.y & PS_MSC_BASE_EDGE))
				box->rect.y = (box->posit.y & PS_MSC_CTX) * prc.h / 1000 - box->rect.h / 2;
			else
				printf("bad:Y:PS_FU\n");

	fbox_get_scr_rect(box, &box->rect_scr);
	//printf("layout: %s: size(%d, %d, %d, %d)\n", box->name, box->rect_scr.x, box->rect_scr.y, box->rect_scr.w, box->rect_scr.h);
	box->attr.dirty = 1;

	/*
	 * walk the sub wins
	 */
	entry = box->sub_box_hdr.next;
	while (entry != &box->sub_box_hdr) {
		subbox = FIELD_TO_STRUCTURE(entry, fbox_t, box_ent);
		fbox_layout(subbox);
		entry = entry->next;
	}

	return 0;
}

int fbox_set_image_(fbox_t *box, LAF_STATE state,
		const char *path, unsigned int mode)
{
	if (state < BS_MIN || state >= BS_MAX)
		return -1;

	if (box->attr.created) {
		kerror("fbox_set_image: MUST called before DFB.\n");
		return -2;
	}

	kmem_free_sz(box->laf.img_path[state]);

	/* FIXME: set NULL for testing */
	if (path) {
		kmem_free_sz(box->laf.img_path[state]);
		box->laf.img_path[state] = kstr_dup(path);
		box->laf.img_mode[state] = mode;
	}

	return 0;
}

void fbox_set_fg_color_(fbox_t *box, LAF_STATE state, short r, short g, short b)
{
	box->laf.fg_cr_r[state] = r;
	box->laf.fg_cr_g[state] = g;
	box->laf.fg_cr_b[state] = b;
}
void fbox_set_bg_color_(fbox_t *box, LAF_STATE state, short r, short g, short b)
{
	box->laf.bg_cr_r[state] = r;
	box->laf.bg_cr_g[state] = g;
	box->laf.bg_cr_b[state] = b;
}
void fbox_set_palette_(fbox_t *box, LAF_STATE state, fbox_palette_t pat[])
{
	box->laf.palette[state] = pat;
}
void fbox_set_font_size_(fbox_t *box, LAF_STATE state, int size)
{
	box->laf.font_size[state] = size;
}

static int fbox_create_img_surface(fbox_t *box)
{
	int i;
	DFBSurfaceDescription desc, dsc;
	IDirectFBSurface *surface;
	IDirectFBImageProvider *provider;

	desc.flags = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_CAPS;
	desc.flags = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_CAPS | DSDESC_PIXELFORMAT;
	//desc.flags = DSDESC_WIDTH | DSDESC_HEIGHT;
	desc.width = box->rect.w;
	desc.height = box->rect.h;
	desc.caps = DSCAPS_NONE;
	desc.caps = DSCAPS_SHARED;
	desc.pixelformat = DSPF_ARGB;

	for (i = 0; i < BS_MAX; i++)
		if (box->laf.img_path[i]) {
			//printf("box:(%s), loading image: %s\n", box->name, box->laf.img_path[i]);

			desktop()->dfb->CreateImageProvider(desktop()->dfb, box->laf.img_path[i], &provider);
			box->laf.img_provider[i] = provider;
			//printf("provider:%p, dsc:%p\n", provider, &dsc);

			provider->GetSurfaceDescription(provider, &dsc);
			box->laf.img_width[i] = dsc.width;
			box->laf.img_height[i] = dsc.height;
			desktop()->dfb->CreateSurface(desktop()->dfb, &desc, &surface);
			box->laf.img_surface[i] = surface;
			surface->Clear(surface, 0x00, 0x00, 0x00, 0x00);
			// surface->SetDrawingFlags(surface, 1);
			// surface->SetRenderOptions(surface, 0);
			// surface->SetBlittingFlags(surface, DSBLIT_BLEND_ALPHACHANNEL);
			/*printf("box:(%s), image:w:%d, h:%d\n", box->name, dsc.width, dsc.height);*/

			/* render is slow, render image to surface, all at once */
			{
				int w = box->rect.w, h = box->rect.h;
				DFBRectangle r;
				unsigned int mode;

				/* h */
				mode = box->laf.img_mode[i] & IM_MASK;
				if (mode == IM_STRETCH) {
					r.w = w;
					r.x = 0;
				} else if (mode == IM_LEFT) {
					r.w = box->laf.img_width[i];
					r.x = 0;
				} else if (mode == IM_CENTER) {
					r.w = box->laf.img_width[i];
					r.x = (w - r.w) / 2;
				} else if (mode == IM_RIGHT) {
					r.w = box->laf.img_width[i];
					r.x = w - r.w;
				}

				/* v */
				mode = (box->laf.img_mode[i] >> IM_SHIFT) & IM_MASK;
				if (mode == IM_STRETCH) {
					r.h = h;
					r.y = 0;
				} else if (mode == IM_LEFT) {
					r.h = box->laf.img_height[i];
					r.y = 0;
				} else if (mode == IM_CENTER) {
					r.h = box->laf.img_height[i];
					r.y = (h - r.h) / 2;
				} else if (mode == IM_RIGHT) {
					r.h = box->laf.img_height[i];
					r.y = h - r.h;
				}

				provider->RenderTo(provider, surface, &r);
			}
		}

	return 0;
}

/* return paint action count */
int fbox_repaint_(fbox_t *box, kbool force)
{
	int cursor_x, strwidth, strheight, x, y;
	unsigned align;
	char *text;
	IDirectFBSurface *sf_top;
	IDirectFBSurface *sf_box;
	DFBRectangle r;
	LAF_STATE state;

	IDirectFBFont *font;

	int i;
	fbox_palette_t *palette;

	if (!box->attr.shown)
		return 0;
	if (!force && !box->attr.dirty)
		return 0;

	gui()->visible = ktrue;

	state = BS_NORMAL;
	/*printf("\n===>attr.disa:%d, attr.checked:%d, attr.focused:%d, attr.can_focus:%d\n",box->attr.disabled, box->attr.checked, box->attr.focused, box->attr.can_focus);*/
	if (box->attr.disabled)
		state = BS_DISABLED;
	else if (box->attr.checked) {
		if (box->attr.focused && box->attr.can_focus)
			state = BS_FOCUSED_CHECK;
		else
			state = BS_CHECKED;
	} else if (box->attr.focused) {
		state = box->attr.can_focus ? BS_FOCUSED : BS_NORMAL;
	} else if (box->attr.hovered) {
		state = BS_HOVER;
	}

	if (box->laf.state != -1)
		state = box->laf.state;

	/* clear every paint */
	box->laf.state = -1;

	if (box->on_paint) {
		box->on_paint(box, state, force);
		box->attr.dirty = 0;
		return 1;
	}

	sf_box = box->laf.img_surface[state];
	sf_top = desktop()->surface;

	r.x = 0;
	r.y = 0;
	r.w = box->rect.w;
	r.h = box->rect.h;

	/*printf("Blit: state:%d, src(.x=%d, .y:%d, .w:%d, .h:%d), dst(.x=%d, .y=%d)\n", */
	/*state, r.x, r.y, r.w, r.h, box->rect_scr.x, box->rect_scr.y); */
	if (sf_box)
		//sf_top->Blit(sf_top, sf_box, &r, box->rect_scr.x, box->rect_scr.y);
		sf_top->Blit(sf_top, sf_box, NULL, box->rect_scr.x, box->rect_scr.y);

	/*
	 * TODO: Add support for Button, Edit, CheckBox(?) fill or gradient color
	 */
	palette = box->laf.palette[state];
	/* if (state == BS_FOCUSED || state == BS_FOCUSED_CHECK) */
	/* palette = &__g_pat_black; */
	if (palette) {
		/* XXX: Hack! palette as the height of palette */
		int h = palette[0].r + (palette[0].g << 8) + (palette[0].b << 16);
		for (i = 0; i < box->rect_scr.h; i++) {
			sf_top->SetColor(sf_top, palette[i * h / r.h + 1].r, palette[i * h / r.h + 1].g,
					palette[i * h / r.h + 1].b, 0xff);
			sf_top->DrawLine(sf_top, box->rect_scr.x, box->rect_scr.y + i,
					box->rect_scr.x + box->rect_scr.w,
					box->rect_scr.y + i);
		}
	} else if (box->laf.bg_cr_r[state] != -1 && box->laf.bg_cr_g[state] != -1 && box->laf.bg_cr_b[state] != -1) {
		sf_top->SetColor(sf_top, box->laf.bg_cr_r[state], box->laf.bg_cr_g[state], box->laf.bg_cr_b[state], 0xff);
		sf_top->FillRectangle(sf_top, box->rect_scr.x, box->rect_scr.y, box->rect_scr.w, box->rect_scr.h);
	}

	if (box->text.buf)
		text = box->text.buf;
	else
		text = box->name;

	text = box->text.buf;
	if (text) {
		int textlen = strlen(text);

		/* FIXME: Hack! quick but dirty */
		if (box->type == BT_EDIT) {
			fedit_t *edit = (fedit_t*)box;
			if (edit->shadow) {
				static char shadow[1024];
				int i;

				for (i = 0; i < textlen; i++)
					shadow[i] = '*';
				shadow[i] = '\0';

				text = shadow;
			}
		}
		/*printf("\n>>>state:%d, text:%s,font_size:%c \n", state, text, box->laf.font_size[state] );*/
		if (box->laf.font_size[state] == 'd')
			font = desktop()->font_d;
		else if (box->laf.font_size[state] == 'e')
			font = desktop()->font_e;
		else if (box->laf.font_size[state] == 'y')
			font = desktop()->font_y;
		else if (box->laf.font_size[state] == 'z')
			font = desktop()->font_z;
		else
			font = desktop()->font_x;

		sf_top->SetFont(sf_top, font);

		if (box->laf.fg_cr_r[state] != -1 && box->laf.fg_cr_g[state] != -1 && box->laf.fg_cr_b[state] != -1)
			sf_top->SetColor(sf_top, box->laf.fg_cr_r[state], box->laf.fg_cr_g[state], box->laf.fg_cr_b[state], 0xff);
		else
			sf_top->SetColor(sf_top, 0x0c, 0x0c, 0x0c, 0xff);

		if (box->text.cursor_pos >= 0)
			font->GetStringWidth(font, text, box->text.cursor_pos, &cursor_x);
		font->GetStringWidth(font, text, textlen, &strwidth);
		font->GetHeight(font, &strheight);

		y = (box->rect.h - strheight) / 2;

		align = box->text.align & TL_MASK;
		if (TL_LEFT == align)
			x = 0;
		else if (TL_RIGHT == align)
			x = box->rect.w - strwidth;
		else
			x = (box->rect.w - strwidth) / 2;

		align = (box->text.align >> TL_SHIFT) & TL_MASK;
		if (TL_LEFT == align)
			y = 0;
		else if (TL_RIGHT == align)
			y = box->rect.h - strheight;
		else
			y = (box->rect.h - strheight) /2 ;

		x += box->rect_scr.x;
		y += box->rect_scr.y;

		if (box->text.cursor_pos >= 0 && box->attr.focused)
			sf_top->FillRectangle(sf_top, x + cursor_x, y + 3, 2, strheight - 6);

		/*printf("DrawString: %s\n", text);*/
		sf_top->DrawString(sf_top, text, -1, x, y, DSTF_LEFT | DSTF_TOP);
	}

	box->attr.dirty = 0;
	return 1;
}

void fbox_set_text_align_(fbox_t *box, int align)
{
	box->text.align = align;
}

int fbox_set_text_(fbox_t *box, const char *text)
{
	if (!text)
		text = "";

	if (!box->text.buf) {
		box->text.len = 256;
		box->text.buf = (char*)kmem_alloz(box->text.len, char);
	}

	/* FIXME: too long string */
	strcpy(box->text.buf, text);

	if (-1 != box->text.cursor_pos)
		box->text.cursor_pos = text ? strlen(text) : 0;

	box->attr.dirty = 1;
	fbox_repaint(box, kfalse);
	return 0;
}

void fbox_set_event_proc_(fbox_t *box, ON_EVT on_evt)
{
	box->on_evt_usr = on_evt;
}

void fbox_evt_do_default(fbox_t *box, DFBWindowEvent *evt)
{
	if (box->on_evt_default)
		box->on_evt_default(box, evt);
}

static void dump_pixelformat()
{
	printf("DSPF_ARGB1555\t:%08x\n", DSPF_ARGB1555);
	printf("DSPF_RGB16\t:%08x\n", DSPF_RGB16);
	printf("DSPF_RGB24\t:%08x\n", DSPF_RGB24);
	printf("DSPF_RGB32\t:%08x\n", DSPF_RGB32);
	printf("DSPF_ARGB\t:%08x\n", DSPF_ARGB);
	printf("DSPF_A8\t:%08x\n", DSPF_A8);
	printf("DSPF_YUY2\t:%08x\n", DSPF_YUY2);
	printf("DSPF_RGB332\t:%08x\n", DSPF_RGB332);
	printf("DSPF_UYVY\t:%08x\n", DSPF_UYVY);
	printf("DSPF_I420\t:%08x\n", DSPF_I420);
	printf("DSPF_YV12\t:%08x\n", DSPF_YV12);
	printf("DSPF_LUT8\t:%08x\n", DSPF_LUT8);
	printf("DSPF_ALUT44\t:%08x\n", DSPF_ALUT44);
	printf("DSPF_AiRGB\t:%08x\n", DSPF_AiRGB);
	printf("DSPF_A1\t:%08x\n", DSPF_A1);
	printf("DSPF_NV12\t:%08x\n", DSPF_NV12);
	printf("DSPF_NV16\t:%08x\n", DSPF_NV16);
	printf("DSPF_ARGB2554\t:%08x\n", DSPF_ARGB2554);
	printf("DSPF_ARGB4444\t:%08x\n", DSPF_ARGB4444);
	printf("DSPF_NV21\t:%08x\n", DSPF_NV21);
	printf("DSPF_AYUV\t:%08x\n", DSPF_AYUV);
	printf("DSPF_A4\t:%08x\n", DSPF_A4);
	printf("DSPF_ARGB1666\t:%08x\n", DSPF_ARGB1666);
	printf("DSPF_ARGB6666\t:%08x\n", DSPF_ARGB6666);
	printf("DSPF_RGB18\t:%08x\n", DSPF_RGB18);
	printf("DSPF_LUT2\t:%08x\n", DSPF_LUT2);
	printf("DSPF_RGB444\t:%08x\n", DSPF_RGB444);
	printf("DSPF_RGB555\t:%08x\n", DSPF_RGB555);
	printf("DSPF_BGR555\t:%08x\n", DSPF_BGR555);
}

static void release_font()
{
	if (desktop()->font_d) {
		desktop()->font_d->Release(desktop()->font_d);
		desktop()->font_d = NULL;
	}
	if (desktop()->font_e) {
		desktop()->font_e->Release(desktop()->font_e);
		desktop()->font_e = NULL;
	}
	if (desktop()->font_z) {
		desktop()->font_z->Release(desktop()->font_z);
		desktop()->font_z = NULL;
	}
	if (desktop()->font_y) {
		desktop()->font_y->Release(desktop()->font_y);
		desktop()->font_y = NULL;
	}
	if (desktop()->font_x) {
		desktop()->font_x->Release(desktop()->font_x);
		desktop()->font_x = NULL;
	}
}

static void create_font()
{
	DFBFontDescription desc;
	IDirectFB *dfb = desktop()->dfb;
	char *font_path = desktop()->font_path;
	IDirectFBFont *font;

	desc.flags = DFDESC_HEIGHT;
	desc.height = desktop()->height / 16;
	DFBCHECK(dfb->CreateFont(dfb, font_path, &desc, &font));
	desktop()->font_d = font;
	desktop()->font_d_height = desc.height;

	desc.flags = DFDESC_HEIGHT;
	desc.height = desktop()->height / 20;
	DFBCHECK(dfb->CreateFont(dfb, font_path, &desc, &font));
	desktop()->font_e = font;
	desktop()->font_e_height = desc.height;

	desc.flags = DFDESC_HEIGHT;
	desc.height = desktop()->height / 26;
	DFBCHECK(dfb->CreateFont(dfb, font_path, &desc, &font));
	desktop()->font_z = font;
	desktop()->font_z_height = desc.height;

	desc.flags = DFDESC_HEIGHT;
	desc.height = desktop()->height / 30 ;
	DFBCHECK(dfb->CreateFont(dfb, font_path, &desc, &font));
	desktop()->font_y = font;
	desktop()->font_y_height = desc.height;

	desc.flags = DFDESC_HEIGHT;
	desc.height = desktop()->height / 48;
	DFBCHECK(dfb->CreateFont(dfb, font_path, &desc, &font));
	desktop()->font_x = font;
	desktop()->font_x_height = desc.height;
}

static void release_main_surface()
{
	if (desktop()->surface) {
		desktop()->surface->Release(desktop()->surface);
		desktop()->surface = NULL;
	}
}

static void create_main_surface()
{
	IDirectFB *dfb = desktop()->dfb;
	DFBSurfaceDescription sdsc;
	IDirectFBSurface *primary_surface;
	IDirectFBSurface *sub_surface;

	printf("debug fbox.c line:%d\n", __LINE__);
	memset((char*)&sdsc, 0 , sizeof(sdsc));
	sdsc.flags = DSDESC_CAPS | DSDESC_PIXELFORMAT;
	sdsc.flags = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_CAPS;
	sdsc.width = desktop()->width;
	sdsc.height = desktop()->height;

	//printf("debug fbox.c line:%d\n", __LINE__);
	/* XXX: FIXME: DSCAPS_DOUBLE will cause the screen splash */
	//sdsc.caps  = DSCAPS_PRIMARY | DSCAPS_DOUBLE;
	//sdsc.caps  = DSCAPS_PRIMARY | DSCAPS_FLIPPING;
	sdsc.caps  = DSCAPS_PRIMARY | DSCAPS_SHARED;
	//sdsc.pixelformat = desktop()->layer_cfg.pixelformat;
	sdsc.pixelformat = DSPF_ARGB;

	//printf("debug fbox.c line:%d\n", __LINE__);
	DFBCHECK(dfb->CreateSurface(dfb, &sdsc, &primary_surface));
	//printf("debug fbox.c line:%d\n", __LINE__);


	/*primary_surface->SetDrawingFlags(primary_surface, 1);*/
	/*primary_surface->SetRenderOptions(primary_surface, 0);*/
	//printf("debug fbox.c line:%d\n", __LINE__);
	primary_surface->SetBlittingFlags(primary_surface, DSBLIT_BLEND_ALPHACHANNEL);
	//printf("debug fbox.c line:%d\n", __LINE__);
	desktop()->surface = primary_surface;
	//printf("debug fbox.c line:%d\n", __LINE__);
}

void mqw_screen_changed(void *ua, void *ub)
{
	int w = (int)ua, h = (int)ub;

	printf(">>>>>>>> mqw_screen_changed: w:%d, h:%d\n", w, h);

	if (w == desktop()->width && h == desktop()->height)
		return;

	desktop()->width = w;
	desktop()->height = h;

	erase_screen();

	release_font();
	create_font();

	release_main_surface();
	create_main_surface();

	fbox_layout(NULL);
	fbox_call(mqw_ui_repaint_all, (void*)ktrue, NULL);
}

static char *get_font_path(int argc, char *argv[])
{
	int i;
	char *font_path = "auv:FIXME: FONT", *env_font;

	env_font = getenv("FBOX_FONT");
	if (env_font)
		font_path = env_font;

	/* Font by argv */
	i = karg_find(argc, argv, "--fbox-font", 1);
	if (i)
		font_path = argv[i + 1];

	printf("font_path: %s\n", font_path);
	return font_path;
}

void fbox_init(int *argc, char ***argv)
{
	IDirectFB *dfb = NULL;
	IDirectFBDisplayLayer *layer = NULL;
	DFBDisplayLayerConfig *layer_cfg = NULL;

	DFBGraphicsDeviceDescription gdesc;

	printf("%s:%d\n", __func__, __LINE__);
	__g_fbcall_sema = spl_sema_new(0);

	printf("%s:%d\n", __func__, __LINE__);
	kdlist_init_head(&desktop()->fbox_hdr);
	__g_gui = (fgui_t*)kmem_alloz(1, fgui_t);

	printf("%s:%d\n", __func__, __LINE__);
	// dump_pixelformat();

	printf("%s:%d\n", __func__, __LINE__);
	desktop()->font_path = kstr_dup(get_font_path(*argc, *argv));

	printf("%s:%d\n", __func__, __LINE__);
	DirectFBInit(0, 0);
	printf("%s:%d\n", __func__, __LINE__);
	DFBCHECK(DirectFBCreate(&dfb));
	printf("%s:%d\n", __func__, __LINE__);
	desktop()->dfb = dfb;

	printf("dfb: %p\n", dfb);

	printf("%s:%d\n", __func__, __LINE__);
	// DFBCHECK(dfb->SetCooperativeLevel(dfb, DLSCL_ADMINISTRATIVE));
	// DFBCHECK(dfb->SetCooperativeLevel(dfb, DFSCL_NORMAL));
	printf("%s:%d\n", __func__, __LINE__);
	// DFBCHECK(dfb->GetDeviceDescription(dfb, &gdesc));
	printf("%s:%d\n", __func__, __LINE__);
	dfb->GetDisplayLayer(dfb, DLID_PRIMARY, &layer);
	printf("%s:%d\n", __func__, __LINE__);
	desktop()->layer = layer;

	printf("%s:%d\n", __func__, __LINE__);
	layer_cfg = &desktop()->layer_cfg;
	printf("%s:%d\n", __func__, __LINE__);
	layer->SetCooperativeLevel(layer, DLSCL_ADMINISTRATIVE);

	printf("%s:%d\n", __func__, __LINE__);
#if 0
	if (!((gdesc.blitting_flags & DSBLIT_BLEND_ALPHACHANNEL) &&
				(gdesc.blitting_flags & DSBLIT_BLEND_COLORALPHA))) {
	printf("%s:%d\n", __func__, __LINE__);
		layer_cfg->flags = DLCONF_BUFFERMODE;
		layer_cfg->buffermode = DLBM_BACKSYSTEM;

		layer->SetConfiguration(layer, layer_cfg);
	}
#endif

	printf("%s:%d\n", __func__, __LINE__);
	layer->GetConfiguration(layer, layer_cfg);
	//layer->EnableCursor(layer, 1);
	layer->SetBackgroundColor( layer, 0x00, 0x00, 0x00, 0x00);
	layer->SetBackgroundMode( layer, DLBM_COLOR);


	printf("%s:%d\n", __func__, __LINE__);
	desktop()->width = layer_cfg->width;
	desktop()->height = layer_cfg->height;

	printf("%s:%d\n", __func__, __LINE__);
	create_font();
	printf("%s:%d\n", __func__, __LINE__);
	create_main_surface();
	printf("%s:%d\n", __func__, __LINE__);
}

/* will recursive set */
void fbox_show_(fbox_t *box, kbool show, kbool recursive)
{
	K_dlist_entry *entry, *qhdr;
	fbox_t *subbox;

	if (!box)
		qhdr = &desktop()->fbox_hdr;
	else {
		if (box->attr.shown != show) {
			box->attr.shown = show;
			box->attr.dirty = 1;
		}
		qhdr = &box->sub_box_hdr;
	}

	if (!recursive)
		return;

	entry = qhdr->next;
	while (entry != qhdr) {
		subbox = FIELD_TO_STRUCTURE(entry, fbox_t, box_ent);
		fbox_show_(subbox, show, recursive);
		entry = entry->next;
	}
}

/* return box count touched */
int fbox_repaint_all_(fbox_t *box, kbool force)
{
	K_dlist_entry *entry, *qhdr;
	fbox_t *subbox;
	int touched = 0;

	if (!box)
		qhdr = &desktop()->fbox_hdr;
	else {
		if (!box->attr.shown)
			return 0;
		if (!force && !box->attr.dirty)
			return 0;

		touched += fbox_repaint(box, force);
		qhdr = &box->sub_box_hdr;
	}

	/* XXX: if self painted, sub box should be painted too */
	if (touched)
		force = ktrue;

	entry = qhdr->next;
	while (entry != qhdr) {
		subbox = FIELD_TO_STRUCTURE(entry, fbox_t, box_ent);
		touched += fbox_repaint_all_(subbox, force);
		entry = entry->next;
	}

	return touched;
}

static void kill_focus(fbox_t *box)
{
	if (box->on_focus)
		box->on_focus(box, kfalse);
	else {
		/* printf("killfocus : %p, %s\n", box, box->name); */
		box->attr.focused = 0;
		if (box->parent)
			box->parent->attr.dirty = 1;
		box->attr.dirty = 1;
	}
}

static void set_focus(fbox_t *box)
{
	fbox_t *topbox = fbox_get_top_parent(box);
	fbox_to_top(topbox);
	topbox->attr.shown = 1;

	/* printf("setfocus : %p, %s, on_focus:%p\n",
	 * box, box->name, box->on_focus); */
	if (box->on_focus)
		box->on_focus(box, ktrue);
	else {
		box->attr.focused = 1;
		if (box && box->parent)
			box->parent->attr.dirty = 1;
		box->attr.dirty = 1;
	}
}

void fbox_focus_(fbox_t *box)
{
	fbox_t *oldbox = gui()->focused;

	if (oldbox == box)
		return;

	if (oldbox)
		kill_focus(oldbox);

	gui()->focused = box;
	if (box)
		set_focus(box);
	else
		kerror("FIXME: reset focus box to wallpaper?\n");

	fbox_set_repaint();
}

void fbox_create_window(fbox_t *box)
{
	K_dlist_entry *entry, *qhdr;
	fbox_t *subbox;

	/* create self at first */
	if (box) {
	printf("box:%p\n", box);
		fbox_create_img_surface(box);
		qhdr = &box->sub_box_hdr;
	} else
		qhdr = &desktop()->fbox_hdr;

	/* create subs */
	entry = qhdr->next;
	while (entry != qhdr) {
		subbox = FIELD_TO_STRUCTURE(entry, fbox_t, box_ent);
		fbox_create_window(subbox);
		entry = entry->next;
	}
}

void fbox_set_quit()
{
	__g_running = kfalse;
}

void fbox_show_full(fbox_t *box, kbool show)
{
	fbox_show_all(box, show);
	if (show) {
		fbox_to_top(box);
		box->attr.shown = 1;
		fbox_focus(box);
	} else {
		fbox_to_bottom(box);
		box->attr.shown = 0;
	}
	fbox_set_repaint();
}

static int hijack_key(int keycode)
{
	if (keycode == DIKS_ESCAPE) {
		fbox_call(mqw_ui_hide_all, (void*)ktrue, NULL);
		return 1;
	} else if (keycode == DIKS_CAPITAL_Q) {
		printf("Kuit ....\n");
		exit(0);
	} else if (keycode == DIKS_CAPITAL_M) {
		/* printf("Will show cfg dialog\n"); */
		return 1;
	}
	return 0;
}

static void fill_key_event(DFBWindowEvent *evt, int keycode)
{
	memset(evt, 0, sizeof(DFBWindowEvent));

	evt->type = DWET_KEYDOWN;
	evt->flags = DWEF_NONE;
	evt->key_code = keycode;
	evt->key_id = keycode;
	evt->key_symbol = keycode;
	evt->modifiers = 0;
	evt->locks = 0;

	gettimeofday(&evt->timestamp, NULL);
	evt->clazz = DFEC_WINDOW;

	evt->window_id = 0;
}

void mqw_ui_remote_key(void *ua, void *ub)
{
	int keycode = (int)ua;
	DFBWindowEvent evt;
	fbox_t *box = gui()->focused;

	//printf(">>> mqw_ui_remote_key: keycode: %x\n", keycode);

	if (__g_drop_key)
		return;

	if (hijack_key(keycode))
		return;

	if (!box) {
		kerror("BUG!!! mqw_ui_remote_key: no focus box\n");
		return;
	} else
		printf("focusBox: %s, on_evt_usr:%p, on_evt_default:%p\n",
				box->name, box->on_evt_usr, box->on_evt_default);

	fill_key_event(&evt, keycode);

	if (box->on_evt_usr)
		box->on_evt_usr(box, &evt);
	else if (box->on_evt_default)
		box->on_evt_default(box, &evt);

	fbox_set_repaint();
}

static void mark_all_hide()
{
	K_dlist_entry *entry, *qhdr;
	fbox_t *subbox;

	/*printf("==>funcin:%s\n", __func__);*/
	qhdr = &desktop()->fbox_hdr;
	entry = qhdr->next;
	while (entry != qhdr) {
		/*printf("==>1:%s\n", __func__);*/
		subbox = FIELD_TO_STRUCTURE(entry, fbox_t, box_ent);
		if (subbox->attr.shown) {
			/*
			 *FIXME
			 *find code don not come here ever
			 */

			/*printf("==>2:%s\n", __func__);*/
			/*printf("\n==>%s:%s\n", __func__, subbox->text.buf);*/
			subbox->attr.shown = 0;
			subbox->attr.dirty = 1;
		}

		subbox->attr.focused = 0;
		subbox->attr.checked = 0;

		entry = entry->next;
	}
}

static void erase_screen()
{
#if 1
	IDirectFBSurface *sf = desktop()->surface;
	char *buffer = NULL;
	int i, pitch = -1, scr_w = -1, scr_h = -1, loop = 16;
	DFBRectangle rect;

	//scr_w = desktop()->width;
	//scr_h = desktop()->height;
	rect.x = rect.y = 0;
	rect.w = scr_w;
	rect.h = scr_h / loop;
	//pitch = 4;

	buffer = kmem_alloz(scr_w * scr_h * 4 / loop, char);
	for (i = 0; i < loop; i++) {
		sf->Write(sf, &rect, buffer, pitch);
		rect.y += scr_h / loop;
	}

	kmem_free_s(buffer);
	sf->Flip(sf, NULL, 0);
#endif
}

void mqw_ui_hide_all(void *ua, void *ub)
{
	gui()->visible = kfalse;
	gui()->focused = NULL;
	printf(">>> mqw_ui_hide_all\n");
	mark_all_hide();
	erase_screen();
}

void mqw_ui_repaint_all(void *ua, void *ub)
{
	K_dlist_entry *entry, *qhdr;
	fbox_t *subbox;
	IDirectFBSurface *sf_top;
	kbool force = (kbool)ua;
	int boxs_painted, shown_cnt = 0;

	if (!__g_paint_pending)
		return;

	__g_paint_pending = kfalse;
	sf_top = desktop()->surface;

	qhdr = &desktop()->fbox_hdr;
	entry = qhdr->next;
	while (entry != qhdr) {
		subbox = FIELD_TO_STRUCTURE(entry, fbox_t, box_ent);
		entry = entry->next;
		if (!subbox->attr.shown)
			continue;

		shown_cnt++;
		boxs_painted = fbox_repaint_all_(subbox, force);
		//break;
	}
	if (!shown_cnt)
		return mqw_ui_hide_all(NULL, NULL);

	//printf("total %d boxs painted\n", boxs_painted);
	sf_top->Flip(sf_top, NULL, 0);
}

static void mqw_waitcall(void *ua, void *ub)
{
	waitcall_t *wc = (waitcall_t*)ua;
	wc->mqw(wc->ua, wc->ub);
	spl_sema_rel(__g_fbcall_sema);
}

void fbox_waitcall(void (*mqw)(void *ua, void *ub), void *ua, void *ub)
{
	waitcall_t wc = { mqw, ua, ub };
	if (!__g_dfb_mque)
		return;

	mentry_post(__g_dfb_mque, mqw_waitcall, NULL, (void*)&wc, NULL);
	spl_sema_get(__g_fbcall_sema, -1);
}

void fbox_call(void (*mqw)(void *ua, void *ub), void *ua, void *ub)
{
	if (__g_dfb_mque)
		mentry_post(__g_dfb_mque, mqw, NULL, ua, ub);
}

int fbox_main()
{
	IDirectFBSurface *sf_top = desktop()->surface;

	__g_dfb_mque = kmque_new();

	__g_running = ktrue;
	__g_quited = kfalse;

	fbox_set_repaint();
	kmque_run(__g_dfb_mque);

	return 0;
}

void split_ipaddr(const char *ipaddr, char seg0[], char seg1[], char seg2[], char seg3[], char port[])
{
	char *tmp, *retaddr[4] = { seg0, seg1, seg2, seg3 };
	int i, index = 0, j = 0;
	char c;
	for (i = 0; (c = ipaddr[i]); i++) {
		if ((c == '.') || (c == ':')) {
			retaddr[index][j] = '\0';
			index++;
			j = 0;
			if (index > 3)
				break;
			continue;
		}
		retaddr[index][j++] = c;
	}

	if (!c)
		retaddr[index][j++] = c;

	if (port) {
		tmp = strchr(ipaddr, ':');
		if (tmp)
			strcpy(port, tmp + 1);
	}
}

/**
 * \brief check edit input number for underflow or overflow.
 *
 * \param box
 * \param newkey DIKS_XXX
 * \param min
 * \param max
 *
 * \return 0 for can continue, 1 for too long, 2 for overflow, -1 for underflow.
 */
int filter_edit_number(fbox_t *box, int newkey, int minnum, int maxnum, int maxlen)
{
	int textlen;
	int new_int;
	char text[1024];

	if (box->text.cursor_pos == -1)
		return 0;

	if (newkey < DIKS_0 || newkey > DIKS_9){
		if( (newkey != DIKS_COLON ) && ( newkey != DIKS_PERIOD )){
			return 0;
		}else{
			/*skip colon and comma*/
			return -2;
		}
	}

	textlen = strlen(box->text.buf);
	if (textlen + 1 > maxlen)
		return 2;

	snprintf(text, box->text.cursor_pos + 1, "%s", box->text.buf);
	sprintf(text + box->text.cursor_pos, "%c", newkey);
	sprintf(text + box->text.cursor_pos + 1, "%s", box->text.buf + box->text.cursor_pos);

	new_int = strtol(text, 0, 10);
	if (new_int > maxnum)
		return 1;
	else if (new_int < minnum)
		return -1;
	else
		return 0;
}

void fbox_set_repaint()
{
	__g_paint_pending = ktrue;
	fbox_call(mqw_ui_repaint_all, (void*)ktrue, NULL);
}

void fbox_drop_key(kbool drop)
{
	__g_drop_key = drop;
}

static void mqw_hover_event(void *ua, void *ub)
{
	static fbox_t *hoverbox;

	int pos = (int)ua;
	int key = (int)ub;
	short sx, sy;

	fbox_t *tmp;

	sx = pos >> 16;
	sy = pos & 0xffff;

	tmp = fbox_from_pt(NULL, sx, sy);
	if (tmp == hoverbox)
		return;

	if (hoverbox) {
		hoverbox->attr.hovered = 0;
		hoverbox->attr.dirty = 1;

		fbox_set_repaint();
	}

	hoverbox = tmp;
	if (hoverbox) {
		hoverbox->attr.hovered = 1;
		hoverbox->attr.dirty = 1;

		fbox_set_repaint();
	}
}

void fbox_mouse_event(int x, int y, int key)
{
	short sx = x, sy = y;
	int pos = sx << 16 | sy;

	fbox_call(mqw_hover_event, (void*)pos, (void*)key);
}

