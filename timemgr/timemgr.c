 * ============================================================================
 */

/**
 * @file     timemgr.c
 * @author   Lianghe Li
 * @date     Feb 15, 2011
 */

#include <glib.h>
#include <dbus/dbus-glib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "timemgr_controller.h"
#include "timemgr.h"
#include "timemgr_internal.h"
// #include "outils.h"
// #include "osignal.h"
#include <sys/prctl.h>

#define TIMEMGR_DIR                         TIMEMGR_CONFIG_FOLDER
#define ACTIVE_SOURCES_KEYPATH              TIMEMGR_CONFIG_FOLDER"/activeSources"        /* Type: string    */
#define COLLECT_INTERVAL_SEC_KEYPATH        TIMEMGR_CONFIG_FOLDER"/collectIntervalSec"   /* Type: int       */
#define SNTP_SERVERS_KEYPATH                TIMEMGR_CONFIG_FOLDER"/sntpServers"          /* Type: string    */
#define PATCH_ZONE_INFO_KEYPATH             TIMEMGR_CONFIG_FOLDER"/patchZoneInfo"        /* Type: int       */
#define PREEMPT_TIME_SEC_KEYPATH            TIMEMGR_CONFIG_FOLDER"/preemptTimeSec"       /* Type: int       */


#define DEFAULT_ACTIVE_SOURCES              "DVB"
#define DEFAULT_COLLECT_INTERVAL_SEC        5
#define DEFAULT_SNTP_SERVERS                "172.22.1.1;nist1.aol-ca.truetime.com;ntp.sjtu.edu.cn"
#define DEFAULT_PATCH_ZONE_INFO             1
#define DEFAULT_PREEMPT_TIME_SEC            60

typedef struct
{
    pthread_mutex_t      mutex;
    int                  priority[TIME_SRC_MAX];/* Store priority. zero means inactive. */
    unsigned int         collect_interval_sec;
    unsigned int         preempt_time_sec;
    char                 sntp_servers[1 + MAX_SERVER * MAX_SERVER_CNT];
    int                  patch_zone_info;

}timemgr_config_t;

static timemgr_config_t         s_timemgr_config;

#define SRC_CONTROL_CMD_STOP    0
#define SRC_CONTROL_CMD_START   1
static int timemgr_source_control(int cmd)
{
    return 0;
}

static val2str_t s_src_names[] =
{
    {TIME_SRC_SNTP, "sntp"},
    {TIME_SRC_DVB,  "dvb"},
    {TIME_SRC_ARIB, "arib"},
    {TIME_SRC_ATSC, "atsc"},
    {TIME_SRC_TEST, "test"},
    {TIME_SRC_APP,  "app"},
    INVALID_PAIR
};

static int timemgr_parse_sources(const char *active_sources, int priority[TIME_SRC_MAX])
{
    int ret = -1;
    int i = 0;
    char buf[16*TIME_SRC_MAX];
    char *argv[TIME_SRC_MAX];
    int argc = 0;
    o_time_source_type_t src_type = TIME_SRC_MAX;

    memset(buf, 0, 16 * TIME_SRC_MAX);
    for(i=0;i<TIME_SRC_MAX;i++)
    {
        argv[i] = &buf[16*i];
    }

    /* Init all sources as inactive */
    for(i=0;i<TIME_SRC_MAX;i++)
    {
        priority[i] = 0;
    }

    argc = timemgr_split_string(active_sources, ";", argv, TIME_SRC_MAX, 16);
    if(argc<=0 || argc>TIME_SRC_MAX)
    {
        TIMEMGR_ERR("Failed to split string %s!\n", active_sources);
        ret = -1;
    }
    else
    {
        ret = 0;
    }

    if(0==ret)
    {
        for(i=0; i<argc; i++)
        {
            src_type = timemgr_str2val(s_src_names, argv[i]);
            if(src_type>=0 && src_type<TIME_SRC_MAX)
            {
                priority[src_type] = argc - i;
            }
            else
            {
                TIMEMGR_ERR("Invalid time source %s!\n", argv[i]);
            }
        }
    }
    return ret;
}

static gboolean emit_event(gpointer userdata)
{
    timemgr_send_event(TIMEMGR_EVENT_TIME_CHANGED);

    return TRUE;
}

static int timemgr_main()
{
    int ret = -1;
    GMainLoop* mainloop = NULL;
    int cfgclient_inited = 0;

    g_type_init();

    ret = timemgr_source_control(SRC_CONTROL_CMD_START);
    ENSURE(0 == ret);

    if(0 == ret)
    {
        TIMEMGR_STA("Timemgr initalization done!\n");

        mainloop = g_main_loop_new(NULL, FALSE);

        g_timeout_add(10000, emit_event, NULL);

		otvdbus_controller_run(NTVDBUS_CONTROLLER(timemgr_controller_get_default()), mainloop, TIMEMGR_ADDRESS);

        g_main_loop_unref(mainloop);
        g_object_unref((GObject *)timemgr_controller_get_default());
    }

    return ret;
}

/*********************************************
           Public Function
 *********************************************/
int timemgr_should_patch_zone_info(void)
{
    int path_zone_info = DEFAULT_PATCH_ZONE_INFO;
    pthread_mutex_lock(&s_timemgr_config.mutex);
    path_zone_info = s_timemgr_config.patch_zone_info;
    pthread_mutex_unlock(&s_timemgr_config.mutex);
    return path_zone_info;
}

const char * timemgr_src_name(o_time_source_type_t src_type)
{
    return timemgr_val2str(s_src_names, src_type);
}

int timemgr_get_preempt_time(void)
{
    int preempt_time = DEFAULT_PREEMPT_TIME_SEC;
    pthread_mutex_lock(&s_timemgr_config.mutex);
    preempt_time = s_timemgr_config.preempt_time_sec;
    pthread_mutex_unlock(&s_timemgr_config.mutex);
    return preempt_time;
}


int main(int argc, char** argv)
{
    return timemgr_main();
}
