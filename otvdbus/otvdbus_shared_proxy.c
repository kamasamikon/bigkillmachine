 * ============================================================================
 */
/**
 * @file     otvdbus_shared_proxy.c
 * @author   NemoTV
 * @date     Mar 21, 2011
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#include "otvdbus_shared_proxy.h"
#include <ntvlog.h>

typedef struct otvdbus_conn  otvdbus_shared_conn_t;
typedef struct otvdbus_proxy otv_shared_proxy_t;

struct otvdbus_proxy {
    DBusGProxy*                 proxy;       /*!< Proxy Object of a Server object which is running in a different process. */
    sem_t*                      mutex;       /*!< Mutex to protect the DBus connection from multiple threads concurent access. */
    void*                       userdata;    /*!< User data that needs to be passed back to the connection lost callback */
    otvdbus_conn_disconnect_cbk callback;    /*!< Callback to be called upon the connection to the server lost */
    otvdbus_shared_conn_t*      shared_conn; /*!< back pointer to the shared connection object on which the shared proxy lies. */
    otv_shared_proxy_t*         next;        /*!< Points to the next element in the list if any. */
    otv_shared_proxy_t*         prev;        /*!< Points to the previous element in list if any. */
};

struct otvdbus_conn {
    DBusGConnection*        conn;             /*!< DBus connection pointer to a remote DBus server. */
    sem_t                   mutex;            /*!< Mutex to protect the DBus connection from multiple threads. */
    char*                   direct_dbus_addr; /*!< DBus Server address at which the connection is been made. */
    char*                   object_path;      /*!< DBus Proxy Client object path. */
    char*                   interface_name;   /*!< DBus Proxy Client interface name. */
    otv_shared_proxy_t*     shared_proxy;     /*!< Points to the next element in the list if any. */
    otvdbus_shared_conn_t*  next;             /*!< Points to the next element in the list if any. */
    otvdbus_shared_conn_t*  prev;             /*!< Points to the previous element in list if any. */
};

static sem_t                   otvdbus_shared_mutex;
static otvdbus_shared_conn_t*  otvdbus_shared_conn_root     = NULL;
static bool                    otvdbus_shared_proxy_init    = false;
static int                     otvdbus_shared_proxy_count   = 0;
static GMainContext*           otvdbus_shared_proxy_context = NULL;
static GMainLoop*              otvdbus_shared_proxy_loop    = NULL;
static GThread*                otvdbus_shared_proxy_thrd_id = NULL;


static void* otvdbus_shared_proxy_library_main(void* unused);

static DBusHandlerResult
otvdbus_shared_proxy_connection_filter_cbk(DBusConnection* connection,
                                           DBusMessage*    message,
                                           void*           data);


/** Removes the sharead proxy object from the list */
static inline void
otvdbus_shared_proxy_remove_from_list(otvdbus_shared_conn_t* shared_conn, otv_shared_proxy_t* shared_proxy)
{
    assert(shared_proxy->prev);

    if (shared_proxy->next)  /* Not the last in the list */
    {
        shared_proxy->next->prev = shared_proxy->prev;
    }
    else
    {
        shared_conn->shared_proxy->prev = shared_proxy->prev;
    }

    /* Here shared_conn->shared_proxy is the root for the shared proxy list */
    if (shared_conn->shared_proxy != shared_proxy)  /* Not the first in the list */
    {
        shared_proxy->prev->next = shared_proxy->next;
    }
    else
    {
        shared_conn->shared_proxy = shared_proxy->next;
    }

    shared_proxy->next = shared_proxy->prev = NULL;
}

/** Removes the passed entry from the available list */
static inline void
otvdbus_shared_conn_remove_from_list(otvdbus_shared_conn_t* shared_conn)
{
    if (shared_conn->next)  /* Not the last in the list */
    {
        shared_conn->next->prev = shared_conn->prev;
    }
    else
    {
        otvdbus_shared_conn_root->prev = shared_conn->prev;
    }

    if (otvdbus_shared_conn_root != shared_conn)  /* Not the first in the list */
    {
        shared_conn->prev->next = shared_conn->next;
    }
    else
    {
        otvdbus_shared_conn_root = shared_conn->next;
    }

    shared_conn->next = shared_conn->prev = NULL;
}

static inline void
otvdbus_shared_conn_delete(otvdbus_shared_conn_t* shared_conn)
{
    sem_destroy(&shared_conn->mutex);
    free(shared_conn->direct_dbus_addr);
    free(shared_conn->object_path);
    free(shared_conn->interface_name);
    free(shared_conn);
}

static void
otvdbus_shared_proxy_delete_all()
{
    otvdbus_shared_conn_t* shared_conn;
    otv_shared_proxy_t*    shared_proxy;

    sem_wait(&otvdbus_shared_mutex);
    while(otvdbus_shared_conn_root)
    {
        shared_conn = otvdbus_shared_conn_root;
        otvdbus_shared_conn_remove_from_list(shared_conn);

        /* Now remove all the shared proxies on this connection */
        while(shared_conn->shared_proxy)
        {
            shared_proxy = shared_conn->shared_proxy;

            /* Remove the sharead proxy object from the list */
            otvdbus_shared_proxy_remove_from_list(shared_conn, shared_proxy);
            free(shared_proxy);
        }

        /* We freed all the shared proxies and now free the shared conenction */
        dbus_g_connection_flush(shared_conn->conn);
        dbus_g_connection_unref(shared_conn->conn);

        otvdbus_shared_conn_delete(shared_conn);
    }
    sem_post(&otvdbus_shared_mutex);

    sem_destroy(&otvdbus_shared_mutex);
}

static void*
otvdbus_shared_proxy_library_main(void* unused)
{
    otvdbus_shared_proxy_context = g_main_context_new();
    if (!otvdbus_shared_proxy_context)
    {
        O_LOG_DEBUG("ClientSharedLib> otvdbus_shared_proxy_library_main: Failed to create a GMainContext\n" );
        return NULL;
    }
    otvdbus_shared_proxy_loop = g_main_loop_new(otvdbus_shared_proxy_context, FALSE);
    if (!otvdbus_shared_proxy_loop)
    {
        g_main_context_unref(otvdbus_shared_proxy_context);
        O_LOG_DEBUG("ClientSharedLib> otvdbus_shared_proxy_library_main: Failed to create a GMainLoop\n" );
        return NULL;
    }

    O_LOG_DEBUG("ClientSharedLib> otvdbus_shared_proxy_library_main: Entering Glib main loop\n");
    g_main_loop_run(otvdbus_shared_proxy_loop);

    g_main_context_unref(otvdbus_shared_proxy_context);
    g_main_loop_unref(otvdbus_shared_proxy_loop);

    return 0;
}

int
otvdbus_shared_proxy_library_init(void)
{
    /* GError* error = NULL; SRK: Not required with new glib */

    if (otvdbus_shared_proxy_init)
    {
		otvdbus_shared_proxy_count++;
        return NTVDBUS_SHARED_PROXY_SUCCESS;
    }

    if (0 != sem_init(&otvdbus_shared_mutex, 0, 1))
    {
        O_LOG_DEBUG("ClientSharedLib> sem_init() FAILED for connection otvdbus_shared_mutex creation!!!\n");
        return NTVDBUS_SHARED_PROXY_FAILURE;
    }

    /* g_type_init(); - SRK: Deprecated since 2.36 */ 
    dbus_threads_init_default();
    /* g_thread_init(NULL); - SRK: Deprecated since 2.32 */

    /* otvdbus_shared_proxy_thrd_id = g_thread_create(otvdbus_shared_proxy_library_main, NULL, FALSE, &error); - SRK: Deprecated */
    otvdbus_shared_proxy_thrd_id = g_thread_new("otvdbus_shared_proxy_library_main", otvdbus_shared_proxy_library_main, NULL);
    if (!otvdbus_shared_proxy_thrd_id)
    {
        O_LOG_DEBUG("ClientSharedLib> g_thread_create() FAILED for otvdbus_shared_proxy_library_main()\n");
        /* g_error_free(error);  - SRK: Not required with new method */
        return NTVDBUS_SHARED_PROXY_FAILURE;
    }
    O_LOG_DEBUG("ClientSharedLib> otvdbus_shared_proxy_library_main() gthread created\n");

    otvdbus_shared_proxy_init = true;
    otvdbus_shared_proxy_count++;

    while (!g_main_loop_is_running(otvdbus_shared_proxy_loop));

    return NTVDBUS_SHARED_PROXY_SUCCESS;
}

void
otvdbus_shared_proxy_delete(otvdbus_proxy_t* proxy)
{
    otv_shared_proxy_t* shared_proxy;

    if (!otvdbus_shared_proxy_init)
    {
        /* Nothing should be there */
        assert(otvdbus_shared_proxy_init);
        return;
    }

    assert(proxy);
    shared_proxy = (otv_shared_proxy_t*)proxy;

    /* Check that the user might have called this from a connection lost callback notification.
     * In this case shared_proxy->prev && shared_proxy->next must be pointing to a NULL.
     */
    if ((shared_proxy->prev == NULL) && (shared_proxy->next == NULL))
    {
        /* Free the associated resources if any now */
        free(shared_proxy);
    }
    else
    {
        otvdbus_shared_conn_t* shared_conn;

        /* Remove it from the list and then free it */
        sem_wait(&otvdbus_shared_mutex);

        shared_conn = shared_proxy->shared_conn;

        /* Remove the sharead proxy object from the list */
        otvdbus_shared_proxy_remove_from_list(shared_proxy->shared_conn, shared_proxy);
        if (!shared_conn->shared_proxy)
        {
            /* Remove the connection object from the list */
            otvdbus_shared_conn_remove_from_list(shared_conn);

            /* Means all the proxies got freed, so free the shared connection object too */
            otvdbus_shared_conn_delete(shared_conn);
        }
        free(shared_proxy);

        sem_post(&otvdbus_shared_mutex);
    }
}

void
otvdbus_shared_proxy_library_exit(void)
{
    if (otvdbus_shared_proxy_init && ((--otvdbus_shared_proxy_count) == 0))
    {
        g_main_loop_quit(otvdbus_shared_proxy_loop); 

        otvdbus_shared_proxy_delete_all();
    }
}

static DBusHandlerResult
otvdbus_shared_proxy_connection_filter_cbk(DBusConnection* connection,
                                           DBusMessage*    message,
                                           void*           data)
{
    if (dbus_message_is_signal(message, DBUS_INTERFACE_LOCAL, "Disconnected"))
    {
        otvdbus_shared_conn_t* shared_conn;
        otv_shared_proxy_t*    shared_proxy;

        O_LOG_DEBUG("ClientSharedLib> Connection disconnected message received.\n");

        assert(data);

        sem_wait(&otvdbus_shared_mutex);
        shared_conn = (otvdbus_shared_conn_t*)data;
        assert(shared_conn->shared_proxy);

        while(shared_conn->shared_proxy)
        {
            shared_proxy = shared_conn->shared_proxy;

            /* Remove the sharead proxy object from the list */
            otvdbus_shared_proxy_remove_from_list(shared_conn, shared_proxy);

            assert(shared_proxy->callback);

            O_LOG_DEBUG("ClientSharedLib> Calling disconnect callback for %p\n", shared_proxy);

            sem_post(&otvdbus_shared_mutex); /* Release the mutex before calling callback */
            shared_proxy->callback((otvdbus_proxy_t*)shared_proxy, shared_proxy->userdata);
            sem_wait(&otvdbus_shared_mutex); /* Reequire the mutex */
        }

        /* Remove the connection object from the list */
        otvdbus_shared_conn_remove_from_list(shared_conn);

        /* Now free the resources */
        otvdbus_shared_conn_delete(shared_conn);
 
        sem_post(&otvdbus_shared_mutex);

        return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

otvdbus_proxy_t*
otvdbus_shared_proxy_create(char*                       direct_dbus_addr,
                            char*                       object_path,
                            char*                       interface_name,
                            otvdbus_conn_disconnect_cbk callback,
                            void*                       userdata)
{
    otvdbus_shared_conn_t* shared_conn;
    otv_shared_proxy_t*    shared_proxy;
    GError*                error = NULL;
    int                    len;
    DBusConnection*        dbus_conn;

    if (!direct_dbus_addr || !object_path || !interface_name || !callback)
    {
        O_LOG_DEBUG("ClientSharedLib> One or more invalid input arguments: direct_dbus_addr=%s, object_path=%s, interface_name=%s\n",
                direct_dbus_addr, object_path, interface_name);
        return NULL;
    }

    if (!otvdbus_shared_proxy_init)
    {
        O_LOG_DEBUG("ClientSharedLib> NTVDBUS-SHARED-LIBRARY not initialized.\n");
        return NULL;
    }

    shared_proxy = (otv_shared_proxy_t*)malloc(sizeof(otv_shared_proxy_t));
    if (!shared_proxy)
    {
        O_LOG_DEBUG("ClientSharedLib> malloc() FAILED!!! while creating a Shared Proxy\n");
        return NULL;
    }
    memset(shared_proxy, 0, sizeof(otv_shared_proxy_t));

    sem_wait(&otvdbus_shared_mutex);
    shared_conn = otvdbus_shared_conn_root;
    while(shared_conn)
    {
        /* If the is already a connection exists, then make use it */ 
        if (!strcmp(shared_conn->direct_dbus_addr, direct_dbus_addr) && !strcmp(shared_conn->object_path, object_path))
        {
            assert(shared_conn->shared_proxy);

            O_LOG_DEBUG("ClientSharedLib> Creating a new proxy from the existing conn(%p) and proxy(%p)\n",
                    shared_conn->conn, shared_conn->shared_proxy->proxy);

            shared_proxy->proxy = dbus_g_proxy_new_from_proxy(shared_conn->shared_proxy->proxy, interface_name, object_path);
            if (!shared_proxy->proxy)
            {
                O_LOG_DEBUG("ClientSharedLib> dbus_g_proxy_new_from_proxy() FAILED!!! while creating new proxy object\n");
                free(shared_proxy);
                sem_post(&otvdbus_shared_mutex);
                return NULL;
            }
            O_LOG_DEBUG("ClientSharedLib> Remote obj %p created\n", shared_proxy->proxy);

            shared_proxy->mutex       = &shared_conn->mutex;
            shared_proxy->userdata    = userdata;
            shared_proxy->callback    = callback;
            shared_proxy->next        = NULL;
            shared_proxy->shared_conn = shared_conn;

            /* Add this new shared_proxy at the end */
            shared_conn->shared_proxy->prev->next = shared_proxy;
            shared_proxy->prev                    = shared_conn->shared_proxy->prev;
            shared_conn->shared_proxy->prev       = shared_proxy;

            sem_post(&otvdbus_shared_mutex);

            return (otvdbus_proxy_t*)shared_proxy;
        }
        shared_conn = shared_conn->next;
    }

    /* We reach here means, there is no connection exists to the server and of course no proxy exits */
    assert(!shared_conn);

    /* Allocate Memory for new shared connection */
    shared_conn = (otvdbus_shared_conn_t*)malloc(sizeof(otvdbus_shared_conn_t));
    if (!shared_conn)
    {
        O_LOG_DEBUG("ClientSharedLib> malloc() FAILED!!! while creating a Shared Connection\n");
        return NULL;
    }
    memset(shared_conn, 0, sizeof(otvdbus_shared_conn_t));

    len = strlen(direct_dbus_addr);
    shared_conn->direct_dbus_addr = (char*) malloc(len + 1);
    if (!shared_conn->direct_dbus_addr)
    {
        O_LOG_DEBUG("ClientSharedLib> malloc() FAILED while allocating memory for direct_dbus_addr\n");
        free(shared_conn);
        free(shared_proxy);
        sem_post(&otvdbus_shared_mutex);
        return NULL;
    }
    strncpy(shared_conn->direct_dbus_addr, direct_dbus_addr, len);
    shared_conn->direct_dbus_addr[len] = '\0';

    len = strlen(object_path);
    shared_conn->object_path = (char*) malloc(len + 1);
    if (!shared_conn->object_path)
    {
        O_LOG_DEBUG("ClientSharedLib> malloc() FAILED while allocating memory for object_path\n");
        free(shared_conn->direct_dbus_addr);
        free(shared_conn);
        free(shared_proxy);
        sem_post(&otvdbus_shared_mutex);
        return NULL;
    }
    strncpy(shared_conn->object_path, object_path, len);
    shared_conn->object_path[len] = '\0';

    len = strlen(interface_name);
    shared_conn->interface_name = (char*) malloc(len + 1);
    if (!shared_conn->interface_name)
    {
        O_LOG_DEBUG("ClientSharedLib> malloc() FAILED while allocating memory for interface_name\n");
        free(shared_conn->object_path);
        free(shared_conn->direct_dbus_addr);
        free(shared_conn);
        free(shared_proxy);
        sem_post(&otvdbus_shared_mutex);
        return NULL;
    }
    strncpy(shared_conn->interface_name, interface_name, len);
    shared_conn->interface_name[len] = '\0';

    memset(&shared_conn->mutex, 0, sizeof(sem_t));
    if (0 != sem_init(&shared_conn->mutex, 0, 1))
    {
        O_LOG_DEBUG("ClientSharedLib> sem_init() FAILED for connection mutex creation!!!\n");
        free(shared_conn->interface_name);
        free(shared_conn->object_path);
        free(shared_conn->direct_dbus_addr);
        free(shared_conn);
        free(shared_proxy);
        sem_post(&otvdbus_shared_mutex);
        return NULL;
    }

    O_LOG_DEBUG("ClientSharedLib> Creating a Connection at Address %s\n", direct_dbus_addr);
    shared_conn->conn = dbus_g_connection_open(direct_dbus_addr, &error);
    if (!shared_conn->conn)
    {
        O_LOG_DEBUG("ClientSharedLib> Unable to open direct address: %s\n", error->message);
        g_error_free(error);
        otvdbus_shared_conn_delete(shared_conn);
        free(shared_proxy);
        sem_post(&otvdbus_shared_mutex);
        return NULL;
    }

    dbus_conn = dbus_g_connection_get_connection(shared_conn->conn);
    if (!dbus_conn)
    {
        O_LOG_DEBUG("ClientSharedLib> dbus_g_connection_get_connection() FAILED!! while getting the DBus Connection Object\n");
        dbus_g_connection_unref(shared_conn->conn);
        otvdbus_shared_conn_delete(shared_conn);
        free(shared_proxy);
        sem_post(&otvdbus_shared_mutex);
        return NULL;
    }

    assert(otvdbus_shared_proxy_context);
    dbus_connection_setup_with_g_main(dbus_conn, otvdbus_shared_proxy_context);

    if (!dbus_connection_add_filter(dbus_conn, otvdbus_shared_proxy_connection_filter_cbk, shared_conn, NULL))
    {
        O_LOG_DEBUG("ClientSharedLib> dbus_connection_add_filter() FAILED!! while adding the Connection lost callback\n");
        dbus_g_connection_unref(shared_conn->conn);
        otvdbus_shared_conn_delete(shared_conn);
        free(shared_proxy);
        sem_post(&otvdbus_shared_mutex);
        return NULL;
    }

    O_LOG_DEBUG("ClientSharedLib> Creating a proxy object on connection %p\n", shared_conn->conn);
    shared_proxy->proxy = dbus_g_proxy_new_for_peer(shared_conn->conn, object_path, interface_name);
    if (!shared_proxy->proxy)
    {
        O_LOG_DEBUG("ClientSharedLib> Error creating proxy object\n");
        dbus_g_connection_unref(shared_conn->conn);
        otvdbus_shared_conn_delete(shared_conn);
        free(shared_proxy);
        sem_post(&otvdbus_shared_mutex);
        return NULL;
    }
    O_LOG_DEBUG("ClientSharedLib> Remote obj %p created\n", shared_proxy->proxy);

    shared_proxy->mutex       = &shared_conn->mutex;
    shared_proxy->userdata    = userdata;
    shared_proxy->callback    = callback;
    shared_proxy->next        = NULL;
    shared_proxy->shared_conn = shared_conn;

    shared_proxy->prev        = shared_proxy; /* Prev is pointing to self as it is the first element */
    shared_conn->shared_proxy = shared_proxy; /* Store the shared client as a first element in shared connection object */

    /* Add this shared connection to the list */
    shared_conn->next = NULL;
    if (!otvdbus_shared_conn_root)
    {
        /* This is the first connection element */
        shared_conn->prev        = shared_conn;  /* Prev is pointing to self as it is the first element */
        otvdbus_shared_conn_root = shared_conn;
    }
    else
    {
        /* Add this new shared_conn at the end */
        otvdbus_shared_conn_root->prev->next = shared_conn;
        shared_conn->prev                    = otvdbus_shared_conn_root->prev;
        otvdbus_shared_conn_root->prev       = shared_conn;
    }

    sem_post(&otvdbus_shared_mutex);

    return (otvdbus_proxy_t*)shared_proxy;
}

