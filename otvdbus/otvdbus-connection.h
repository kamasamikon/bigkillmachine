
/**
 *\file otvdbus-connection.h
 * @defgroup OtvdbusConnection OtvdbusConnection
 * @ingroup  NTVDBUS
 * @author Rohan
 * @date    01/04/2011
 *
 *@brief Connection that manages low level DBus connections
 * use dbus-glib and DBus libraries to handle client connections.
 *
 */


#ifndef NTVDBUS_CONNECTION_H_
#define NTVDBUS_CONNECTION_H_
#include <glib-object.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus.h>

/**@{*/

/*
 * Type macros.
 */
///@cond
/* define GOBJECT macros */
#define NTVDBUS_TYPE_CONNECTION             \
        (otvdbus_connection_get_type ())
#define NTVDBUS_CONNECTION(obj)             \
        (G_TYPE_CHECK_INSTANCE_CAST ((obj), NTVDBUS_TYPE_CONNECTION, OtvdbusConnection))
#define NTVDBUS_CONNECTION_CLASS(klass)     \
        (G_TYPE_CHECK_CLASS_CAST ((klass), NTVDBUS_TYPE_CONNECTION, OtvdbusConnectionClass))
#define NTVDBUS_IS_CONNECTION(obj)          \
        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NTVDBUS_TYPE_CONNECTION))
#define NTVDBUS_IS_CONNECTION_CLASS(klass)  \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), NTVDBUS_TYPE_CONNECTION))
#define NTVDBUS_CONNECTION_GET_CLASS(obj)   \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), NTVDBUS_TYPE_CONNECTION, OtvdbusConnectionClass))
///@endcond
G_BEGIN_DECLS
/**
 * MACRO for Connection Disconnected
 */
#define NTVDBUS_CONNECTION_DISCONNECTED_SIG "disconnected"
/**
 * OtvdbusConnection
 */
typedef struct _OtvdbusConnection OtvdbusConnection;
/**
 * OtvdbusConnectionClass
 */
typedef struct _OtvdbusConnectionClass OtvdbusConnectionClass;

/**
 *internal object
 * @internal
 */
GType otvdbus_connection_get_type (void) G_GNUC_CONST;

/**
 * OtvdbusConnection:
 *
 * An opaque data type representing an OtvdbusConnection.
 */
struct _OtvdbusConnection {
    GObject parent;/*!< parent object */
    /* instance members */
    GMainContext* pConnectionContext;
};
/**
 * _OtvdbusConnectionClass
 *
 * An opaque data type representing an OtvdbusConnectionClass.
 */
struct _OtvdbusConnectionClass {
    GObjectClass parent; /*!< parent object */

    /* signals */
    /**
     * authenticate-client
     * @param[in] connection  The \ref OtvdbusConnection object which received the signal.
     * @param[in]  Linux/Unix user id.
     *
     * This signal is emitted when
     * Returns: TRUE if succeed; FALSE otherwise.
     */
    gboolean    (* authenticate_client)
    (OtvdbusConnection   *connection,
            gulong            uid);
    /**
     * otvdbus_signal Function pointer for handling low level DBUS Messages
     * @param[in] connection  The \ref OtvdbusConnection object which received the signal.
     * @param[in] message , the low level DBusMessage.
     *
     * Returns: TRUE if succeed; FALSE otherwise.
     */

    gboolean    (* otvdbus_signal)     (OtvdbusConnection   *connection,
            DBusMessage      *message);

    /**
     * disconnected Function pointer for handling Disconnected signal from the client
     * @param[in] connection  The \ref OtvdbusConnection object which received the signal.
     *
     * Returns: None
     */


    void        (* disconnected)    (OtvdbusConnection   *connection);

    /*< private >*/
    /** padding */
    gpointer pdummy[4];
};

/**
 * otvdbus_connection_new
 * @param pContext The Context with which the connection has to be executed
 *@retval returns a new instance of OtvdbusConnection
 * Set an OtvdbusConnection as data of a D-Bus connection.
 */
OtvdbusConnection * otvdbus_connection_new (GMainContext* pContext);

/**
 * Set an OtvdbusConnection as data of a D-Bus connection.
 * @param[in] connection: An OtvdbusConnection.
 * @param[in] dbus_connection: A D-Bus connection.
 *
 * This function sets the connection Low level DBus Connection to the @ref OtvdbusConnection
 * Object.
 * Setup the connection with the current GMainLoop Context.
 * All the functions related to this connection from here on will be executed in the context of the thread from which this function is called.
 *
 * @note  Currently only single thread is supported.
 * i.e The server @ref OtvdbusServer and the connections @ref OtvdbusConnection are executed in the same thread.
 *
 */
void             otvdbus_connection_set_connection     (OtvdbusConnection     *connection,
        DBusConnection     *dbus_connection);

/**
 * otvdbus_connection_is_connected
 * @param[in] connection An OtvdbusConnection.
 * @retval TRUE for connected; FALSE otherwise.
 *
 * Whether an OtvdbusConnection is connected.
 */
gboolean         otvdbus_connection_is_connected       (OtvdbusConnection     *connection);

/**
 * otvdbus_connection_is_authenticated
 * @param[in] connection: An OtvdbusConnection.
 * @retval TRUE for authenticated; FALSE otherwise.
 *
 * Whether an OtvdbusConnection is authenticated.
 */
gboolean         otvdbus_connection_is_authenticated   (OtvdbusConnection     *connection);

/**
 * otvdbus_connection_get_connection
 * Function to get the low level DBusGConnection
 * @param[in] connection An OtvdbusConnection.
 * @retval  The corresponding DBusGConnection.
 *
 * Return corresponding DBusGConnection.
 */
DBusGConnection  *otvdbus_connection_get_connection     (OtvdbusConnection     *connection);

/**
 * Function to get the peer client UID
 *
 * @param[in] connection An OtvdbusConnection.
 * @retval  The LINUX/UNIX UID of peer client.
 *
 * Return The UNIX UID of peer client.
 */
glong            otvdbus_connection_get_unix_uid      (OtvdbusConnection     *connection);

/**
 * Function to get the peer client PID
 *
 * @param[in] connection An OtvdbusConnection.
 * @retval  The LINUX/UNIX PID of peer client.
 *
 * Return The UNIX PID of peer client.
 */
glong            otvdbus_connection_get_unix_pid      (OtvdbusConnection     *connection);

/**
 * otvdbus_connection_flush
 * @param[in] connection An OtvdbusConnection.
 *
 * Blocks until the outgoing message queue is empty.
 * This function is a wrapper of dbus_connection_flush().
 *
 */
void             otvdbus_connection_flush              (OtvdbusConnection     *connection);

/**
 * otvdbus_connection_register_object
 * @param[in] connection An OtvdbusConnection.
 * @param[in] path Object path to be register.
 * @param[in] Object GObject to register.
 * @param[in] user_data User data to pass.
 * @return FALSE if fail because of out of memory or illegal
 *        params ; TRUE otherwise.
 *
 * Registers a GObject  at  a given path in the object hierarchy.
 * It is the responsibility of the caller to cleanup  after losing the connection.
 * This function is a wrapper of dbus_g_connection_register_object().
 *
 */
gboolean         otvdbus_connection_register_object
(OtvdbusConnection     *connection,
        const gchar        *path,
        GObject* Object,
        gpointer           user_data);

/**
 * otvdbus_connection_unregister_object
 * @param[in] connection An OtvdbusConnection.
 * @param[in] path Object path to be unregister.
 * @param[out] object GObject at this path.
 * @return FALSE if fail because of out of memory; TRUE otherwise.
 *
 * Unregisters  a GObject at the given path, returns the object
 * found at the object path, it is the responsibility of the
 * caller to clean up the object if necessary
 * This function is a wrapper of
 * dbus_g_connection_unregister_object()
 */
gboolean         otvdbus_connection_unregister_object
(OtvdbusConnection     *connection,
        const gchar        *path,
        GObject** object);

/**
 * otvdbus_g_connection_open
 * @param[in] pServerAddress Server path for which the connection has to be opened.
 * @param[in] pMainContext The GMainContext to which the connection must be attached. Can be NULL (default Context)
 * @param[in] error The GError containing error details, Can be NULL.
 * @return DBusGConnection if Success otherwise NULL.
 *
 */
DBusGConnection*         otvdbus_g_connection_open(const gchar *pServerAddress,  GMainContext* pMainContext, GError **error);

/**
 * otvdbus_g_connection_open_private
 * @param[in] pServerAddress Server path for which the connection has to be opened.
 * @param[in] pMainContext The GMainContext to which the connection must be attached. Can be NULL (default Context)
 * @param[in] error The GError containing error details, Can be NULL.
 * @return DBusGConnection if Success otherwise NULL.
 *
 */
DBusGConnection*         otvdbus_g_connection_open_private(const gchar *pServerAddress,  GMainContext* pMainContext, GError **error);

/**
 * otvdbus_get_connection_from_service_obj
 * @param obj
 * @return OtvdbusConnection
 *
 * This function returns the OtvDbusConnection object to which the given Dbus GObject is registered using
 * @ref  otvdbus_connection_register_object
 */
OtvdbusConnection* otvdbus_get_connection_from_service_obj(GObject  *obj);

G_END_DECLS

/** @}  */

#endif /* NTVDBUS_CONNECTION_H_ */
