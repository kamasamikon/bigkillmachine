/**
 * @file     otvdbus-controller.c
 * @author   Rohan
 * @date     Jan 8, 2011
 * @addtogroup OtvdbusController
 */

/**
 * @{
 */

#include <dbus/dbus-glib.h>
#include "otvdbus-controller.h"
#include <ntvlog.h>
#include <string.h>

/**
 * Macro for getting Private data
 */
#define NTVDBUS_CONTROLLER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), NTVDBUS_TYPE_CONTROLLER, OtvdbusControllerPrivate))

/**
 * Controller private data
 */
struct _OtvdbusControllerPrivate
{
    GSList* pConnections; /*!< List of connections */
    GMainContext* pContext;
    GMainLoop* pMainLoop; /*!< reference to the GLib Main Loop */
    OtvdbusServer* pServer;/*!< reference to \ref OtvdbusServer */


};

/**
 * Typedef for controller private data
 */
typedef struct _OtvdbusControllerPrivate OtvdbusControllerPrivate ;

static void _otvdbus_controller_server_new_connection_cb (OtvdbusServer *server,OtvdbusConnection* connection, gpointer user_data);
static void _otvdbus_controller_connection_lost_cb (OtvdbusConnection* connection, gpointer  user_data);

/* default new connection function
 * called if the derived class has not implemented, asserts by default*/
static gboolean _otvdbus_controller_handle_new_connection (OtvdbusController *self, OtvdbusConnection* connection)
{
    O_LOG_ERROR(" \n The Caller has to override this method");

    return FALSE;
}
/* default connection lost function
 * called if the derived class has not implemented, asserts by default*/

static gboolean _otvdbus_controller_handle_connection_lost (OtvdbusController *self, OtvdbusConnection* connection)
{
    O_LOG_ERROR(" \n The Caller has to override this method");
    return FALSE;
}

/**
 * GObject Definition of @ref _OtvdbusControllerClass
 * @internal
 * @param OtvdbusController
 * @param otvdbus_controller
 * @param G_TYPE_OBJECT
 */
G_DEFINE_TYPE (OtvdbusController, otvdbus_controller, G_TYPE_OBJECT);

#ifndef NDEBUG

#define LOG_SERVER_ADDRESS(priv) do {\
    const gchar * pServer = otvdbus_server_get_address(priv->pServer); \
    O_LOG_DEBUG("\n Server = %s ", pServer); \
    g_free((void*)pServer); \
	}while(0);
#else
#define LOG_SERVER_ADDRESS(priv)
#endif


/*
 * controller initialization function
 */
static void otvdbus_controller_init (OtvdbusController *pController)
{
    OtvdbusControllerPrivate *priv;
    priv = NTVDBUS_CONTROLLER_GET_PRIVATE (pController);
    priv->pConnections = NULL;

    /* Create a new server object */
    priv->pServer = g_object_new(NTVDBUS_TYPE_SERVER ,"otvdbus-connection-type", NTVDBUS_TYPE_CONNECTION, NULL);
    O_DEBUG_ASSERT( NULL != priv->pServer);

    g_signal_connect(priv->pServer , "new-connection",G_CALLBACK(_otvdbus_controller_server_new_connection_cb),pController);


}

/*
 * controller finalize function
 */
static void _otvdbus_controller_finalize (GObject *pController)
{
    OtvdbusControllerPrivate *priv;
    O_LOG_ENTER("\n");
    priv = NTVDBUS_CONTROLLER_GET_PRIVATE (pController);


    if (priv->pConnections != NULL)
    {
        O_LOG_DEBUG("\n Cleaning up existing connections");

        g_slist_foreach(priv->pConnections,(GFunc) g_object_unref , NULL);
        g_slist_free(priv->pConnections);

        priv->pConnections = NULL;
    }
    if (priv->pServer != NULL)
    {
        otvdbus_server_disconnect(priv->pServer);
        g_object_unref(priv->pServer);
        priv->pServer = NULL;
    }
    if (priv->pContext)
    {
    	O_LOG_DEBUG("\n Releasing Context Loop");
    	g_main_context_unref(priv->pContext);
    	priv->pContext = NULL;
    }
    if (priv->pMainLoop)
    {
        O_LOG_DEBUG("\n Quitting Main Loop");
        /* TODO: Need to see if we have to really stop the loop or
         * it will be a responsibility of the derived class.
         * The issue is if the derived class passes the pointer of the MainLoop.
         *
         */
        g_main_loop_quit(priv->pMainLoop);
        g_main_loop_unref(priv->pMainLoop);
        priv->pMainLoop = NULL;
    }
    G_OBJECT_CLASS (otvdbus_controller_parent_class)->finalize (pController);

    O_LOG_EXIT("\n");
}

/*
 * controller class initialization
 */
static void otvdbus_controller_class_init (OtvdbusControllerClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);

    O_LOG_ENTER("\n");
    g_type_class_add_private(klass , sizeof(OtvdbusControllerPrivate ));
    object_class->finalize = _otvdbus_controller_finalize;
    klass->handle_connection_lost = _otvdbus_controller_handle_connection_lost;
    klass->handle_new_connection = _otvdbus_controller_handle_new_connection ;
    O_LOG_EXIT("\n");
}


gboolean otvdbus_controller_send_signal_on_all_connections (OtvdbusController *self, DBusGObjectPath* object_path, gint signal_id, GQuark detail , ...)
{
    OtvdbusControllerPrivate *priv;
    DBusGConnection* gconnection;
    O_LOG_ENTER("\n");

    O_DEBUG_ASSERT( NTVDBUS_IS_CONTROLLER(self) == TRUE);

    g_return_val_if_fail (NTVDBUS_IS_CONTROLLER (self), FALSE);
    g_return_val_if_fail ((NULL != object_path), FALSE);

    priv = NTVDBUS_CONTROLLER_GET_PRIVATE (self);

    GSList *element = priv->pConnections;


    va_list args, var_copy;

    va_start (args, detail);
    G_VA_COPY(var_copy, args);

    while (element != NULL)
    {
        OtvdbusConnection *connection = NTVDBUS_CONNECTION (element->data);

        gconnection = otvdbus_connection_get_connection(connection);

        GObject*  obj =     dbus_g_connection_lookup_g_object (gconnection, object_path);

        if (obj != NULL)
        {
            g_signal_emit_valist(obj,signal_id,detail,var_copy);
        }

        element = element->next;

    }
    va_end(var_copy);
    va_end (args);

    O_LOG_EXIT("\n");

    return TRUE;

}


gboolean otvdbus_controller_remove_from_all_connections (OtvdbusController *self, DBusGObjectPath *object_path)
{
    OtvdbusControllerPrivate *priv;

    O_LOG_ENTER("\n");
    O_DEBUG_ASSERT( NTVDBUS_IS_CONTROLLER(self) == TRUE);

    g_return_val_if_fail (NTVDBUS_IS_CONTROLLER (self), FALSE);
    g_return_val_if_fail ((NULL != object_path), FALSE);

    priv = NTVDBUS_CONTROLLER_GET_PRIVATE (self);
    GSList *element = priv->pConnections;

    while (element != NULL)
    {

        OtvdbusConnection *connection = NTVDBUS_CONNECTION (element->data);

        if (FALSE == otvdbus_connection_unregister_object(connection , object_path , NULL))
        {
            O_LOG_DEBUG(" Unable to unregister %s from connection %p", object_path, connection);
        }

        element = element->next;
    }
    O_LOG_EXIT("\n");

    return TRUE;
}
OtvdbusServer*  otvdbus_controller_get_server(OtvdbusController *self)
{
    OtvdbusControllerPrivate *priv;
    O_LOG_ENTER("\n");

    O_DEBUG_ASSERT( NTVDBUS_IS_CONTROLLER(self) == TRUE);

    g_return_val_if_fail( (NTVDBUS_IS_CONTROLLER(self) == TRUE), NULL);
    priv = NTVDBUS_CONTROLLER_GET_PRIVATE (self);

    return (priv->pServer);

}

gboolean otvdbus_controller_run(OtvdbusController *self , GMainLoop* pLoop , char* dbus_server_address)
{
    OtvdbusControllerPrivate *priv;
    gboolean bResult = FALSE;

    O_LOG_ENTER("\n");

    O_DEBUG_ASSERT( NTVDBUS_IS_CONTROLLER(self) == TRUE);

    g_return_val_if_fail( (NTVDBUS_IS_CONTROLLER(self) == TRUE), bResult);
    g_return_val_if_fail( (NULL != dbus_server_address), bResult);



    priv = NTVDBUS_CONTROLLER_GET_PRIVATE (self);

    if( (bResult =   otvdbus_controller_init_mainloop(self , pLoop )) == TRUE)
    {

    	O_LOG_DEBUG("\n Trying to Start DBus Server");

    	bResult = otvdbus_server_listen(priv->pServer ,  dbus_server_address ,  priv->pContext);

    	if ((bResult == TRUE ) && g_main_loop_is_running(priv->pMainLoop) == FALSE)
    	{
    		O_LOG_DEBUG("\n !! Server %s Added Successfully  !! Running the GMain Loop !!\n\n",dbus_server_address);
    		g_main_loop_run(priv->pMainLoop);
    	}
    	else
    	{
    		O_LOG_WARNING("\n !! Failed to Start the Server \t  %s ",dbus_server_address);
    		g_main_loop_unref(priv->pMainLoop);
    		priv->pMainLoop = NULL;
    		g_main_context_unref(priv->pContext);
    		priv->pContext = NULL;
    	}
    	O_LOG_EXIT("\n");

    }
    return bResult;

}


/*
 * This function is called when a connection is lost.
 * The connection pointer is removed from the list of connections and the derived class lost connection function
 * is called.
 *
 */
static void _otvdbus_controller_connection_lost_cb (OtvdbusConnection* connection, gpointer        user_data)
{

    OtvdbusController * controller = NTVDBUS_CONTROLLER(user_data);
    OtvdbusControllerPrivate *priv;
    OtvdbusControllerClass* klass = NTVDBUS_CONTROLLER_GET_CLASS(controller);

    O_LOG_ENTER("\n Connection Lost ");

    priv = NTVDBUS_CONTROLLER_GET_PRIVATE (controller);
    LOG_SERVER_ADDRESS(priv);

    klass->handle_connection_lost(controller , connection);

    priv->pConnections = g_slist_remove(priv->pConnections , connection);

    g_signal_handlers_disconnect_by_func (connection,
            (GCallback) _otvdbus_controller_connection_lost_cb,
            controller);
    g_object_unref(connection);
    O_LOG_EXIT("\n");

}


/*
 * signal callback function for OtvDBusServer new connection function
 * This function calls the derived class implementation of handle_new_connection
 * If the derived class function returns TRUE, the controller adds the connection object to
 * the list of active connections
 * else it destroys the connection object
 */
static void _otvdbus_controller_server_new_connection_cb (OtvdbusServer *server,OtvdbusConnection* connection, gpointer        user_data)
{
    OtvdbusController * controller = NTVDBUS_CONTROLLER(user_data);
    OtvdbusControllerPrivate *priv;
    OtvdbusControllerClass* klass = NTVDBUS_CONTROLLER_GET_CLASS(controller);

    O_LOG_ENTER("\n");

    O_DEBUG_ASSERT( NTVDBUS_IS_CONNECTION(connection) == TRUE);

    priv = NTVDBUS_CONTROLLER_GET_PRIVATE (controller);

    LOG_SERVER_ADDRESS(priv);

    O_LOG_DEBUG(" \n New Connection to server = %p , connection = %p , user_data = %p", server , connection , user_data);

    O_DEBUG_ASSERT(server == priv->pServer);

    if (TRUE == klass->handle_new_connection(controller ,  connection))
    {
        O_LOG_DEBUG(" \n Connection Accepted by derived class ");

        priv->pConnections = g_slist_append(priv->pConnections , g_object_ref(connection));
        g_signal_connect(connection , NTVDBUS_CONNECTION_DISCONNECTED_SIG,G_CALLBACK(_otvdbus_controller_connection_lost_cb),user_data);

    }
    else
    {
        O_LOG_DEBUG(" \n Connection rejected by derived class ");
        g_object_unref(connection);
    }
    O_LOG_EXIT("\n");

}
gboolean  otvdbus_controller_init_mainloop(OtvdbusController *self ,  GMainLoop *pLoop )
{
    OtvdbusControllerPrivate *priv;
    gboolean bResult = FALSE;

    O_LOG_ENTER("\n");

    O_DEBUG_ASSERT( NTVDBUS_IS_CONTROLLER(self) == TRUE);

    g_return_val_if_fail( (NTVDBUS_IS_CONTROLLER(self) == TRUE), bResult);

    priv = NTVDBUS_CONTROLLER_GET_PRIVATE (self);

    O_DEBUG_ASSERT(priv->pMainLoop == NULL);


    if (pLoop != NULL)
    {
    	priv->pMainLoop = g_main_loop_ref(pLoop);

    	priv->pContext = g_main_context_ref(g_main_loop_get_context(pLoop));
    }
    else
    {
    	O_LOG_DEBUG(" \n Creating a  GMain Loop and Using the Global Default Context for the server");
    	priv->pContext = g_main_context_ref(g_main_context_default());
    	O_DEBUG_ASSERT(priv->pContext != NULL);

    	priv->pMainLoop = g_main_loop_new(priv->pContext , FALSE);
    	O_DEBUG_ASSERT(NULL != priv->pMainLoop);
        O_LOG_DEBUG(" \n Pushing the current thread default Context %p", priv->pContext);

    }

    return TRUE;
}

const gchar*  otvdbus_controller_add_server(OtvdbusController *self ,  char* dbus_server_address )
{
    OtvdbusControllerPrivate *priv;

    gboolean bResult;
    O_LOG_ENTER("\n");

    O_DEBUG_ASSERT( NTVDBUS_IS_CONTROLLER(self) == TRUE);

    g_return_val_if_fail( (NTVDBUS_IS_CONTROLLER(self) == TRUE), NULL);
    g_return_val_if_fail( (NULL != dbus_server_address), NULL);

    priv = NTVDBUS_CONTROLLER_GET_PRIVATE (self);

    O_DEBUG_ASSERT(NULL != priv->pMainLoop);

    g_return_val_if_fail(priv->pMainLoop != NULL, NULL);
    O_LOG_DEBUG("\n Listening DBus Server");

    bResult = otvdbus_server_listen(priv->pServer ,  dbus_server_address ,  priv->pContext);

    if(bResult == TRUE)
    {
        O_LOG_DEBUG("\n Listening DBus Server Done");
        return otvdbus_server_get_address(priv->pServer);
    }
    O_LOG_EXIT("\n");
    return NULL;

}

void otvdbus_controller_start(OtvdbusController *self )
{
    OtvdbusControllerPrivate *priv;

    O_LOG_ENTER("\n");

    O_DEBUG_ASSERT( NTVDBUS_IS_CONTROLLER(self) == TRUE);
    priv = NTVDBUS_CONTROLLER_GET_PRIVATE (self);
    O_DEBUG_ASSERT(priv->pMainLoop != NULL);

    g_return_if_fail(priv->pMainLoop != NULL);

    if (g_main_loop_is_running(priv->pMainLoop) == FALSE)
    {
    	O_LOG_DEBUG("\n Starting MainLoop ");
        g_main_loop_run(priv->pMainLoop);
    }
    /* This call will not return */
    O_LOG_EXIT("\n");

}


void otvdbus_controller_stop(OtvdbusController *self )
{
    OtvdbusControllerPrivate *priv;

    O_LOG_ENTER("\n");
    O_DEBUG_ASSERT( NTVDBUS_IS_CONTROLLER(self) == TRUE);
    priv = NTVDBUS_CONTROLLER_GET_PRIVATE (self);
    O_DEBUG_ASSERT(priv->pMainLoop != NULL);

    g_return_if_fail(priv->pMainLoop != NULL);

    if (g_main_loop_is_running(priv->pMainLoop)) 
    {
    	O_LOG_DEBUG("\n Stopping MainLoop ");
        g_main_loop_quit(priv->pMainLoop);
    }
    /* This call will not return */
    O_LOG_EXIT("\n");

}

GMainContext* otvdbus_controller_get_context(OtvdbusController *self)
{
	OtvdbusControllerPrivate *priv;

	O_LOG_ENTER("\n");

	O_DEBUG_ASSERT( NTVDBUS_IS_CONTROLLER(self) == TRUE);
	priv = NTVDBUS_CONTROLLER_GET_PRIVATE (self);

	O_LOG_EXIT("\n");
	return priv->pContext;
}


GMainLoop* otvdbus_controller_get_mainloop(OtvdbusController *self)
{
	OtvdbusControllerPrivate *priv;

	O_LOG_ENTER("\n");

	O_DEBUG_ASSERT( NTVDBUS_IS_CONTROLLER(self) == TRUE);
	priv = NTVDBUS_CONTROLLER_GET_PRIVATE (self);

	O_LOG_EXIT("\n");
	return priv->pMainLoop;
}
/** @}  */
