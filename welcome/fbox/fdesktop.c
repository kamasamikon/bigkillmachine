/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <fdesktop.h>

/*-----------------------------------------------------------------------
 * desktop.c
 */

static fdesktop_t __g_desktop;
fdesktop_t *desktop()
{
	return &__g_desktop;
}


