 * ============================================================================
 */
/**
 * @file     otvdbus_shared_proxy.h
 * @author   NemoTV
 * @date     Mar 21, 2011
 */
#ifndef __NTVDBUS_SHARED_PROXY_H__
#define __NTVDBUS_SHARED_PROXY_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

#include <glib.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus.h>

/**
 * @defgroup SHARED_PROXY_CLIENT NemoTV Shared DBus Proxy Client Library
 * @ingroup NTVDBUS
 * @brief Provides a framework to share a DBus Proxy Client Object across multiple threads within a client process.
 * @par Description
 * This is a DBus client side library that provides you the following.
 *  - Creates a separate GThread, GMainContext and GMainLoop for processing the signals and asynchronous method responses.
 *    Means all the signals and async responses will be executed from this GThread.
 *  - Creates a separate proxy object for each request to a server on the same underlying DBUs connection. Means all the
 *    clients will share the DBus connection irrespective of number of clients.
 *  - Protects the DBus connection from concurrent access by multiple threads.
 * @par Attention
 *  - This is implemented and tested for the components that uses only the DBus clients without DBus server in its processes.
 *  - This is not tested for the components that uses both the DBus clients and the DBus servers as well. If this is the case,
 *    this library needs to be tested and updated if necessary.
 */

/**
 * @ingroup SHARED_PROXY_CLIENT 
 * @brief Context structure required to share a proxy object across multiple threads.
 */ 
typedef struct {
    DBusGProxy* proxy; /*!< Proxy Object of a Server object which is running in a different process. */
    sem_t*      mutex; /*!< Mutex to protect the DBus connection from multiple threads concurrent access. */
}   otvdbus_proxy_t;

/**
 * @ingroup SHARED_PROXY_CLIENT 
 * @brief Status codes returned by NTV DBus Shared Proxy library APIs.
 */ 
typedef enum {
    NTVDBUS_SHARED_PROXY_FAILURE = -1, /*!< Specifies that the requested operation was FAILED. */
    NTVDBUS_SHARED_PROXY_SUCCESS = 0   /*!< Specifies that the requested operation was SUCCESS. */
}   otvdbus_shared_proxy_status_t;

/**
 * @ingroup SHARED_PROXY_CLIENT 
 * @brief Signature of the connection lost notification Callback.
 *
 * @param [in] shared_proxy Shared Proxy object that was created using otvdbus_shared_proxy_create().
 * @param [in] user_data    This was the user specific data that was passed to otvdbus_shared_proxy_create().
 *
 * @par Description
 * This is the prototype of the callback that will be notified by library upon the underlying DBus connection lost to the server.
 * All the interested clients needs to pass an address of a routine of this type along with any user specific data.
 * This is basically to free any resources that are binded to this proxy object upon the connection to the server object is lost.
 */
typedef void (*otvdbus_conn_disconnect_cbk)(otvdbus_proxy_t* shared_proxy, void* user_data);

/**
 * @ingroup SHARED_PROXY_CLIENT 
 * @brief Callback signature to be used for all asynchronous method executions.
 *
 * @param [in] proxy       Represents a proxy object on which the methods is being executed. DBus will passe this proxy object when this call back is called.
 * @param  [in] call_cookie Represents the callback cookie that was returned when the corresponding async method invoked. DBus will passe this callback cookie
 *                         when this call back is called.
 * @param [in] user_data   This was the user specific data that was passed while executing the corresponding async method.
 *
 * @par Description
 * A pointer of type otvdbus_shared_proxy_async_cbk() should be passed to all asynchronous methods while calling it. The definition of this
 * callback must call NTVDBUS_SHARED_PROXY_METHOD_ASYNC_FINISH() with proper out parameters based on the actual method signature on server.\n
 * When a response of an asynchronous method is available, the GMainLoop running at client side will call this callback and the callback is
 * responsible to read the output data that is available at GMainLoop by passing the address of all the output parameters to 
 * NTVDBUS_SHARED_PROXY_METHOD_ASYNC_FINISH().
 */
typedef void (*otvdbus_shared_proxy_async_cbk)(DBusGProxy* proxy, DBusGProxyCall* call_cookie, void* user_data);

/**
 * @ingroup SHARED_PROXY_CLIENT 
 * @brief Callback signature to be used to free any user specific data that is passed while invoking an asynchronous method.
 *
 * @param [in] user_data  This was the user specific data that was passed while invoking the corresponding async method.
 *
 * @par Description
 * A pointer of this type may be passed to all asynchronous methods to free any user specific data that is passed to a async method and would
 * required to be freed by DBus after the method receives response and finished the execution.
 * @par Attention
 *  - The callback should not contain any synchronous primitive that makes the callback wait for an event or a signal.
 *  - The callback should not invoke either DBus async or DBus sync method through any proxy object.
 */
typedef void (*otvdbus_shared_proxy_free_cbk)(gpointer user_data);

/**
 * @ingroup SHARED_PROXY_CLIENT 
 * @brief Initilizes the infra structure required by a shared proxy library.
 *
 * @par Description
 * It initializes the shread-proxy-library when called. This must be called at least once before start creating the proxy objects
 * that needs to be shared across multiple threads. It creates a GThread and runs GMainLoop inside this newly created thread for
 * processing the signals and asynchronous method responses as well.\n
 * Inorder to have clean exit one must call otvdbus_shared_proxy_library_close().
 */
int otvdbus_shared_proxy_library_init(void);

/**
 * @ingroup SHARED_PROXY_CLIENT 
 * @brief Cleans up the resources that are allocated by the Shared-Proxy library.
 *
 * @par Description
 * It cleans up all the resources that are allocated by the Shared-Proxy library since its initialization.
 */
void otvdbus_shared_proxy_library_exit(void);

/**
 * @ingroup SHARED_PROXY_CLIENT 
 * @brief Creates a proxy object to a remote server object.
 *
 * @param [in] direct_dbus_addr  DBus Server direct address to which the DBus connection to be established to create a proxy object.
 * @param [in] object_path       Path to the server object to which the client proxy to be created.
 * @param [in] interface_name    Name of the interface that needs to be part of proxy object.
 * @param [in] disconn_callback  Underlaying DBus-connection disconnect notification callback.
 * @param [in] userdata          User specific data that needs to be passed back to @a disconn_callback.
 *
 * @retval On FAILURE it returns a NULL pointer.
 * @retval On SUCCESS it returns a valid pointer to a shared proxy object.
 *
 * @par Description
 * It Creates a client proxy object to a remote server object that is running at the given direct DBus Server object. If there is 
 * alrady a proxy client object exists, then it creates new proxy object from the existing proxy client object, so that there will be
 * a single DBus connection and a single server object at any point of time though here all the clients get different proxy object.
 * Means, each client thread that wants to share a single DBus connection and single server object needs to create a separate proxy
 * object using this API. Also if each thread wants to receive signals, they need to perform the following separately.
 *  - Need to register all the signal Marshalers for those client is looking for notifications.
 *  - Need to add the signals for those client is looking for notifications.
 *  - Need to connect all the signals for those client is looking for notifications.
 */
otvdbus_proxy_t* otvdbus_shared_proxy_create(char* direct_dbus_addr, char* object_path, char* interface_name,
                                                otvdbus_conn_disconnect_cbk disconn_callback, void* userdata);

/**
 * @ingroup SHARED_PROXY_CLIENT 
 * @brief It frees the resources associated with the given proxy object.
 *
 * @param [in] shared_proxy Specifies the proxy object that was created using otvdbus_shared_proxy_create().
 *
 * @par Description
 * It frees the resources that are associated with the proxy object. It also closes the DBus connection if the object being freed is
 * the last object on a DBus connection.
 */
void otvdbus_shared_proxy_delete(otvdbus_proxy_t* shared_proxy);

/**
 * @ingroup SHARED_PROXY_CLIENT 
 * @brief It executes a synchronous method of a server object through a proxy client.
 *
 * @param [in] result        On return it contains true if the method was SUCCESS or otherwise false.
 * @param [in] shared_proxy  Shared Proxy object that was created using otvdbus_shared_proxy_create().
 * @param [in] method        A synchronous method that needs to be executed.
 * @param [in] ...           Specifies the variable number of arguments that are vary method to method depends on method being executed.
 *
 * @retval On FAILURE the @a result will be having a false (0). 
 * @retval On SUCCESS the @a result will be having a true (1).
 *
 * @par Description
 * It executes a synchronous method of a server object through a proxy client as like an ordinary function execution. This needs to 
 * called with all the input and output parameters.
 * @par Note
 * See the DBus specifications how to construct the variable number of arguments based on the method signature.
 */
#define NTVDBUS_SHARED_PROXY_METHOD_SYNC_CALL(result, shared_proxy, method, ...) \
do                                                                               \
{                                                                                \
    sem_wait(shared_proxy->mutex);                                               \
    result = dbus_g_proxy_call(shared_proxy->proxy, method, __VA_ARGS__);        \
    sem_post(shared_proxy->mutex);                                               \
} while(0)

/**
 * @ingroup SHARED_PROXY_CLIENT 
 * @brief It executes an asynchronous method of a server object through a proxy client.
 *
 * @param [in] shared_proxy  Proxy object that was created using otvdbus_shared_proxy_create().
 * @param [in] method        An asynchronous method that needs to be executed.
 * @param [in] callback      A callback routine that gets called when a response is available at client side by GMainLoop.
 * @param [in] userdata      Any user specific data that needs to passed back to the @a callback when a response is available at client side by GMainLoop. This can be a NULL.
 * @param [in] data_free_cbk A callback routine that needs to called to free the user data after the response is processed. This can be a NULL.
 *
 * @retval On FAILURE it returns a NULL pointer.
 * @retval On SUCCESS it returns a pointer to a cookie that will be also passed back to the @a callback after the response is ready.
 *
 * @par Description
 * It executes an synchronous method on the server object through proxy client and returns without waiting for the response. When the 
 * response is available, the @a callback routine will gets called by GMainLoop and then the user must read the
 * output paramters if any by calling NTVDBUS_SHARED_PROXY_METHOD_ASYNC_FINISH() with proper arguments. For additional details see 
 * @ref otvdbus_shared_proxy_async_cbk.\n
 * If the user passes any user-specific data, the same user-specifi data will be passed back to the @a callback.
 * The user can also pass another call back function to free the user-specific data if needed.
 */
#define NTVDBUS_SHARED_PROXY_METHOD_ASYNC_CALL(call_cookie, shared_proxy, method, callback, userdata, data_free_cbk)       \
do                                                                                                                         \
{                                                                                                                          \
    sem_wait(shared_proxy->mutex);                                                                                         \
    call_cookie = dbus_g_proxy_begin_call(shared_proxy->proxy, method, callback, userdata, data_free_cbk, G_TYPE_INVALID); \
    sem_post(shared_proxy->mutex);                                                                                         \
}   while(0)

/**
 * @ingroup SHARED_PROXY_CLIENT 
 * @brief It must be called for each asynchronous method from the callback routine.
 *
 * @param [in] proxy       Proxy object that is passed to the callback routine by GMainLoop.
 * @param [in] call_cookie Call cookie that is passed to the callback routine by GMailLoop.
 * @param [in] error_p     Pointer to a GError variable that must be assigned to a NULL.
 * @param [in] ...         Specifies the variable number of arguments that are vary method to method depends on method being executed.
 *
 * @retval On FAILURE the GError points to a valid error that specifies the error. 
 * @retval On SUCCESS the GError points to a NULL and all the out parameters will be pointing to the actual data that are meant for.
 *
 * @par Description
 * It must be called with all the output parameters for each asynchronous method from the callback routine that was passed
 * to @ref otvdbus_shared_proxy_call_async_method.
 * @par Note
 * See the DBus specifications how to construct the variable number of arguments based on the method signature.
 */
#define NTVDBUS_SHARED_PROXY_METHOD_ASYNC_FINISH(proxy, call_cookie, error_p, ...)  dbus_g_proxy_end_call(proxy, call_cookie, &error, __VA_ARGS__);

/**
 * @ingroup SHARED_PROXY_CLIENT 
 * @brief It registers the signal marshaler with a client proxy object.
 *
 * @param [in] marshaller Specifies the user defined marshaler of a signal that required notifications through a proxy object.
 * @param [in] ...        Specifies the variable number of arguments that are vary signal to signal depends on the signal being executed.
 *
 * @par Description
 * It registers the signal marshaler function with the G Object, so that the GObject can understand the exact syntax of the signal payload.
 * The signal marshaler function needs to be registered only once.
 * @par Note
 * See the DBus specifications how to construct the variable number of arguments based on the signal signature.
 */
#define NTVDBUS_SHARED_PROXY_GOBJ_REG_SIGNAL_MARSHALLER(marshaller, ...) dbus_g_object_register_marshaller(marshaller, __VA_ARGS__);

/**
 * @ingroup SHARED_PROXY_CLIENT 
 * @brief It adds the signal with a client proxy object.
 *
 * @param [in] shared_proxy Shared Proxy object that was created using otvdbus_shared_proxy_create().
 * @param [in] sig_name     Name of a signal for which the proxy object should enable of receing the notifications.
 * @param [in] first_type   Specifies the first argument type, or G_TYPE_INVALID if none.
 * @param [in] ...          Specifies the variable number of arguments that are vary method to method depends on method being executed.
 *
 * @par Description
 * It enables the given signal, so that the registered signal callback will get called by GMainLoop when a server object emit the corresponding
 * signal. If the user does not add the signal by using this macro, then the user will not receive the signal. Only once the signal needs to added.
 * @par Note
 * See the DBus specifications how to construct the variable number of arguments based on the signal signature.
 */
#define NTVDBUS_SHARED_PROXY_ADD_SIGNAL(shared_proxy, sig_name, first_type, ...) dbus_g_proxy_add_signal(shared_proxy->proxy, sig_name, first_type, __VA_ARGS__);

/**
 * @ingroup SHARED_PROXY_CLIENT 
 * @brief It executes a synchronous method of a server object through a proxy client.
 *
 * @param [in] shared_proxy   Shared Proxy object that was created using otvdbus_shared_proxy_create().
 * @param [in] sig_name       The DBus signal name to listen for.
 * @param [in] handler        The handler to connect to the specified @a signal.
 * @param [in] userdata       Specifies the data to pass to handler.
 * @param [in] free_data_func Specifies the callback function to destroy data.
 *
 * @par Description
 * Connect a signal handler to a proxy for a remote interface. When the remote interface emits the specified signal, the proxy will emit a
 * corresponding GLib signal.
 */
#define NTVDBUS_SHARED_PROXY_CONNECT_SIGNAL(shared_proxy, sig_name, handler, userdata, free_data_func) dbus_g_proxy_connect_signal(shared_proxy->proxy, sig_name, handler, userdata, free_data_func);

/**
 * @ingroup SHARED_PROXY_CLIENT 
 * @brief It executes a synchronous method of a server object through a proxy client.
 *
 * @param [in] shared_proxy Shared Proxy object that was created using otvdbus_shared_proxy_create().
 * @param [in] sig_name     The DBus signal name to disconnect.
 * @param [in] handler      The handler to disconnect.
 * @param [in] userdata     Specifies the data that was registered with handler using NTVDBUS_SHARED_PROXY_CONNECT_SIGNAL.
 *
 * @par Description
 * Disconnect all signal handlers from a proxy that match the given criteria.
 */
#define NTVDBUS_SHARED_PROXY_DISCONNECT_SIGNAL(shared_proxy, sig_name, handler, userdata) dbus_g_proxy_disconnect_signal(shared_proxy->proxy, sig_name, handler, userdata);

#endif   /* __NTVDBUS_SHARED_PROXY_H__ */
