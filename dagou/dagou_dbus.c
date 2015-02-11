/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <helper.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#define DALOG_MODU_NAME "NHDBUS"

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
	static int skip_dalog = -1;

	if (dagou_unlikely(skip_dalog == -1)) {
		if (getenv("NH_DBUS_SKIP"))
			skip_dalog = 1;
		else
			skip_dalog = 0;
	}
	if (dagou_unlikely(skip_dalog))
		return;

	if (dagou_unlikely(print_body == -1)) {
		if (getenv("NH_DBUS_NOBODY"))
			print_body = 0;
		else
			print_body = 1;
	}

	dalog_setup();
	print_message(message, 0, print_body);
}

#endif

