
/**
 * @file     otvdbus-server.c
 * @author   Rohan
 * @date     Jan 8, 2011
 * @addtogroup OtvdbusServer
 */
#include <dbus/dbus.h>
#include "otvdbus-server.h"
#include "otvdbus-connection.h"
#include "otv-marshal.h"
#include <unistd.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/socket.h>
#include <ntvlog.h>


/**
 * @addtogroup OtvdbusServer
 * @{
 */

/**
 * Define for handling validation multiple server instances
 * Enable it to avoid running multiple instances of server
 */
#define NTVDBUS_VALIDATE_SERVER_ADDRESS_INSTANCES

/**
 * convert the string to standard canonical form
 */
#define I_(string) g_intern_static_string(string)

/**
 * macro for getting object private data
 */
#define NTVDBUS_SERVER_GET_PRIVATE(o)  \
        (G_TYPE_INSTANCE_GET_PRIVATE ((o), NTVDBUS_TYPE_SERVER, OtvdbusServerPrivate))

/**
 *  Enumeration for signals emitted from @ref OtvdbusServer
 */
enum
{
    NTVDBUS_NEW_CONNECTION,//!< NEW_CONNECTION
    NTVDBUS_LAST_SIGNAL,   //!< LAST_SIGNAL
};

/**
 * Enumeration for properites registered for @ref OtvdbusServer object
 */
enum
{
    NTVDBUS_PROP_0, /*!< Place holder */
    NTVDBUS_PROP_CONNECTION_TYPE,/*!< Connection Type (Connection object Type)*/
};


/**
 * OtvdbusServerPriv server private data
 */
struct _OtvdbusServerPrivate
{
    DBusServer *server;	/*!< low level DBus server */
    /**
     * The Connection Type.
     * This is the Type of connection object that this @ref OtvdbusServer Object
     * will create when a new connection is initiated to the server from the client.
     * Currently only an @ref OtvdbusConnection is defined.
     */
    GType       conn_type;
};

/**
 * Typedef for \ref _OtvdbusServerPrivate
 */
typedef struct _OtvdbusServerPrivate OtvdbusServerPrivate;

static guint             s_signals[NTVDBUS_LAST_SIGNAL] = { 0};

/* functions prototype */
static void     _otvdbus_server_dispose     (GObject         *server);
static void     _otvdbus_server_set_property(OtvdbusServer         *server,
        guint               prop_id,
        const GValue       *value,
        GParamSpec         *pspec);
static void      _otvdbus_server_get_property(OtvdbusServer         *server,
        guint               prop_id,
        GValue             *value,
        GParamSpec         *pspec);
static gboolean _otvdbus_server_listen_internal (OtvdbusServer  *server,
        const gchar *address, GMainContext* pContext);
static void     _otvdbus_server_new_connection (OtvdbusServer         *server,
        DBusConnection     *connection);

G_DEFINE_TYPE (OtvdbusServer, otvdbus_server, G_TYPE_OBJECT)


OtvdbusServer *
otvdbus_server_new (void)
{
    OtvdbusServer *server;

    server = NTVDBUS_SERVER (g_object_new (NTVDBUS_TYPE_SERVER, NULL));
    O_DEBUG_ASSERT(NULL != server);

    return server;
}


gboolean
otvdbus_server_listen  (OtvdbusServer  *server,
        const gchar *address, GMainContext* pContext)
{
    O_DEBUG_ASSERT (NTVDBUS_IS_SERVER (server) == TRUE);
    O_DEBUG_ASSERT (address != NULL);

    g_return_val_if_fail ( (NTVDBUS_IS_SERVER (server) == TRUE), FALSE);
    g_return_val_if_fail ((NULL != address), FALSE);

    O_LOG_DEBUG(" \n otv dbus server listen ");
    return _otvdbus_server_listen_internal (server, address, pContext);

}

/*
 * class initialization function
 */
static void
otvdbus_server_class_init (OtvdbusServerClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

    O_LOG_ENTER("\n");
    g_type_class_add_private (klass, sizeof (OtvdbusServerPrivate));

    gobject_class->set_property = (GObjectSetPropertyFunc) _otvdbus_server_set_property;
    gobject_class->get_property = (GObjectGetPropertyFunc) _otvdbus_server_get_property;
    gobject_class->dispose = _otvdbus_server_dispose;
    klass->new_connection = (OtvdbusNewConnectionFunc)_otvdbus_server_new_connection;
    /* install properties */
    g_object_class_install_property (gobject_class,
            NTVDBUS_PROP_CONNECTION_TYPE,
            g_param_spec_gtype ("otvdbus-connection-type",
                    "otvdbus connection type",
                    "The connection type of otvdbus server object",
                    NTVDBUS_TYPE_CONNECTION,
                    G_PARAM_READWRITE ));
    /* install signals */
    s_signals[NTVDBUS_NEW_CONNECTION] =
            g_signal_new (I_("new-connection"),
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (OtvdbusServerClass, new_connection),
                    NULL, NULL,
                    otv_marshal_VOID__POINTER,
                    G_TYPE_NONE, 1,
                    G_TYPE_POINTER);
    O_LOG_EXIT("\n");
}

/* object initialization function */
static void
otvdbus_server_init (OtvdbusServer *server)
{
    OtvdbusServerPrivate *priv;
    O_LOG_DEBUG(" \n otv dbus server object  init");
    priv = NTVDBUS_SERVER_GET_PRIVATE (server);
    priv->server = NULL;
    priv->conn_type = NTVDBUS_TYPE_CONNECTION;
}

/* object destructor function */
static void
_otvdbus_server_dispose (GObject *server)
{
    OtvdbusServerPrivate *priv;
    O_DEBUG_ASSERT( NTVDBUS_IS_SERVER(server) == TRUE);

    priv = NTVDBUS_SERVER_GET_PRIVATE (server);

    if (priv->server)
    {
        dbus_server_unref (priv->server);
        priv->server = NULL;
    }

    G_OBJECT_CLASS(otvdbus_server_parent_class)->dispose(G_OBJECT (server));
}

/*
 * Function to set server properties
 */
static void
_otvdbus_server_set_property    (OtvdbusServer     *server,
        guint           prop_id,
        const GValue   *value,
        GParamSpec     *pspec)
{
    OtvdbusServerPrivate *priv;
    O_DEBUG_ASSERT( NTVDBUS_IS_SERVER(server) == TRUE);

    priv = NTVDBUS_SERVER_GET_PRIVATE (server);

    switch (prop_id)
    {
        case NTVDBUS_PROP_CONNECTION_TYPE:
        {
            GType type;
            type = g_value_get_gtype (value);
            O_DEBUG_ASSERT (g_type_is_a (type, NTVDBUS_TYPE_CONNECTION));
            priv->conn_type = type;
            break;
        }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (server, prop_id, pspec);
            break;
    }
}

/*
 * Function to get server properties
 */
static void
_otvdbus_server_get_property    (OtvdbusServer     *server,
        guint           prop_id,
        GValue         *value,
        GParamSpec     *pspec)
{
    OtvdbusServerPrivate *priv;
    O_DEBUG_ASSERT( NTVDBUS_IS_SERVER(server) == TRUE);
    priv = NTVDBUS_SERVER_GET_PRIVATE (server);

    switch (prop_id)
    {
        case NTVDBUS_PROP_CONNECTION_TYPE:
            g_value_set_gtype (value, priv->conn_type);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (server, prop_id, pspec);
            break;
    }
}

/*
 * default function to handle new connection callback
 */
static void
_otvdbus_server_new_connection (OtvdbusServer      *server,
        DBusConnection  *connection)
{
    /* Do Nothing */
}

/*
 * DBus signal callback function for new connection.
 * This function is called by DBus library when a new connection attempt is made.
 * a new Object of type NTVDBUS_CONNECTION is created and a NEW_CONNECTION signal is emitted.
 * If the object is still floating i.e no one has referenced it then the connection is no accepted.
 */
static void
_otvdbus_new_connection_callback (DBusServer      *dbus_server,
        DBusConnection  *new_connection,
        OtvdbusServer      *server)
{



    OtvdbusConnection *connection;

    connection = otvdbus_connection_new(server->pServerContext);
    O_DEBUG_ASSERT(NULL != connection);
    otvdbus_connection_set_connection (connection, new_connection);


    g_signal_emit (server, s_signals[NTVDBUS_NEW_CONNECTION], 0, connection);

    if (g_object_is_floating (connection))
    {
        /* release connection if it is still floating */
        O_LOG_DEBUG(" \n No references to the connection ");
        g_object_unref (connection);
    }

}

#ifdef NTVDBUS_VALIDATE_SERVER_ADDRESS_INSTANCES

/*
 * Function to check if the socket address is free.
 *
 */
static gboolean _is_server_address_free(const gchar* address)
{

    /* Check if the server is already running, This is just a hack, the low level dbus
       currently does not bail out if there is already a running server*/
    gboolean bIsSingleInstance = TRUE;
    DBusAddressEntry **entries;
    DBusError error = DBUS_ERROR_INIT;;
    const char *method;
    int len , i;
    O_LOG_ENTER("\n");
    if (!dbus_parse_address (address, &entries, &len, &error))
    {

        O_LOG_DEBUG("Bad Address  %s\n", address);
        return FALSE;
    }
    i = 0;
    do
    {

        /* currently validating only addresses of format unix:path= ,not tmpdir or abstract sockets and only one server address
           in the address parameter*/
        method = dbus_address_entry_get_method (entries[i]);

        if (strcmp (method, "unix") == 0)
        {

            const char *path = dbus_address_entry_get_value (entries[i], "path");

            if (NULL != path)
            {
                struct stat sb;

                if (stat (path, &sb) == 0)
                {
                    O_LOG_WARNING("There is an Already running server at: %s\n",address);
                    bIsSingleInstance = FALSE;
                    break;

                }

            }/* end path != NULL */
        }/* end strcmp*/
        i++;
    } while (i < len);/* end while*/

    dbus_address_entries_free (entries);
    O_LOG_EXIT("\n");
    return bIsSingleInstance;
}
#endif

/*
 * Internal helper function to start a server
 */
static gboolean
_otvdbus_server_listen_internal (OtvdbusServer  *server,
        const gchar *address, GMainContext* pContext)
{
    gboolean bResult = FALSE;

    OtvdbusServerPrivate *priv;
    DBusError error;

    priv = NTVDBUS_SERVER_GET_PRIVATE (server);
    O_LOG_DEBUG(" Starting Server at Address : %s\n",address);


#ifdef NTVDBUS_VALIDATE_SERVER_ADDRESS_INSTANCES
    /* check if server address is free */

    if((bResult = _is_server_address_free(address)) == FALSE)
    {
#ifndef NDEBUG
    	/* In debug builds, just bail out returning FALSE */
    	{
    		return bResult;
    	}
#else
    	/* In Non debug builds, remove the socket and continue to start the server */

    	{
    		const gchar *pPath = address;
    		while ( ( *pPath != '\n' ) && ( *pPath != '/' ) )
    		{
    			pPath++;
    		}
    		O_LOG_WARNING("Unlinking Path = %s\n",pPath);
    		unlink( pPath );
    	}
#endif

    }
#endif
    O_DEBUG_ASSERT (priv->server == NULL);


    dbus_error_init (&error);

    priv->server = dbus_server_listen (address, &error);

    if (priv->server == NULL)
    {
        O_LOG_WARNING ("Can not listen on '%s':\n" "  %s:%s", address, error.name, error.message);
        return bResult;
    }
    /* register new connection callback */
    dbus_server_set_new_connection_function (priv->server,
            (DBusNewConnectionFunction) _otvdbus_new_connection_callback,
            server, NULL);

  /*  dbus_server_set_auth_mechanisms (priv->server, NULL);*/

    O_LOG_DEBUG(" Binding server %s to context %p\n",address,pContext);
    /* bind the server to the current thread context */
    dbus_server_setup_with_g_main(priv->server, pContext);
    server->pServerContext = pContext;

    bResult = TRUE;

    return bResult;
}


void
otvdbus_server_disconnect (OtvdbusServer *server)
{
    OtvdbusServerPrivate *priv;

    O_DEBUG_ASSERT (NTVDBUS_IS_SERVER (server) == TRUE);
    g_return_if_fail(NTVDBUS_IS_SERVER (server) == TRUE);

    priv = NTVDBUS_SERVER_GET_PRIVATE (server);

    if(priv->server != NULL)
    {
#if !defined(NDEBUG)
        const gchar * server_address = otvdbus_server_get_address (server);
        O_LOG_DEBUG(" Stopping Server at Address : %s\n",server_address);
        g_free((gpointer)server_address);
#endif
        dbus_server_disconnect (priv->server);
    }
}


const gchar *
otvdbus_server_get_address (OtvdbusServer *server)
{
    gchar *server_address;
    char *temp;
    OtvdbusServerPrivate *priv;

    O_DEBUG_ASSERT (NTVDBUS_IS_SERVER (server));
    g_return_val_if_fail((NTVDBUS_IS_SERVER (server) == TRUE) , NULL);

    priv = NTVDBUS_SERVER_GET_PRIVATE (server);

    O_DEBUG_ASSERT (priv->server != NULL);

    temp = dbus_server_get_address (priv->server);
    server_address = g_strdup (temp);
    dbus_free (temp);
    return server_address;
}


gboolean
otvdbus_server_is_connected (OtvdbusServer *server)
{
    gboolean bIsConnected = FALSE;
    OtvdbusServerPrivate *priv;

    O_DEBUG_ASSERT (NTVDBUS_IS_SERVER (server) == TRUE);
    g_return_val_if_fail((NTVDBUS_IS_SERVER (server) == TRUE) , bIsConnected);

    priv = NTVDBUS_SERVER_GET_PRIVATE (server);

    if (priv->server != NULL)
    {
        bIsConnected =  dbus_server_get_is_connected (priv->server);
    }

    return bIsConnected;
}

/* TODO: security issue need to work with security manager */

gboolean
otvdbus_server_set_auth_mechanisms (OtvdbusServer   *server,
        const gchar **mechanisms)
{

    OtvdbusServerPrivate *priv;

    O_DEBUG_ASSERT (NTVDBUS_IS_SERVER (server) == TRUE);
    g_return_val_if_fail((NTVDBUS_IS_SERVER (server) == TRUE) , FALSE);

    priv = NTVDBUS_SERVER_GET_PRIVATE (server);

    O_DEBUG_ASSERT (priv->server != NULL);

    return dbus_server_set_auth_mechanisms (priv->server, mechanisms);
}

/** @}  */
