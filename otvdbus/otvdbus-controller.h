
/**
 * \file otvdbus-controller.h
 *
 * \author  Rohan
 * \date    01/04/2011
 * \defgroup NTVDBUS NemoTV Dbus Framework
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
 * Users have to derive this class and implement @ref OtvdbusControllerHandleLostConnFn and @ref OtvdbusControllerHandleNewConnFn functions.
 *
 * @ingroup NTVDBUS
 */

/**@{*/
#ifndef _NTVDBUS_DBUS_CONTROLLER_H_
#define _NTVDBUS_DBUS_CONTROLLER_H_

#include <glib-object.h>
#include "otvdbus-connection.h"
#include "otvdbus-server.h"
/*
 * default to char for old dbus glib versions where DBusGObjectPath is not defined
 */
#ifndef DBusGObjectPath
/**
 * DBusObjectPath
 *
 * Added for support if using with old DBUS-GLIB version
 * @todo Should move to Makefiles
 */
#define DBusGObjectPath char
#endif
///@cond
G_BEGIN_DECLS

#define NTVDBUS_TYPE_CONTROLLER             (otvdbus_controller_get_type ())
#define NTVDBUS_CONTROLLER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), NTVDBUS_TYPE_CONTROLLER, OtvdbusController))
#define NTVDBUS_CONTROLLER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), NTVDBUS_TYPE_CONTROLLER, OtvdbusControllerClass))
#define NTVDBUS_IS_CONTROLLER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NTVDBUS_TYPE_CONTROLLER))
#define NTVDBUS_IS_CONTROLLER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), NTVDBUS_TYPE_CONTROLLER))
#define NTVDBUS_CONTROLLER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), NTVDBUS_TYPE_CONTROLLER, OtvdbusControllerClass))


///@endcond
/**
 * OtvdbusControllerClass
 */
typedef struct _OtvdbusControllerClass OtvdbusControllerClass;

/**
 * OtvdbusController
 */
typedef struct _OtvdbusController OtvdbusController;

/**
 * OtvdbusControllerHandleNewConnFn:
 * @param[in] pController An OtvdbusController.
 * @param[in] pConnection The corresponding OtvdbusConnection.
 *
 * Prototype of controller's new connection  function.
 * A function of this type has to be over ridden by the derived classes of the controller
 * The overridden function is called by the framework when a new connection is established from the client
 * @return TRUE : The derived class function should register the necessary objects that they want to expose and return TRUE.
 * 				  The framework will add the connection to the list of connections.
 *         FALSE : The derived class function should return FALSE if they do not want to handle the client.
 *         The Framework will close the connection.
 */
typedef gboolean (*OtvdbusControllerHandleNewConnFn) (OtvdbusController *pController, OtvdbusConnection *pConnection);

/**
 * OtvdbusControllerHandleLostConnFn:
 * @param[in] pController An OtvdbusController.
 * @param[in] pConnection The corresponding OtvdbusConnection.
 *
 * Prototype of controller's connection Lost function.
 * A function of this type has to be over ridden by the derived classes of the controller
 * The overridden function is called by the framework when a connection is lost from the client
 * @return boolean, (unused currently)
 */
typedef gboolean (*OtvdbusControllerHandleLostConnFn) (OtvdbusController *pController, OtvdbusConnection *pConnection);

/** Opaque Class pointer */
struct _OtvdbusControllerClass
{
    /** Parent Class */
    GObjectClass parent_class;
    /**
     *OtvdbusController::handle_connection_lost
     *
     *This is a virtual function to handle connection losses. This
     *function is called when the corresponding client is
     *disconnected.
     *The derived class implementations should unregister the objects
     *they have registered and may have to cleanup if necessary.
     *@param[in] controller  The controller object.
     *@param[in] connection The connection object which was disconnected.
     *@retval TRUE if the derived implementation needs to hold this
     *       connection; FALSE otherwise.
     */
    gboolean (* handle_connection_lost)  (OtvdbusController* pController, OtvdbusConnection* pConnection);
    /**
     *OtvdbusController::handle_new_connection
     *
     *This is a virtual function to handle new connections.
     *This function is called when new client is connected.
     *The derived class implementations should register the DBus
     *objects.
     *@param[in] controller  The controller object.
     *@param[in] connection The connection object which was disconnected.
     * \ref OtvdbusController The controller object.
     * OtvdbusConnection The connection object which referring to the connection.
     */
    gboolean (* handle_new_connection)  (OtvdbusController* pController, OtvdbusConnection* pConnection);
};

/**
 * OtvdbusController:
 *
 * An opaque data type representing an OtvdbusController.
 */
struct _OtvdbusController
{
    GObject parent_instance;/*!< parent object */

};

/**
 * GObject typedef
 * @internal
 * @return GType
 */
GType otvdbus_controller_get_type (void) G_GNUC_CONST;

/**
 * This function sends a signal on all connections at the given object_path
 * @param self       controller object
 * @param object_path  The object path at which the signal has to be emitted
 * @param signal_id    The signal id
 * @param detail       signal details
 * The actual parameters have to be passed at the end
 * @return
 */
gboolean otvdbus_controller_send_signal_on_all_connections (OtvdbusController *self, DBusGObjectPath* object_path, gint signal_id, GQuark detail,...);


/**
 * This function Starts the controller, which in turn starts the dbus server  and listens for connections
 * @param self   The controller object
 * @param pLoop  A GMainloop , if this is NULL the controller shall create a mainloop
 * @param dbus_server_address  The server socket address at which the server should run.
 *  @retval FALSE if the server is unable to run.
 *
 *  Calling This function is equivalent to calling
 * - @ref otvdbus_controller_init_mainloop
 * - @ref otvdbus_controller_add_server
 * - @ref otvdbus_controller_start in sequence.
 */
gboolean otvdbus_controller_run(OtvdbusController *self , GMainLoop* pLoop , char* dbus_server_address);


/**
 * This function Removes the objects registered at the object path on all connections to the server
 * @param self    The controller object
 * @param object_path the object path at which the DBus Object has to be removed
 * @return TRUE if the Objects are removed else FALSE.
 */
gboolean otvdbus_controller_remove_from_all_connections (OtvdbusController *self, DBusGObjectPath *object_path);

/**
 * This Function is used to get the OtvdbusServer Instance of the controller
 * @param self The controller object
 * @return \ref OtvdbusServer instance
 */
OtvdbusServer* otvdbus_controller_get_server (OtvdbusController *self);

/**
 * This Function will initialize the controller with the given Mainloop.
 * If the MainLoop is NULL the controller will create a Mainloop and use the Global default Context.
 *
 * @param self The controller object
 * @param pLoop The GMainLoop.
 * @return TRUE if the loop and context are initialized properly else FALSE.
 *
 * @note This function should be called first before adding a server to the \ref OtvdbusController using
 * @ref otvdbus_controller_add_server.
 * Use
 * - @ref otvdbus_controller_init_mainloop
 * - @ref otvdbus_controller_add_server
 * - @ref otvdbus_controller_start in sequence.
 *
 */
gboolean  otvdbus_controller_init_mainloop(OtvdbusController *self ,  GMainLoop *pLoop );


/**
 * This Function adds a server to the controller.
 * Currently only one server is supported per controller.
 *
 * @param self  The controller object
 * @param dbus_server_address The dbus server address.
 * @returns The Complete dbus server address with guid if the server is added successfully
 * 	otherwise NULL.
 *
 * @note  Use
 * - @ref otvdbus_controller_init_mainloop
 * - @ref otvdbus_controller_add_server
 * - @ref otvdbus_controller_start in sequence.

 */
const gchar*  otvdbus_controller_add_server(OtvdbusController *self ,  char* dbus_server_address );

/**
 * This function will start the controller, i.e the Mainloop of the controller.
 * This function should be called after calling \ref  otvdbus_controller_init_mainloop and \ref otvdbus_controller_add_server
 *
 * @param self The controller object
 * This call will not return.
 *
 * @note  Use
 * - @ref otvdbus_controller_init_mainloop
 * - @ref otvdbus_controller_add_server
 * - @ref otvdbus_controller_start in sequence.
 *
 */
void otvdbus_controller_start(OtvdbusController *self );

/**
 * This function will stop the controller, i.e the Mainloop of the controller.
 * 
 *
 * @param self The controller object
 * 
 *
 *
 */
void otvdbus_controller_stop(OtvdbusController *self );
	

/**
 * This Function returns the GMainContext of the controller.
 * @param self
 * @return The GMainContext of the controller.
 */
GMainContext* otvdbus_controller_get_context(OtvdbusController *self);

/**
 * This Function returns the GMainLoop of the controller.
 * @param self
 * @return The GMainLoop of the controller.
 */
GMainLoop* otvdbus_controller_get_mainloop(OtvdbusController *self);

G_END_DECLS

/** @}  */

#endif /* _NTVDBUS_DBUS_CONTROLLER_H_ */
