/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include "nh_helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#define KMODU_NAME "NHDBUS"
#include <hilda/klog.h>

#define NH_DBUS

/*-----------------------------------------------------------------------
 * dbus
 */
#ifdef NH_DBUS

#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "dbus-print-message.h"

void dbus_connection_dispatch_hook(DBusMessage *message);
void dbus_connection_dispatch_hook(DBusMessage *message)
{
	static int print_body = -1;
	static int skip_klog = -1;

	if (unlikely(skip_klog == -1)) {
		if (getenv("NH_DBUS_SKIP"))
			skip_klog = 1;
		else
			skip_klog = 0;
	}
	if (unlikely(skip_klog))
		return;

	if (unlikely(print_body == -1)) {
		if (getenv("NH_DBUS_NOBODY"))
			print_body = 0;
		else
			print_body = 1;
	}

	klogmon_init();
	print_message(message, 0, print_body);
}

#endif

