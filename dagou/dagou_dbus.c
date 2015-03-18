/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <helper.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#define DALOG_MODU_NAME "DADBUS"

#define DAGOU_DBUS

/*-----------------------------------------------------------------------
 * dbus
 */
#ifdef DAGOU_DBUS

#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "dbus-print-message.h"

void dagou_dump_dbus_message(DBusMessage *message);
void dagou_dump_dbus_message(DBusMessage *message)
{
	static int skip_dalog = -1;

	if (dagou_unlikely(skip_dalog == -1)) {
		if (getenv("DAGOU_DBUS_SKIP"))
			skip_dalog = 1;
		else
			skip_dalog = 0;
	}
	if (dagou_unlikely(skip_dalog))
		return;

	dalog_setup();
	print_message(message);
}

#endif

