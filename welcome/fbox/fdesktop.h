/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __FDESKTOP_H__
#define __FDESKTOP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <fbox.h>
#include <fimage.h>

#include <fbutton.h>

/*---------------------------------------------------------------------------------
 * desktop.c
 */
typedef struct _fdesktop_t fdesktop_t;
struct _fdesktop_t {

	IDirectFB *dfb;
	IDirectFBSurface *surface;
	IDirectFBDisplayLayer *layer;
	DFBDisplayLayerConfig layer_cfg;
	IDirectFBEventBuffer *evt_buf;

	char *font_path;
	IDirectFBFont *font_d;
	IDirectFBFont *font_e;
	IDirectFBFont *font_z;
	IDirectFBFont *font_y;
	IDirectFBFont *font_x;
	unsigned char font_d_height;
	unsigned char font_e_height;
	unsigned char font_z_height;
	unsigned char font_y_height;
	unsigned char font_x_height;

	int width;
	int height;

	K_dlist_entry fbox_hdr;
};

fdesktop_t *desktop();

#ifdef __cplusplus
}
#endif
#endif /* __FDESKTOP_H__ */
