/**
 * @file     otvdbus_utilr.c
 * @author   Albert
 * @date     Jan 8, 2011
 * @addtogroup OtvdbusController
 */

/**
 * @{
 */



#include <unistd.h>
#include <ntvlog.h>
#include "otvdbus_util.h"



typedef struct
{
	gchar* service_name;
	gchar* service_addr;
	gchar* service_path;
	gchar* service_intf;
	DBusGProxy* proxy;
} service_info_t;



#define MAX_PROXY_ATTEMPT  10
#define PROXY_TIMEOUT 300


service_info_t s_configman_info = {CONFIGMAN_SERVICE_NAME, CONFIGMAN_SERVICE_ADDRESS, CONFIGMAN_SERVICE_PATH, CONFIGMAN_SERVICE_INTERFACE, NULL};

gchar* s_sys = "/sys/";
gchar* s_service_address = "/service-address";
gchar* s_service_path = "/object-path";
gchar* s_service_intf = "/interface-path";



static DBusGProxy* s_ConfMan_p = NULL;

static DBusGProxy* _otvbus_get_service_proxy(service_info_t* service_info, gboolean sync_flag);

static gboolean _otvdbus_get_service_info(service_info_t* service_info, gchar* service_name);

static GHashTable * otvutil_var_dict_insert_internal(GHashTable *asv, gboolean bIsDuped, const char* first_key, va_list args) ;

static gboolean _get_data_from_configman(DBusGProxy* pConfmanProxy, gchar*pKey, GValue* pValue)
{
    gboolean bResult = FALSE;
     GError* error = NULL;


    if (dbus_g_proxy_call (pConfmanProxy, "getValue", &error, G_TYPE_STRING, pKey, G_TYPE_INVALID, G_TYPE_VALUE, pValue, G_TYPE_INVALID) == FALSE)
    {

        if (error->domain == DBUS_GERROR && error->code == DBUS_GERROR_REMOTE_EXCEPTION)
        {
            O_LOG_WARNING("Remote exception %s: %s",
                    dbus_g_error_get_name(error),
                    error->message);
        }
        else
        {
            O_LOG_WARNING("Configman:: Returned Error: %s for Key = %s", error->message, pKey);

        }
        g_error_free(error);
        bResult = FALSE;
    }

    return  bResult;
}

static DBusGProxy* _otvdbus_connect_and_create_proxy(service_info_t* p_info)
{
	GError* error = NULL;
	DBusGConnection* pConnection;
	DBusGProxy* pProxy;

	pConnection = dbus_g_connection_open(p_info->service_addr, &error);
	if (pConnection == NULL)
	{
		O_LOG_DEBUG("\n  dbus_g_connection fail\n");
		return NULL;
	}

	O_LOG_DEBUG("\n  dbus_g_connection suceed\n");
	p_info->proxy = pProxy = dbus_g_proxy_new_for_peer(pConnection, p_info->service_path, p_info->service_intf);
	return pProxy;

}


static DBusGProxy* _otvbus_get_service_proxy(service_info_t* service_info, gboolean sync_flag)
{
	DBusGProxy* pProxy = NULL;
	int attempt = 0;

	pProxy = _otvdbus_connect_and_create_proxy(service_info);
	if ((sync_flag == TRUE) && pProxy == NULL)
	{
		do
		{
			sleep(PROXY_TIMEOUT);
			pProxy = _otvdbus_connect_and_create_proxy(service_info);
			attempt++;
		}
		while ((pProxy == NULL) && (attempt <= MAX_PROXY_ATTEMPT));
		if (attempt >= MAX_PROXY_ATTEMPT)
		{
			O_LOG_DEBUG("\n getting proxy (%s) failed\n", service_info->service_name);
	 	}

	}
	return pProxy;

}





/*
   get configman values for the following keys :
    "/sys/" + service-name + "/service-address"
    "/sys/" + service-name + "/service-path"
    "/sys/" + service-name + "/interface-path"


   -each service-name should start with small capital (for example...aim),
   -xml entry name should follow this convention  ("/sys/aim/service-address", "/sys/aim/service-path", "/sys/aim/interface-path")


*/

static gboolean _otvdbus_get_service_info(service_info_t* service_info, gchar* service_name)
{

	/* if configman is enabled, get it from configman */
	gchar* configman_key;
	GValue val = {0};


	if (service_info->proxy == NULL)
		{
			O_LOG_DEBUG("\n no configman proxy available \n");
			return FALSE;
		}

	/* key is /sys/service_name/service-address,
			/sys/service_name/service-path (object-path)
			/sys/service_name/interface-path */

	configman_key = g_strconcat(s_sys,service_name, s_service_address, NULL);
	O_LOG_DEBUG("\n key constructed (%s)\n", configman_key);
	_get_data_from_configman(service_info->proxy, configman_key, &val);
	service_info->service_addr = g_value_dup_string(&val);
	g_value_unset(&val);
	O_LOG_DEBUG("\n service addr (%s)\n", service_info->service_addr);
	g_free(configman_key);

	configman_key = g_strconcat(s_sys,service_name, s_service_path, NULL);
	O_LOG_DEBUG("\n key constructed (%s)\n", configman_key);
	_get_data_from_configman(service_info->proxy, configman_key, &val);
	service_info->service_path= g_value_dup_string(&val);
	O_LOG_DEBUG("\n service path (%s)\n", service_info->service_path);
	g_value_unset(&val);
    g_free(configman_key);

	configman_key = g_strconcat(s_sys,service_name, s_service_intf, NULL);
	O_LOG_DEBUG("\n key constructed (%s)\n", configman_key);
	_get_data_from_configman(service_info->proxy, configman_key, &val);
	service_info->service_intf= g_value_dup_string(&val);
	O_LOG_DEBUG("\n service intf (%s)\n", service_info->service_intf);
	g_value_unset(&val);
    g_free(configman_key);

	return TRUE;
}

DBusGProxy* otvdbus_get_service_proxy_extended(gchar* service_name, gchar* service_path, gchar* service_intf, gboolean sync_flag)
{
#if 0
	service_info_t service_info;


		service_info.proxy = s_ConfMan_p;

		O_LOG_DEBUG("\n otvdbus_get_service_proxy2...getting service info \n");
		if (_otvdbus_get_service_info(&service_info, service_name) == FALSE)
			{
				return NULL;
			}

		return _otvbus_get_service_proxy(&service_info, sync_flag);

#endif

	return NULL;

}



DBusGProxy* otvdbus_get_service_proxy( gchar* service_name, gboolean  sync_flag)
{


	service_info_t service_info;
	service_info.proxy = NULL;
	DBusGProxy* ret_proxy = NULL;

	service_info.service_name = NULL;
	service_info.service_addr= NULL;
	service_info.service_intf= NULL;
	service_info.service_path= NULL;

	O_LOG_ENTER ("\n");


	if (s_ConfMan_p == NULL)
		{
			s_ConfMan_p = _otvbus_get_service_proxy( &s_configman_info, sync_flag);
			O_DEBUG_ASSERT(s_ConfMan_p != NULL);
		}


	if (g_strcmp0(CONFIGMAN_SERVICE_NAME, service_name) == 0)
		{
			return s_ConfMan_p;
		}
	else
		{
			service_info.proxy = s_ConfMan_p;
			service_info.service_name = service_name;
			if (_otvdbus_get_service_info(&service_info, service_name) == FALSE)
				{
					O_LOG_DEBUG("\n retrieving service info (%s) from configman failed \n", service_name);

					return ret_proxy;
				}

			ret_proxy =  _otvbus_get_service_proxy(&service_info, sync_flag);
			/* free service_info.service_addr, intf, path . */
			if (service_info.service_addr)
			{
				g_free(service_info.service_addr);
			}
			if (service_info.service_path)
			{
				g_free(service_info.service_path);
			}
			if (service_info.service_intf)
			{
				g_free(service_info.service_intf);
			}


		}

	O_LOG_EXIT("\n");

	return ret_proxy;

}

/**
 * otvutil_alloc_gvalue
 * @param[in] type The type of the new GValue that has to be created.
 *
 * This function will use the glib slice allocater for creating an empty GValue.
 *
 *
 * @return a newly allocated, newly initialized GValue, to be freed with
 * destroy_gvalue().
 *
 * @context  This function may return NULL, for uninitialized GTypes, it will just print a warning and
 * will result in a leak, It is the responsibility of the caller to pass in proper types.
 * handing the failure is possible but not done for efficiency reasons.
 *
 */
GValue *
otvutil_alloc_gvalue(GType type)
{
    GValue *value = g_slice_new0(GValue);
    O_DEBUG_ASSERT(NULL != value);
    value = g_value_init(value, type);
    return value;
}

/**
 * otvutil_destroy_gkey
 * @param[in] value A GValue which was allocated with the @ref alloc_gvalue
 *
 * This function will unset and free a GValue.
 */
void
otvutil_destroy_gkey(gchar *pkey)
{
    O_DEBUG_ASSERT(NULL != pkey);
    g_return_if_fail(NULL != pkey);
    g_free(pkey);
}

/**
 * otvutil_destroy_gvalue
 * @param[in] value A GValue which was allocated with the @ref alloc_gvalue
 *
 * This function will unset and free a GValue.
 */
void
otvutil_destroy_gvalue(GValue *pValue)
{
    O_DEBUG_ASSERT(NULL != pValue);
    g_return_if_fail(NULL != pValue);

    g_value_unset(pValue);
    g_slice_free(GValue, pValue);
}

/* return a GType for dbus  a{sv} */
GType otvutil_var_dict_gtype(void)
{
    static GType type;
    static int is_initalized = 0;
    if (is_initalized == 0)
    {
        type = dbus_g_type_get_map("GHashTable",G_TYPE_STRING, G_TYPE_VALUE);
        is_initalized = 1;
    }
    return type;
}

GType otvutil_string_dict_gtype(void)
{
    static GType type;
    static int is_initalized = 0;
    if (is_initalized == 0)
    {
        type = dbus_g_type_get_map("GHashTable", G_TYPE_STRING, G_TYPE_STRING);
        is_initalized = 1;
    }
    return type;
}

GHashTable *
otvutil_string_dict_new(const char *first_key, ...)
{
    va_list args;
    const char *key;
    gchar  *value , *temp_val;


    /* create a GHashTable */
    O_LOG_DEBUG("New Hashtable ");
    GHashTable *table = g_hash_table_new_full(g_str_hash, g_str_equal, NULL,
            (GDestroyNotify)g_free);

    va_start(args, first_key);
    for (key = first_key; key != NULL; key = va_arg(args, const char *))
    {
        temp_val =  va_arg(args, gchar *);
        value = g_strdup(temp_val);

        g_hash_table_insert(table, (char *)key, value);
    }
    va_end(args);

    return table;
}


static GHashTable * otvutil_var_dict_insert_internal(GHashTable *asv,
        gboolean bIsDuped, const char* first_key, va_list args) {
    const char *key;
    gchar* pTempKey;
    GType type;
    GValue * value;
    char *error = NULL; /* NB: not a GError! */

    O_DEBUG_ASSERT(NULL != asv);
    g_return_val_if_fail((asv != NULL), NULL);
   

    for (key = first_key; key != NULL; key = va_arg(args, const char *))
    {
        type = va_arg(args, GType);

        value = g_slice_new0(GValue);

        G_VALUE_COLLECT_INIT(value, type, args, 0, &error);

        if (G_UNLIKELY(error)) {
            O_LOG_WARNING("key %s: %s", key, error);
            g_free(error);
            error = NULL;
            otvutil_destroy_gvalue(value);
            continue;
        }

        if (bIsDuped) {
            pTempKey = g_strdup((char *) key);
        } else {
            pTempKey = (char *) key;

        }
        g_hash_table_insert(asv, pTempKey, value);
    }

    return asv;
}

GHashTable * otvutil_var_dict_insert(GHashTable *asv, const char *first_key,
        ...) {
    GHashTable *new_asv;
    va_list var_copy;

    va_start(var_copy, first_key);
    new_asv = otvutil_var_dict_insert_internal(asv, FALSE, first_key, var_copy);
    va_end(var_copy);
    return new_asv;
}

GHashTable * otvutil_var_dict_insert_dup(GHashTable *asv, const char *first_key,
        ...) {
    GHashTable *new_asv;
    va_list var_copy;

    va_start(var_copy, first_key);
    new_asv = otvutil_var_dict_insert_internal(asv, TRUE, first_key, var_copy);
    va_end(var_copy);
    return new_asv;
}

GHashTable * otvutil_var_dict_new(const char *first_key, ...) {
    va_list var_copy;

    /* create a GHashTable */
    O_LOG_DEBUG("New Hashtable ");
    GHashTable *asv = g_hash_table_new_full(g_str_hash, g_str_equal, NULL,
            (GDestroyNotify) otvutil_destroy_gvalue);
    if (first_key) {
        va_start(var_copy, first_key);
        asv = otvutil_var_dict_insert_internal(asv, FALSE, first_key, var_copy);
        va_end(var_copy);
    }
    return asv;
}

GHashTable * otvutil_var_dict_new_with_free_func() {

    /* create a GHashTable */
    O_LOG_DEBUG("New Hashtable ");
    GHashTable *asv = g_hash_table_new_full(g_str_hash, g_str_equal,
            (GDestroyNotify) otvutil_destroy_gkey,
            (GDestroyNotify) otvutil_destroy_gvalue);

    return asv;
}

/**
 * otvutil_dbus_asv_destroy
 * @param[in] pAsv a GHashTable of type STRING and G_VALUE
 *
 * Destroy and clean up a dbus asv.
 */
void
otvutil_var_dict_destroy(const GHashTable *pAsv)
{
    O_LOG_DEBUG("Free Hashtable ");
    O_DEBUG_ASSERT(NULL != pAsv);
    g_hash_table_destroy((GHashTable *)pAsv);
}

GObject *
g_object_clone(GObject *src)
{
    GObject *dst;
    GParameter *params;
    GParamSpec **specs;
    guint n, n_specs, n_params;

    specs = g_object_class_list_properties(G_OBJECT_GET_CLASS(src), &n_specs);
    params = g_new0(GParameter, n_specs);
    n_params = 0;

    for (n = 0; n < n_specs; ++n)
        if (strcmp(specs[n]->name, "parent") &&
                (specs[n]->flags & G_PARAM_READWRITE) == G_PARAM_READWRITE)
        {
            params[n_params].name = g_intern_string(specs[n]->name);
            g_value_init(&params[n_params].value, specs[n]->value_type);
            g_object_get_property(src, specs[n]->name, &params[n_params].value);
            ++ n_params;
        }

    dst = g_object_newv(G_TYPE_FROM_INSTANCE(src), n_params, params);
    g_free(specs);
    g_free(params);

    return dst;
}

void throw_dbus_error(DBusGMethodInvocation *context,
        GQuark quark,
        gint error_code,
        const gchar* format,
        ...)
{
    GError *error;

    va_list args;
    gchar*  message;

    O_DEBUG_ASSERT( NULL != context);
    O_LOG_ERROR(" Sending DBUS Error %d, context = %p",error_code,context);

    va_start(args,format);
    message = g_strdup_vprintf(format , args);
    va_end (args);

    error = g_error_new(quark, error_code , "%s", message);

    dbus_g_method_return_error(context , error);
    O_LOG_ERROR("[%s]%s\n",__FUNCTION__, error->message );
    g_error_free(error);
    g_free(message);
}


#if !defined(NDEBUG)
void otvutil_debug_gvalue(GValue *param_value);
void otvutil_asv_table_data_debug( GHashTable* data);
void otvutil_ass_table_data_debug( GHashTable* data);

void otvutil_debug_gvalue(GValue *param_value)
{

    char *pStr = g_strdup_value_contents(param_value);
    O_LOG_DEBUG ("\n %s", pStr);
    g_free(pStr);

}

void otvutil_asv_table_data_debug( GHashTable* data)
{
    gchar *key = NULL;
    GValue *value = NULL;
    GHashTableIter it;
    O_LOG_ENTER("\n");

    if(NULL != data )
    {
        O_LOG_DEBUG("\n");
        g_hash_table_iter_init(&it, data);
        while (g_hash_table_iter_next(&it, (void**)&key, (void**)&value))
        {
            otvutil_debug_gvalue(value);
        }
    }
    else
    {
        O_LOG_DEBUG("\n No Data");
    };

    O_LOG_EXIT("\n");

}
void otvutil_ass_table_data_debug( GHashTable* data)
{
    gchar *key = NULL;
    gchar *value = NULL;
    GHashTableIter it;
    O_LOG_ENTER("\n");

    if(NULL != data )
    {
        O_LOG_DEBUG("\n");
        g_hash_table_iter_init(&it, data);
        while (g_hash_table_iter_next(&it, (void**)&key, (void**)&value))
        {
            O_LOG_DEBUG("\n\t key = %s, value= %s", key,value);
        }
    }
    else
    {
        O_LOG_DEBUG("\n No Data");
    };

    O_LOG_EXIT("\n");

}

#endif

