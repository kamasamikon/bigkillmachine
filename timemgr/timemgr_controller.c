#include <pthread.h>
#include "timemgr.h"
#include "timemgr_controller.h"
#include "timemgr_internal.h"

static void timemgr_controller_class_init (TimemgrControllerClass *klass);
static void timemgr_controller_init       (TimemgrController *obj);
static void timemgr_controller_finalize   (GObject *obj);
static void timemgr_controller_dispose   (GObject *obj);

static gboolean timemgr_controller_handle_new_connection(TimemgrController *self , OtvdbusConnection* con);
static gboolean timemgr_controller_handle_connection_lost(TimemgrController *self ,  OtvdbusConnection* con);

G_DEFINE_TYPE (TimemgrController, timemgr_controller, NTVDBUS_TYPE_CONTROLLER);

/* private data for TimemgrController */
typedef struct
{
    guint reserve;

}TimemgrControllerPrivate;
/***************************************************
                 Static Members
 ***************************************************/
static TimemgrController *      s_timemgr_controller = NULL;
static OtvdbusControllerClass * s_parent_class = NULL;
static pthread_mutex_t          s_controller_mutex = PTHREAD_MUTEX_INITIALIZER;


static void timemgr_controller_class_init (TimemgrControllerClass *klass)
{
    GObjectClass *gobject_class;
    OtvdbusControllerClass *p_class;

    gobject_class = (GObjectClass*) klass;
    p_class = (OtvdbusControllerClass*) klass;

    s_parent_class            = g_type_class_peek_parent (klass);
    gobject_class->finalize = timemgr_controller_finalize;
    gobject_class->dispose = timemgr_controller_dispose;

    g_type_class_add_private (gobject_class, sizeof(TimemgrControllerPrivate));
    
    /* overriding base class functions to handle new connection & connection lost */
    p_class->handle_new_connection = (OtvdbusControllerHandleNewConnFn)timemgr_controller_handle_new_connection;
    p_class->handle_connection_lost = (OtvdbusControllerHandleNewConnFn)timemgr_controller_handle_connection_lost;

}

static void timemgr_controller_init (TimemgrController *obj)
{
}

static void timemgr_controller_dispose (GObject *obj)
{
    G_OBJECT_CLASS(s_parent_class)->dispose (obj);
}

static void timemgr_controller_finalize (GObject *obj)
{
    G_OBJECT_CLASS(s_parent_class)->finalize (obj);
}

TimemgrController* timemgr_controller_get_default (void)
{
    pthread_mutex_lock(&s_controller_mutex);
    if (s_timemgr_controller == NULL)
    {
        s_timemgr_controller =  (TimemgrController *)g_object_new(TIMEMGR_TYPE_CONTROLLER, NULL);
        O_ASSERT( NULL != s_timemgr_controller);
    }
    pthread_mutex_unlock(&s_controller_mutex);
    return s_timemgr_controller;
}

/* Timemgr implementation of handle new connection*/
static gboolean timemgr_controller_handle_new_connection(
    TimemgrController *self , 
    OtvdbusConnection* con)
{
    TIMEMGR_DBG("Timemgr controller handle new connection.\n");
    otvdbus_connection_register_object(con, TIMEMGR_PATH, g_object_new(timemgr_get_type(), NULL), self);
    return TRUE;
}

/* Timemgr implementation of handle  connection lost */
static gboolean timemgr_controller_handle_connection_lost(
    TimemgrController *self,  
    OtvdbusConnection* con)
{
    GObject* Object = NULL;
    TIMEMGR_DBG("Timemgr controller handle connection lost.\n");
    if (TRUE ==  otvdbus_connection_unregister_object(con, TIMEMGR_PATH, &Object))
    {
        O_ASSERT( NULL != Object);
        g_object_unref(Object);
    }
    return TRUE;
}


