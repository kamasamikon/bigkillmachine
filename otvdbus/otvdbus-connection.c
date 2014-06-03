
/**
 * @file     otvdbus-connection.c
 * @author   Rohan
 * @date     Jan 8, 2011
 * @addtogroup OtvdbusConnection
 */
#include <stdarg.h>
#include <unistd.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>


#include "otvdbus-connection.h"
#include "otv-marshal.h"
#include <ntvlog.h>
#include <glib.h>

/**
 * @addtogroup OtvdbusConnection
 * @{
 */

/**
 * Macro for private data
 */
#define NTVDBUS_CONNECTION_GET_PRIVATE(o)  \
        (G_TYPE_INSTANCE_GET_PRIVATE ((o), NTVDBUS_TYPE_CONNECTION, OtvdbusConnectionPrivate))

/**
 * Enumeration for Signals emitted by @ref OtvdbusConnection object
 */
enum
{
    NTVDBUS_CONNECTION_AUTHENTICATE_CLIENT,     /*!< AUTHENTICATE_CLIENT*///!< NTVDBUS_CONNECTION_AUTHENTICATE_CLIENT
    NTVDBUS_CONNECTION_DISCONNECTED,            /*!< DISCONNECTED*/       //!< NTVDBUS_CONNECTION_DISCONNECTED
    LAST_SIGNAL,                                /*!< LAST_SIGNAL*/        //!< LAST_SIGNAL
};


/* OtvdbusConnectionPriv */
/**
 * private data
 */
struct _OtvdbusConnectionPrivate
{
    DBusGConnection *pLowlevelConnection;/*!< Low level dbus connection */
};

/**
 * Connection private data type
 */
typedef struct _OtvdbusConnectionPrivate OtvdbusConnectionPrivate;

static guint            s_connection_signals[LAST_SIGNAL] = { 0};

/* functions prototype */
static void     _otvdbus_connection_dispose     (GObject         *pConnection);

static gboolean _otvdbus_connection_authenticate_client
(OtvdbusConnection         *pConnection,
        gulong                  uid);

static void     _otvdbus_connection_disconnected(OtvdbusConnection         *pConnection);


G_DEFINE_TYPE (OtvdbusConnection, otvdbus_connection, G_TYPE_OBJECT)


OtvdbusConnection * otvdbus_connection_new (GMainContext* pContext)
{
	OtvdbusConnection *object;

    O_LOG_ENTER("\n");

    object = NTVDBUS_CONNECTION(g_object_new (NTVDBUS_TYPE_CONNECTION, NULL));
    O_ASSERT(NULL != object);

    object->pConnectionContext = pContext;

    O_LOG_EXIT("\n");

    return object;
}

/*
 * DBusConnection filter function, this function checks if the message is a client disconnected message
 * and emits a disconnected message.
 */
static DBusHandlerResult _otvdbus_connection_filter_func (DBusConnection *conn,
        DBusMessage *message,
        void *user_data)
{
    OtvdbusConnection *connection = NTVDBUS_CONNECTION(user_data);

    if (dbus_message_is_signal (message,
            DBUS_INTERFACE_LOCAL,
            "Disconnected"))
    {
        g_signal_emit(connection, s_connection_signals[NTVDBUS_CONNECTION_DISCONNECTED],0, NULL);
    }
    O_LOG_DEBUG("[PID=%d] Connection Message Path is %s",getpid(),dbus_message_get_path(message));
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

}

/*
 * class initialization function
 */
static void
otvdbus_connection_class_init (OtvdbusConnectionClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    O_LOG_ENTER("\n");
    g_type_class_add_private (klass, sizeof (OtvdbusConnectionPrivate));

    object_class->dispose =  _otvdbus_connection_dispose;

    klass->authenticate_client = _otvdbus_connection_authenticate_client;
    klass->disconnected = _otvdbus_connection_disconnected;

    /* install signals */

    s_connection_signals[NTVDBUS_CONNECTION_AUTHENTICATE_CLIENT] =
            g_signal_new (("authenticate-client"),
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (OtvdbusConnectionClass, authenticate_client),
                    NULL, NULL,
                    otv_marshal_BOOLEAN__ULONG,
                    G_TYPE_BOOLEAN, 1,
                    G_TYPE_ULONG);


    s_connection_signals[NTVDBUS_CONNECTION_DISCONNECTED] =
            g_signal_new (("disconnected"),
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (OtvdbusConnectionClass, disconnected),
                    NULL, NULL,
                    otv_marshal_VOID__VOID,
                    G_TYPE_NONE, 0);
    O_LOG_EXIT("\n");

}

/* object initialization function */
static void
otvdbus_connection_init (OtvdbusConnection *connection)
{
    OtvdbusConnectionPrivate *priv;
    O_LOG_ENTER("\n");
    priv = NTVDBUS_CONNECTION_GET_PRIVATE (connection);

    priv->pLowlevelConnection = NULL;
    O_LOG_EXIT("\n");

}

/*
 *  connection destroy function
 */
static void
_otvdbus_connection_dispose (GObject *pConnection)
{
    OtvdbusConnectionPrivate *priv;
    O_LOG_ENTER("\n");
    O_ASSERT( NTVDBUS_IS_CONNECTION(pConnection) == TRUE);

    priv = NTVDBUS_CONNECTION_GET_PRIVATE (pConnection);

    if (priv->pLowlevelConnection)
    {
        dbus_connection_remove_filter (dbus_g_connection_get_connection(priv->pLowlevelConnection),
                (DBusHandleMessageFunction) _otvdbus_connection_filter_func,
                pConnection);
        dbus_g_connection_unref (priv->pLowlevelConnection);

        priv->pLowlevelConnection = NULL;
    }
    G_OBJECT_CLASS (otvdbus_connection_parent_class)->dispose (pConnection);

    O_LOG_EXIT("\n");
}

/*
 * connection authorization function
 * If this function returns TRUE the client will be authenticated and the connection is established
 * else connection will not be authorized
 */
static gboolean
_otvdbus_connection_authenticate_client (OtvdbusConnection *pConnection,
        gulong          uid)
{
    /* TODO: Currently authentication all the users , NEED to plugin security HERE*/
    O_LOG_ENTER("\n");
    O_LOG_EXIT("\n");

    return TRUE;
}

/*
 * connection disconnected function that will unref the object
 */
static void
_otvdbus_connection_disconnected (OtvdbusConnection         *pConnection)
{
    O_LOG_ENTER("\n");
    g_object_unref (G_OBJECT (pConnection));
    O_LOG_EXIT("\n");

}

/*
 * This is a callback function that will be executed by the DBus
 * when a connection needs to be authenticated.
 * This function will emit a signal and expects the signal handlers to return a boolean
 * if no handlers are registered the default handler will get executed see otvdbus_connection_authenticate_unix_user
 *
 */
static dbus_bool_t
_otvdbus_connection_allow_unix_user_cb (DBusGConnection *dbus_connection,
        gulong          uid,
        OtvdbusConnection *connection)
{
    gboolean retval = FALSE;
    dbus_bool_t return_val = FALSE;

    O_LOG_ENTER("\n");

    g_signal_emit (connection, s_connection_signals[NTVDBUS_CONNECTION_AUTHENTICATE_CLIENT], 0, uid, &retval);

    if (retval == TRUE)
    {
        O_LOG_DEBUG("Connection Accepted UID = %lu , connection = %p",uid,connection);
        return_val =  TRUE;
    }
#if !defined(NDEBUG)
    else
    {
        O_LOG_DEBUG("Connection Rejected UID = %lu , connection = %p",uid,connection);
    }
#endif
    O_LOG_EXIT("\n");
    return return_val;
}


void
otvdbus_connection_set_connection (OtvdbusConnection *connection, DBusConnection *dbus_connection)
{
    OtvdbusConnectionPrivate *priv;

    O_LOG_ENTER("\n");

    O_ASSERT (NTVDBUS_IS_CONNECTION (connection));
    O_ASSERT (dbus_connection != NULL);
    O_ASSERT (dbus_connection_get_is_connected (dbus_connection));

    g_return_if_fail (NTVDBUS_IS_CONNECTION (connection));
    g_return_if_fail ((NULL != dbus_connection));

    priv = NTVDBUS_CONNECTION_GET_PRIVATE (connection);
    O_ASSERT (priv->pLowlevelConnection == NULL);

    O_LOG_DEBUG("\n Setting Connection = %p with Context = %p", connection, connection->pConnectionContext);

    dbus_connection_setup_with_g_main(dbus_connection, connection->pConnectionContext );

    priv->pLowlevelConnection =    dbus_g_connection_ref(dbus_connection_get_g_connection(dbus_connection));

    /*
     * register authentication function
     */

    dbus_connection_set_unix_user_function (dbus_connection,
            (DBusAllowUnixUserFunction) _otvdbus_connection_allow_unix_user_cb,
            connection, NULL);
    /* add filter function to handle client disconnects */
    dbus_connection_add_filter (dbus_connection, _otvdbus_connection_filter_func, connection, NULL);
    dbus_connection_set_allow_anonymous (dbus_connection,TRUE);
    dbus_connection_flush(dbus_connection);

    O_LOG_EXIT("\n");

}


gboolean
otvdbus_connection_is_connected (OtvdbusConnection *connection)
{
    OtvdbusConnectionPrivate *priv;
    gboolean result = FALSE;

    O_LOG_ENTER("\n");
    O_ASSERT (NTVDBUS_IS_CONNECTION (connection));
    g_return_val_if_fail (NTVDBUS_IS_CONNECTION (connection), FALSE);

    priv = NTVDBUS_CONNECTION_GET_PRIVATE (connection);

    if (priv->pLowlevelConnection != NULL)
    {
        result =  dbus_connection_get_is_connected (dbus_g_connection_get_connection(priv->pLowlevelConnection));
    }
    O_LOG_EXIT("\n");
    return result;
}


gboolean
otvdbus_connection_is_authenticated (OtvdbusConnection *connection)
{
    OtvdbusConnectionPrivate *priv;
    gboolean result = FALSE;

    O_LOG_ENTER("\n");

    O_ASSERT (NTVDBUS_IS_CONNECTION (connection));
    g_return_val_if_fail (NTVDBUS_IS_CONNECTION (connection), FALSE);

    priv = NTVDBUS_CONNECTION_GET_PRIVATE (connection);

    if (priv->pLowlevelConnection != NULL)
    {
        result = dbus_connection_get_is_authenticated ( dbus_g_connection_get_connection(priv->pLowlevelConnection));
    }
    O_LOG_EXIT("\n");
    return result;
}

DBusGConnection *
otvdbus_connection_get_connection (OtvdbusConnection *connection)
{
    OtvdbusConnectionPrivate *priv;
    O_ASSERT (NTVDBUS_IS_CONNECTION (connection));

    g_return_val_if_fail (NTVDBUS_IS_CONNECTION (connection), NULL);

    priv = NTVDBUS_CONNECTION_GET_PRIVATE (connection);

    return priv->pLowlevelConnection;
}


glong
otvdbus_connection_get_unix_uid (OtvdbusConnection *connection)
{
    OtvdbusConnectionPrivate *priv;
    gulong uid = G_MAXUINT32;
    DBusConnection* pConnection ;
    GMainContext * pContext = g_main_context_get_thread_default();
    O_LOG_ENTER("\n");

    O_ASSERT (NTVDBUS_IS_CONNECTION (connection));
    g_return_val_if_fail (NTVDBUS_IS_CONNECTION (connection), G_MAXUINT32);

    priv = NTVDBUS_CONNECTION_GET_PRIVATE (connection);

    pConnection = dbus_g_connection_get_connection(priv->pLowlevelConnection);

    while((priv->pLowlevelConnection) &&  dbus_connection_get_is_authenticated(pConnection) == FALSE)
    {
        g_main_context_iteration(pContext,FALSE);
        O_LOG_DEBUG("\n Waiting for Client authentication");
    }
    if((NULL == priv->pLowlevelConnection) || (FALSE == dbus_connection_get_unix_user(pConnection, &uid)))
    {
        O_LOG_WARNING("\n  Unable to get Unix user for connection = %p", connection);
        uid = G_MAXUINT32;
    }
    O_LOG_EXIT("\n");

    return uid;
}

glong
otvdbus_connection_get_unix_pid (OtvdbusConnection *connection)
{
    OtvdbusConnectionPrivate *priv;
    gulong pid = G_MAXUINT32;
    DBusConnection* pConnection ;
    GMainContext * pContext = g_main_context_get_thread_default();
    O_LOG_ENTER("\n");

    O_ASSERT (NTVDBUS_IS_CONNECTION (connection));
    g_return_val_if_fail (NTVDBUS_IS_CONNECTION (connection), G_MAXUINT32);

    priv = NTVDBUS_CONNECTION_GET_PRIVATE (connection);

    pConnection = dbus_g_connection_get_connection(priv->pLowlevelConnection);

    while((priv->pLowlevelConnection) &&  dbus_connection_get_is_authenticated(pConnection) == FALSE)
    {
        g_main_context_iteration(pContext,FALSE);
        O_LOG_DEBUG("\n Waiting for Client authentication");
    }
    if((NULL == priv->pLowlevelConnection) || (FALSE == dbus_connection_get_unix_process_id(pConnection, &pid)))
    {
        O_LOG_WARNING(" \n Unable to get PID for Client connection = %p", connection);
        pid = G_MAXUINT32;
    }
    O_LOG_DEBUG("\n Client PID for connection <%p> = %ld ",connection, pid);
    O_LOG_EXIT("\n");

    return pid;
}

#define NTV_SEC

#ifdef NTV_SEC

#define CONNECTION_INFO   "conn-info"

static void
_dbus_service_obj_destroy_notfiy (gpointer data)
{

	/* TODO: Need to check if g_object_ref/unref is required for OtvDbusConnection */
    g_slist_free((GSList*)data);
}

static GSList*
_otvdbus_get_connection_info_from_service_obj(GObject  *obj)
{
    GSList *connectionList = NULL;

    if(obj)
    {
    	connectionList = (GSList *) g_object_get_data ((GObject *)obj, CONNECTION_INFO);
    }

    return connectionList;
}

OtvdbusConnection*
otvdbus_get_connection_from_service_obj(GObject  *obj)
{
    GSList *connectionList = NULL;
    OtvdbusConnection *conn = NULL;

    O_ASSERT(obj != NULL);


    if(obj)
    {
    	connectionList = (GSList *) g_object_get_data ((GObject *)obj, CONNECTION_INFO);
    	if(connectionList)
    	{
    		/* by default return the first element */
    		O_LOG_DEBUG(" Connection List = %p",connectionList);
    		conn = (OtvdbusConnection *)  g_slist_nth_data(connectionList, 0);
    		O_ASSERT(NTVDBUS_IS_CONNECTION(conn));

#ifndef NDEBUG
    		{
    			gint num_connections = g_slist_length(connectionList);
    			if(num_connections > 1)
    				O_LOG_WARNING("The same object is registered on [%d] Connections",num_connections);
    		}
#endif

    	}
#ifndef NDEBUG
    	else
    	{
    		O_ASSERT(0);
    	}
#endif


    }

    return conn;
}


#endif



gboolean
otvdbus_connection_register_object (OtvdbusConnection *connection,
        const gchar *path,GObject* object,  gpointer user_data)
{
    gboolean result = FALSE;
    OtvdbusConnectionPrivate *priv;

    O_LOG_ENTER("\n");

    O_ASSERT (NTVDBUS_IS_CONNECTION (connection));
    O_ASSERT (path != NULL);

    g_return_val_if_fail (NTVDBUS_IS_CONNECTION (connection), FALSE);
    g_return_val_if_fail ((path != NULL), FALSE);

    priv = NTVDBUS_CONNECTION_GET_PRIVATE (connection);
    if (priv->pLowlevelConnection)
    {
        dbus_g_connection_register_g_object (priv->pLowlevelConnection, path, object);
        result = TRUE;

#ifdef NTV_SEC
        /* Get the List of connections from the object */
        GSList* connectionList = _otvdbus_get_connection_info_from_service_obj(object);

        /*Append connection to the list */
        O_LOG_DEBUG(" Connection List = %p", connectionList);
	if(NULL == connectionList)
	{
		connectionList =  g_slist_append(connectionList, (gpointer )connection);
		g_object_set_data_full ((GObject *)object, CONNECTION_INFO, (gpointer) connectionList, _dbus_service_obj_destroy_notfiy);
	}
	else
	{
		connectionList =  g_slist_append(connectionList, (gpointer )connection);
	}
        /* Set the connection list */
#endif

    }
    O_LOG_EXIT("\n");

    return result;
}


gboolean
otvdbus_connection_unregister_object (OtvdbusConnection *connection, const gchar *path, GObject** object)
{

    gboolean retval = FALSE;
    GObject* obj;
    OtvdbusConnectionPrivate *priv;

    O_LOG_ENTER("\n");

    O_ASSERT (NTVDBUS_IS_CONNECTION (connection));
    O_ASSERT (path != NULL);

    g_return_val_if_fail (NTVDBUS_IS_CONNECTION (connection), FALSE);
    g_return_val_if_fail ((path != NULL), FALSE);

    priv = NTVDBUS_CONNECTION_GET_PRIVATE (connection);

    if (priv->pLowlevelConnection)
    {

        if (( obj = dbus_g_connection_lookup_g_object(priv->pLowlevelConnection, path)) != NULL)
        {
            O_LOG_DEBUG( "\n Lookup object found, unregistering %p", obj);
            dbus_g_connection_unregister_g_object (priv->pLowlevelConnection, obj);
            if (object != NULL)
            {
                *object = obj;
            }
            retval = TRUE;
        }
    }
    O_LOG_EXIT("\n");
    return retval;
}

static DBusGConnection *
_otvdbus_g_connection_open_internal(const gchar *pServerAddress,  GMainContext* pMainContext, GError **error, gboolean is_private)
{
	DBusGConnection* pDBusGConnection = NULL;
	DBusConnection*  pDBusConnection  = NULL;

	DBusError derror;
	static gint bIsGValueTypesInitialized = 0;

	O_ASSERT (pServerAddress != NULL);

	g_return_val_if_fail ((pServerAddress != NULL), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* This is a hack to initialize the dbus-glib stack for the GLib Types
	 * The function _dbus_g_value_types_init which is internal to dbus-glib has to be called.
	 * Changing the dbus-glib may cause licensing issues, so this is a work-around */
	if(g_atomic_int_compare_and_exchange( &bIsGValueTypesInitialized, 0, 1))
	{
		dbus_g_connection_open("", NULL);

	}

	dbus_error_init (&derror);

	if (is_private)
	{
		pDBusConnection = dbus_connection_open_private(pServerAddress, &derror);
	}
	else
	{
		pDBusConnection = dbus_connection_open(pServerAddress, &derror);
	}


	if(NULL != pDBusConnection)
	{
		dbus_connection_setup_with_g_main(pDBusConnection, pMainContext);
		pDBusGConnection =  dbus_connection_get_g_connection (pDBusConnection);
		O_ASSERT(NULL != pDBusGConnection);

	}
	else
	{
		dbus_set_g_error (error, &derror);
		dbus_error_free (&derror);
	}

	return pDBusGConnection;
}

DBusGConnection *        otvdbus_g_connection_open(const gchar *pServerAddress,  GMainContext* pMainContext, GError **error)
{
	return _otvdbus_g_connection_open_internal(pServerAddress, pMainContext, error, FALSE);
}

DBusGConnection *        otvdbus_g_connection_open_private(const gchar *pServerAddress,  GMainContext* pMainContext, GError **error)
{
	return _otvdbus_g_connection_open_internal(pServerAddress, pMainContext, error, TRUE);
}

/** @} */



