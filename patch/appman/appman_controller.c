
#include <config.h>
#include "outils.h"

#include "appman_enums.h"
#include "appman_errors.h"
#include "appman_util.h"
#include "appman_applicationx.h"
#include "appman_config.h"
#include "appman_policy.h"
#include "appman_controller.h"
#include "appman_dbus_service.h"
#include "appman_rte.h"
#include "appman_application.h"
#include "appman-marshal.h"
#ifndef WINMAN_DISABLED
#include "winman_dbus_service.h"
#include "winman_dbus_internal_service.h"
#include "windowmgrx.h"
#endif
#include <ntvlog.h>
#include <app_permission_gen.h>

#include "pcdapi.h"
// For LPM
#include "appman_lpm.h"
#include "pwrproxy.h"


/**
 * \file appman_controller.c
 *
 * \author Rohan
 * \date   12/10/2010
 * \ingroup AppManController
 */
/**@{*/

/* include other impl specific header files */

#define RECONNECT_DELAY	3 /* seconds */
/** Signal emission enumeration from Appman controller */
enum
{
    APPMAN_CONTROLLER_AIM_STATE_CHANGED_SIG, /*!< Notify AIM connected/disconnected signal */
    APPMAN_CONTROLLER_APP_LAUNCHED_SIG,     /*!< App launched signal */
    APPMAN_CONTROLLER_APP_DESTROYED_SIG,    /*!< App destroyed signal */
    APPMAN_CONTROLLER_DESTROYED_ALL_SIG,    /*!< All apps destroyed signal */
    LAST_SIGNAL,
};

/** State of Appman controller */
typedef enum
{
    APPMAN_CONTROLLER_INITIALING,       /*!< initializing state */
    APPMAN_CONTROLLER_ACTIVE,           /*!< active state, ready to accept requests */
    APPMAN_CONTROLLER_SUSPENDING_APPS,  /*!< in the process of suspending Apps */
    APPMAN_CONTROLLER_FINALIZING        /*!< going to exit doing some cleanup*/

}appman_controller_state_t;

/**
 * Appman Controller Private Data
 * @internal
 */
struct _AppmanControllerPrivate
{

    GHashTable* pRunningApplications;	/*!< Hashtable of Running Apps based on pid*/
    GQueue* pWaitingApplications;	/*!< Queue of Application contexts waiting to launch*/
    GHashTable* pWaitingConnections;
    GPtrArray       *pCalls;		/*!< Pointer to hold pending calls */

    AppmanConfig* pAppmanConfigObject;	/*!< Reference to Config Object */
    AppmanPolicy* pAppmanPolicyObject;	/*!< Reference to Policy object */

    DBusGConnection* pAimConnection;	/*!< Reference to Aim DBus Connection */
    DBusGProxy* pAimProxy;              /*!< Reference to Aim Proxy */

    DBusGConnection* pConfManConnection;    /*!< Reference to ConfMan DBus Connection */
    DBusGProxy* pConfManProxy;              /*!< Reference to ConfMan Proxy */

    gint aim_reconnect_timeout_id;	/*!< GSource Id to hold the Aim reconnect timeout callback */
    gint confman_reconnect_timeout_id;      /*!< GSource Id to hold the Aim reconnect timeout callback */
#ifndef WINMAN_DISABLED
    WindowManager *pWindowManager;	/*!< Reference to Window Manager object */
#endif
    appman_controller_state_t State;	/*!< State of Appman Controller */
    /* TODO: Consolidate into 1*/
    gboolean bLaunchedOnBootApps;	/*!< Boolean to hold if this instance of appman is launched from boot */
    gboolean bLaunchedSuspendedApps;	/*!< Boolean to hold if this instance of appman is launched from suspend */
    gboolean bLaunchPending;		/*!< Boolean to hold if any Application Launches are pending */

    AppmanLpmInfo *pLpmInfo;            /*!< Holds LPM related information */
    AppmanApplication *pControlApplication;	/*!< Pointer to hold the Current Control Application */

    GMainLoop *pMainLoop;					/*! the main loop */
    GMutex    *pMutex;						/*! mutex used to protect the running application hash table. this table can be
     	 	 	 	 	 	 	 	 	 	    accessed by main thread and winman/sawman thread. */

};
/**
 * Result for Aim Query
 */
typedef struct
{
    GPtrArray          *pResultArray;	/*!< Result Array */
}AimResultSet;

/**
 * Launch Context Info used while launching Application
 */
typedef struct
{
    o_appman_launch_reason_t launch_reason;	/*!< launch reason */
    GSimpleAsyncResult  *pAsyncresult;	    	/*!< Async result pointer */
    AppmanController    *controller;	    	/*!< Appman controller */
    gboolean            ret;			/*!< Result used at various check points while launching App */
    DBusGProxyCall      *call;			/*!< Dbus proxy call */
    AimResultSet       ResultSet;		/*!< Aim Result Set */
    AppmanApplication  *pApplication;		/*!< Application Object */

} AppmanLaunchContext;

/**
 *  Appman Destroy Context Info
 */
typedef struct
{
    o_appman_exit_reason_t destroy_reason;	/*!< Destroy reason */
    GSimpleAsyncResult  *pAsyncresult;		/*!< Async result pointer */
    AppmanController    *controller;		/*!< Appman controller */
    gboolean            ret;			/*!< Result used at various check points while destroying App */
    AppmanApplication  *pApplication;		/*!< Application Object */

} AppmanAppDestroyContext;

/**
 * Appman SuspendAll Context Info
 */
typedef struct
{
    o_appman_exit_reason_t destroy_reason;	/*!< suspend all reason */
    GSimpleAsyncResult  *pAsyncresult;		/*!< Async result pointer */
    AppmanController    *controller;		/*!< Appman controller */
    gboolean            ret;				/*!< Result used at various check points*/
    gboolean			saveAppContext;		/*!< TRUE - save the application context after suspending apps.
     	 	 	 	 	 	 	 	 	 	 	 FALSE - don't save the application context */

} AppmanSuspendAllContext;

/**
 * Appman Setcontrol helper Info
 */
typedef struct
{
    GSimpleAsyncResult  *pAsyncresult;		/*!< Async result pointer */
    AppmanController    *controller; 		/*!< Appman controller */
    gchar* param_type;				/*!< App Param type , currently only appPid or UUID */
    GValue* param_data;				/*!< App param value for the type, currently only appPid , or UUID */

} AppmanControlHelperData;

/**
 * Appman LPM Context Info
 */
typedef struct {
        GSimpleAsyncResult *pAsyncresult; /*!< Async result pointer */
        AppmanController *controller; /*!< Appman controller */
        gssize maxDelay;  /*!< store the maximum of all app's delay time*/
        gssize runningApps; /*!< Used to keep track of app responses */
        gint timeout_id; /* timer for all LPM */
} AppmanLpmAllContext_t;

#define APPMAN_CONTROLLER_GET_PRIVATE(o)      (((AppmanController *)(o))->_priv)
/* globals */
static OtvdbusControllerClass *s_parent_class = NULL;
static guint    s_controller_signals[LAST_SIGNAL] = { 0};
static AppmanController *s_appman_controller;

static void     appman_controller_class_init (AppmanControllerClass *klass);
static void     appman_controller_init       (AppmanController *obj);

static void     _appman_controller_finalize   (GObject *obj);
static void     _appman_controller_dispose   (GObject *obj);
static void     _appman_controller_get_required_services   (AppmanController *obj);

static gboolean _appman_controller_handle_new_connection(AppmanController *self , OtvdbusConnection* con);
static gboolean _appman_controller_handle_connection_lost(AppmanController *self ,  OtvdbusConnection* con);

//static gboolean _appman_controller_is_client_an_app(AppmanController* self, OtvdbusConnection *con, gint* pid,AppmanApplication** pApp);
static gboolean _appman_controller_is_app_running( AppmanController* self,  o_appman_app_info_type_t param_type,  GValue *data, AppmanApplication** pApp);
static gboolean _appman_controller_is_app_launchable(AppmanController* self,AppmanApplication* pApp);
static gboolean _appman_controller_is_aim_reconnect_reqd(AppmanController* self);
static gboolean _appman_controller_is_configman_reconnect_reqd(AppmanController* self);
static gint     _appman_controller_compare_with_waiting_to_launch(gconstpointer pContextQueue, gconstpointer pAimUuid);

static void     _appman_controller_aim_disconnected_cb(GObject* pObject, gpointer user_data);
static void     _appman_controller_confman_disconnected_cb(GObject* pObject, gpointer user_data);
static void     _appman_controller_app_launch_async_cb (GObject      *object,  GAsyncResult *result, gpointer      user_data);
static void     _appman_controller_aim_delete_signal_handler(DBusGProxy *proxy, gchar * uuid[], void * user_data);
static void     _appman_controller_suspend_helper (gpointer key, gpointer value, gpointer user_data);
static void     _appman_controller_suspend_async_cb (GObject      *object, GAsyncResult *result, gpointer      user_data);
static void     _appman_controller_on_boot_app_launch_complete_cb (GObject *object, GAsyncResult *result, gpointer user_data);
static void     _appman_controller_stopped_on_aim_delete_cb(GObject *object,  GAsyncResult *result,  gpointer  user_data);
static void     _appman_controller_aim_state_changed_handler(AppmanController* self , gint state);
static void     _appman_controller_remove_waiting_to_launch_app(gpointer data , gpointer user_data);
static void     _appman_controller_aim_query_finish (AppmanLaunchContext *state, GError *error);
static void     _appman_controller_app_state_changed(GObject* pApplication , gint previous_state,  gint current_state, gint reason, gpointer user_data);
// Forward Declaration for LPM related controller code
static gboolean appman_controller_lpm_all_applications_finish(
		AppmanController *self, GAsyncResult *result, GError **error);
static gssize appman_controller_lpm_finish(AppmanApplication *self,
		GAsyncResult *result, GError **error);
static void _appman_controller_lpm_async_cb(GObject *object,
		GAsyncResult *result, gpointer user_data);
static void _appman_controller_lpm_helper(gpointer key, gpointer value,
		gpointer user_data);


G_DEFINE_TYPE (AppmanController, appman_controller, NTVDBUS_TYPE_CONTROLLER);

/* callback function for on boot application launch is done */
static void _appman_controller_on_boot_app_launch_complete_cb (GObject      *object,
        GAsyncResult *result,
        gpointer      user_data)
{
    GError* error = NULL;
    O_LOG_ENTER("\n");

    g_value_unset((GValue*)user_data);
    g_slice_free(GValue, user_data);

    AppmanApplication * pApp = appman_controller_launch_app_finish ((AppmanController*)object, result, &error);
    if (error != NULL)
    {
        O_LOG_WARNING(" Unable to Launch On Boot Application: %s", error->message);
        g_error_free(error);
    }
    else
    {
        O_DEBUG_ASSERT( NULL != pApp);
    }

    if( 0 != PCD_api_send_process_ready() )
    {
        O_LOG_WARNING("PCD_api_send_process_ready() failed.\n");
    }

    O_LOG_EXIT("\n");
}
/* signal handler for aim state changed
 * for the first time aim is connected on boot/suspended apps are launched*/
static void _appman_controller_aim_state_changed_handler(AppmanController* self , gint state)
{
    O_LOG_ENTER("\n");

    if (state == APPMAN_CONTROLLER_AIM_CONNECTED_SIGNAL )
    {
        O_LOG_DEBUG(" Aim Connected");
        if (self->_priv->bLaunchedOnBootApps == FALSE || self->_priv->bLaunchedSuspendedApps == FALSE )
        {
            O_LOG_DEBUG(" Launching On Boot or Suspended  Applications");

            o_appman_launch_reason_t reason;
            GVariant* pApps = NULL;
            if (self->_priv->bLaunchedOnBootApps == FALSE)
            {
                pApps = appman_config_get_on_boot_apps(self->_priv->pAppmanConfigObject);
                reason = O_APPMAN_APP_LAUNCH_FROM_BOOT;
            }
            else
            {
                /* suspended apps */
                pApps = appman_config_get_suspended_apps(self->_priv->pAppmanConfigObject);
                reason = O_APPMAN_APP_LAUNCH_FROM_SUSPEND;
            }

            if (NULL != pApps)
            {
                O_LOG_DEBUG("  On Boot or Suspended  Applications Found ");
                GEnumClass *app_info_class =  g_type_class_ref(O_TYPE_APPMAN_INFO);

                GEnumValue *pEnum = g_enum_get_value_by_nick(app_info_class,APPMAN_UUID_STR);

                gint n, i;
                n = g_variant_n_children (pApps);
                for (i = 0; i < n; i++)
                {
                    gchar *uri;
                    GValue* param_value;
                    param_value = g_slice_new0(GValue);
                    O_DEBUG_ASSERT(NULL != param_value);

                    g_value_init(param_value, G_TYPE_STRING);

                    g_variant_get_child (pApps, i, "&s", &uri);
                    g_value_set_string(param_value,uri);

                    appman_controller_launch_app_async(self,
                            pEnum,
                            param_value,
                            reason ,
                            NULL,
                            (GAsyncReadyCallback)_appman_controller_on_boot_app_launch_complete_cb,
                            param_value);


                }
                g_variant_unref(pApps);

            }
            self->_priv->bLaunchedOnBootApps = TRUE;
            self->_priv->bLaunchedSuspendedApps = TRUE;

        }

    }
    else
    {
        O_LOG_DEBUG(" Aim DisConnected");

    }
    O_LOG_EXIT("\n");
}
/* controller class init function */
static void appman_controller_class_init (AppmanControllerClass *klass)
{
    GObjectClass *gobject_class;
    OtvdbusControllerClass *otvdbus_class;

    O_LOG_ENTER("\n");
    gobject_class = (GObjectClass*) klass;
    otvdbus_class = (OtvdbusControllerClass*) klass;

    s_parent_class            = g_type_class_peek_parent (klass);
    gobject_class->finalize = _appman_controller_finalize;
    gobject_class->dispose = _appman_controller_dispose;

    /* register signals */
    s_controller_signals[APPMAN_CONTROLLER_AIM_STATE_CHANGED_SIG] =
            g_signal_new ((APPMAN_CONTROLLER_AIM_STATE_SIG_STR),
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (AppmanControllerClass, aim_state_changed),
                    NULL, NULL,
                    g_cclosure_marshal_VOID__INT,
                    G_TYPE_NONE, 1,
                    G_TYPE_INT);

    s_controller_signals[APPMAN_CONTROLLER_APP_LAUNCHED_SIG] =
            g_signal_new ((APPMAN_CONTROLLER_APP_LAUNCHED_SIG_STR),
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST,
                    0,
                    NULL, NULL,
                    appman_marshal_VOID__OBJECT_INT,
                    G_TYPE_NONE,2,
                    G_TYPE_OBJECT, G_TYPE_INT);

    s_controller_signals[APPMAN_CONTROLLER_APP_DESTROYED_SIG] =
            g_signal_new ((APPMAN_CONTROLLER_APP_DESTROYED_SIG_STR),
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST,
                    0,
                    NULL, NULL,
                    appman_marshal_VOID__OBJECT_INT,
                    G_TYPE_NONE, 2,
                    G_TYPE_OBJECT, G_TYPE_INT);

    s_controller_signals[APPMAN_CONTROLLER_DESTROYED_ALL_SIG] =
            g_signal_new ((APPMAN_CONTROLLER_DESTROYED_ALL_SIG_STR),
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST,
                    0,
                    NULL, NULL,
                    g_cclosure_marshal_VOID__VOID,
                    G_TYPE_NONE,
                    0);


    klass->aim_state_changed = _appman_controller_aim_state_changed_handler;

    g_type_class_add_private (gobject_class, sizeof(AppmanControllerPrivate));



    /* overriding base class functions to handle new connection & connection lost */
    otvdbus_class->handle_new_connection = (OtvdbusControllerHandleNewConnFn) _appman_controller_handle_new_connection;
    otvdbus_class->handle_connection_lost = (OtvdbusControllerHandleLostConnFn)_appman_controller_handle_connection_lost;

    O_LOG_EXIT("\n");
}
/* controller object init function */
static void appman_controller_init (AppmanController *obj)
{
    AppmanControllerPrivate* priv  = NULL;

    obj->_priv =  G_TYPE_INSTANCE_GET_PRIVATE((obj), APPMAN_TYPE_CONTROLLER,AppmanControllerPrivate) ;

    priv = APPMAN_CONTROLLER_GET_PRIVATE(obj);
    /* initialize params */

    priv->pRunningApplications = g_hash_table_new_full (g_direct_hash, g_direct_equal,NULL,g_object_unref);
    priv->pAimProxy = NULL;
    priv->pAimConnection = NULL;
    priv->pConfManProxy = NULL;
    priv->pConfManConnection = NULL;
    priv->pAppmanConfigObject = NULL;
    priv->pAppmanPolicyObject = NULL;
    priv->pControlApplication = NULL;
    priv->aim_reconnect_timeout_id = 0;
    priv->confman_reconnect_timeout_id = 0;
    priv->pCalls = g_ptr_array_new ();
    priv->bLaunchedOnBootApps = FALSE;
    priv->bLaunchedSuspendedApps = FALSE;
    priv->bLaunchPending = FALSE;
    priv->pWaitingApplications = g_queue_new();
    priv->pWaitingConnections = g_hash_table_new_full (g_direct_hash, g_direct_equal,NULL,g_object_unref);
    priv->pMainLoop = NULL ;
    priv->pMutex =  g_mutex_new();

#ifndef WINMAN_DISABLED
    priv->pWindowManager = NULL;
#endif

    init_utils();

    O_LOG_EXIT("\n");
}
/* helper function to remove apps from waiting to launch queue and cleanup calling context */
static void _appman_controller_remove_waiting_to_launch_app(gpointer data , gpointer user_data)
{
    AppmanLaunchContext *context = (AppmanLaunchContext *)data;

    O_LOG_ENTER("\n");
    O_DEBUG_ASSERT(NULL != context->pApplication);
    if (context->pApplication != NULL)
    {
        g_object_unref(context->pApplication);
        context->pApplication = NULL;
    }
    if(G_IS_SIMPLE_ASYNC_RESULT(context->pAsyncresult))
    {
        g_simple_async_result_set_error(context->pAsyncresult,
                O_APPMAN_ERROR,
                O_APPMAN_ERROR_FAILED,
                "Application Manager Shutting Down");

        g_simple_async_result_complete(context->pAsyncresult);
        g_object_unref(context->pAsyncresult);
    }

    g_object_unref(context->controller);

    g_slice_free(AppmanLaunchContext, context );
    O_LOG_EXIT("\n");
}
/* helper function to remove apps from waiting to launch queue */
static void _appman_controller_remove_all_waiting_apps(AppmanController *self)
{

    O_LOG_ENTER("\n");
    if (NULL != self->_priv->pWaitingApplications)
    {
        g_queue_foreach(self->_priv->pWaitingApplications,
                _appman_controller_remove_waiting_to_launch_app,
                NULL);
        g_queue_free(self->_priv->pWaitingApplications);
        self->_priv->pWaitingApplications = NULL;
    }
    O_LOG_EXIT("\n");
}
/*
 * appman_controller_cancel_all_dbus_methods, cancel all the pending dbus methods while shutdown
 */
static gboolean
_appman_controller_cancel_all_dbus_methods (AppmanController *self)
{
    const AppmanLaunchContext *pPendingContext;
    guint i;
    O_LOG_ENTER("\n");
    if( NULL != self->_priv->pCalls)
    {
        GPtrArray *pArray;

        /* just cancel the call */
        pArray = self->_priv->pCalls;

        for (i=0; i<pArray->len; i++)
        {
            pPendingContext = g_ptr_array_index (pArray, i);
            O_DEBUG_ASSERT(pPendingContext != NULL);
            if (pPendingContext->call == NULL)
                continue;
            O_LOG_DEBUG ("cancel in query call");
            dbus_g_proxy_cancel_call (self->_priv->pAimProxy, pPendingContext->call);
        }
    }
    O_LOG_EXIT("\n");
    return TRUE;
}
/*
 * This function is called while starting Controller
 * Appman tries to connect to configuration manager.
 *
 * It waits till the Configuration Manager service is up and queries for the
 * required configuration Info.
 *  All the information is cached in the AppmanConfig Object.
 * [NOTE] The Confiman connection is no more required but kept for future requirements.
 *
 * Appman then tries to connect to AIM if in any case AIM is not available yet,
 * it still continues launching the Appman service but tries periodically to connect
 * to AIM if connection fails
 *
 * */
static void _appman_controller_get_required_services   (AppmanController *obj)
{
    AppmanControllerPrivate* priv  = APPMAN_CONTROLLER_GET_PRIVATE(obj);
    O_LOG_ENTER("\n");

    priv->pAppmanConfigObject = g_object_new(APPMAN_TYPE_CONFIG, NULL);
    O_DEBUG_ASSERT( NULL != priv->pAppmanConfigObject);

#ifndef CONFIGMAN_DISABLED
    if (_appman_controller_is_configman_reconnect_reqd((AppmanController*)obj) == FALSE)
    {
        O_LOG_DEBUG(" ConfMan Available ");
        O_DEBUG_ASSERT( NULL != priv->pConfManProxy);

    }
    else
    {
        /* ConfigMan is not available start a periodic timer callback to connect to Configman */
        priv->confman_reconnect_timeout_id = g_timeout_add_seconds (RECONNECT_DELAY,
                (GSourceFunc) _appman_controller_is_configman_reconnect_reqd,
                obj);
    }
    while( ((priv->State == APPMAN_CONTROLLER_INITIALING) && NULL == priv->pConfManProxy))
    {
        g_main_context_iteration(g_main_context_get_thread_default(),FALSE);
        O_LOG_DEBUG("Waiting for next iteration for ConfigMan\n");
    }
#endif

    if(priv->State == APPMAN_CONTROLLER_INITIALING)
    {

#ifndef CONFIGMAN_DISABLED
        if(FALSE == appman_config_initialize_data(priv->pAppmanConfigObject, priv->pConfManProxy))
        {
            O_DEBUG_ASSERT(0);
        }
#endif

        priv->pLpmInfo = g_slice_new0(AppmanLpmInfo);
        O_ASSERT(priv->pLpmInfo != NULL);
        if ( priv->pLpmInfo == NULL )
            O_LOG_ERROR("Failed to initialize lpminfo .\n");

        if (0 != appman_controller_lpm_init(obj))
        {
            O_ASSERT(0);
            O_LOG_ERROR("Unable to initialize LPM\n");
        }
        priv->State = APPMAN_CONTROLLER_ACTIVE;

        if (_appman_controller_is_aim_reconnect_reqd((AppmanController*)obj) == FALSE)
        {
            O_LOG_DEBUG(" AIM Available ");
            O_DEBUG_ASSERT( NULL != priv->pAimProxy);

        }
        else
        {
            priv->aim_reconnect_timeout_id = g_timeout_add_seconds (RECONNECT_DELAY,
                    (GSourceFunc) _appman_controller_is_aim_reconnect_reqd,
                    obj);
        }
    }
    O_LOG_EXIT("\n");
}
/* controller destructor function */
static void _appman_controller_dispose (GObject *obj)
{
    /* 	free/unref instance resources here */

    O_LOG_ENTER("\n");
    AppmanControllerPrivate* priv  = APPMAN_CONTROLLER_GET_PRIVATE(obj);

    priv->State = APPMAN_CONTROLLER_FINALIZING;

    _appman_controller_cancel_all_dbus_methods (APPMAN_CONTROLLER(obj));

    _appman_controller_remove_all_waiting_apps(APPMAN_CONTROLLER(obj));
    /* Stop emit of aim-state changed signals if any*/
    g_signal_stop_emission(obj, s_controller_signals[APPMAN_CONTROLLER_AIM_STATE_CHANGED_SIG],
            0);

    if(NULL != priv->pAimProxy)
    {
        g_signal_handlers_disconnect_by_func (priv->pAimProxy,
                G_CALLBACK(_appman_controller_aim_disconnected_cb),
                obj);
    }
    if (priv->aim_reconnect_timeout_id != 0)
    {
        O_LOG_DEBUG("  Releasing pending timeout id");
        g_source_remove (priv->aim_reconnect_timeout_id);
        priv->aim_reconnect_timeout_id = 0;
    }

    if (priv->confman_reconnect_timeout_id != 0)
    {
        O_LOG_DEBUG("  Releasing pending timeout id");
        g_source_remove (priv->confman_reconnect_timeout_id);
        priv->confman_reconnect_timeout_id = 0;

    }
    if(NULL != priv->pMainLoop)
    {
        g_main_loop_quit(priv->pMainLoop);
        g_main_loop_unref(priv->pMainLoop);
    }

    appman_controller_lpm_deinit( appman_controller_get_default() );

    G_OBJECT_CLASS(s_parent_class)->dispose (obj);
    O_LOG_EXIT("\n");
}
/* controller finalize function */
static void _appman_controller_finalize (GObject *obj)
{

    O_LOG_ENTER("\n");
    AppmanControllerPrivate* priv  = APPMAN_CONTROLLER_GET_PRIVATE(obj);
    /* 	free/unref instance resources here */
    if (NULL != priv->pAimProxy)
    {
        O_LOG_DEBUG("Appman Controller Finalize: Releasing AIM Proxy\n");
        g_object_unref(priv->pAimProxy);
        priv->pAimProxy = NULL;

    }
    if (NULL != priv->pAimConnection)
    {
        O_LOG_DEBUG("Releasing AIM Connection\n");
        /* TODO: dbus_g_connection_close(priv->pAimConnection); */
        dbus_g_connection_unref(priv->pAimConnection);
        priv->pAimConnection = NULL;
    }
    if (NULL != priv->pConfManProxy)
    {
        O_LOG_DEBUG("Appman Controller Finalize: Releasing Configman Proxy\n");
        g_object_unref(priv->pConfManProxy);
        priv->pConfManProxy = NULL;

    }
    if (NULL != priv->pConfManConnection)
    {
        O_LOG_DEBUG("Releasing Configman Connection\n");
        /* TODO: dbus_g_connection_close(priv->pConfManConnection);*/
        dbus_g_connection_unref(priv->pConfManConnection);
        priv->pConfManConnection = NULL;
    }

    if (NULL != priv->pAppmanConfigObject)
    {
        O_LOG_DEBUG("Releasing Appman Config Object\n");
        g_object_unref(priv->pAppmanConfigObject);
        priv->pAppmanConfigObject = NULL;
    }
    if (NULL != priv->pAppmanPolicyObject)
    {
        O_LOG_DEBUG("Releasing Appman Policy Object\n");
        g_object_unref(priv->pAppmanPolicyObject);
        priv->pAppmanPolicyObject = NULL;
    }


    if (NULL != priv->pRunningApplications)
    {
        O_LOG_DEBUG("Releasing Running Apps\n");
        g_hash_table_destroy(priv->pRunningApplications);
        priv->pRunningApplications = NULL;
    }
    if(NULL != priv->pMutex)
    {
    	g_mutex_free(priv->pMutex);
    	priv->pMutex = NULL;
    }

    if (NULL != priv->pWaitingConnections)
    {
        O_LOG_DEBUG("Releasing Connections Apps\n");
        g_hash_table_destroy(priv->pWaitingConnections);
        priv->pWaitingConnections = NULL;
    }

	if ( NULL != priv->pLpmInfo ) {
	    AppmanController *controller = appman_controller_get_default();
	    appman_controller_lpm_info_destroy (controller) ;
	}

#ifndef WINMAN_DISABLED
    if (priv->pWindowManager != NULL)
    {
        O_LOG_DEBUG("Releasing Window Manager\n");
        priv->pWindowManager->Release();
    }
#endif
    O_LOG_DEBUG("Releasing Pending Calls\n");
    g_ptr_array_unref (priv->pCalls);


    G_OBJECT_CLASS(s_parent_class)->finalize (obj);
    O_LOG_EXIT("\n");
}

gboolean appman_controller_start(AppmanController* self, int argc, char** argv)
{
    O_LOG_ENTER("\n");
    gboolean bResult = FALSE;

	/* initializes the system module */
	if( o_utils_init() )  {
		O_LOG_ERROR("Failed to initialize utils.\n");
		O_ASSERT(0);
		return bResult;
	}

    _appman_controller_get_required_services(self);

    /* initialize the otv security lib */
        /* initialize the otvsec lib */
        if( TRUE != otvsec_appman_init() )
        {
        	O_LOG_ERROR("NTV Permission Lib Init failed\n");
        	O_ASSERT(0);
        	return bResult;
        }

#ifndef WINMAN_DISABLED
    s_appman_controller->_priv->pWindowManager = WindowManagerCreate(argc, argv);
    O_DEBUG_ASSERT(NULL != s_appman_controller->_priv->pWindowManager);
#endif

    if(self->_priv->State == APPMAN_CONTROLLER_ACTIVE)
    {
        self->_priv->pAppmanPolicyObject = g_object_new(APPMAN_TYPE_POLICY, NULL);
        O_DEBUG_ASSERT( NULL != self->_priv->pAppmanPolicyObject);

        if( NULL != self->_priv->pAppmanConfigObject)
        {
            const gchar* pAddress ;
            AppmanServicesInfo* pServicesInfo = appman_config_get_services_info(self->_priv->pAppmanConfigObject);
        /*    otvdbus_controller_run(NTVDBUS_CONTROLLER(self),self->_priv->pMainLoop,pServicesInfo->pAppmanServerAddress);*/

            bResult =  otvdbus_controller_init_mainloop(NTVDBUS_CONTROLLER(self) , self->_priv->pMainLoop );

            pAddress =  otvdbus_controller_add_server(NTVDBUS_CONTROLLER(self),  pServicesInfo->pAppmanServerAddress);

            if(NULL != pAddress)
            {
                gchar** pEnv = (gchar**)g_new( gchar * , 2);
                gchar* pAppmanEnv =  g_strdup_printf ("%s=%s","APPMAN_ADDRESS",pAddress );

                pEnv[0] = pAppmanEnv;
                pEnv[1] = NULL;

#include "/home/auv/eoe/auv/bigkillmachine/patch/appman/set_nm_env_for_appman.snippet"

                appman_config_append_rte_env(self->_priv->pAppmanConfigObject, pEnv);

                g_free((gpointer)pAddress);
                g_strfreev(pEnv);

             }
        }
    }

    O_LOG_EXIT("\n");
    return bResult;

}

void appman_controller_run(AppmanController* self)
{
	otvdbus_controller_start(NTVDBUS_CONTROLLER(self));
}


void appman_controller_stop(AppmanController* self)
{
	if( self != NULL )
	{
		if( self->_priv->pMainLoop != NULL )
		{
			O_LOG_DEBUG("Stopping Appman\n");
			if (g_main_loop_is_running(self->_priv->pMainLoop) == TRUE)
			{
				g_main_loop_quit (self->_priv->pMainLoop);
			}
		}
	}
}


AppmanController* appman_controller_new (int argc, char** argv, GMainLoop* pLoop)
{
    O_LOG_ENTER("\n");

    O_DEBUG_ASSERT(NULL == s_appman_controller);

    if(NULL == s_appman_controller)
    {
        s_appman_controller =  APPMAN_CONTROLLER(g_object_new(APPMAN_TYPE_CONTROLLER, NULL));
        O_DEBUG_ASSERT( NULL != s_appman_controller);
    }
    s_appman_controller->_priv->pMainLoop = (pLoop)? pLoop : g_main_loop_new(NULL, FALSE);
    O_LOG_EXIT("\n");
    return s_appman_controller;
}
void appman_controller_destroy(void)
{
    O_LOG_ENTER("\n");
    O_DEBUG_ASSERT(NULL != s_appman_controller);
    if( NULL != s_appman_controller)
    {
        g_object_unref(s_appman_controller);
        s_appman_controller = NULL;
    }

    /* close the module */
	o_utils_exit();

    O_LOG_EXIT("\n");
}
AppmanController* appman_controller_get_default (void)
{
    O_DEBUG_ASSERT( NULL != s_appman_controller);
    return s_appman_controller;
}

/*
 * DBusConnection filter function, this function checks if the message is a Hello
 * and registers a application object
 *
 * Appman expects the App/RTE to send a "Hello" Message on the Appman Interface.
 * The App object which emits the GainControl, LostControl, SendMessageSignals
 */
static DBusHandlerResult _appman_connection_filter_func (DBusConnection *conn,
        DBusMessage *message,
        void *user_data)
{

    AppmanController *controller = appman_controller_get_default();
    AppmanServicesInfo* pServiceInfo = appman_config_get_services_info (controller->_priv->pAppmanConfigObject);

    if (dbus_message_is_method_call (message, pServiceInfo->pAppmanInterfacePath, pServiceInfo->pRegistrationMessage)) {

        AppmanDbusService* pAppmanServiceObj = APPMAN_DBUS_SERVICE(user_data);
        gulong pid;
        DBusMessage *reply;
        AppmanApplication* pApp = NULL;

        O_LOG_DEBUG("Appman controller:: Hello Message");
        O_DEBUG_ASSERT( TRUE == dbus_connection_get_is_authenticated(conn));
        /*
         * Get the process id and look if its an App Object
         */
        if( (dbus_connection_get_unix_process_id(conn , &pid) ) &&
                (   NULL != ( pApp = (AppmanApplication*)g_hash_table_lookup (controller->_priv->pWaitingConnections , GINT_TO_POINTER(pid)))) )
        {
            app_dbus_service_info_t dbus_info;
#ifndef WINMAN_DISABLED
            /* Registering the Winman Object */
            WinmanDbusServiceObject* pWinmanService = winman_dbus_service_new();
            pWinmanService->pConnection = pAppmanServiceObj->pConnection;
            O_DEBUG_ASSERT( NULL != pWinmanService);
            otvdbus_connection_register_object(pAppmanServiceObj->pConnection, pServiceInfo->pWinmanObjectPath,G_OBJECT(pWinmanService),controller);
            dbus_info.pWinmanDbusObject = pWinmanService;
#endif
            /* register application object */
            O_DEBUG_ASSERT( NULL != pApp);
            O_DEBUG_ASSERT(NULL != pAppmanServiceObj->pConnection);
            O_LOG_DEBUG("Appman controller:: registering Application Object <%p> on PID = %ld",pApp,pid);
            otvdbus_connection_register_object(pAppmanServiceObj->pConnection,  pServiceInfo->pApplicationObjectPath,G_OBJECT(g_object_ref(pApp)),controller);

            g_hash_table_remove(controller->_priv->pWaitingConnections,GINT_TO_POINTER(pid));
            appman_application_set_dbus_info(pApp, &dbus_info);

            /* make the reply message */
            reply = dbus_message_new_method_return (message);
            dbus_connection_send (conn, reply, NULL);

            appman_application_connected(pApp);
            pAppmanServiceObj->pApplication = pApp;
            g_object_add_weak_pointer(G_OBJECT(pApp) , (gpointer)&pAppmanServiceObj->pApplication);

        }
        else
        {
            O_LOG_WARNING(" \n received Hello Message from %ld and Appman is not waiting for it",pid);
            reply = dbus_message_new_error (message, DBUS_ERROR_FAILED, "");
            dbus_connection_send (conn, reply, NULL);
        }

        if (reply != NULL) {
            dbus_message_unref (reply);
        }

        return DBUS_HANDLER_RESULT_HANDLED;
    }

    O_LOG_TRACE("[PID=%d] Connection Message Path is %s",getpid(),dbus_message_get_path(message));

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

}
/* Overriden function to handle new connections
 * This function registers the appman, application, winman DBus objects */
static gboolean _appman_controller_handle_new_connection(AppmanController *self , OtvdbusConnection* con)
{
    AppmanControllerPrivate* priv  = APPMAN_CONTROLLER_GET_PRIVATE(self);
    O_LOG_ENTER("\n");


    AppmanServicesInfo* pServiceInfo = appman_config_get_services_info (priv->pAppmanConfigObject);
    AppmanDbusServiceObject* pAppmanService = appman_dbus_service_new();
#ifndef WINMAN_DISABLED
    WinmanDbusInternalServiceObject* pWinmanInternalService = winman_dbus_internal_service_new();
    pWinmanInternalService->pConnection = con;
#endif
    pAppmanService->pConnection = con;
    g_object_add_weak_pointer(G_OBJECT(con),(gpointer)&pAppmanService->pConnection);
    O_DEBUG_ASSERT(NULL != pAppmanService->pConnection);
    dbus_connection_add_filter (dbus_g_connection_get_connection( otvdbus_connection_get_connection(con)), _appman_connection_filter_func,pAppmanService , NULL);

    O_DEBUG_ASSERT( NULL != pAppmanService);

    otvdbus_connection_register_object(con, pServiceInfo->pAppmanObjectPath,G_OBJECT(pAppmanService),self);

#ifndef WINMAN_DISABLED
    O_DEBUG_ASSERT( NULL != pWinmanInternalService);
    otvdbus_connection_register_object(con, pServiceInfo->pWinmanInternalObjectPath, G_OBJECT(pWinmanInternalService), self);
#endif
    /* If the caller is an app */
    O_LOG_EXIT("\n");
    return TRUE;
}
/* Overriden function to handle lost connections
 * This function Unregisters the appman, application, winman DBus objects */
static gboolean _appman_controller_handle_connection_lost(AppmanController *self,  OtvdbusConnection* con)
{
    AppmanControllerPrivate* priv  = APPMAN_CONTROLLER_GET_PRIVATE(self);
    GObject* Object = NULL;
    AppmanServicesInfo* pServiceInfo = appman_config_get_services_info (priv->pAppmanConfigObject);
    O_LOG_ENTER("\n");

#ifndef WINMAN_DISABLED
    /* unregister the winman service object */
    if (TRUE ==  otvdbus_connection_unregister_object(con,   pServiceInfo->pWinmanObjectPath,&Object))
    {
        O_DEBUG_ASSERT( NULL != Object);

        g_object_unref(Object);
        Object = NULL;
    }
    /* unregister the winman internal object */
    if (TRUE == otvdbus_connection_unregister_object(con,   pServiceInfo->pWinmanInternalObjectPath,&Object))
    {
        O_DEBUG_ASSERT( NULL != Object);
        g_object_unref(Object);
        Object = NULL;
    }
#endif
    /* unregister the appman service object */
    if (TRUE ==  otvdbus_connection_unregister_object(con,   pServiceInfo->pAppmanObjectPath,&Object))
    {
        AppmanDbusService* pServiceObj = ((AppmanDbusService*)Object);

        O_DEBUG_ASSERT( NULL != Object);

        /* remove the weak reference pointer */
        g_object_remove_weak_pointer(G_OBJECT(con), (gpointer)&pServiceObj->pConnection);
        if(NULL != pServiceObj->pApplication)
        {
        	/* unregister the application object if present */
            if (TRUE ==  otvdbus_connection_unregister_object(con, pServiceInfo->pApplicationObjectPath, &Object))
            {
                O_DEBUG_ASSERT(G_OBJECT(pServiceObj->pApplication) == Object);
            }
            g_object_remove_weak_pointer(G_OBJECT(con), (gpointer)&pServiceObj->pApplication);

            /* unref the application object */
            g_object_unref(Object);
            Object = NULL;
        }

        /* remove the dbus connection filter function */
        dbus_connection_remove_filter (dbus_g_connection_get_connection( otvdbus_connection_get_connection(con)), _appman_connection_filter_func,pServiceObj);

        /* unref the appman object */
        g_object_unref(pServiceObj);
    }
    else
    {
        O_DEBUG_ASSERT(0);
    }

    O_LOG_EXIT("\n");
    return TRUE;
}
static void _appman_controller_return_app_launch(AppmanLaunchContext *pContext)
{
    O_DEBUG_ASSERT(NULL != pContext);

    g_simple_async_result_complete_in_idle (pContext->pAsyncresult);
    g_object_unref (pContext->pAsyncresult);
    g_object_unref (pContext->controller);
    g_slice_free (AppmanLaunchContext, pContext);

}
/* Helper function to handle the result of AIM Query
 * If Query is valid tries to continue launching App, else returns error */
static void _appman_controller_aim_query_finish (AppmanLaunchContext *pContext, GError *error)
{
    O_LOG_ENTER("\n");
    /* remove from list */
    g_ptr_array_remove (pContext->controller->_priv->pCalls, pContext);


    if (pContext->ret == TRUE )
    {
        GError *new_error = NULL;

        /* Check if the application with meta data is running */
        /*Check Running Apps list and Waiting Queue */

        pContext->pApplication = appman_application_new(pContext->ResultSet.pResultArray , &new_error);


        if (pContext->pApplication != NULL)
        {
            if ( (_appman_controller_is_app_launchable(pContext->controller,pContext->pApplication) == FALSE))
            {
                O_LOG_DEBUG("Trying to run an already Running Application\n");

                g_object_unref(pContext->pApplication);
                pContext->pApplication = NULL;

                g_simple_async_result_set_error        (pContext->pAsyncresult,
                        O_APPMAN_ERROR,
                        O_APPMAN_ERROR_FAILED,
                        "Application Running Already");

                _appman_controller_return_app_launch(pContext);
            }
            else
            {
                O_LOG_DEBUG("Adding Application to Launch Queue %p\n",pContext->pApplication);

                g_queue_push_tail(pContext->controller->_priv->pWaitingApplications, pContext);

                if (pContext->controller->_priv->bLaunchPending == FALSE)
                {
                    O_LOG_DEBUG("Connecting to Object %p for state changed signal\n",pContext->pApplication);
                    g_signal_connect(pContext->pApplication,APP_SIGNAL_STATE_CHANGED_STR,G_CALLBACK(_appman_controller_app_state_changed), pContext->controller);
                    pContext->controller->_priv->bLaunchPending = TRUE;
                    O_LOG_DEBUG("calling appman_application_launch_async from aim reply\n");
                    appman_application_launch_async(pContext->pApplication,
                            pContext->launch_reason,
                            NULL,
                            _appman_controller_app_launch_async_cb,
                            pContext);
                }
            }

        }
        else
        {
            O_DEBUG_ASSERT(NULL != new_error);
            g_simple_async_result_set_from_error (pContext->pAsyncresult, new_error);
            O_LOG_ERROR("controller Launch Error %s\n",new_error->message);
            g_error_free(new_error);
            /* complete */
            _appman_controller_return_app_launch(pContext);
        }

    }
    else
    {
        O_DEBUG_ASSERT(NULL != error);
        g_simple_async_result_set_from_error (pContext->pAsyncresult, error);
        O_LOG_ERROR("controller Launch Error %s\n",error->message);
        g_error_free(error);
        /* complete */
        _appman_controller_return_app_launch(pContext);
    }
    if(NULL != pContext->ResultSet.pResultArray)
    {
    	g_ptr_array_set_free_func (pContext->ResultSet.pResultArray, (GDestroyNotify)g_hash_table_destroy);
        g_ptr_array_unref ( pContext->ResultSet.pResultArray);
    }
    O_LOG_EXIT("\n");
}
/*
 * appman_controller_aim_query_cb:
 * dbus callback function for AIM Query, collects the result of AIM Query
 */
static void
appman_controller_aim_query_cb (DBusGProxy *proxy, DBusGProxyCall *call, AppmanLaunchContext *pContext)
{


    GError *error = NULL;

    O_LOG_DEBUG("Aim reply received\n");
    /* finished this call */
    pContext->call = NULL;

    pContext->ret  = dbus_g_proxy_end_call (proxy, call, &error,
            dbus_g_type_get_collection ("GPtrArray", otvutil_var_dict_gtype()),
            &pContext->ResultSet.pResultArray, G_TYPE_INVALID);

    /* get the result */
    if (( pContext->ret == FALSE) || (error != NULL))
    {
        /* fix up the D-Bus error */
        appman_convert_aim_dbus_error (&error);
        O_LOG_WARNING ("AIm Query failed: %s \n", error->message);
        pContext->ret = FALSE;

    }
    _appman_controller_aim_query_finish (pContext, error);
}

/* Helper function to make a AIM query request
 * formulated AIM filter and make a dbus request with a callback */
/** Application Sort Criteria */
static gchar *application_sort_criteria = NULL;
static void appman_controller_query_aim_async(AppmanController* self, GEnumValue* launch_type,GValue* launch_data,AppmanLaunchContext* pContext)
{
    GError *error = NULL;
    gchar* aim_filter = NULL;
    gint i;
    O_LOG_ENTER("Requesting Aim Query\n");

    O_LOG_DEBUG("application_metadata=%s\n",application_metadata);

    O_LOG_DEBUG("Launch Type = %s\n",launch_type->value_nick);

    i = make_aim_filter(launch_type->value,launch_data, &aim_filter);
    O_DEBUG_ASSERT( NULL != aim_filter);

    if(i > 0)
    {
        O_LOG_DEBUG("Aim Filter =  %s\n",aim_filter);
        /* call D-Bus method async */
        pContext->ret = FALSE;
        pContext->call = dbus_g_proxy_begin_call (self->_priv->pAimProxy, "getAppinfoByQuery",
                (DBusGProxyCallNotify) appman_controller_aim_query_cb,
                pContext,
               /* (GDestroyNotify) appman_controller_aim_query_call_destroy_cb*/ NULL,
                G_TYPE_STRING, application_metadata,
                G_TYPE_STRING, aim_filter,
                G_TYPE_STRING, application_sort_criteria,
                G_TYPE_UINT, 1,
                G_TYPE_INVALID);

        if (pContext->call == NULL)
        {
            O_LOG_WARNING("AIM Call setup Failure\n");

            error = g_error_new(O_APPMAN_ERROR,O_APPMAN_ERROR_FAILED, "AIM Not Found");
            _appman_controller_aim_query_finish (pContext, error);
        }
        else
        {
            /* track state */
            g_ptr_array_add (pContext->controller->_priv->pCalls, pContext);
        }
        g_free(aim_filter);
    }
    else
    {
        error = g_error_new(O_APPMAN_ERROR,O_APPMAN_ERROR_FAILED, "Unknown error, Filter Creation Failed");
        _appman_controller_aim_query_finish (pContext, error);
    }
}
/* currently unused function. will be used in future. */
#ifdef UNUSED
/* Helper function to set the control to next application */
/* TODO: The current strategy is just to get the next app in the list
 * But the logic may need to be more adaptive based on the Window stack on the screen etc.
 */
static void _appman_controller_set_next_application(AppmanController* pController)
{

    AppmanApplication* pTempApp = NULL;

    AppmanControllerPrivate* priv  = APPMAN_CONTROLLER_GET_PRIVATE(pController);
    O_LOG_ENTER("\n");

    O_LOG_DEBUG("Getting the Next Application in the list\n");

    GList *pList =  g_hash_table_get_values ( priv->pRunningApplications);

    if(pList != NULL)
    {
        O_LOG_DEBUG("Running Application List is valid\n");
        GList *pApp  =  g_list_find(pList , priv->pControlApplication);

        if(NULL != pApp)
        {
            if( (pTempApp = (AppmanApplication*)g_list_next(pApp)) != NULL )
            {
                priv->pControlApplication = pTempApp;
            }
            else
            {
                priv->pControlApplication = (AppmanApplication*) g_list_previous(pApp);
            }
        }
        else
        {
            priv->pControlApplication = NULL;
        }
        g_list_free(pList);
    }
    else
    {
    	O_LOG_DEBUG("Running application list is empty.\n");
        priv->pControlApplication = NULL;
    }
    O_LOG_DEBUG(" Setting the Control App to <%x>",GPOINTER_TO_INT(priv->pControlApplication));

    O_LOG_EXIT("\n");
}
#endif
/* application objects signal handler function for State changed signal
 * Emits a APP LAUNCHED/APP DESTROYED SIGNAL
 * sets the control to the next app if the control app dies
 * removes the application object from the hash table if app dies
 * */
static void _appman_controller_app_state_changed(GObject* pApplication , gint previous_state,  gint current_state, gint reason, gpointer user_data)
{
    AppmanController* pController = (AppmanController *)user_data;
    AppmanControllerPrivate* priv  = APPMAN_CONTROLLER_GET_PRIVATE(pController);
    int numOfRunningApps;

    O_LOG_DEBUG("pApplication = %p , previous_state = %d, current_state = %d\n",pApplication,previous_state,current_state);

    if (current_state == APP_STATE_EXITED )
    {
        O_LOG_DEBUG("Application Exited  current state = [%d], previous state = [%d] , reason = [%d].\n",current_state, previous_state, reason);

        gint pid;
        /* Add the App object to list of running apps*/
        g_object_get(pApplication, APP_PROP_PID_STR,&pid, NULL);

        if ((previous_state == APP_STATE_RUNNING)  || (previous_state == APP_STATE_STOPPING))
        {
#ifndef NDEBUG
            g_mutex_lock(priv->pMutex);
            O_DEBUG_ASSERT(g_hash_table_lookup( priv->pRunningApplications, GINT_TO_POINTER(pid)) != NULL);
            g_mutex_unlock(priv->pMutex);
#endif
            /* Emit Application Destroyed signal */
            g_signal_emit(pController,s_controller_signals[APPMAN_CONTROLLER_APP_DESTROYED_SIG],0,pApplication,reason);

            /* remove the app from running list */
            g_mutex_lock(priv->pMutex);
            g_hash_table_remove(priv->pRunningApplications,GINT_TO_POINTER(pid));
            numOfRunningApps = g_hash_table_size(priv->pRunningApplications);
            g_mutex_unlock(priv->pMutex);

            /* emit signal if all apps are destroyed */
            if (numOfRunningApps == 0)  {
            	g_signal_emit(pController, s_controller_signals[APPMAN_CONTROLLER_DESTROYED_ALL_SIG], 0);
            }

            /* this code is required in case appman has to set the next application to get focus/control.
             * don't remove it. */
#if 0
            if((AppmanApplication *)pApplication == pController->_priv->pControlApplication)
            {
                _appman_controller_set_next_application(pController);
            }
#endif
        }
        else
        {

            g_hash_table_remove(pController->_priv->pWaitingConnections,GINT_TO_POINTER(pid));
        }
    }
    else if(current_state == APP_STATE_RUNNING)
    {
        /* Emit Application Launched  signal */

        g_signal_emit(pController,s_controller_signals[APPMAN_CONTROLLER_APP_LAUNCHED_SIG],0,pApplication,reason);
    }
    else if(current_state == APP_STATE_STARTING)
    {
        O_LOG_DEBUG(" Application Starting  current state = [%d], previous state = [%d] , reason = [%d]",current_state, previous_state, reason);
        gint pid;
        /* Add the App object to list of running apps*/
        g_object_get(pApplication, APP_PROP_PID_STR,&pid, NULL);
        g_hash_table_insert(priv->pWaitingConnections,GINT_TO_POINTER(pid),g_object_ref(pApplication));
    }
    O_LOG_EXIT("\n");
}
/* callback called by application object after app launch request
 * inserts app into the hash table if launched successfully
 * returns the orignal caller about the launch result
 * */
static void _appman_controller_app_launch_async_cb (GObject      *object,
        GAsyncResult *result,
        gpointer      user_data)
{
	O_LOG_ENTER("\n");

    AppmanApplication* pApp = APPMAN_APPLICATION(object);
    AppmanLaunchContext *pAppmanHelperData =  (AppmanLaunchContext*) user_data;
    AppmanControllerPrivate* priv  = APPMAN_CONTROLLER_GET_PRIVATE(pAppmanHelperData->controller);

    GError *error = NULL;
    gboolean bResult = FALSE;

    O_DEBUG_ASSERT( NULL != object);

    bResult = appman_application_launch_finish(pApp, result, &error);

    /* If app is not running*/
    if (bResult == FALSE)
    {

        O_DEBUG_ASSERT(NULL != error);
        g_simple_async_result_set_from_error(pAppmanHelperData->pAsyncresult, error);

        g_clear_error(&error);
        /* unref the app object*/
        g_object_unref(pApp);
    }
    else
    {

        gint pid;
        /* Add the App object to list of running apps*/
        g_object_get(pApp, APP_PROP_PID_STR,&pid, NULL);

        /* No App object should exist with this pid*/
#ifndef NDEBUG
            g_mutex_lock(priv->pMutex);
            O_DEBUG_ASSERT(g_hash_table_lookup( priv->pRunningApplications, GINT_TO_POINTER(pid)) == NULL);
            g_mutex_unlock(priv->pMutex);
#endif

        PRINT_APP_PID("Adding the app to pRunningApplication list",pApp);
        /* Not referencing the object here because the initial reference is held while creating*/
        g_mutex_lock(priv->pMutex);
        g_hash_table_insert(priv->pRunningApplications,GINT_TO_POINTER(pid),pApp);
        g_mutex_unlock(priv->pMutex);


        /* setting the control application */
        /* TODO: Need to check a property of APP the set if this application has to set the control application or not.
         * currently the launched application has give the control. don't remove it. */
        if(1)
        {
            priv->pControlApplication = pApp;
        }

        g_simple_async_result_set_op_res_gpointer (pAppmanHelperData->pAsyncresult,
                g_object_ref(pApp),
                g_object_unref);

    }

    g_simple_async_result_complete(pAppmanHelperData->pAsyncresult);

    g_object_unref(pAppmanHelperData->pAsyncresult);
    g_object_unref(pAppmanHelperData->controller);

    if (pAppmanHelperData->controller->_priv->bLaunchPending == TRUE)
    {

        gint length;
        length = g_queue_get_length(pAppmanHelperData->controller->_priv->pWaitingApplications);
        O_DEBUG_ASSERT(length >=1 );

        O_LOG_DEBUG(" Check Application Launch pending %d",length);

        O_DEBUG_ASSERT(pAppmanHelperData == g_queue_peek_head(pAppmanHelperData->controller->_priv->pWaitingApplications ));

        g_queue_pop_head(pAppmanHelperData->controller->_priv->pWaitingApplications);

        if (length > 1)
        {
            O_LOG_DEBUG("  Launching Next App in Queue ");
            AppmanLaunchContext* pNextContext;
            pNextContext = g_queue_peek_head(pAppmanHelperData->controller->_priv->pWaitingApplications);
            if (NULL != pNextContext)
            {

                pNextContext->controller->_priv->bLaunchPending = TRUE;
                g_signal_connect(pNextContext->pApplication,APP_SIGNAL_STATE_CHANGED_STR,G_CALLBACK(_appman_controller_app_state_changed), pNextContext->controller);
                appman_application_launch_async(pNextContext->pApplication,
                        pNextContext->launch_reason,
                        NULL,
                        _appman_controller_app_launch_async_cb,
                        pNextContext);
            }
            else
            {
                O_DEBUG_ASSERT(0);
                pAppmanHelperData->controller->_priv->bLaunchPending = FALSE;
            }
        }
        else
        {
            pAppmanHelperData->controller->_priv->bLaunchPending = FALSE;
        }

    }
    else
    {
        O_LOG_DEBUG("No  Application to Launch from Queue");
    }

    g_slice_free (AppmanLaunchContext, pAppmanHelperData);

    O_LOG_EXIT("\n");
}

void appman_controller_launch_app_async (AppmanController               *self,
                                        GEnumValue                          *launch_type,
                                        GValue                         *launch_data,/*GVariant                                 *launch_data,*/
                                        o_appman_launch_reason_t     request_reason,
                                        GCancellable                  *cancellable,
                                        GAsyncReadyCallback           callback,
                                        gpointer                      user_data)
{
    GSimpleAsyncResult *simple;
    AppmanControllerPrivate* priv  = APPMAN_CONTROLLER_GET_PRIVATE(self);
    AppmanApplication *pRunningApp = NULL;


    simple = g_simple_async_result_new (G_OBJECT (self),
            callback
            /*_app_contoller_launch_async_cb */, user_data,
            appman_controller_launch_app_async);



    if (NULL ==  priv->pAimProxy)
    {
        O_LOG_WARNING("AIM Manager Not Available.\n");

        g_simple_async_result_set_error(simple,O_APPMAN_ERROR,O_APPMAN_ERROR_FAILED, "AIM Not Available");

        g_simple_async_result_complete_in_idle (simple);

    }
    else if (self->_priv->State != APPMAN_CONTROLLER_ACTIVE)
	{

    	O_LOG_WARNING("Appman Not Ready.\n");

		g_simple_async_result_set_error(simple, O_APPMAN_ERROR, O_APPMAN_ERROR_FAILED , "Appman Not Ready");

		g_simple_async_result_complete_in_idle(simple);
	}
    else if ( (_appman_controller_is_app_running(self, launch_type->value, launch_data, &pRunningApp) ==TRUE) &&
            ((appman_compare_property(pRunningApp , APP_PROP_MULTI_INSTANCE_STR,NULL)) == FALSE ))
    {
        /* If the app is not multi instance return*/
        O_LOG_DEBUG("Trying to run an already Running Application.\n");

        g_simple_async_result_set_error(simple, O_APPMAN_ERROR, O_APPMAN_ERROR_FAILED , "Application Running Already");

        g_simple_async_result_complete_in_idle (simple);

    }
    else
    {
        AppmanLaunchContext *pContext;
        pContext = g_slice_new0 (AppmanLaunchContext);
        O_DEBUG_ASSERT( NULL != pContext);

        pContext->controller = g_object_ref (self);
        pContext->pAsyncresult  = g_object_ref(simple);
        pContext->launch_reason = request_reason;

        if (pRunningApp == NULL)
        {
            appman_controller_query_aim_async(self, launch_type, launch_data, pContext);
        }
        else
        {/* TODO: This Code may never be executed if multiple instances of the same app is not supported */

            pContext->pApplication =  appman_application_clone(pRunningApp);

            pContext->controller->_priv->bLaunchPending = TRUE;
            g_queue_push_tail(pContext->controller->_priv->pWaitingApplications, pContext);
            appman_application_launch_async(pContext->pApplication,
                    pContext->launch_reason,
                    NULL,
                    _appman_controller_app_launch_async_cb,
                    pContext);
            g_signal_connect(pContext->pApplication,APP_SIGNAL_STATE_CHANGED_STR,G_CALLBACK(_appman_controller_app_state_changed), pContext->controller);

        }
    }


    g_object_unref (simple);
    return;
}

AppmanApplication *
appman_controller_launch_app_finish (AppmanController* self,
        GAsyncResult         *result,
        GError              **error)
{

    GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (result);

    g_return_val_if_fail (g_simple_async_result_is_valid (result, G_OBJECT (self), appman_controller_launch_app_async), NULL);

    if (g_simple_async_result_propagate_error (simple, error))
    {
        O_LOG_DEBUG("appman_controller_launch_app_finish: Error  %s", (*error)->message);
        return NULL;
    }
    return g_simple_async_result_get_op_res_gpointer (simple);

}
static void _appman_controller_stopped_on_aim_delete_cb(GObject      *object,
        GAsyncResult *result,
        gpointer      user_data)
{
    AppmanApplication* pApp = APPMAN_APPLICATION(object);

    GError *error = NULL;
    gboolean bResult = FALSE;

    bResult = appman_application_stop_finish(pApp, result, &error);

    if (bResult == FALSE)
    {
        O_DEBUG_ASSERT(NULL != error);
        g_clear_error(&error);
    }
    g_object_unref(pApp);

}
/* callback called by appication object*/
static void _appman_controller_destroy_async_cb (GObject      *object,
        GAsyncResult *result,
        gpointer      user_data)
{

    AppmanApplication* pApp = APPMAN_APPLICATION(object);
    AppmanAppDestroyContext *pAppmanHelperData =  (AppmanAppDestroyContext*) user_data;


    GError *error = NULL;
    gboolean bResult = FALSE;


    O_DEBUG_ASSERT( NULL != object);
    O_LOG_DEBUG("_app_contoller_destroy_async_cb");

    bResult = appman_application_stop_finish(pApp, result, &error);

    /* If app is not running*/
    if (bResult == FALSE)
    {

        O_DEBUG_ASSERT(NULL != error);
        g_simple_async_result_set_from_error(pAppmanHelperData->pAsyncresult, error);

        g_clear_error(&error);

    }
    else
    {
        g_simple_async_result_set_op_res_gpointer (pAppmanHelperData->pAsyncresult,
                g_object_ref(pApp),
                g_object_unref);

    }

    g_simple_async_result_complete(pAppmanHelperData->pAsyncresult);

    g_object_unref(pApp);

    g_object_unref(pAppmanHelperData->pAsyncresult);

    g_object_unref(pAppmanHelperData->controller);

    g_slice_free (AppmanAppDestroyContext, pAppmanHelperData);

}
AppmanApplication * appman_controller_destroy_app_finish (AppmanController* self, GAsyncResult *result,  GError  **error)
{
    GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (result);

    g_return_val_if_fail (g_simple_async_result_is_valid (result, G_OBJECT (self), appman_controller_destroy_app_async), NULL);

    if (g_simple_async_result_propagate_error (simple, error))
    {
        O_LOG_DEBUG("appman_controller_destroy_app_finish: Error  %s", (*error)->message);
        return NULL;
    }
    return g_simple_async_result_get_op_res_gpointer (simple);
}


void appman_controller_destroy_app_async (AppmanController               *self,
        GEnumValue                          *destroy_type,
        GValue                         *destroy_data,/*GVariant                                 *launch_data,*/
        o_appman_exit_reason_t       request_reason,
        GCancellable                  *cancellable,
        GAsyncReadyCallback           callback,
        gpointer                      user_data)
{
    GSimpleAsyncResult *simple;
    GError *error = NULL;
    AppmanControllerPrivate* priv  = APPMAN_CONTROLLER_GET_PRIVATE(self);
    AppmanApplication *pRunningApp = NULL;


    simple = g_simple_async_result_new (G_OBJECT (self),
            callback
            /*_app_contoller_launch_async_cb */, user_data,
            appman_controller_destroy_app_async);

    if (priv->State != APPMAN_CONTROLLER_ACTIVE)
    {
        if (priv->State == APPMAN_CONTROLLER_SUSPENDING_APPS )
        {
            error = g_error_new(O_APPMAN_ERROR, O_APPMAN_ERROR_FAILED , "Suspending all Applications ");
        }
        else
        {
            error = g_error_new(O_APPMAN_ERROR, O_APPMAN_ERROR_FAILED , "Appman Not Ready");
        }

        g_simple_async_result_set_from_error(simple, error);

        g_clear_error(&error);
        g_simple_async_result_complete_in_idle (simple);
    }
    else if (_appman_controller_is_app_running(self, destroy_type->value, destroy_data, &pRunningApp) ==TRUE)
    {

        app_state_t appstate;
        g_object_get(pRunningApp,APP_PROP_STATE_STR ,&appstate,NULL);
        if (appstate == APP_STATE_RUNNING)
        {
            AppmanAppDestroyContext *pDestroyContext ;
            pDestroyContext = g_slice_new0 (AppmanAppDestroyContext);

            pDestroyContext->controller = g_object_ref (self);
            pDestroyContext->pAsyncresult  = g_object_ref(simple);
            pDestroyContext->destroy_reason = request_reason;

            appman_application_stop_async(g_object_ref(pRunningApp), pDestroyContext->destroy_reason,NULL,_appman_controller_destroy_async_cb,pDestroyContext);
        }
        else
        {
            g_simple_async_result_set_error(simple, O_APPMAN_ERROR, O_APPMAN_ERROR_APP_NOT_RUNNING , "App Not Running");

            g_simple_async_result_complete_in_idle (simple);
        }
    }
    else
    {
        g_simple_async_result_set_error(simple, O_APPMAN_ERROR, O_APPMAN_ERROR_APP_NOT_FOUND , "App Not Found");

        g_simple_async_result_complete_in_idle (simple);

    }
    /* Kill all apps matching the criteria*/
    g_object_unref (simple);
    return;
}




GHashTable *
appman_controller_get_running_apps (AppmanController* self)
{
    return self->_priv->pRunningApplications;
}

void  appman_controller_get_running_apps_async(AppmanController* self,
        GCancellable                  *cancellable,
        GAsyncReadyCallback           callback,
        gpointer                      user_data)
{
    GSimpleAsyncResult *simple;

    O_LOG_DEBUG("appman_controller_get_running_apps_async");

    simple = g_simple_async_result_new (G_OBJECT (self),
            callback
            /*_app_contoller_launch_async_cb */, user_data,
            appman_controller_get_running_apps_async);



    g_simple_async_result_set_op_res_gpointer (simple , g_object_ref(self) , g_object_unref);

    g_simple_async_result_complete_in_idle (simple);

    g_object_unref(simple);

}
GHashTable *
appman_controller_get_running_apps_finish (AppmanController* self,
        GAsyncResult         *result,
        GError              **error)
{

    O_LOG_ENTER("\n");
    GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (result);

    g_return_val_if_fail (g_simple_async_result_is_valid (result, G_OBJECT (self), appman_controller_get_running_apps_async), NULL);

    if (g_simple_async_result_propagate_error (simple, error))
    {
        O_LOG_DEBUG("appman_controller_get_running_apps_finish: Error  %s", (*error)->message);
        return NULL;
    }

    AppmanController *pController = g_simple_async_result_get_op_res_gpointer (simple);
    O_LOG_EXIT("\n");
    return pController->_priv->pRunningApplications;

}

/* callback called by application object after suspending an app*/
static void _appman_controller_suspend_async_cb (GObject      *object,
        GAsyncResult *result,
        gpointer      user_data)
{

    AppmanApplication* pApp = APPMAN_APPLICATION(object);
    AppmanSuspendAllContext *pAppmanHelperData =  (AppmanSuspendAllContext*) user_data;
    AppmanControllerPrivate* priv  = APPMAN_CONTROLLER_GET_PRIVATE(pAppmanHelperData->controller);
    GError *error = NULL;

    O_LOG_ENTER("\n");
    O_DEBUG_ASSERT( NULL != object);


    appman_application_stop_finish(pApp, result, &error);
    g_clear_error(&error);

    /* save app info only if requested */
    if(pAppmanHelperData->saveAppContext)  {
    	appman_config_save_suspended_app_info(priv->pAppmanConfigObject,  pApp);
    }

    g_object_unref(pApp);

    O_LOG_EXIT("\n");
}

/* helper function to suspend one application */
static void _appman_controller_suspend_helper (gpointer key, gpointer value, gpointer user_data)
{
    AppmanApplication *pRunningApp =     (AppmanApplication *)value;
    AppmanSuspendAllContext *pContext = (AppmanSuspendAllContext *)user_data;

    O_LOG_ENTER("Suspending App %p \n",pRunningApp);

    appman_application_stop_async(g_object_ref(pRunningApp), pContext->destroy_reason,NULL,_appman_controller_suspend_async_cb,user_data);
    O_LOG_EXIT("\n");
}

static void
_appman_controller_destroyall_signal_cb(AppmanController *pController, gpointer user_data)
{
    O_LOG_ENTER("All Applications suspended.\n");

	AppmanSuspendAllContext *pAppmanHelperData =  (AppmanSuspendAllContext*) user_data;
    AppmanControllerPrivate *priv  = APPMAN_CONTROLLER_GET_PRIVATE(pAppmanHelperData->controller);

	/* NO More Apps Running*/
	g_simple_async_result_set_op_res_gboolean (pAppmanHelperData->pAsyncresult, TRUE);
	g_simple_async_result_complete_in_idle(pAppmanHelperData->pAsyncresult);
	g_object_unref(pAppmanHelperData->pAsyncresult);
	g_object_unref(pAppmanHelperData->controller);
	g_slice_free (AppmanSuspendAllContext, pAppmanHelperData);

	/* remove this signal handler */
	g_signal_handlers_disconnect_by_func (pController, (GCallback) _appman_controller_destroyall_signal_cb, user_data);

	priv->State = APPMAN_CONTROLLER_ACTIVE;

	O_LOG_EXIT("\n");
}


gboolean appman_controller_suspend_all_applications_finish (AppmanController* self, GAsyncResult *result,  GError  **error)
{
    GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (result);

    g_return_val_if_fail (g_simple_async_result_is_valid (result, G_OBJECT (self), appman_controller_suspend_all_applications_async), FALSE);

    if (g_simple_async_result_propagate_error (simple, error))
    {
        return FALSE;
    }
    return g_simple_async_result_get_op_res_gboolean(simple);
}


void  appman_controller_suspend_all_applications_async(AppmanController* self,
		GCancellable                  *cancellable,
        GAsyncReadyCallback           callback,
        gpointer                      user_data,
        gboolean					  saveAppContext)
{

    GSimpleAsyncResult *simple;
    GError *error = NULL;

    O_LOG_ENTER("Destroying all applications...\n");

    simple = g_simple_async_result_new (G_OBJECT (self),
            callback, user_data,
            appman_controller_suspend_all_applications_async);

    if (self->_priv->State != APPMAN_CONTROLLER_ACTIVE)
    {
        if (self->_priv->State == APPMAN_CONTROLLER_SUSPENDING_APPS )
        {
            error = g_error_new(O_APPMAN_ERROR, O_APPMAN_ERROR_FAILED , "Suspending Already");
        }
        else
        {
            error = g_error_new(O_APPMAN_ERROR, O_APPMAN_ERROR_FAILED , "Appman Not Ready");
        }

        g_simple_async_result_set_from_error(simple, error);
        g_clear_error(&error);
        g_simple_async_result_complete_in_idle (simple);
    }
    else
    {
    	g_mutex_lock(self->_priv->pMutex);
    	int numOfRunningApps = g_hash_table_size(self->_priv->pRunningApplications);
    	g_mutex_unlock(self->_priv->pMutex);

        if( numOfRunningApps == 0 )  {
        	g_simple_async_result_set_op_res_gboolean (simple, TRUE);
            g_simple_async_result_complete(simple);
    	}
    	else {
			self->_priv->State = APPMAN_CONTROLLER_SUSPENDING_APPS;

			AppmanSuspendAllContext *pContext ;
			pContext = g_slice_new0 (AppmanSuspendAllContext);

			pContext->controller = g_object_ref (self);
			pContext->pAsyncresult  = g_object_ref(simple);
			pContext->destroy_reason = O_APPMAN_APP_EXIT_SUSPENDED;
			pContext->saveAppContext = saveAppContext;

			g_hash_table_foreach (self->_priv->pRunningApplications,(GHFunc) _appman_controller_suspend_helper,pContext);

			/* connect to destroy all signal to clean up the suspendAllContext */
			g_signal_connect( self, APPMAN_CONTROLLER_DESTROYED_ALL_SIG_STR, G_CALLBACK(_appman_controller_destroyall_signal_cb), pContext);
    	}
    }

    g_object_unref(simple);
    O_LOG_EXIT("\n");
}


static void _appman_controller_destroyall_in_idle_cb(GObject *object, GAsyncResult *result, gpointer user_data)
{
	AppmanController *self = (AppmanController*)user_data;
	GError           *error = NULL;

	appman_controller_suspend_all_applications_finish(self, result, &error);
	if (error != NULL)  {
		O_LOG_ERROR("Destroyall failed. Error %s", error->message);
		g_error_free(error);
	}
	else  {
		/* destroyall successful. stop controller */
		appman_controller_stop(self);
	}
}

gboolean appman_controller_destroyall_in_idle(gpointer data)
{
	AppmanController* self = (AppmanController*)data;

	O_LOG_ENTER("\n");

	appman_controller_suspend_all_applications_async(self, NULL, (GAsyncReadyCallback)_appman_controller_destroyall_in_idle_cb, self, FALSE);

	O_LOG_EXIT("\n");
	return FALSE;
}



AppmanApplication *
appman_controller_get_control_app (AppmanController* self)
{
    return self->_priv->pControlApplication;
}

void  appman_controller_request_application_action_async(AppmanController* self,
      GEnumValue                        *param_type,
        GValue                         *param_data,
        guint                           actionType,
        GHashTable                     *payload,
        GCancellable                  *cancellable,
        GAsyncReadyCallback           callback,
        gpointer                      user_data)
{


    GSimpleAsyncResult *simple;
    AppmanApplication* pApp = NULL;
    O_LOG_ENTER("\n");
    simple = g_simple_async_result_new (G_OBJECT (self),
            callback
            /*_app_contoller_launch_async_cb */, user_data,
            appman_controller_request_application_action_async);

    if (self->_priv->State != APPMAN_CONTROLLER_ACTIVE)
    {

        g_simple_async_result_set_error(simple, O_APPMAN_ERROR, O_APPMAN_ERROR_FAILED , "Appman Not Ready");

        g_simple_async_result_complete(simple);
    }
    else
    {

        if (_appman_controller_is_app_running(self, param_type->value, param_data, &pApp) ==TRUE)
        {
            O_DEBUG_ASSERT(NULL != pApp);
            /*
             *  For now we allow sending messages from either an App or Native service
             *  In case of native service the source  uuid is empty.
             */
            if(0 != appman_application_request_action(pApp,
                    APP_SIGNAL_REQUEST_ACTION,
                    actionType,
                    payload))


            {
                O_DEBUG_ASSERT(FALSE);
            }
            g_simple_async_result_set_op_res_gboolean(simple , TRUE);
        }
        else
        {
            g_simple_async_result_set_error(simple , O_APPMAN_ERROR,O_APPMAN_ERROR_APP_NOT_FOUND, "Unable to Send Message, App Not Found");

        }


        g_simple_async_result_complete(simple);
    }
    g_object_unref(simple);
    O_LOG_EXIT("\n");

}
gboolean
appman_controller_request_application_action_finish (AppmanController* self,
        GAsyncResult         *result,
        GError              **error)
{

    gboolean bResult = FALSE;
    GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (result);

    O_LOG_ENTER("\n");

    g_return_val_if_fail (g_simple_async_result_is_valid (result, G_OBJECT (self), appman_controller_request_application_action_async), FALSE);
    if (g_simple_async_result_propagate_error (simple, error))
    {
        O_LOG_DEBUG("appman_controller_set_application_control_finish: Error  %s", (*error)->message);
    }
    else
    {
        bResult = TRUE;
    }
    O_LOG_EXIT("\n");
    return bResult;

}

void  appman_controller_is_app_running_async(AppmanController* self,
        GEnumValue                      *param_type,
        GValue                          *param_data,
        GCancellable                    *cancellable,
        GAsyncReadyCallback             callback,
        gpointer                        user_data)
{

    GSimpleAsyncResult *simple;
    AppmanApplication* pApp = NULL;
    O_LOG_ENTER("\n");
    simple = g_simple_async_result_new (G_OBJECT (self),
            callback
            /*_app_contoller_launch_async_cb */, user_data,
            appman_controller_is_app_running_async);

    if (self->_priv->State != APPMAN_CONTROLLER_ACTIVE)
    {

        g_simple_async_result_set_error(simple, O_APPMAN_ERROR, O_APPMAN_ERROR_FAILED , "Appman Not Ready");

        g_simple_async_result_complete(simple);
    }
    else
    {

        if (_appman_controller_is_app_running(self, param_type->value, param_data, &pApp) ==TRUE)
        {
            O_DEBUG_ASSERT(NULL != pApp);
            g_simple_async_result_set_op_res_gpointer(simple, g_object_ref(pApp), (GDestroyNotify)g_object_unref);
            g_simple_async_result_complete_in_idle(simple);
        }
        else
        {
            g_simple_async_result_set_op_res_gpointer(simple,NULL,NULL);
            g_simple_async_result_complete(simple);
        }
    }
    g_object_unref(simple);
    O_LOG_EXIT("\n");
}
gboolean
appman_controller_is_app_running_finish (AppmanController* self,
        GAsyncResult         *result,
        AppmanApplication **pApp,
        GError              **error)
{

    gboolean bResult = FALSE;
    GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (result);


    O_LOG_ENTER("\n");

    g_return_val_if_fail (g_simple_async_result_is_valid (result, G_OBJECT (self), appman_controller_is_app_running_async), FALSE);
    if (g_simple_async_result_propagate_error (simple, error))
    {
        O_LOG_DEBUG("appman_controller_send_message_finish: Error  %s", (*error)->message);
    }
    else
    {
        AppmanApplication* pApplication = (AppmanApplication*)g_simple_async_result_get_op_res_gpointer (simple);
        if(NULL != pApp)
        {
            *pApp = pApplication;
            app_state_t appstate = APP_STATE_INITIAL;
            g_object_get(pApplication,APP_PROP_STATE_STR ,&appstate,NULL);
            if (appstate == APP_STATE_RUNNING)
            {
                bResult = TRUE;
            }
        }
    }
    O_LOG_EXIT("\n");
    return bResult;
}


void  appman_controller_send_message_async(AppmanController* self,
        GEnumValue                      *param_type,
        GValue                          *param_data,
        GHashTable                      *pSenderInfo,
        GHashTable                      *pData,
        GCancellable                    *cancellable,
        GAsyncReadyCallback             callback,
        gpointer                        user_data)
{

    GSimpleAsyncResult *simple;
    AppmanApplication* pApp = NULL;
    O_LOG_ENTER("\n");
    simple = g_simple_async_result_new (G_OBJECT (self),
            callback
            /*_app_contoller_launch_async_cb */, user_data,
            appman_controller_send_message_async);

    if (self->_priv->State != APPMAN_CONTROLLER_ACTIVE)
    {

        g_simple_async_result_set_error(simple, O_APPMAN_ERROR, O_APPMAN_ERROR_FAILED , "Appman Not Ready");

        g_simple_async_result_complete(simple);
    }
    else
    {

        if (_appman_controller_is_app_running(self, param_type->value, param_data, &pApp) ==TRUE)
        {
            O_DEBUG_ASSERT(NULL != pApp);
            /*
             *  For now we allow sending messages from either an App or Native service
             *  In case of native service the source  uuid is empty.
             */


            if (0 != appman_application_send_message (pApp,
                    pSenderInfo,
                    (GHashTable*)  pData))
            {
                O_DEBUG_ASSERT(FALSE);
            }
            g_simple_async_result_set_op_res_gboolean(simple , TRUE);
        }
        else
        {
            g_simple_async_result_set_error(simple , O_APPMAN_ERROR,O_APPMAN_ERROR_APP_NOT_FOUND, "Unable to Send Message, App Not Found");

        }


        g_simple_async_result_complete(simple);
    }
    g_object_unref(simple);
    O_LOG_EXIT("\n");
}
gboolean
appman_controller_send_message_finish (AppmanController* self,
        GAsyncResult         *result,
        GError              **error)
{

    gboolean bResult = FALSE;
    GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (result);


    O_LOG_ENTER("\n");

    g_return_val_if_fail (g_simple_async_result_is_valid (result, G_OBJECT (self), appman_controller_send_message_async), FALSE);
    if (g_simple_async_result_propagate_error (simple, error))
    {
        O_LOG_DEBUG("appman_controller_send_message_finish: Error  %s", (*error)->message);
    }
    else
    {
        bResult = TRUE;
    }
    O_LOG_EXIT("\n");
    return bResult;
}
void  appman_controller_get_app_permission_info_async(AppmanController* self,GCancellable *cancellable,
        GAsyncReadyCallback           callback,
        gpointer                      user_data)
{

    GSimpleAsyncResult *simple;
    O_LOG_ENTER("\n");
    simple = g_simple_async_result_new (G_OBJECT (self),
            callback
            /*_app_contoller_launch_async_cb */, user_data,
            appman_controller_get_app_permission_info_async);

    if (self->_priv->State != APPMAN_CONTROLLER_ACTIVE)
    {

        g_simple_async_result_set_error(simple, O_APPMAN_ERROR, O_APPMAN_ERROR_FAILED , "Appman Not Ready");

        g_simple_async_result_complete_in_idle (simple);
    }
    else
    {
        /* TODO: IMPLEMENT*/
        g_simple_async_result_set_error(simple, O_APPMAN_ERROR, O_APPMAN_ERROR_FAILED , "Method Not Yet Implemented");

        g_simple_async_result_complete_in_idle (simple);
    }

    g_object_unref(simple);

    O_LOG_EXIT("\n");
}

/*TODO: Change signature, Unimplemented*/
gboolean
appman_controller_get_app_permission_info_finish (AppmanController* self,
        GAsyncResult         *result,
        GError              **error)
{
    gboolean bResult = FALSE;
    O_LOG_ENTER("\n");

    g_return_val_if_fail (g_simple_async_result_is_valid (result, G_OBJECT (self), appman_controller_get_app_permission_info_async), FALSE);

    O_LOG_EXIT("\n");
    return bResult;
}

AppmanConfig* appman_controller_get_config_object(AppmanController* self)
{
    AppmanControllerPrivate* priv  = APPMAN_CONTROLLER_GET_PRIVATE(self);

    return priv->pAppmanConfigObject;
}

AppmanPolicy* appman_controller_get_policy_object(AppmanController* self)
{
    AppmanControllerPrivate* priv  = APPMAN_CONTROLLER_GET_PRIVATE(self);

    return priv->pAppmanPolicyObject;
}

AppmanApplication*    appman_controller_get_application(AppmanController* self, gint pid)
{
    AppmanControllerPrivate* priv  = APPMAN_CONTROLLER_GET_PRIVATE(self);
    O_LOG_ENTER("\n");
    AppmanApplication* pApp = NULL;

    if (priv->pRunningApplications != NULL)
    {
    	g_mutex_lock(priv->pMutex);
        pApp = (AppmanApplication*)g_hash_table_lookup (priv->pRunningApplications , GUINT_TO_POINTER(pid));
        g_mutex_unlock(priv->pMutex);

    }
    O_LOG_EXIT("\n");
    return pApp;
}
/* Helper function to compare if the given app is in the waiting queue */
static gint _appman_controller_compare_with_waiting_to_launch(gconstpointer pContextQueue, gconstpointer pAimUuid)
{
    gint result = 1;
    O_LOG_ENTER("\n");
    if (pContextQueue != NULL && pAimUuid != NULL)
    {
        AppmanLaunchContext *pQueueInfo = (AppmanLaunchContext *)pContextQueue;

        if (TRUE == appman_compare_property(pQueueInfo->pApplication, APP_PROP_AIM_UUID_STR, (GValue*)pAimUuid ))
        {
            result = 0;
        }

    }
    O_LOG_EXIT("\n");
    return result;
}
/* Function to check if the app is launchable
 * Checks if it is running in the running apps hash table
 *  checks if it is in the waiting to launch queue
 *  In either case if it is present in one of the list then
 *  checks if it is multi instance capable
 *  */

static gboolean _appman_controller_is_app_launchable(AppmanController *self , AppmanApplication* pApp)
{

    gboolean bResult = TRUE;
    AppmanApplication* pTempApp = NULL;
    gchar* pAim_uuid = NULL;
    gboolean bIsMultInstance;

    O_LOG_ENTER("\n");

    O_DEBUG_ASSERT(NULL != pApp);
    GValue v = {0};

    g_object_get(pApp,
            APP_PROP_AIM_UUID_STR, &pAim_uuid,
            NULL);
    g_value_init(&v , G_TYPE_STRING);
    g_value_set_static_string(&v , (gchar*)pAim_uuid);

    /* Check Running Applications*/
    if (_appman_controller_is_app_running(self ,AIM_UUID,  &v, &pTempApp) == TRUE)
    {

        bIsMultInstance = appman_compare_property(pTempApp,APP_PROP_MULTI_INSTANCE_STR, NULL);
        if (bIsMultInstance == FALSE)
        {
            bResult = FALSE;
        }

    }
    else
    {
        AppmanLaunchContext *  pAppLaunchInfo = NULL;

        /* Check Waiting  Applications*/
        if (NULL != self->_priv->pWaitingApplications && !g_queue_is_empty(self->_priv->pWaitingApplications))
        {
            pAppLaunchInfo =  ( AppmanLaunchContext * )g_queue_find_custom(self->_priv->pWaitingApplications,
                    &v,
                    _appman_controller_compare_with_waiting_to_launch);
            if (NULL != pAppLaunchInfo )
            {
                O_DEBUG_ASSERT( pAppLaunchInfo->pApplication != NULL);

                bIsMultInstance = appman_compare_property(pAppLaunchInfo->pApplication,APP_PROP_MULTI_INSTANCE_STR, NULL);
                if (bIsMultInstance == FALSE)
                {
                    bResult = FALSE;
                }
            }
        }

    }
    g_value_unset(&v);
    g_free(pAim_uuid);
    O_LOG_EXIT("\n");
    return bResult;
}
/* This function iterates through the list of app and returns TRUE if a running app is found matching the criteria*/
static gboolean _appman_controller_is_app_running(AppmanController* self, o_appman_app_info_type_t param_type,  GValue *data, AppmanApplication** pApp)
{
    gboolean bIsAppRunning = FALSE;
    GHashTableIter iter;
    gpointer key, value;
    AppmanApplication* pTempApp = NULL;
    GHashTable* pRunningApplications = self->_priv->pRunningApplications;
    const gchar* property_name = NULL;

    O_LOG_ENTER("\n");
    if (pRunningApplications == NULL || data == NULL)
    {
        O_DEBUG_ASSERT(NULL != data);
        O_LOG_EXIT("NO Running Apps or invalid params\n");
        return FALSE;
    }

    switch(param_type)
    {
    	case APP_NAME:
			property_name = APP_PROP_NAME_STR;
			break;

    	case APP_ID:
    		property_name = APP_PROP_ID_STR;
    		break;

    	case AIM_UUID:
    		property_name = APP_PROP_AIM_UUID_STR;
    		break;

    	case APP_LOCATION:
    		property_name = APP_PROP_LOC_STR;
    		break;

    	case APP_USAGE:
    		property_name = APP_PROP_USAGE_STR;
    		break;

    	case APP_PID:
    	{
			gint pid = g_value_get_int(data);
			g_mutex_lock(self->_priv->pMutex);
			*pApp = g_hash_table_lookup(pRunningApplications,GINT_TO_POINTER(pid));
			g_mutex_unlock(self->_priv->pMutex);

			if(NULL != *pApp)
			{
				O_LOG_DEBUG("Application is RUNNING.\n");
				O_LOG_EXIT("\n");
				return TRUE;
			}
			else
			{
				O_LOG_DEBUG("Application is NOT RUNNING.\n");
				O_LOG_EXIT("\n");
				return FALSE;
			}
    	}
    	break;

    	default:
    	{
			O_LOG_WARNING("ILLEGAL/UNSUPPORTED PARAMETER %d.\n", param_type);
			O_LOG_EXIT("\n");
			return FALSE;
    	}
    }

    g_mutex_lock(self->_priv->pMutex);

    g_hash_table_iter_init (&iter, pRunningApplications);
    while (g_hash_table_iter_next (&iter, &key, &value))
    {
        pTempApp = (AppmanApplication*) value;

        bIsAppRunning = appman_compare_property(pTempApp, property_name, data);
        if (bIsAppRunning == TRUE)
        {
            break;
        }
    }

    g_mutex_unlock(self->_priv->pMutex);

    if ( (bIsAppRunning == TRUE) && ( NULL != pApp) )
    {
        *pApp = pTempApp;
    }

    O_LOG_DEBUG("Application is %s.\n",bIsAppRunning?"RUNNING":"NOT RUNNING");

    O_LOG_EXIT("\n");
    return bIsAppRunning;
}

/* callback function called when Confman Dbus proxy disconnects */
static void _appman_controller_confman_disconnected_cb(GObject* pObject, gpointer user_data)
{

    O_LOG_ENTER("\n");
    O_LOG_WARNING(" \n Config Manager Disconnected ");

    AppmanController* self = (AppmanController*) user_data;
    AppmanControllerPrivate* priv  = APPMAN_CONTROLLER_GET_PRIVATE(self);

    g_signal_handlers_disconnect_by_func (pObject,
            (GCallback) _appman_controller_confman_disconnected_cb,
            user_data);


    O_DEBUG_ASSERT( pObject == (GObject*)priv->pConfManProxy);

    g_object_unref(pObject);


    dbus_g_connection_unref(priv->pConfManConnection);

    priv->pConfManConnection = NULL;
    priv->pConfManProxy = NULL;

    priv->confman_reconnect_timeout_id = g_timeout_add_seconds (RECONNECT_DELAY,
            (GSourceFunc) _appman_controller_is_configman_reconnect_reqd,
            user_data);

    O_LOG_EXIT("\n");
}
/* callback function called when Aim Dbus proxy disconnects */
static void _appman_controller_aim_disconnected_cb(GObject* pObject, gpointer user_data)
{

    O_LOG_ENTER("\n");
    O_LOG_WARNING(" \n Aim Manager Disconnected ");

    AppmanController* self = (AppmanController*) user_data;
    AppmanControllerPrivate* priv  = APPMAN_CONTROLLER_GET_PRIVATE(self);

    g_signal_handlers_disconnect_by_func (pObject,
            (GCallback) _appman_controller_aim_disconnected_cb,
            user_data);


    O_DEBUG_ASSERT( pObject == (GObject*)priv->pAimProxy);

    g_object_unref(pObject);


    dbus_g_proxy_disconnect_signal(priv->pAimProxy , "aim_delete",
            G_CALLBACK(_appman_controller_aim_delete_signal_handler),
            self);
    dbus_g_connection_unref(priv->pAimConnection);

    priv->pAimConnection = NULL;
    priv->pAimProxy = NULL;

    priv->aim_reconnect_timeout_id = g_timeout_add_seconds (RECONNECT_DELAY,
            (GSourceFunc) _appman_controller_is_aim_reconnect_reqd,
            user_data);
    g_signal_emit(self,s_controller_signals[APPMAN_CONTROLLER_AIM_STATE_CHANGED_SIG],0,APPMAN_CONTROLLER_AIM_DISCONNECTED_SIGNAL);
    O_LOG_EXIT("\n");
}
/* signal handler for AIM delete signal
 * called when AIM deletes an entry
 * TODO: Need to kill the App if the corresponding App/Apps are running*/
static void _appman_controller_aim_delete_signal_handler(DBusGProxy *proxy, gchar * uuid[], void * user_data)
{

    int i = 0;

    GValue value = {0};

    AppmanApplication* pApp = NULL;
    O_LOG_ENTER("\n");

    g_value_init(&value, G_TYPE_STRING);

    while (NULL != uuid[i])
    {
        O_LOG_DEBUG("aim_delete_signal_handler::uuid[%d]::<%s>\n", i, uuid[i]);
        g_value_set_static_string(&value, uuid[i]);

        if(_appman_controller_is_app_running((AppmanController*)user_data,AIM_UUID,  &value, &pApp) == TRUE)
        {
            O_DEBUG_ASSERT(NULL != pApp);
            appman_application_stop_async(g_object_ref(pApp),O_APPMAN_APP_EXIT_APP_DELETED,NULL,_appman_controller_stopped_on_aim_delete_cb,NULL);
        }

        g_value_unset(&value);
        i++;
    }
    O_LOG_EXIT("\n");
}


/* This function is used for both initialization and reconnect timeout
 * If this function returns TRUE  in the case if AIM is not available after a timeout Appman tries to reconnect
 * */
static gboolean _appman_controller_is_configman_reconnect_reqd(AppmanController* self)
{

    GError*          error  = NULL;
    gboolean bResult  = TRUE;
    AppmanControllerPrivate* priv  = APPMAN_CONTROLLER_GET_PRIVATE(self);

    O_LOG_ENTER("\n");

    priv->pConfManConnection =  dbus_g_connection_open(CONFIGMAN_ADDRESS , &error);


    if ( priv->pConfManConnection == NULL  || error != NULL)
    {
        O_LOG_ERROR("APPMAN::Failed to connect to ConfMan Trying to Reconnect !!!  %s \n", error->message );
        g_error_free( error);
        O_LOG_EXIT("\n");
        return bResult;

    }
    else
    {
        priv->pConfManProxy =  dbus_g_proxy_new_for_peer(priv->pConfManConnection , CONFIGMAN_PATH, CONFIGMAN_INTERFACE);

        if (NULL ==  priv->pConfManProxy)
        {
            O_LOG_ERROR("APPMAN::Failed to connect to ConfMan service  Trying to Reconnect !!! \n");
            dbus_g_connection_unref(priv->pConfManConnection);
            O_LOG_EXIT("\n");
            return bResult;
        }
        g_signal_connect( priv->pConfManProxy, "destroy",G_CALLBACK(_appman_controller_confman_disconnected_cb),self);

#ifndef NDEBUG
        dbus_g_proxy_set_default_timeout(priv->pConfManProxy,6000);
#endif
    }

    priv->confman_reconnect_timeout_id = 0;

    bResult = FALSE;

    O_LOG_ENTER("\n");
    return bResult;
}
/* This function is used for both initialization and reconnect timeout
 * If this function returns TRUE  in the case if AIM is not available after a timeout Appman tries to reconnect
 * */
static gboolean _appman_controller_is_aim_reconnect_reqd(AppmanController* self)
{

    GError*          error  = NULL;
    gboolean bResult  = TRUE;
    AppmanControllerPrivate* priv  = APPMAN_CONTROLLER_GET_PRIVATE(self);

    O_LOG_ENTER("\n");

    AppmanServicesInfo* pServiceInfo = appman_config_get_services_info (priv->pAppmanConfigObject);

    priv->pAimConnection =  dbus_g_connection_open(pServiceInfo->pAimServerAddress , &error);


    if ( priv->pAimConnection == NULL  || error != NULL)
    {
        O_LOG_ERROR("APPMAN::Failed to connect to AIM Trying to Reconnect !!!  %s \n", error->message );
        g_error_free( error);
        O_LOG_EXIT("\n");
        return bResult;

    }
    else
    {
        priv->pAimProxy =  dbus_g_proxy_new_for_peer(  priv->pAimConnection , pServiceInfo->pAimObjectPath, pServiceInfo->pAimInterface);

        if (NULL ==  priv->pAimProxy)
        {
            O_LOG_ERROR("APPMAN::Failed to connect to AIM service  Trying to Reconnect !!! \n");
            dbus_g_connection_unref(priv->pAimConnection);
            O_LOG_EXIT("\n");
            return bResult;
        }

        dbus_g_proxy_add_signal( priv->pAimProxy  , "aim_delete" , G_TYPE_STRV, G_TYPE_INVALID);
        dbus_g_proxy_connect_signal(priv->pAimProxy , "aim_delete",G_CALLBACK(_appman_controller_aim_delete_signal_handler),self, NULL);
        g_signal_connect( priv->pAimProxy, "destroy",G_CALLBACK(_appman_controller_aim_disconnected_cb),self);
        g_signal_emit(self,s_controller_signals[APPMAN_CONTROLLER_AIM_STATE_CHANGED_SIG],0,APPMAN_CONTROLLER_AIM_CONNECTED_SIGNAL);
    }

    priv->aim_reconnect_timeout_id = 0;

    bResult = FALSE;

    O_LOG_ENTER("\n");
    return bResult;
}

/*
 * Access lpminfo pointer
 */
AppmanLpmInfo * appman_controller_lpm_info_get(AppmanController *self)
{
    return self->_priv->pLpmInfo ;
}

/*
 * Destroy lpminfo
 */
void appman_controller_lpm_info_destroy(AppmanController *self)
{
    AppmanControllerPrivate* priv  = APPMAN_CONTROLLER_GET_PRIVATE(self);
    if ( priv->pLpmInfo ) {
        g_slice_free( AppmanLpmInfo, priv->pLpmInfo );
        priv->pLpmInfo = NULL;
    }
}

/*
 * Call back for the LPM All async
 */
void _appman_controller_lpm_all_async_cb(GObject *object, GAsyncResult *result,
        gpointer user_data) {
    AppmanController *self = (AppmanController *) user_data;
    GError *error = NULL;

    O_LOG_ENTER("\n");

    guint max_delay = appman_controller_lpm_all_applications_finish(self,
            result, &error);
    if (error != NULL) {
        O_LOG_ERROR("LPMAll failed. Error %s", error->message);
        g_error_free(error);
    } else {
        O_LOG_DEBUG("Sending delay to pwrmgr\n");
        if ( pwrproxy_client_response(appman_controller_lpm_info_get(self)->iAppmanClientHandle, max_delay, appman_controller_lpm_info_get(self)->uiTag) == PWRMGR_INVALID_PARAM )
            O_LOG_ERROR("Error in sending response \n");

    }
    O_LOG_EXIT("\n");
}

/*
 * Finish function to extract the value out of the simple (lpm_all) pointer
 */
static gboolean appman_controller_lpm_all_applications_finish(
        AppmanController *self, GAsyncResult *result, GError **error) {
    GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT(result);

    g_return_val_if_fail(
            g_simple_async_result_is_valid(result, G_OBJECT(self),
                    appman_controller_lpm_all_applications_async), FALSE);

    if (g_simple_async_result_propagate_error(simple, error)) {
        return FALSE;
    }
    return g_simple_async_result_get_op_res_gssize(simple);
}

/* This function is used when the async LPM all is completed. It sets the
 * result and frees the memory allocated for the context
 */
static void _app_lpm_all_async_complete(AppmanLpmAllContext_t *ctx, gboolean timedout) {
    GSimpleAsyncResult *simple = ctx->pAsyncresult;

    O_DEBUG_ASSERT(simple);
    O_LOG_ENTER("\n");
    if (simple) {
        O_LOG_DEBUG("Application LPM Delay successful\n");
        g_simple_async_result_set_op_res_gssize(simple, ctx->maxDelay);
        g_simple_async_result_complete(simple);
        g_object_unref(simple);
        simple = NULL;
    }

    // remove timer
    if (ctx->timeout_id) {
        g_source_remove(ctx->timeout_id);
        ctx->timeout_id = 0;
    }

    if (!timedout) {
        if (ctx) {
            g_slice_free (AppmanLpmAllContext_t, ctx);
        }
    }

}

/*
 * callback function for lpm all timer.
 */
static gboolean _app_lpm_all_timeout_cb(AppmanLpmAllContext_t *ctx) {
    O_LOG_ENTER("\n");
#if !defined(NDEBUG)
    //PRINT_APP_STATE(ctx->controller);
#endif

    O_LOG_TRACE("Controller Timeout for LPM \n");

    if (ctx->controller) {
        _app_lpm_all_async_complete(ctx, 1);
    }
    O_LOG_EXIT("\n");
    return FALSE;
}

/*
 * The async call that iterates though all apps and sends out the request action signal
 */
void appman_controller_lpm_all_applications_async(AppmanController *self,
        GCancellable *cancellable, GAsyncReadyCallback callback,
        gpointer user_data) {

    GSimpleAsyncResult *all_simple;
    GError *error = NULL;

    O_LOG_ENTER("Sending LPM signal to all applications...\n");

    all_simple = g_simple_async_result_new(G_OBJECT(self), callback, user_data,
            appman_controller_lpm_all_applications_async);

    if (self->_priv->State != APPMAN_CONTROLLER_ACTIVE) {
        error = g_error_new(O_APPMAN_ERROR, O_APPMAN_ERROR_FAILED,
                "Appman Not Ready");
        g_simple_async_result_set_from_error(all_simple, error);
        g_clear_error(&error);

        g_simple_async_result_complete_in_idle(all_simple);
    } else {
        g_mutex_lock(self->_priv->pMutex);
        int numOfRunningApps = g_hash_table_size(
                self->_priv->pRunningApplications);
        g_mutex_unlock(self->_priv->pMutex);

        if (numOfRunningApps == 0) {
            g_simple_async_result_set_op_res_gssize(all_simple, 0);
            g_simple_async_result_complete(all_simple);
        } else {
            AppmanLpmAllContext_t *pContext;
            pContext = g_slice_new0(AppmanLpmAllContext_t);
            if (pContext != NULL) {
                pContext->controller = g_object_ref(self);
                pContext->pAsyncresult = g_object_ref(all_simple);
                pContext->runningApps = numOfRunningApps;
                //start the timer
                AppmanPolicyInfo *policy_info =
                        appman_config_get_policy_info(
                                appman_controller_get_default()->_priv->pAppmanConfigObject);
                guint lpm_timeout = APP_ALL_LPM_TIMEOUT;
                if (policy_info != NULL)
                    lpm_timeout = policy_info->lpm_all_timeout;

                pContext->timeout_id = g_timeout_add_seconds(lpm_timeout,
                        (GSourceFunc) _app_lpm_all_timeout_cb, pContext);
                g_hash_table_foreach(self->_priv->pRunningApplications,
                        (GHFunc) _appman_controller_lpm_helper, pContext);
            }
        }
    }

    g_object_unref(all_simple);
    O_LOG_EXIT("\n");
}

// extracts the value from simple/result.
static gssize appman_controller_lpm_finish(AppmanApplication *self,
        GAsyncResult *result, GError **error) {
    GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT(result);
    self->priv->lpm_info.simple = NULL;

    g_return_val_if_fail(
            g_simple_async_result_is_valid(result, G_OBJECT(self),
                    appman_application_lpm_async), FALSE);

    if (g_simple_async_result_propagate_error(simple, error)) {
        O_LOG_DEBUG("appman_application_stop_finish: Error  %s\n", (*error)->message);
        return -1;
    }

    return g_simple_async_result_get_op_res_gssize(simple);

} /* appman_controller_lpm_finish */

/*
 * The callback for each application 's async call that sent the signal to the apps
 */
static void _appman_controller_lpm_async_cb(GObject *object,
        GAsyncResult *result, gpointer user_data) {
    O_LOG_ENTER("\n");
    AppmanApplication *pApp = APPMAN_APPLICATION(object);
    AppmanLpmAllContext_t *pAppmanHelperData =
            (AppmanLpmAllContext_t *) user_data;
    GError *error = NULL;

    O_DEBUG_ASSERT(NULL != object);

    gssize defer = appman_controller_lpm_finish(pApp, result, &error);

    if (pAppmanHelperData->maxDelay < defer)
        pAppmanHelperData->maxDelay = defer;

    if (--pAppmanHelperData->runningApps <= 0) {
        O_LOG_DEBUG("  Last App  max defer time = %d \n ", pAppmanHelperData->maxDelay);
        _app_lpm_all_async_complete(pAppmanHelperData, 0);
    } else
        O_LOG_DEBUG("total running apps = %d \n ", pAppmanHelperData->runningApps);

    g_clear_error(&error);
    g_object_unref(pApp);
    O_LOG_EXIT("\n");
}

/*
 * Helper function to send lpm signal
 */
static void _appman_controller_lpm_helper(gpointer key, gpointer value,
        gpointer user_data) {
    AppmanApplication *pRunningApp = (AppmanApplication *) value;

    O_LOG_ENTER("Iterating App %p \n", pRunningApp);

    appman_application_lpm_async(g_object_ref(pRunningApp), NULL,
            _appman_controller_lpm_async_cb, user_data);
    O_LOG_EXIT("\n");
}



/**@}*/
