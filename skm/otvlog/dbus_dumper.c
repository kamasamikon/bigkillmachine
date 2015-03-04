/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <helper.h>
#include <ntvlog.h>

void dbus_message_show(const char *str);
void dbus_message_show_full(const char *str);

void dbus_message_show(const char *str)
{
	NSULOG_CHK_AND_CALL(NSULOG_INFO,   'I', "DBUS", __FILE__, __func__, __LINE__, "%s", str);
}

void dbus_message_show_full(const char *str)
{
	NSULOG_CHK_AND_CALL(NSULOG_DEBUG,   'D', "DBUS", __FILE__, __func__, __LINE__, "%s", str);
}


