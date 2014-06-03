/**
 * \file otvdbus-server.h
 *
 * \author Rohan
 * \date    01/04/2011
 * @defgroup OtvdbusServer OtvdbusServer
 * @ingroup NTVDBUS
 *
 * @brief Server that listen on a socket and wait for connection requests.
 *  An OtvDBUSServer listen on a socket and wait for connections requests,
 * just like DBusServer.
 *
 */

/**@{*/
#ifndef __NTVDBUS_SERVER_H_
#define __NTVDBUS_SERVER_H_

#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include "otvdbus-connection.h"
/*
 * Type macros.
 */
///@cond

/* define GOBJECT macros */
#define NTVDBUS_TYPE_SERVER             \
        (otvdbus_server_get_type ())
#define NTVDBUS_SERVER(obj)             \
        (G_TYPE_CHECK_INSTANCE_CAST ((obj), NTVDBUS_TYPE_SERVER, OtvdbusServer))
#define NTVDBUS_SERVER_CLASS(klass)     \
        (G_TYPE_CHECK_CLASS_CAST ((klass), NTVDBUS_TYPE_SERVER, OtvdbusServerClass))
#define NTVDBUS_IS_SERVER(obj)          \
        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NTVDBUS_TYPE_SERVER))
#define NTVDBUS_IS_SERVER_CLASS(klass)  \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), NTVDBUS_TYPE_SERVER))
#define NTVDBUS_SERVER_GET_CLASS(obj)   \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), NTVDBUS_TYPE_SERVER, OtvdbusServerClass))
///@endcond

G_BEGIN_DECLS

/**
 * Typedef for @ref _OtvdbusServer
 */
typedef struct _OtvdbusServer OtvdbusServer;

/**
 * Typedef for @ref _OtvdbusServerClass
 */
typedef struct _OtvdbusServerClass OtvdbusServerClass;

/**
 * OtvdbusNewConnectionFunc:
 * @param[in] server An OtvdbusServer.
 * @param[in] connection The corresponding OtvdbusConnection.
 *
 * Prototype of new connection callback function.
 * This callback should be connected to signal new_connection
 * to handle the event that a new connection is coming in.
 * In this handler, Otvdbus could add a reference and continue processing the connection.
 * If no reference is added, the new connection will be released and closed after this signal.
 */

typedef void (* OtvdbusNewConnectionFunc) (OtvdbusServer *server, OtvdbusConnection *connection);

/**
 * OtvdbusServer:
 *
 * An opaque object representing an OtvdbusServer.
 */
struct _OtvdbusServer {


    GObject parent; /*!< Parent GObject */
    /* instance members */
    GMainContext* pServerContext;

};

/**
 * An opaque definition representing an OtvdbusServerClass.
 */
struct _OtvdbusServerClass {
    GObjectClass parent; /*!< Parent class */

    /* signals */
    /**
     * new connection function pointer
     * used for default handler
     * @param server    dbus server
     * @param connection connetion object
     */
    void  (* new_connection)    (OtvdbusServer     *server,
            OtvdbusConnection *connection);
    /*< private >*/
};

/**
 * GObject typedef
 * @internal
 * @return GType
 */
GType            otvdbus_server_get_type           (void);

/**
 * otvdbus_server_new:
 * @return A newly allocated OtvdbusServer instance.
 *
 * New an OtvdbusServer.
 */
OtvdbusServer      *otvdbus_server_new                (void);

/**
 * Listens for new connections on the given address.
 *
 * If there are multiple semicolon-separated address entries in the address,
 * tries each one and listens on the first one that works.
 *
 * Returns FALSE if listening fails for any reason.
 *
 * To free the server, applications must call first otvdbus_server_disconnect() and delete the \ref OtvdbusServer object .
 *
 * @param[in] server: An OtvdbusServer.
 * @param[in] address: Address of this server.
 * @return TRUE if succeed ; FALSE otherwise.
 */
gboolean         otvdbus_server_listen             (OtvdbusServer     *server,
        const gchar    *address,  GMainContext* pContext);

/**
 * otvdbus_server_disconnect:
 * @param[in] server An OtvdbusServer.
 *
 * Releases the server's address and stops listening for new clients.
 *
 */
void             otvdbus_server_disconnect         (OtvdbusServer     *server);

/**
 * Returns the address of the server, as a newly-allocated string which must be freed by the caller.
 * @param[in] server An OtvdbusServer.
 * @return A newly allocated string which contain address.
 *
 */
const gchar     *otvdbus_server_get_address        (OtvdbusServer     *server);

/**
 * Returns TRUE if the server is still listening for new connections.
 *
 * @param[in] server: An OtvdbusServer.
 * @return TRUE if the server is still listening for new connections; FALSE otherwise.
 *
 */
gboolean         otvdbus_server_is_connected       (OtvdbusServer     *server);

/**
 * otvdbus_server_set_auth_mechanisms:
 * @param[in] server: An OtvdbusServer.
 * @param[in] mechanisms: NULL-terminated array of mechanisms.
 * @return  TRUE if succeed; FALSE if insufficient memory.
 *
 * Sets the authentication mechanisms that this server offers to clients,
 * as a NULL-terminated array of mechanism names.
 *
 * This function only affects connections created after it is called.
 * Pass NULL instead of an array to use all available mechanisms (this is the default behavior).
 *
 * The D-Bus specification describes some of the supported mechanisms.
 */
gboolean         otvdbus_server_set_auth_mechanisms(OtvdbusServer     *server,
        const gchar   **mechanisms);

G_END_DECLS

/** @}  */

#endif

