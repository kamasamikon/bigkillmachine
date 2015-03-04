/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <helper.h>
#include <ntvlog.h>

void skmdbus_dump_message_head(const char *head);
void skmdbus_dump_message_head_body(const char *head, const char *body);

void skmdbus_dump_message_head(const char *head)
{
	NSULOG_CHK_AND_CALL(NSULOG_INFO,   'I', "DBUS", __FILE__, __func__, __LINE__, "%s", head);
}

void skmdbus_dump_message_head_body(const char *head, const char *body)
{
	NSULOG_CHK_AND_CALL(NSULOG_DEBUG,   'D', "DBUS", __FILE__, __func__, __LINE__, "%s%s", head, body);
}


