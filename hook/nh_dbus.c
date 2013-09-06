/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include "nh_helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#include <klog.h>

#define NH_DBUS

/*-----------------------------------------------------------------------
 * dbus
 */
#ifdef NH_DBUS

#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "dbus-print-message.h"

static int __g_dbus_klog = 0;

void dbus_connection_dispatch_hook(DBusMessage *message);
void dbus_connection_dispatch_hook(DBusMessage *message)
{
	static void (*realfunc)(DBusMessage *message) = NULL;
	if (!realfunc)
		realfunc = dlsym(RTLD_NEXT, "dbus_connection_dispatch_hook");

	realfunc(message);
	if (__g_dbus_klog)
		klog("NEMOHOOK: dbus_connection_dispatch_hook\n");
}
#endif

