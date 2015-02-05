/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __FBOX_H__
#define __FBOX_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <hilda/ktypes.h>
#include <hilda/sdlist.h>

#include <directfb.h>

/*-----------------------------------------------------------------------
 * Layout : Size
 */
#define SZ_MSC_JDXD             0x8000 /* JueDui or XiangDui Mask */
#define SZ_JD                   0x8000 /* JueDui: in pixels */
#define SZ_XD                   0x0000 /* XiangDui: per mill for client area or parent window */

#define SZ_MSC_FU_KEHU          0x4000 /* FuChuangkou or KeHu area mask */
#define SZ_FU                   0x4000 /* FuChuangkou */
#define SZ_KH                   0x0000 /* KeHu area */

#define SZ_MSC_CTX              0x3FFF /* Real width or height */

/*-----------------------------------------------------------------------
 * Layout : posit
 */
#define PS_MSC_JDXD             0x8000 /* JueDui or XiangDui Mask */
#define PS_JD                   0x8000 /* JueDui: in pixels */
#define PS_XD                   0x0000 /* XiangDui: per mill for client area or parent window */

#define PS_MSC_FU_KEHU          0x4000 /* FuChuangkou or KeHu area mask */
#define PS_FU                   0x4000 /* FuChuangkou */
#define PS_KH                   0x0000 /* KeHu area */

#define PS_MSC_BASE_EDGE        0x3000 /* Map base mask */
#define PS_R                    0x3000 /* Base on right edge */
#define PS_B                    0x3000 /* Base on bottom edge */
#define PS_L                    0x2000 /* Base on left edge */
#define PS_T                    0x2000 /* Base on top edge */
#define PS_C                    0x1000 /* Base on center point */
#define PS_BASE_EDGE_FF         0x0000 /* FeiFa: illegal */

#define PS_MSC_CTX              0x0FFF /* Real x or y */

#define PS_OCP_N                0      /* None */
#define PS_OCP_L                1      /* Left */
#define PS_OCP_T                2      /* Top */
#define PS_OCP_R                3      /* Right */
#define PS_OCP_B                4      /* Bottom */

typedef struct _fbox_t fbox_t;
typedef struct _krect_t krect_t;
typedef struct _ksize_t ksize_t;
typedef struct _kpos_t kpos_t;

struct _krect_t {
	short x, y, w, h;
};

struct _ksize_t {
	short w, h;
};

struct _kpos_t {
	short x, y;
};

/* Box Type */
#define BT_IMAGE                0x00000001
#define BT_BUTTON               0x00000002
#define BT_EDIT                 0x00000003

/* Text aLigin */
#define TL_LEFT                 1
#define TL_RIGHT                2
#define TL_CENTER               3

#define TL_MASK                 0xff
#define TL_SHIFT                8

#define TL_HLEFT                (TL_LEFT << 0)
#define TL_HRIGHT               (TL_RIGHT << 0)
#define TL_HCENTER              (TL_CENTER << 0)

#define TL_VLEFT                (TL_LEFT << TL_SHIFT)
#define TL_VRIGHT               (TL_RIGHT << TL_SHIFT)
#define TL_VCENTER              (TL_CENTER << TL_SHIFT)

/* Image Mode */
#define IM_STRETCH              0
#define IM_LEFT                 1
#define IM_CENTER               2
#define IM_RIGHT                3

#define IM_MASK                 0xff
#define IM_SHIFT                8

#define IM_HSTRETCH             (IM_STRETCH << 0)
#define IM_HLEFT                (IM_LEFT << 0)
#define IM_HCENTER              (IM_CENTER << 0)
#define IM_HRIGHT               (IM_RIGHT << 0)

#define IM_VSTRETCH             (IM_STRETCH << IM_SHIFT)
#define IM_VLEFT                (IM_LEFT << IM_SHIFT)
#define IM_VCENTER              (IM_CENTER << IM_SHIFT)
#define IM_VRIGHT               (IM_RIGHT << IM_SHIFT)

typedef enum {
	BS_MIN,
	BS_NORMAL = BS_MIN,     /**< Button is in a normal draw state */
	BS_DISABLED,            /**< disabled window won't hover and focus */
	BS_HOVER,
	BS_HOVER_CHECK,
	BS_FOCUSED,
	BS_FOCUSED_CHECK,
	BS_PRESSED,
	BS_PRESSED_CHECK,
	BS_CHECKED,
	BS_MAX
} LAF_STATE;

typedef struct _fbox_palette_t fbox_palette_t;
struct _fbox_palette_t {
	unsigned char r, g, b;
};

typedef int (*ON_EVT)(fbox_t *box, DFBWindowEvent *evt);

struct _fbox_t {
	/*
	 * Layout part
	 */
	K_dlist_entry box_ent;
	K_dlist_entry sub_box_hdr;

	fbox_t *parent;

	char *name;

	int type;

	struct {
		kuint align : 3;

		kuint will_free : 1;
		kuint created : 1;

		kuint can_focus : 1;
		kuint can_check : 1;

		kuint disabled : 1; /* gray */
		kuint hovered : 1; /* mouse on */
		kuint pressed : 1;
		kuint checked : 1;
		kuint focused : 1; /* has input focus */

		/* visible */
		kuint shown : 1;

		/* paint */
		kuint dirty : 1;
	} attr;

	ksize_t size;
	kpos_t posit;
	krect_t rect;
	krect_t rect_scr;

	/*
	 * WinProc
	 */

	/* user defined on_event callback, return 1 will call on_evt_default */
	int (*on_evt_usr)(fbox_t *box, DFBWindowEvent *evt);

	/* default on_event callback */
	int (*on_evt_default)(fbox_t *box, DFBWindowEvent *evt);
void (*on_destory)(fbox_t *box); void (*on_paint)(fbox_t *box, int state, kbool force);
	void (*on_focus)(fbox_t *box, kbool set);

	/*
	 * Text
	 */
	struct {
		char *buf;
		int len;
		int align;
		int cursor_pos;                /* -1 means not show, or should be zero by default */
	} text;

	/*
	 * Look and Feel
	 */
	struct {
		/* XXX: Not all the state can be used in certain widget */
		LAF_STATE state;

		/* desktop()->dfb->CreateSurface(desktop()->dfb, &desc, &bgsurface); */
		IDirectFBSurface *img_surface[BS_MAX];

		/* desktop()->dfb->CreateImageProvider(desktop()->dfb, DATADIR"/desktop.png", &provider); */
		IDirectFBImageProvider *img_provider[BS_MAX];

		char *img_path[BS_MAX];

		/* IM_STRETCH etc */
		short img_mode[BS_MAX];
		short img_width[BS_MAX];
		short img_height[BS_MAX];

		/*
		 * color
		 */
		/* set_fg_color(state, r, g, b) */
		short fg_cr_r[BS_MAX], fg_cr_g[BS_MAX], fg_cr_b[BS_MAX];

		/* set_bg_color(state, r, g, b) */
		short bg_cr_r[BS_MAX], bg_cr_g[BS_MAX], bg_cr_b[BS_MAX];

		fbox_palette_t *palette[BS_MAX];

		/*
		 * font
		 */
		short font_size[BS_MAX];
	} laf;
};

/*---------------------------------------------------------------------------------
 * Helper
 */
/* macro for a safe call to DirectFB functions */
#define DFBCHECK(x...) \
{ \
	int err = x; \
	if (err != DFB_OK) { \
		fprintf(stderr, "%s <%d>:\n\t", __FILE__, __LINE__); \
		DirectFBErrorFatal(#x, err); \
	} \
}

typedef struct _fgui_t fgui_t;
struct _fgui_t {
	kbool visible;
	fbox_t *focused;
	fbox_t *hovered;
};

fgui_t *gui();

/*-----------------------------------------------------------------------
 * C part
 */
fbox_t *fbox_cons(fbox_t *box, char *name, fbox_t *parent);

void fbox_set_parent(fbox_t *box, fbox_t *parent);

#define fbox_set_posit(box, align, x, y) \
	fbox_set_posit_((fbox_t*)box, align, x, y)
void fbox_set_posit_(fbox_t *box, kuint align, kushort x, kushort y);

#define fbox_set_size(box, w, h) \
	fbox_set_size_((fbox_t*)box, w, h)
void fbox_set_size_(fbox_t *box, kushort w, kushort h);

#define fbox_set_name(box, name) \
	fbox_set_name_((fbox_t*)box, name)
void fbox_set_name_(fbox_t *box, const char *name);

/*
 * a_minus_b = a - b
 * minus_type : b's type
 */

void fbox_get_scr_rect(fbox_t *box, krect_t *rect);
kuint fbox_layout(fbox_t *box);

void fbox_to_top(fbox_t *box);
void fbox_to_bottom(fbox_t *box);

#define fbox_set_image(box, state, path, mode) \
	fbox_set_image_((fbox_t*)box, state, path, mode)
int fbox_set_image_(fbox_t *box, LAF_STATE state,
		const char *path, unsigned int mode);

#define fbox_set_text_align(box, align) \
	fbox_set_text_align_((fbox_t*)box, align)
void fbox_set_text_align_(fbox_t *box, int align);

#define fbox_set_text(box, text) \
	fbox_set_text_((fbox_t*)box, text)
int fbox_set_text_(fbox_t *box, const char *text);

#define fbox_set_fg_color(box, s, r, g, b) \
	fbox_set_fg_color_((fbox_t*)box, s, r, g, b)
void fbox_set_fg_color_(fbox_t *box, LAF_STATE state,
		short r, short g, short b);

#define fbox_set_bg_color(box, s, r, g, b) \
	fbox_set_bg_color_((fbox_t*)box, s, r, g, b)
void fbox_set_bg_color_(fbox_t *box, LAF_STATE state,
		short r, short g, short b);

#define fbox_set_palette(box, s, p) \
	fbox_set_palette_((fbox_t*)box, s, p)
void fbox_set_palette_(fbox_t *box, LAF_STATE state,
		fbox_palette_t pat[]);

#define fbox_set_font_size(box, s, sz) \
	fbox_set_font_size_((fbox_t*)box, s, sz)
void fbox_set_font_size_(fbox_t *box, LAF_STATE state, int size);

#define fbox_set_next_focus(box, n, p) \
	fbox_set_next_focus_((fbox_t*)box, (fbox_t**)n, (fbox_t**)p)
int fbox_set_next_focus_(fbox_t *box, fbox_t **next, fbox_t **prev);

#define fbox_repaint(box, f) \
	fbox_repaint_((fbox_t*)box, f)
int fbox_repaint_(fbox_t *box, kbool force);

void fbox_set_post_create_proc(fbox_t *box,
		void (*post_create)(fbox_t *box));

#define fbox_set_event_proc(box, on_evt) \
	fbox_set_event_proc_((fbox_t*)box, on_evt)
void fbox_set_event_proc_(fbox_t *box, ON_EVT on_evt);

void fbox_evt_do_default(fbox_t *box, DFBWindowEvent *evt);

#define fbox_get_top_parent(box) \
	fbox_get_top_parent_((fbox_t*)box)
fbox_t *fbox_get_top_parent_(fbox_t *box);

fbox_t *fbox_get_top_visable();

void fbox_init(int *argc, char ***argv);
int fbox_main();
void fbox_set_quit();

void fbox_show_full(fbox_t *box, kbool show);
void fbox_set_repaint();

void fbox_drop_key(kbool drop);

#define fbox_repaint_all(box, f) \
	fbox_repaint_all_((fbox_t*)box, f)
int fbox_repaint_all_(fbox_t *box, kbool force);

#define fbox_show(box, show) \
	fbox_show_((fbox_t*)box, show, kfalse)
#define fbox_show_all(box, show) \
	fbox_show_((fbox_t*)box, show, ktrue)
void fbox_show_(fbox_t *box, kbool show, kbool recursive);

#define fbox_active(box, active) \
	fbox_active_((fbox_t*)box, active)
void fbox_active_(fbox_t *box, kbool active);

#define fbox_focus(box) \
	fbox_focus_((fbox_t*)(box))
void fbox_focus_(fbox_t *box);

void fbox_create_window(fbox_t *box);

void fbox_call(void (*mqw)(void *ua, void *ub), void *ua, void *ub);
void fbox_waitcall(void (*mqw)(void *ua, void *ub), void *ua, void *ub);

/* Helper functions */
void split_ipaddr(const char *ipaddr,
		char seg0[], char seg1[], char seg2[], char seg3[],
		char port[]);
int filter_edit_number(fbox_t *box, int newkey,
		int minnum, int maxnum, int maxlen);

fbox_t *fbox_from_pt(fbox_t *box, int x, int y);

/*
 * public mqw function
 */
void mqw_ui_repaint_all(void *ua, void *ub);
void mqw_ui_hide_all(void *ua, void *ub);

#ifdef __cplusplus
}
#endif
#endif /* __FBOX_H__ */

