/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#include <helper.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#define DALOG_MODU_NAME "NHGCONF"
#include <dalog.h>

#define DAGOU_GCONF

/*-----------------------------------------------------------------------
 * gconf
 */
#ifdef DAGOU_GCONF
#include <gconf/gconf-client.h>

/* Return buf, if ret != buf, should call free(ret) */
static char *entry_value(const GConfValue *val, char *buf, int len)
{
	const char *tmp;

	switch (val->type) {
	case GCONF_VALUE_STRING:
		tmp = gconf_value_get_string(val);
		if (!tmp)
			return NULL;
		if (strlen(tmp) >= len)
			return strdup(tmp);
		strcpy(buf, tmp);
		return buf;

	case GCONF_VALUE_INT:
		sprintf(buf, "%d", gconf_value_get_int(val));
		return buf;

	case GCONF_VALUE_FLOAT:
		sprintf(buf, "%f", gconf_value_get_float(val));
		return buf;

	case GCONF_VALUE_BOOL:
		sprintf(buf, "%s", gconf_value_get_bool(val) ? "True" : "False");
		return buf;

	case GCONF_VALUE_SCHEMA:
	case GCONF_VALUE_LIST:
	case GCONF_VALUE_PAIR:
	default:
		/* TODO: do it */
		sprintf(buf, "??? Not supported !!!");
		return buf;
	}
}

void gconf_client_set(GConfClient* client, const gchar* key,
		const GConfValue* val, GError** err)
{
	char buf[256], *vstr;

	static void *(*realfunc)(GConfClient*, const gchar*, const GConfValue*, GError**) = NULL;
	if (!realfunc)
		realfunc = dlsym(RTLD_NEXT, "gconf_client_set");

	dalog_setup();
	realfunc(client, key, val, err);

	vstr = entry_value(val, buf, sizeof(buf));
	dalog_info("NEMOHOOK: gconf_sources_query_value: key: <%s>, vstr:<%s>\n", key, vstr);
	if (vstr != buf)
		free(vstr);
}

GConfValue* gconf_client_get(GConfClient* client, const gchar* key, GError** err)
{
	char buf[256], *val;

	static GConfValue *(*realfunc)(GConfClient*, const gchar*, GError**) = NULL;
	if (!realfunc)
		realfunc = dlsym(RTLD_NEXT, "gconf_client_get");

	dalog_setup();
	GConfValue *ret = realfunc(client, key, err);

	val = entry_value(ret, buf, sizeof(buf));
	dalog_info("NEMOHOOK: gconf_client_get: key: <%s>, val:<%s>\n", key, val);
	if (val != buf)
		free(val);

	return ret;
}

#endif

