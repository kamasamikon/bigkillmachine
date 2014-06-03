 * ============================================================================
 */

#include <pthread.h>
#include <glib.h>
#include <dbus/dbus-glib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "timemgr.h"
#include "timemgr_internal.h"
#include "timesrc_app_controller.h"
#include "outils.h"

/**********************************************************
 *                    DBus stuff                          *
 **********************************************************/
#define ATS_DBG(fmt, args...)   printf("[APP TIME SOURCE DBG] "fmt, ##args); TIMEMGR_DBG(fmt, ##args)
#define ATS_ERR(fmt, args...)   printf("[APP TIME SOURCE ERR] "fmt, ##args); TIMEMGR_ERR(fmt, ##args)

typedef struct
{
    int                     running;
    unsigned int            sleep_period_s;
    pthread_mutex_t         mutex;
    pthread_cond_t          cond;
    pthread_t               thread;

    GMainLoop *             mainloop;
    GMainContext *          context;
    TimesrcAppController *  controller;

    int                     update_cnt;

}app_src_t;

/**********************************************************
 *                    DBus stuff                          *
 **********************************************************/
#define APP_TIME_SOURCE_MAGIC       0xABCDEF01

typedef struct
{
    GObject                     parent;
    int                         magic;
} AppTimeSource;

typedef struct 
{
    GObjectClass                parent;
} AppTimeSourceClass;

void app_time_source_set_time_data(
    AppTimeSource* obj, 
    gint remote_utc, 
    gint system_utc, 
    DBusGMethodInvocation  *context);


#include "timesrc_app_ipc_server.h"

G_DEFINE_TYPE(AppTimeSource, app_time_source, G_TYPE_OBJECT)

/**********************************************************
 *                    Static Members                      *
 **********************************************************/
static app_src_t        s_app_src;
static pthread_mutex_t  s_app_src_mutex = PTHREAD_MUTEX_INITIALIZER;
static int              s_app_src_initialized = 0;

void app_time_source_set_time_data(
    AppTimeSource* obj, 
    gint remote_utc, 
    gint system_utc, 
    DBusGMethodInvocation  *context)
{
    int ret = -1;
    o_time_data_t time_data;
    time_t cur_utc = (time_t)0;

    REQUIRE(obj);
    REQUIRE(APP_TIME_SOURCE_MAGIC==obj->magic);

    if(obj && APP_TIME_SOURCE_MAGIC==obj->magic)
    {
        time_data.remote.tv_sec     = (time_t)remote_utc;
        time_data.remote.tv_usec    = 0;
        time_data.local.tv_sec      = (time_t)system_utc;
        time_data.local.tv_usec     = 0;
        cur_utc = time(NULL);

        ATS_DBG("+++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        ATS_DBG("remote_utc time    :   %s", asctime(gmtime(&time_data.remote.tv_sec)));
        ATS_DBG("system_utc time    :   %s", asctime(gmtime(&time_data.local.tv_sec)));
        ATS_DBG("current_utc time   :   %s", asctime(gmtime(&cur_utc)));

        ret = -1;
        ret = timemgr_update(TIME_SRC_APP, &time_data, NULL);
        if(TIMEMGR_RET_GLITCH==ret)
        {
            /* Confirm the glitch immediately to prevent source app recollect the time again */
            ret = timemgr_update(TIME_SRC_APP, &time_data, NULL);
        }
        
        if(TIMEMGR_RET_SUCCESS==ret)
        {
            ATS_DBG("Succeed to update time!\n");
        }
        else
        {
            ATS_ERR("Failed to update system time!\n");
            if(TIMEMGR_RET_ERR_SRC==ret)
            {
                ATS_ERR("       due to time source is not registerred!\n");
            }
            else if(TIMEMGR_RET_ERR_SRC_STATE==ret)
            {
                ATS_ERR("       due to time source is not running! Please modify config value /timemgr/activeSources to let it run first! \n");
            }
            else if(TIMEMGR_RET_ERR_SRC_PERM==ret)
            {
                ATS_ERR("       due to higher priority source has recently updated system time!\n");
            }
            else if(TIMEMGR_RET_ERR_PARAM==ret)
            {
                ATS_ERR("       due to the paramter (system_time) is invalid!\n");
            }
            else
            {
                ATS_ERR("       due to unknow reason! Please contact Lianghe.Li@nemotv.com\n");
            }
        }
        ATS_DBG("+++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    }
    dbus_g_method_return( context, ret );
}

static void app_time_source_init(AppTimeSource* obj)
{
    REQUIRE(obj);
    if(obj)
    {
        obj->magic = APP_TIME_SOURCE_MAGIC;
    }
}

static void app_time_source_class_init(AppTimeSourceClass* klass) 
{
    REQUIRE(klass);
    dbus_g_object_type_install_info(
        (app_time_source_get_type()), 
        &dbus_glib_app_time_source_object_info);    
}


/**********************************************************
 *          Time Source Internal Functions                *
 **********************************************************/

static int timesrc_app_start(unsigned int sleep_period_s)
{
    app_src_t * src = &s_app_src;
    pthread_mutex_lock(&src->mutex);
    src->sleep_period_s = sleep_period_s;
    src->running = 1;
    pthread_cond_signal(&src->cond);
    pthread_mutex_unlock(&src->mutex);
    return 0;
}

static int timesrc_app_stop(void)
{
    app_src_t * src = &s_app_src;
    pthread_mutex_lock(&src->mutex);
    src->sleep_period_s = 0;
    src->running = 0;
    pthread_cond_signal(&src->cond);
    pthread_mutex_unlock(&src->mutex);
    return 0;
}

static void* timesrc_app_main(void *arg)
{
    int ret = -1;
    app_src_t * src = (app_src_t *)arg;
    g_type_init();

    pthread_mutex_lock(&src->mutex);
    src->context = g_main_context_new ();
    ENSURE(src->context);
    ret = src->context ? 0 : -1;

    if(0==ret)
    {
        src->mainloop = g_main_loop_new(src->context, FALSE);
        ENSURE(src->mainloop);
        ret = src->mainloop ? 0 : -1;
    }

    if(0==ret)
    {
        src->controller = timesrc_app_controller_get_default();
        ENSURE(src->controller);
        ret = src->controller ? 0 : -1;
    }

    pthread_mutex_unlock(&src->mutex);
    
    if(0==ret)
    {
        otvdbus_controller_run(
            NTVDBUS_CONTROLLER(src->controller), 
            src->mainloop, 
            APP_TIME_SOURCE_ADDRESS);
    }

    if(src->controller)
    {
        g_object_unref(src->controller);
        src->controller = NULL;
    }

    if(src->mainloop)
    {
        g_main_loop_unref(src->mainloop);
        src->mainloop = NULL;
    }

    if(src->context)
    {
        g_main_context_unref(src->context);
        src->context = NULL;
    }

    src->thread = (pthread_t)0;
    return (NULL);
}


/**********************************************************
 *                    Public Function                     *
 **********************************************************/

/**
 * @brief   Init APP time source.
 *
 * @retval  0 for Success, non-0 for failure.
 *
 * @pre     
 * @post    
 *
 */
int timesrc_app_init(void)
{
    int ret = -1;
    app_src_t * src = &s_app_src;

    pthread_mutex_lock(&s_app_src_mutex);

    /* Nothing will be changed if reinit the sntp */
    if(s_app_src_initialized)
    {
        ret = 0;
    }
    else
    {
        ret = o_pmutex_create(&src->mutex, false);
        ENSURE(0 == ret);

        pthread_mutex_lock(&src->mutex);

        ret = pthread_cond_init(&src->cond, NULL);
        ENSURE(0 == ret);

        src->update_cnt = 0;

        ret = timemgr_register(
            TIME_SRC_APP, 
            timesrc_app_start, 
            timesrc_app_stop);
        ENSURE(0 == ret);

        src->thread = O_PTHREAD_CREATE_NO_EXIT(
            timesrc_app_main,
            src,
            8*1024 );
        ENSURE(src->thread);
        ret = src->thread ? 0 : -1;

        if(0==ret)
        {
            s_app_src_initialized = 1;
            ret = 0;
        }

        pthread_mutex_unlock(&src->mutex);
    }
    pthread_mutex_unlock(&s_app_src_mutex);    
    return ret;
}


