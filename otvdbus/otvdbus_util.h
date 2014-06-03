
/**
 * \file otvdbus-util.h
 *
 * \author  Albert
 * \date    01/04/2011
 * \defgroup NTVDBUS NemoTV Dbus Framework utility
 * \ingroup  NTVDBUS
 *
 * The NemoTV Dbus Framework provides a set of classes that are useful to manage NemoTV services.
 * The framework classes are built on top of low level DBus API and uses GObject utility classes.
 *
 */



/**
 ** @defgroup OtvdbusController OtvdbusController
 * @brief Controller that manages multiple DBus connections and a DBus server.
 *
 * A Controller that manages a DBus server and multiple
 * connections.
 *
 * Users have to derive this class and implement handle_connection_lost and handle_new_connection functions.
 *
 * Here is an example implementation of how to use the DBus
 * controller
 *
 * @ingroup NTVDBUS
 */

/**@{*/
#ifndef _NTVDBUS_DBUS_UTIL_H_
#define _NTVDBUS_DBUS_UTIL_H_


#include <glib-object.h>
#include <dbus/dbus-glib.h>
#include "otvdbus-controller.h"
#include <glib-object.h>
#include <gobject/gvaluecollector.h>

#if 0
typedef enum
{
	AIM_SERVICE,
	APPMAN_SERVICE,
	CONFIGMAN_SERVICE
}otvdbus_server_name_t;

typedef struct
{
    gchar* key;			/*!< Key */
    GType key_type;		/*!< Key Type */
    gpointer placeholder;	/*!< Address of the location to store the retreived value */
}otvdbus_configman_data_t;
#endif

#define CONFIGMAN_SERVICE_NAME "Configman"
#define CONFIGMAN_SERVICE_PATH "/com/nemotv/ConfigCenter"
#define CONFIGMAN_SERVICE_INTERFACE "com.nemotv.ConfigCenter"
#define CONFIGMAN_SERVICE_ADDRESS "unix:path=/tmp/ConfigCenter"

typedef GHashTable dbus_var_dict_t;

typedef GHashTable dbus_aiasv;

typedef GHashTableIter dbus_asv_iterator;


/**
 * This function  returns a proxy of the service, given service_name
 * @param service_name   service name
 * @param sync_flag  if FALSE,  this function will make a single attempt to get a proxy , and return the result, regardless of the result.
                              TRUE flag means that if this function cannot get a proxy the first time,
                              this function will wait and re-try to get a proxy until it gets a proxy successfully,
                              or until it reaches the maximum number of times it's allowed to retry. (the number of times it can re-try is limited by MAX_PROXY_ATTEMPT)


 *
 * @retval NULL if it doesn't acquire a proxy, or a valid proxy pointer.

 */


DBusGProxy* otvdbus_get_service_proxy(gchar* service_name, gboolean sync_flag);



/**
 * This function returns a proxy of the service, given service address, service path and service interface.
 * Possible use cases:



 * @param service name      service name
 * @param service path      service object path
 * @param service intf      service interface
 * @param sync_flag			  if TRUE, this function doesn't return until a proxy is acquired.  Else, it returns right away, regardless of the acquisition of the proxy.

 * @return Proxy pointer if successful, and NULL if not.
 */
DBusGProxy* otvdbus_get_service_proxy_extended(gchar* service_addr, gchar* service_path, gchar* service_intf, gboolean sync_flag);

GType otvutil_var_dict_gtype(void);


/* create a new dbus_asv instance */
/**
 * Helper Function to create a hashtable of type a{sv} 
 * The key in the hash table is not duped and is not unrefed or destroyed, while destroying the hash table.
 * This function can be used for sending hash tables for method signatures or dbus signals. In other words,
 * you can use this function if your keys are global.
 * @param first_key	arguments
 * @return HashTable
 */
dbus_var_dict_t *otvutil_var_dict_new(const char *first_key, ...) ;

/*
 * Variation of otvutil_var_dict_new function with both key & value cleanup callback built into the wrapper.
 * @param none
 * @return HashTable
 * Notes : Used with otvutil_var_dict_insert_dup
*/
GHashTable * otvutil_var_dict_new_with_free_func(void);


/**
 * Helper Function to append to a hashtable of type a{sv}
 * @param pAsvTable Non NULL hashtable of type asv
 * @param first_key     arguments
 * @return HashTable with appended values
 */
GHashTable *
otvutil_var_dict_insert(GHashTable *pAsvTable , const char *first_key, ...);

/*
 * Variation of otvutil_var_dict_insert but with keys being duped for you internally.
 * @param pAsvTable Non NULL hashtable of type asv
 * @param first_key     arguments
 * @return HashTable with appended values
 * Notes : Used with otvutil_var_dict_new_with_free_func
 */
GHashTable * otvutil_var_dict_insert_dup(GHashTable *asv, const char *first_key, ...);


/* destroy a dbus_var_dict_t instance */
void otvutil_var_dict_destroy(const dbus_var_dict_t *asv);

/* common string destroy cb */
void otvutil_destroy_gkey(gchar *pkey);

GType otvutil_string_dict_gtype(void);
/* create a new dbus_ass instance */
/**
 * Helper Function to create a hashtable of type a{ss}
 * @param first_key     arguments
 * @return HashTable
 */
GHashTable *otvutil_string_dict_new(const char *first_key, ...) ;

/**
 * Helper to create new GValue
 * @param type
 * @return
 */
GValue *otvutil_alloc_gvalue(GType type);
/**
 * helper to free GValue
 * @param value
 */
void otvutil_destroy_gvalue(GValue *value);

/**
 * Function to create a GObject from an existing Object
 * @param src   Input Object
 * @return  Cloned Object
 */
GObject * g_object_clone(GObject *src);


/**
 * Throw dbus exception
 * @param context DBusGMethodInvocation context
 * @param quark
 * @param error_code
 * @param format printf like var args
 */
void throw_dbus_error(DBusGMethodInvocation *context,
        GQuark quark,
        gint error_code,
        const gchar* format,
        ...);


#endif




