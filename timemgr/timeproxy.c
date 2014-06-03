 * ============================================================================
 */

/**
 * @file     timemgr_proxy.c
 * @author   Lianghe Li
 * @date     Feb 15, 2011
 */

/**
 * @brief    Time manager proxy library
 * @ingroup  TIMEMGR
 * This file contains code to be linked into every time-client process and gives
 * a simple program to simulate a time client for test.
 * @{
 */
#include <pthread.h>
#include <glib.h>
#include <glib-object.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <ntvlog.h>
#include <sys/prctl.h>
#include "timeproxy.h"
// #include "timemgr_ipc_client.h"

#define TIMELIB_DBG(fmt, arg...) O_LOG_DEBUG("[timemgr proxy][pid=%d]"fmt, s_timeproxy_cookie.pid, ##arg)
#define TIMELIB_TRC(fmt, arg...) O_LOG_TRACE("[timemgr proxy][pid=%d]"fmt, s_timeproxy_cookie.pid, ##arg)
#define TIMELIB_ERR(fmt, arg...) O_LOG_ERROR("[timemgr proxy][pid=%d]"fmt, s_timeproxy_cookie.pid, ##arg)

/********************************************************************/
/*                        Static Members                            */
/********************************************************************/
static const char* g_time_event_names[TIMEMGR_EVENT_ENDDEF] =
{
    "timeChanged",          /* SIGNAL_NAME_TIME_CHANGED */
    "timeZoneChanged",      /* SIGNAL_NAME_TIME_ZONE_CHANGED */
    "daylightSavingsOn",    /* SIGNAL_NAME_DAYLIGHT_SAVINGS_ON */
    "daylightSavingsOff"    /* SIGNAL_NAME_DAYLIGHT_SAVINGS_OFF */
};

typedef struct
{
    timeproxy_event_handler_t callback;
    void * cookie;
}timeproxy_user_data_t;

#define MAX_TIMEPROXY_USER          16

typedef struct
{
    int                     pid;
    char                    proc_name[32];
    DBusGConnection *       conn;
    DBusGProxy *            proxy;
    int                     signal_connected;
    timeproxy_user_data_t   user_data[MAX_TIMEPROXY_USER];

}timeproxy_cookie_t;

/**
 * Singleton instance for each time client process
 */
static pthread_mutex_t          s_mutex = PTHREAD_MUTEX_INITIALIZER;
static int                      s_timeproxy_initialized = 0;
static timeproxy_cookie_t       s_timeproxy_cookie;

/**
 * @brief   Allocate a user data from cookie structure
 *
 * @param   fctCallback [in]   Callback function pointer
 * @param   pCookie     [in]   Cookie with callback function
 *
 * @retval  Valid user data pointer      Success
 * @return  NULL                         Failure
 *
 * @pre     None
 * @post    None
 *
 */
static timeproxy_user_data_t * timeproxy_alloc_user_data(
    timeproxy_event_handler_t fctCallback,
    void* pCookie )
{
    int i = 0;
    timeproxy_cookie_t * cookie = &s_timeproxy_cookie;
    timeproxy_user_data_t * user_data = NULL;

    if(fctCallback)
    {
        /* Look if the fct-cookie pair already exist */
        for(i=0;i<MAX_TIMEPROXY_USER;i++)
        {
            if( fctCallback==cookie->user_data[i].callback &&
                pCookie==cookie->user_data[i].cookie )
            {
                user_data = &cookie->user_data[i];
                TIMELIB_ERR("User register same callback and cookie multiple times!\n");
                break;
            }
        }
        if(NULL==user_data)
        {
            /* Find an unused user_data */
            for(i=0;i<MAX_TIMEPROXY_USER;i++)
            {
                if(NULL==cookie->user_data[i].callback)
                {
                    user_data = &cookie->user_data[i];
                    user_data->callback = fctCallback;
                    user_data->cookie = pCookie;
                    break;
                }
            }
        }
    }

    if(NULL==user_data)
    {
        TIMELIB_ERR("Failed to allocate user data!\n");
    }
    return user_data;
}

/**
 * @brief   Callback user registerred callback functions
 *
 * @param   cookie [in]   cookie pointer
 * @param   event [in]    time event to send
 *
 * @retval  NONE
 *
 * @pre     None
 * @post    None
 *
 */
static void timeproxy_event_callback(timeproxy_cookie_t * cookie, timemgr_event_t event)
{
    int i = 0;
    timeproxy_user_data_t * user_data = NULL;
    int cnt = 0;

    if(cookie && event>=0 && event<TIMEMGR_EVENT_ENDDEF)
    {
        for(i=0;i<MAX_TIMEPROXY_USER;i++)
        {
            user_data = &cookie->user_data[i];
            if(user_data->callback)
            {
                user_data->callback(event, user_data->cookie);
                cnt++;
            }
        }
    }
    TIMELIB_TRC("%d user registerred callback function were called!\n", cnt);
}

/**
 * @brief   Event handler for time updater object
 *
 * @param   timemgr [in]   Object on DBus
 * @param   time_event [in]     Time event posted by time manager
 * @param   userData [in]       User data comes with the event
 *
 * @retval  0     Success
 * @return  non-0 Failure
 *
 * @pre     None
 * @post    None
 *
 * @par Description
 * Handle DBus event, just call user registered callback function
 */
static void timemgr_signal_handler(
    DBusGProxy* timemgr,
    timemgr_event_t event,
    gpointer userData)
{
    timeproxy_cookie_t * cookie = (timeproxy_cookie_t *)userData;
    timeproxy_cookie_t * cookie_s = &s_timeproxy_cookie;

    ENSURE( cookie == cookie_s);

    if(cookie != cookie_s)
    {
        TIMELIB_ERR("Invalid cookie pointer %08x!\n", (uint32_t)cookie);
        return;
    }

    if(event>=0 && event<TIMEMGR_EVENT_ENDDEF)
    {
        TIMELIB_TRC("Timemgr event %s(%d) was received!\n", g_time_event_names[event], event);
    }
    else
    {
        TIMELIB_ERR("Invalid event (%d) was received!\n", event);
        return;
    }

    /* Ensure the proxy havn't been destroyed! */
    pthread_mutex_lock(&s_mutex);
    if(!s_timeproxy_initialized)
    {
        TIMELIB_DBG("Time proxy has been destroyed!");
        pthread_mutex_unlock(&s_mutex);
        return;
    }
    pthread_mutex_unlock(&s_mutex);

    timeproxy_event_callback(cookie, event);
}

/********************************************************************/
/*                        Public Function                           */
/********************************************************************/

/**
 * @brief   Initialisation for the time manager proxy
 *
 * @param[in] pGMainCtxt : Pointer to GMainLoop context
 * @retval  0 indicates success
 * @retval  -1 indicates error
 *
 * @pre  None
 * @pre  Protected against multiple initialisation
 * @post Initialisation is launched
 *
 * @par Description
 * Initialises the time manager proxy, creates any internal resources, and establishes the
 * connection to the time manager server. If the context pointer is non-NULL then it will be used.
 * If it is NULL, then the default context will be used. If the pointer is NULL and the default
 * context cannot be used then the proxy will return failure. It will \b NOT create it's own
 * GMainLoop context.
 */
int timeproxy_init( GMainContext* pGMainCtxt )
{
    int ret = -1;
    DBusError derror;
    DBusConnection *dbus_conn;
    int             i;
    timeproxy_cookie_t * cookie = &s_timeproxy_cookie;

    pthread_mutex_lock(&s_mutex);
    if(!s_timeproxy_initialized)
    {
        memset(cookie, 0, sizeof(timeproxy_cookie_t));

        cookie->pid = getpid();
        prctl(PR_GET_NAME, cookie->proc_name);
        TIMELIB_DBG("Process name: %s\n", cookie->proc_name);

        // g_type_init();

        /* A fake open call to initialize the default marshallers */
        dbus_g_connection_open("", NULL);

        dbus_error_init(&derror);
        dbus_conn = dbus_connection_open(TIMEMGR_ADDRESS, &derror);
        if (dbus_conn)
        {
            dbus_connection_setup_with_g_main(dbus_conn, pGMainCtxt);
            cookie->conn = dbus_connection_get_g_connection(dbus_conn);
        }

        else
        {
            if ( dbus_error_is_set( &derror ) )
            {
                TIMELIB_ERR( "Cannot open connection for %s: %s", TIMEMGR_ADDRESS, derror.message );
                dbus_error_free(&derror);
            }
            cookie->conn = NULL;
        }

        if (cookie->conn)
        {
            cookie->proxy = dbus_g_proxy_new_for_peer (cookie->conn, TIMEMGR_PATH, TIMEMGR_INTERFACE);
            if(cookie->proxy)
            {
                for(i=0;i<TIMEMGR_EVENT_ENDDEF;i++)
                {
                    /* signal setup */
                    dbus_g_proxy_add_signal(
                        cookie->proxy,
                        g_time_event_names[i],
                        G_TYPE_UINT,
                        G_TYPE_INVALID);
                }
                TIMELIB_TRC("Succeed to create timemgr proxy!\n");
                ret = 0;
                s_timeproxy_initialized = 1;
            }
            else
            {
                TIMELIB_ERR("Failed to create timemgr proxy!\n");
            }
        }
    }
    else
    {
        TIMELIB_ERR("Timeproxy has already be initialized!\n");
        ret = 0;
    }
    pthread_mutex_unlock(&s_mutex);
    return ret;
}

/**
 * @brief   Shutdown for the time manager manager proxy
 *
 * @retval  0 indicates success
 * @retval  -1 indicates error
 *
 * @pre     None
 * @post    All resources are released
 *
 * @par Description
 * Cleanup for forced process shutdown, releases any resources. TIMEPROXY is
 * protected against multiple exit.
 */
int timeproxy_exit( void )
{
    int ret = -1;
    timeproxy_cookie_t * cookie = &s_timeproxy_cookie;

    TIMELIB_TRC("timeproxy_exit() was called!\n");

    pthread_mutex_lock(&s_mutex);
    if(s_timeproxy_initialized)
    {
        g_object_unref(cookie->proxy);
        cookie->proxy = NULL;

        dbus_g_connection_unref(cookie->conn);
        cookie->conn = NULL;

        memset(cookie->user_data, 0, MAX_TIMEPROXY_USER * sizeof(timeproxy_user_data_t));
        cookie->signal_connected = 0;

        s_timeproxy_initialized = 0;

        ret = 0;
    }
    else
    {
        TIMELIB_ERR("Call timeproxy_init() first!\n");
    }
    pthread_mutex_unlock(&s_mutex);
    return ret;
}


/**
 * @brief  Register for time events.
 *
 * @param[in] fctCallback: Event callback function
 * @param[in] pCookie    : Unique cookie
 *
 * @retval      0       SUCCESS
 * @retval      non-0   FAILURE
 *
 * @pre  None
 * @post None
 *
 * @par Description
 * This call allows a user to register for time events. Once registered, the user will receive
 * all time events. There is no need to register for individual events as they are infrequent.
 * The cookie allows multiple users in the same process to register unique callbacks.
 */
int timeproxy_register_for_events(
    timeproxy_event_handler_t fctCallback,
    void*                     pCookie )

{
    int ret = -1;
    int i = 0;
    timeproxy_cookie_t * cookie = &s_timeproxy_cookie;
    timeproxy_user_data_t * user_data = NULL;

    TIMELIB_TRC("timeproxy_register_for_events(fctCallback=%08x, pCookie=%08x) was called!\n",
        (uint32_t)fctCallback, (uint32_t)pCookie);

    if(NULL==fctCallback)
    {
        TIMELIB_ERR("Invalid event handler!\n");
        return ret;
    }

    pthread_mutex_lock(&s_mutex);
    if(!s_timeproxy_initialized)
    {
        TIMELIB_ERR("Call timeproxy_init() first!\n");
    }
    else
    {
        user_data = timeproxy_alloc_user_data(fctCallback, pCookie);
        if(user_data)
        {
            user_data->callback = fctCallback;
            user_data->cookie = pCookie;

            if(!cookie->signal_connected)
            {
                for(i=0;i<TIMEMGR_EVENT_ENDDEF;i++)
                {
                    dbus_g_proxy_connect_signal(
                        cookie->proxy,
                        g_time_event_names[i],
                        G_CALLBACK(timemgr_signal_handler),
                        cookie,
                        NULL);
                }
                cookie->signal_connected = 1;
            }
            ret = 0;
        }
    }
    pthread_mutex_unlock(&s_mutex);
    return ret;
}

/**
 * @brief       Query if current system time is valid
 *
 * @retval true  System time is valid
 * @retval false System time is not yet valid
 *
 * @pre  Proxy is intitialised
 * @post None
 *
 * @par Description
 * Query time manager if current system time is valid.
 */
_Bool timeproxy_is_time_valid( void )
{
    return true;
}

/********************************************************************/
/*                                TEST                              */
/********************************************************************/

#ifdef BUILD_TIME_CLIENT_PROGRAM

/**
 * Time changed handler for test. It call o_is_time_valid() once
 * received time changed event. This demostrate sychornized RPC.
 */
static void time_event_handler(timemgr_event_t event, void * cookie)
{
    _Bool valid = 0;
    time_t t = (time_t)0;

    valid = timeproxy_is_time_valid();
    t = time(NULL);;

    TIMELIB_DBG("Received time Event %d [%s]. %s system time is %s",
        event,
        g_time_event_names[event],
        valid ? "Valid" : "Invalid",
        asctime(gmtime(&t)));
}

static gboolean timeout_callback(gpointer data)
{
    struct tm tm_local;
    memset(&tm_local, 0, sizeof(struct tm));
    time_t t = time(NULL);
    localtime_r(&t, &tm_local);
    TIMELIB_DBG("DST:%d at: %s", tm_local.tm_isdst, asctime(&tm_local));
    return TRUE;
}

#include "timesrc_app_ipc_client.h"
static int timeclient_test_app_time_source(int offset)
{
    DBusError derror;
    DBusConnection *        dbus_conn = NULL;
    DBusGConnection *       conn = NULL;
    DBusGProxy *            proxy = NULL;
    char *                  obj_path = APP_TIME_SOURCE_PATH;
    char *                  obj_interface = APP_TIME_SOURCE_INTERFACE;
    char *                  obj_address = APP_TIME_SOURCE_ADDRESS;

    time_t                  local_utc, remote_utc;
    GError *                error = NULL;

    g_type_init();

    /* A fake open call to initialize the default marshallers */
    dbus_g_connection_open("", NULL);

    dbus_error_init(&derror);
    dbus_conn = dbus_connection_open(obj_address, &derror);
    if (NULL==dbus_conn)
    {
        if ( dbus_error_is_set( &derror ) )
        {
            TIMELIB_ERR( "Cannot open connection for %s: %s", obj_address, derror.message );
            dbus_error_free(&derror);
        }
        return -1;
    }

    dbus_connection_setup_with_g_main(dbus_conn, NULL);
    conn = dbus_connection_get_g_connection(dbus_conn);
    if (NULL==conn)
    {
        TIMELIB_ERR("Failed to get g_connection!\n");
        return -1;
    }

    proxy = dbus_g_proxy_new_for_peer (conn, obj_path, obj_interface);
    if(NULL==proxy)
    {
        TIMELIB_ERR("Failed to create proxy!\n");
        return -1;
    }

    local_utc = time(NULL);
    remote_utc = local_utc + offset;
    TIMELIB_DBG("Calling com_nemotv_AppTimeSource_set_time_data()...\n");
    com_nemotv_AppTimeSource_set_time_data(
        proxy,
        (gint)remote_utc,
        (gint)local_utc,
        &error);
    TIMELIB_DBG("com_nemotv_AppTimeSource_set_time_data() returned!\n");
    if(error)
    {
        TIMELIB_ERR("Failed to call setTimeData(). Error message: %s\n", error->message);
        g_clear_error(&error);
        return -1;
    }

    TIMELIB_DBG("Succeed to call setTimeData()!\n");
    return 0;
}

/**
 * Test program main.
 */
int main(int argc, char** argv)
{
    int ret = -1;
    _Bool valid = FALSE;
    GMainLoop* mainloop = NULL;

    g_type_init();

    if(argc>=2 && 0==strcasecmp(argv[1], "monitor"))
    {
        if(0==timeproxy_init(NULL))
        {
            mainloop = g_main_loop_new(NULL, FALSE);
            ret = timeproxy_register_for_events(time_event_handler, NULL);
            if(ret)
            {
                TIMELIB_ERR("Failed to register time events handler!\n");
                return ret;
            }
            else
            {
                TIMELIB_DBG("Succeed to register time events handler!\n");
            }

            g_main_loop_run(mainloop);
            g_main_loop_unref(mainloop);
            return 0;
        }
    }
    else if(argc>=2 && 0==strcasecmp(argv[1], "check"))
    {
        if(0==timeproxy_init(NULL))
        {
            valid = timeproxy_is_time_valid();
            TIMELIB_DBG("Current time is %s!\n", valid?"valid":"not valid");
            return 0;
        }
    }
	else if(argc>=2 && 0==strcasecmp(argv[1], "dst"))
    {
        if(0==timeproxy_init(NULL))
        {
            mainloop = g_main_loop_new(NULL, FALSE);
            g_timeout_add_seconds(1, (GSourceFunc)timeout_callback, NULL);
            g_main_loop_run(mainloop);
            g_main_loop_unref(mainloop);
            return 0;
        }
    }
    else if(argc>2 && 0==strcasecmp(argv[1], "add") )
    {
        return timeclient_test_app_time_source(atoi(argv[2]));
    }
    else
    {
        TIMELIB_DBG("Usage: %s\n", argv[0]);
        TIMELIB_DBG("       %s monitor      :  Monitor time events.\n", argv[0]);
        TIMELIB_DBG("       %s check        :  Check if current system time is valid.\n", argv[0]);
        TIMELIB_DBG("       %s dst          :  Monitor the DST status.\n", argv[0]);
        TIMELIB_DBG("       %s add offset   :  Adjust system time by add current time with offset.\n", argv[0]);
        return -1;
    }

    return -1;
}

#endif
