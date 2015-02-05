/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <hilda/xtcool.h>
#include <hilda/klog.h>
#include <hilda/karg.h>
#include <hilda/kmque.h>

#include <hilda/sdlist.h>
#include <fbox.h>
#include <fimage.h>

#include <fbutton.h>
#include <fedit.h>

typedef struct _fb_login_t fb_login_t;
struct _fb_login_t {
	fbutton_t frame;
	fimage_t panel;

	fimage_t user_icon;
	fedit_t user_edit;
	fimage_t pass_icon;
	fedit_t pass_edit;

	fbutton_t login_button;
	fbutton_t set_button;
};

static fb_login_t __g_login;

static int __g_argc;
static char **__g_argv;

static void *gui_main(void *param);

#define IMGDIR ""

int main(int argc, char *argv[])
{
	__g_argc = argc;
	__g_argv = argv;
	if (spl_thread_create(gui_main, NULL, 10) == NULL) {
		printf("Create GUI main thread error!!!\n");
		return -1;
	}

	while(1)
		sleep(1);
	return 0;
}

static int user_edit_on_evt(fbox_t *box, DFBWindowEvent *evt)
{
	int rec;
	fb_login_t *me = &__g_login;

	if (evt->type == DWET_KEYDOWN) {

		switch (evt->key_symbol) {
		case DIKS_CURSOR_UP:
		case DIKS_CURSOR_DOWN:
			break;
		case DIKS_CURSOR_LEFT:
			fbox_focus(&me->set_button);
			break;
		case DIKS_CURSOR_RIGHT:
			fbox_focus(&me->pass_edit);
			break;
		case DIKS_ENTER:
		default:
			break;
		}
	}

	fbox_evt_do_default(box, evt);
	return 0;
}

static int pass_edit_on_evt(fbox_t *box, DFBWindowEvent *evt)
{
	int rec;
	fb_login_t *me = &__g_login;

	if (evt->type == DWET_KEYDOWN) {

		switch (evt->key_symbol) {
		case DIKS_CURSOR_UP:
		case DIKS_CURSOR_DOWN:
			break;
		case DIKS_CURSOR_LEFT:
			fbox_focus(&me->user_edit);
			break;
		case DIKS_CURSOR_RIGHT:
			fbox_focus(&me->login_button);
			break;
		case DIKS_ENTER:
		default:
			break;
		}
	}

	fbox_evt_do_default(box, evt);
	return 0;
}
static int login_button_on_evt(fbox_t *box, DFBWindowEvent *evt)
{
	int rec;
	fb_login_t *me = &__g_login;

	if (evt->type == DWET_KEYDOWN) {

		switch (evt->key_symbol) {
		case DIKS_CURSOR_UP:
		case DIKS_CURSOR_DOWN:
			break;
		case DIKS_CURSOR_LEFT:
			fbox_focus(&me->pass_edit);
			break;
		case DIKS_CURSOR_RIGHT:
			fbox_focus(&me->set_button);
			break;
		case DIKS_ENTER:
			break;
		default:
			break;
		}
	}

	fbox_evt_do_default(box, evt);
	return 0;
}
static int set_button_on_evt(fbox_t *box, DFBWindowEvent *evt)
{
	int rec;
	fb_login_t *me = &__g_login;

	if (evt->type == DWET_KEYDOWN) {

		switch (evt->key_symbol) {
		case DIKS_CURSOR_UP:
		case DIKS_CURSOR_DOWN:
			break;
		case DIKS_CURSOR_LEFT:
			fbox_focus(&me->login_button);
			break;
		case DIKS_CURSOR_RIGHT:
			fbox_focus(&me->user_edit);
			break;
		case DIKS_ENTER:
			break;

		default:
			break;
		}
	}

	fbox_evt_do_default(box, evt);
	return 0;
}

static void setup_login_window(int argc, char *argv[])
{
	int i;
	fbox_t *b;
	char *frame_image = IMGDIR"images/desktop05.jpg";
	char *box_image = IMGDIR"images/box_bg3.png";

	b = fbox_image_new(&__g_login.frame, NULL);
	fbox_set_size(b, SZ_FU|SZ_XD|1000, SZ_FU|SZ_XD|1000);
	fbox_set_posit(b, PS_OCP_N, PS_FU|PS_XD|PS_C|500, PS_FU|PS_XD|PS_C|500);
	// fbox_set_image(b, BS_NORMAL, frame_image, IM_HSTRETCH|IM_VSTRETCH);

	b = fbox_image_new(&__g_login.panel, &__g_login.frame);
	fbox_set_size(b, SZ_FU|SZ_XD|1000, SZ_KH|SZ_XD|55);
	fbox_set_posit(b, PS_OCP_T, PS_KH|PS_XD|PS_L|0, PS_KH|PS_XD|PS_T|700);
	// fbox_set_image(b, BS_NORMAL, box_image, IM_HSTRETCH|IM_VSTRETCH);

	b = fbox_image_new(&__g_login.user_icon, &__g_login.panel);
	fbox_set_size(b, SZ_FU|SZ_XD|18, SZ_KH|SZ_XD|500);
	fbox_set_posit(b, PS_OCP_L, PS_KH|PS_XD|PS_L|300, PS_KH|PS_XD|PS_C|500);
	fbox_set_image(b, BS_NORMAL, IMGDIR"images/user.png", IM_HSTRETCH|IM_VSTRETCH);

	b = fbox_edit_new(&__g_login.user_edit, &__g_login.panel);
	fbox_set_size(b, SZ_FU|SZ_XD|90, SZ_KH|SZ_XD|500);
	fbox_set_posit(b, PS_OCP_L, PS_KH|PS_XD|PS_L|0, PS_KH|PS_XD|PS_C|500);
	fbox_set_text_align(b, TL_VRIGHT | TL_HCENTER);
	fbox_set_palette(b, BS_FOCUSED, NULL);
	fbox_set_bg_color(b, BS_NORMAL, 0xff, 0xff, 0xff);
	fbox_set_fg_color(b, BS_NORMAL, 0x2f, 0x2f, 0x2f);
	fbox_set_font_size(b, BS_FOCUSED, 'x');
	fbox_set_font_size(b, BS_NORMAL, 'x');
	fbox_set_event_proc(b, user_edit_on_evt);

	b = fbox_image_new(&__g_login.pass_icon, &__g_login.panel);
	fbox_set_size(b, SZ_FU|SZ_XD|18, SZ_KH|SZ_XD|500);
	fbox_set_posit(b, PS_OCP_L, PS_KH|PS_XD|PS_L|20, PS_KH|PS_XD|PS_C|500);
	fbox_set_image(b, BS_NORMAL, IMGDIR"images/password.png", IM_HSTRETCH|IM_VSTRETCH);

	b = fbox_edit_new(&__g_login.pass_edit, &__g_login.panel);
	fbox_set_size(b, SZ_FU|SZ_XD|90, SZ_KH|SZ_XD|500);
	fbox_set_posit(b, PS_OCP_L, PS_KH|PS_XD|PS_L|0, PS_KH|PS_XD|PS_C|500);
	fbox_set_text_align(b, TL_VRIGHT | TL_HCENTER);
	fbox_set_palette(b, BS_FOCUSED, NULL);
	fbox_set_bg_color(b, BS_NORMAL, 0xff, 0xff, 0xff);
	fbox_set_fg_color(b, BS_NORMAL, 0x2f, 0x2f, 0x2f);
	fbox_set_font_size(b, BS_FOCUSED, 'x');
	fbox_set_font_size(b, BS_NORMAL, 'x');
	fbox_set_event_proc(b, pass_edit_on_evt);
	((fedit_t *)b)->shadow = 1;

	b = fbox_button_new(&__g_login.login_button, &__g_login.panel);
	fbox_set_size(b, SZ_FU|SZ_XD|50, SZ_KH|SZ_XD|500);
	fbox_set_posit(b, PS_OCP_L, PS_KH|PS_XD|PS_L|30, PS_KH|PS_XD|PS_C|500);
	fbox_set_palette(b, BS_FOCUSED, NULL);
	fbox_set_image(b, BS_NORMAL, IMGDIR"images/button1.png", IM_HSTRETCH|IM_VSTRETCH);
	fbox_set_image(b, BS_FOCUSED, IMGDIR"images/button1_focused.png", IM_HSTRETCH|IM_VSTRETCH);
	fbox_set_fg_color(b, BS_NORMAL, 0xff, 0xff, 0x00);
	fbox_set_fg_color(b, BS_FOCUSED, 0xff, 0xff, 0xff);
	fbox_set_text_align(b, TL_VRIGHT | TL_HCENTER);
	fbox_set_font_size(b, BS_NORMAL, 'x');
	fbox_set_font_size(b, BS_FOCUSED, 'x');
	fbox_set_text(b, "login");
	fbox_set_event_proc(b, login_button_on_evt);

	b = fbox_button_new(&__g_login.set_button, &__g_login.panel);
	fbox_set_size(b, SZ_FU|SZ_XD|70, SZ_KH|SZ_XD|500);
	fbox_set_posit(b, PS_OCP_L, PS_KH|PS_XD|PS_L|50, PS_KH|PS_XD|PS_C|500);
	fbox_set_palette(b, BS_FOCUSED, NULL);
	fbox_set_image(b, BS_NORMAL, IMGDIR"images/button1.png", IM_HSTRETCH|IM_VSTRETCH);
	fbox_set_image(b, BS_FOCUSED, IMGDIR"images/button1_focused.png", IM_HSTRETCH|IM_VSTRETCH);
	fbox_set_fg_color(b, BS_NORMAL, 0xff, 0xff, 0x00);
	fbox_set_fg_color(b, BS_FOCUSED, 0xff, 0xff, 0xff);
	fbox_set_text_align(b, TL_VRIGHT | TL_HCENTER);
	fbox_set_font_size(b, BS_NORMAL, 'x');
	fbox_set_font_size(b, BS_FOCUSED, 'x');
	fbox_set_text(b, "setting"); fbox_set_event_proc(b, set_button_on_evt);
}

static void *gui_main(void *param)
{
	unsigned int flg = KLOG_DFT;

	int argc = __g_argc;
	char **argv = __g_argv;

	printf("%s:%d\n", __func__, __LINE__);
	fbox_init(&argc, &argv);
	printf("%s:%d\n", __func__, __LINE__);

	setup_login_window(argc, argv);
	printf("%s:%d\n", __func__, __LINE__);

	fbox_layout(NULL);
	printf("%s:%d\n", __func__, __LINE__);
	fbox_create_window(NULL);
	printf("%s:%d\n", __func__, __LINE__);

	fbox_show_full((fbox_t*)&__g_login.frame, ktrue);
	printf("%s:%d\n", __func__, __LINE__);

	fbox_main();
	printf("%s:%d\n", __func__, __LINE__);

	return NULL;
}

