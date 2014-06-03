 * ============================================================================
 */

/**
 * @file     timemgr_updater.c
 * @author   Lianghe Li
 * @date     Feb 15, 2011
 * @brief    Time updater
 * @ingroup  TIMEMGR
 * This file implement time manager's core component: time updater (dispatcher)
 * Time updater manages all the time sources and serves time clients.
 * @{
 */
#include <errno.h>
#include <pthread.h>
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
// #include "configman_lib.h"

/**********************************************************
 *                 Config values                          *
 **********************************************************/
#define TIME_STATUS_KEYPATH                 "/timemgr/timeStatus"           /* Type: int       */


/**********************************************************
 *                 Time Source Managment                  *
 **********************************************************/

/**
 * Structure of time source.
 */
typedef struct
{
    int                     registerred;        /*!< The time source is registerred. */
    int                     running;            /*!< The time source has been started sucessfully. */
    int                     priority;           /*!< The priority of the time source */
    pthread_mutex_t         mutex;              /*!< Mutex to protect the time source structure*/

    int                     sleep_period_s;     /*!< Interval in second between each collecting */

    o_time_source_start_t   start;              /*!< Registreed start function */
    o_time_source_stop_t    stop;               /*!< Registreed stop function */

    int                     rebase_cnt;         /*!< Count of time rebasing to this source */
    o_time_data_t           prev_update_data;   /*!< The time value from the last update */
    struct timespec         prev_sync_timestamp;/*!< The time stamp when "time" was last collected. */

    int                     is_glitching;       /*!< Indicate the time source is in glitching state. */
    int                     glitching_delta;    /*!< Record the time data which was determined as a glitch */

}o_time_source_t;

typedef struct
{
    int                 running;
    int                 pending_request;
    pthread_mutex_t     mutex;
    pthread_cond_t      cond;
    pthread_t           thread;
    o_local_time_info_t ltinfo;

}o_dst_notifier_t;

/**********************************************************
 *                      Time Updater                      *
 **********************************************************/

/**
 * Timemgr treat a time update as glitch (which need 2nd update to confirm it) when
 *                   abs(delta) > GLITCH_SECONDS
 */
#define GLITCH_SECONDS          10

/**
 * When timemgr got time data 2nd from a glitching time source.
 * The new delta may be little different from the glitch_delta.
 */
#define DELTA_ALMOST_SAME(delta_a, delta_b)     (abs((delta_a)-(delta_b)) <= 1)

/*
 * Timemgr doesn't really adjust system time when:
 *                   abs(delta) <= DELTA_TO_IGNORE
 */
#define DELTA_TO_IGNORE         10

/**
 * Reject the too old time data which was collected MAX_TIME_DATA_DELAY seconds ago
 */
#define MAX_TIME_DATA_DELAY_SECONDS     (10)

/********************************************************************/
/*                        Static Members                            */
/********************************************************************/
static const char* g_time_event_names[TIMEMGR_EVENT_ENDDEF] =
{
    SIGNAL_NAME_TIME_CHANGED,
    SIGNAL_NAME_TIME_ZONE_CHANGED,
    SIGNAL_NAME_DAYLIGHT_SAVINGS_ON,
    SIGNAL_NAME_DAYLIGHT_SAVINGS_OFF
};


typedef struct
{
    GObject                     parent;
} Timemgr;

typedef struct
{
    GObjectClass                parent;
} TimemgrClass;

static gboolean timemgr_is_time_valid(Timemgr* obj, gboolean *valid, GError** error);

#include "timemgr_ipc_server.h"

G_DEFINE_TYPE(Timemgr, timemgr, G_TYPE_OBJECT)

/***********************************************
 *              STATIC VARIABLES               *
 ***********************************************/

/* Signals */
static guint                s_timemgr_signals[TIMEMGR_EVENT_ENDDEF];
static int                  s_timemgr_signal_created = 0;

/* All the time sources */
static o_time_source_t      s_timesrc[TIME_SRC_MAX];

/* Last time source who rebase system time. */
static o_time_source_t *    s_rebasing_src = NULL;

/* Count of system time rebasing */
static int                  s_rebasing_cnt = 0;

/* This flag indicates if the system time is valid. Non-0 means valid. */
static int                  s_time_is_valid = 0;

/* Following member store the current local times zone info */
static o_dst_notifier_t     s_dst_notifier = {0, 0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0};

/* Mutex and flag to prevent static variables be re-initialized */
static pthread_mutex_t      s_mutex = PTHREAD_MUTEX_INITIALIZER;
static int                  s_initialized = 0;

/**
 * @brief   Check if RTC is avaiable in system
 *
 * @param   None
 * @retval  0       RTC device is avaiable in system
 * @retval  non-0   RTC device is not avaiable in system
 *
 * @pre     None
 * @post    None
 *
 * @par Description
 * Check RTC to decide if the system time is valid after bootup.
 */
static int timemgr_is_rtc_available()
{
    return 0;
}

/**
 * Init function for TimeUpdater object
 */
static void timemgr_init(Timemgr* tu)
{
    O_ASSERT(tu != NULL);
}

/**
 * Init function for TimeUpdater class
 */
static void timemgr_class_init(TimemgrClass* klass)
{
    int i;
    O_ASSERT(klass != NULL);

    for (i = 0; i < TIMEMGR_EVENT_ENDDEF; i++)
    {
        s_timemgr_signals[i] = g_signal_new(
            g_time_event_names[i],          /* str name of the signal */
            G_OBJECT_CLASS_TYPE(klass),     /* GType to which signal is bound to */
            (G_SIGNAL_RUN_LAST),
            0,
            NULL,
            NULL,
            g_cclosure_marshal_VOID__UINT,
            G_TYPE_NONE,
            1,
            G_TYPE_UINT);
        TIMEMGR_DBG("TimeUpdaterSignal created [%s=%d]\n", g_time_event_names[i], s_timemgr_signals[i]);
    }
    s_timemgr_signal_created = 1;

    dbus_g_object_type_install_info(
        (timemgr_get_type()),
        &dbus_glib_timemgr_object_info);
}

/**
 * Query if system time is valid.
 */
static gboolean timemgr_is_time_valid(
    Timemgr* tu,
    gint *valid,
    GError** error)
{
    TIMEMGR_TRC("is_time_valid() called!\n");
    *valid = s_time_is_valid;
    return TRUE;
}

/**
 * @brief   Update the time status (valid or invalid)
 *
 * @param[in]   isTimeValid
 * @retval      0     Success
 * @return      non-0 Failure
 *
 * @pre  None
 * @post None
 *
 * @par Description
 * To be called by when time status changed.
 */
static int timemgr_update_status(int isTimeValid)
{
    return 0;
}

/**
 * @brief   Rebase system time based on a time source
 *
 * @param   type [in]   Time source class indicate time source causing the rebasing
 * @param   data [in]   Time data used to rebasing system time
 *
 * @retval  0       Success
 * @retval  non-0   Failure
 *
 * @pre
 * @post    The system time is updated and TIME_CHANGED event was broadcasted if success.
 *
 */
static int timemgr_rebase_sys_time(o_time_source_type_t type, o_time_data_t * data)
{
    int ret = 0;
    o_time_t delta;
    o_time_t cur;
    o_time_t set_to;

    timersub(&data->remote, &data->local, &delta);

    if( abs(delta.tv_sec) <= DELTA_TO_IGNORE)
    {
		return 0;
	}

    TIMEMGR_DBG("[Remote Time]: %s", asctime(gmtime(&data->remote.tv_sec)));
    TIMEMGR_DBG("[System Time]: %s", asctime(gmtime(&data->local.tv_sec)));
    TIMEMGR_DBG("[Delta      ]: %d seconds\n", (int)delta.tv_sec );

    gettimeofday(&cur, (o_time_zone_t *)NULL);
    timeradd(&cur, &delta, &set_to);

    TIMEMGR_STA("Timemgr is to rebased system time to %s time at %s",
        timemgr_src_name(type), asctime(gmtime(&cur.tv_sec)));

    ret = settimeofday(&set_to, (o_time_zone_t *)NULL);
    if(ret)
    {
        TIMEMGR_ERR("settimeofday() return %d. errno=%d. Failed to set time to %s",
            ret, errno, asctime(gmtime(&set_to.tv_sec)));
    }
    else
    {
        TIMEMGR_STA("New system time is: %s", asctime(gmtime(&set_to.tv_sec)));
    }

    if(0==ret)
    {
        if(0==s_time_is_valid)
        {
            s_time_is_valid = 1;
            timemgr_update_status(s_time_is_valid);
        }
        else
        {
            /* Adjust system time when time is valid. Print an error message here. */
            TIMEMGR_ERR("System time has been adjusted %ld seconds!\n", (long)delta.tv_sec);
        }
        timemgr_send_event(TIMEMGR_EVENT_TIME_CHANGED);
    }

#ifdef DEBUG_TIME_JUMP
    {
        static pthread_mutex_t s_lock = PTHREAD_MUTEX_INITIALIZER;
        static struct tm s_tm;
        static char s_str[64];
        static struct timespec s_ts;
        static int s_first_time = 1;

        pthread_mutex_lock(&s_lock);
        if(0==clock_gettime(CLOCK_MONOTONIC, &s_ts))
        {
            gmtime_r(&s_ts.tv_sec, &s_tm);
            asctime_r(&s_tm, s_str);
            s_str[strlen(s_str)-1] = 0;//delete the newline
            printf("[%s] System time has been adjusted %+ld seconds!\n", s_str, (long)delta.tv_sec);

            FILE * fp = fopen("/root/time_jump.log", "a");
            if(fp)
            {
                if(s_first_time)
                {
                    fprintf(fp, "------------------------------------------\n");
                    s_first_time = 0;
                }
                fprintf(fp, "[%s] System time has been adjusted %+ld seconds!\n", s_str, (long)delta.tv_sec);
                fclose(fp);
            }
        }
        pthread_mutex_unlock(&s_lock);
    }
#endif

    return ret;
}

/**
 * @brief   Check if time source is in the position to change system time
 *
 * @param   type [in] Time source class where the time data is collected from
 *
 * @retval  0       The time source can change system time. It is the highest source or all higher source are bad
 * @retval  Non-0   The time source can not change system time.
 *
 * @pre
 * @post
 *
 */
static int timemgr_check_source(o_time_source_type_t type)
{
    int i;
    int ok_to_change;
    o_time_source_t * src = NULL;
    int priority = s_timesrc[type].priority;
    struct timespec cur;
    int seconds_since_last_update = 0;

    ok_to_change = 1;

    for(i=0;i<TIME_SRC_MAX;i++)
    {
        if(i==type)
        {
            /* Skip the source itself */
            continue;
        }

        src = &s_timesrc[i];

        /* CHECK IF THERE IS TIME SOURCE WITH HIGHER PRIORITY HAS RECENTLY REBASED SYSTEM TIME */
        if(src->registerred && (src->priority>priority) && (src->rebase_cnt>0) )
        {
            /* Found a source with higher priority, so check if it is recently updated */
            clock_gettime( CLOCK_MONOTONIC, &cur );
            seconds_since_last_update = cur.tv_sec - src->prev_sync_timestamp.tv_sec;

            if(seconds_since_last_update < timemgr_get_preempt_time())
            {
                /* The higher source has updated recently, so
                 * it is his job to update system time.
                 * So current update should be rejected. */
                ok_to_change = 0;
                break;
            }
        }
    }

    return ok_to_change ? 0 : (-1);
}

/**
 * @brief   Check if the time data can be accepted
 *
 * @param   type [in] Time source class where the time data is collected from
 * @param   data [in] Time data collected from time source
 *
 * @retval  0       The time data can be accepted to rebase the system time
 * @retval  non-0   The time data is glitch and should be discarded.
 *
 * @pre
 * @post
 *
 */
static int timemgr_check_time_data(o_time_source_type_t type, o_time_data_t *data)
{
    o_time_source_t * src = &s_timesrc[type];
    int delta = DELTA_SEC(data);
    time_t cur_utc = (time_t)0;
    int seconds_since_collect = 0;

    /* The collecting time should be passed and not far from right now. */
    cur_utc = time(NULL);
    seconds_since_collect = (int)(cur_utc - data->local.tv_sec);
    if(seconds_since_collect<0 || seconds_since_collect>MAX_TIME_DATA_DELAY_SECONDS)
    {
        /* 1. Reject the time data collected at future time.
           2. Reject the time data which is too old.*/
        TIMEMGR_ERR("Invalid time_data! seconds_since_collect=%d!", seconds_since_collect);
        return TIMEMGR_RET_ERR_PARAM;
    }

    /* Check if glitch happens. */
    if( abs(delta) > GLITCH_SECONDS && src->rebase_cnt>0 )
    {
        /* 1. data is a glitch if
         * THE SYSTEM HAS REBASED TO THIS SOURCE BUT A BIG DELTA STILL EXIST */

        if(src->is_glitching)
        {
            /* 1.1 Already in glitch state */

            if(DELTA_ALMOST_SAME(delta, src->glitching_delta))
            {
                /* 1.1.1 glitching_delta happens more than 1 time, accept the new time. */
                TIMEMGR_DBG("Accept the glitch! delta=%d!\n", delta);
                src->is_glitching = 0;
                src->glitching_delta = 0;
            }
            else
            {
                /* 1.1.2 Already in glitch state but glitch delta changed, update glitch_delta! */
                TIMEMGR_DBG("Glitch changed! old_delta=%d new_delta=%d!\n", src->glitching_delta, delta);
                src->glitching_delta = delta;
            }
        }
        else
        {
            /* 1.2 This is a new glitch */
            TIMEMGR_DBG("Found new glitch...delta=%d\n", delta);
            src->is_glitching = 1;
            src->glitching_delta = delta;
        }
    }
    else
    {
        /* 2. data is valid */
        src->is_glitching = 0;
        src->glitching_delta = 0;
    }

    return (src->is_glitching) ? TIMEMGR_RET_GLITCH: 0;
}

/**********************************************************
 *                    DST notifier                    *
 **********************************************************/

/**
 * @brief   Entry function for thread to send DST ON|OFF event
 *
 * @param   arg     Pointer to a structure of o_dst_notifier_t
 *
 * @retval  always NULL
 */
static void * timemgr_dst_notifier_thread(void *arg)
{
    int             ret = -1;
    o_dst_notifier_t *ctx = (o_dst_notifier_t *)arg;
    struct timespec target_ts;
    struct timespec cur_ts;
    time_t cur_utc = (time_t)0;
    timemgr_event_t next_evt = TIMEMGR_EVENT_ENDDEF;

    if(NULL==ctx)
    {
        return NULL;
    }

    pthread_mutex_lock(&ctx->mutex);
    while(ctx->running)
    {
        while(0==ctx->pending_request)
        {
            pthread_cond_wait(&ctx->cond, &ctx->mutex);
        }

        ctx->pending_request = 0;

        /* Update local variables */
        target_ts.tv_sec = 0;
        target_ts.tv_nsec = 0;
        next_evt = TIMEMGR_EVENT_ENDDEF;
        cur_utc = time(NULL);

        ret = timemgr_get_ltinfo(cur_utc, &ctx->ltinfo);
        ENSURE(0==ret);
        if(ret)
        {
            continue;
        }

        if(1==ctx->ltinfo.is_dst)
        {
            next_evt = TIMEMGR_EVENT_DAYLIGHT_SAVINGS_OFF;
            target_ts.tv_sec = ctx->ltinfo.next_trans_utc;
        }
        else if(0==ctx->ltinfo.is_dst)
        {
            next_evt = TIMEMGR_EVENT_DAYLIGHT_SAVINGS_ON;
            target_ts.tv_sec = ctx->ltinfo.next_trans_utc;
        }
        else if(-1==ctx->ltinfo.is_dst)
        {
            TIMEMGR_DBG("DST is not used in current zone! No DST events to send.\n");
            continue;
        }
        else
        {
            TIMEMGR_ERR("Bad is_dst value!\n");
            ENSURE(0);
            continue;
        }

        ret = clock_gettime(CLOCK_REALTIME, &cur_ts);
        ENSURE(0 == ret);

        if(cur_ts.tv_sec >= target_ts.tv_sec)
        {
            timemgr_send_event(next_evt);
        }
        else
        {
            TIMEMGR_DBG("Timemgr will send event %s at %s",
                g_time_event_names[next_evt],
                asctime(gmtime(&target_ts.tv_sec)));

            ret = pthread_cond_timedwait(&ctx->cond, &ctx->mutex, &target_ts);
            if(ETIMEDOUT==ret)
            {
                ret = clock_gettime(CLOCK_REALTIME, &cur_ts);
                ENSURE(0 == ret);

                if(cur_ts.tv_sec >= target_ts.tv_sec)
                {
                    timemgr_send_event(next_evt);
                }
            }
        }
    }

    ctx->running = 0;
    ctx->thread = 0;

    pthread_mutex_unlock(&ctx->mutex);
    return NULL;
}

/* Convert a timeoffset to a local time info structure */
static int timemgr_convert_timeoffset_to_ltinfo(
    time_t                  query_utc,
    o_time_offset_t *       offset,
    o_local_time_info_t *   ltinfo )
{
    int ret = -1;
    int cur_offset_to_utc_in_sec = 0;
    int next_offset_to_utc_in_sec = 0;

    if(NULL==ltinfo)
    {
        return -1;
    }

    ltinfo->cur_utc = query_utc;

    cur_offset_to_utc_in_sec = offset->offset1_to_utc;
    next_offset_to_utc_in_sec = offset->offset2_to_utc;

    if(cur_offset_to_utc_in_sec  < next_offset_to_utc_in_sec)
    {
        /* At time_of_change local time become bigger (later) means the switching is a DST_ON.
         * US/Pacific           Mar     gmtoff=-28800   ==> gmtoff=-25200
         * Europe/Paris         Mar     gmtoff=3600     ==> gmtoff=7200
         * Chile/Continental    Oct     gmtoff=-14400   ==> gmtoff=-10800
         */
        if( query_utc < offset->time_of_change.tv_sec )
        {
            /* It is standard time before the time_of_change */
            ltinfo->is_dst = 0;
            ltinfo->next_trans_utc = offset->time_of_change.tv_sec;
            ltinfo->offset = cur_offset_to_utc_in_sec;
            ltinfo->next_offset = next_offset_to_utc_in_sec;
            ltinfo->prev_trans_utc = 0;
            ltinfo->prev_offset = 0;
        }
        else
        {
            /* It is daylight saving time after the time_of_change */
            ltinfo->is_dst = 1;
            ltinfo->offset = next_offset_to_utc_in_sec;
            ltinfo->next_trans_utc = 0;/* next_change is not avaviable in time_offset */
            ltinfo->next_offset = 0;
            ltinfo->prev_trans_utc = offset->time_of_change.tv_sec;
            ltinfo->prev_offset = cur_offset_to_utc_in_sec;
        }
        ret = 0;
    }
    else if(cur_offset_to_utc_in_sec  > next_offset_to_utc_in_sec)
    {
        /* At time_of_change local time become smaller (eariler) means the switching is a DST_OFF.
         * US/Pacific           Nov     gmtoff=-25200   ==> gmtoff=-28800
         * Europe/Paris         Oct     gmtoff=7200     ==> gmtoff=3600
         * Chile/Continental    Mar     gmtoff=-10800   ==> gmtoff=-14400
         */
        if( query_utc < offset->time_of_change.tv_sec )
        {
            /* It is daylight saving time before the time_of_change */
            ltinfo->is_dst = 1;
            ltinfo->offset = cur_offset_to_utc_in_sec;
            ltinfo->next_trans_utc = offset->time_of_change.tv_sec;
            ltinfo->next_offset = next_offset_to_utc_in_sec;
            ltinfo->prev_trans_utc = 0;
            ltinfo->prev_offset = 0;
        }
        else
        {
            /* It is standard time after the time_of_change */
            ltinfo->is_dst = 0;
            ltinfo->offset = next_offset_to_utc_in_sec;
            ltinfo->next_trans_utc = 0;/* next_change is not avaviable in time_offset */
            ltinfo->next_offset = 0;
            ltinfo->prev_trans_utc = offset->time_of_change.tv_sec;
            ltinfo->prev_offset = cur_offset_to_utc_in_sec;
        }
        ret = 0;
    }
    else
    {
        /* There is no DST policy */
        ltinfo->is_dst = -1;
        ltinfo->offset = cur_offset_to_utc_in_sec;
        ltinfo->next_trans_utc = 0;/* next_change is not avaviable in time_offset */
        ltinfo->next_offset = 0;
        ltinfo->prev_trans_utc = 0;
        ltinfo->prev_offset = 0;
        ret = 0;
    }
    TIMEMGR_TRC("TOT translated zone info: <---------------[%d:%d][%d:%d:%d][%d:%d]------------------>\n",
        (int)ltinfo->prev_trans_utc, ltinfo->prev_offset,
        (int)ltinfo->cur_utc, ltinfo->offset, ltinfo->is_dst,
        (int)ltinfo->next_trans_utc, ltinfo->next_offset);
    return ret;
}

#define MAX_OFFSET                              (12 * HOUR_SEC)
#define MAX_OFFSET_DIFF                         (2 * HOUR_SEC)
/* Return 0 if the ltinfo is valid. */
static int timemgr_is_valid_ltinfo(o_local_time_info_t *ltinfo)
{
    int ret = -1;
    int cur_offset = 0;
    int another_offset = 0;
    if(ltinfo)
    {
        ret = 0;
        if(-1==ltinfo->is_dst)
        {
            if( (ltinfo->next_trans_utc > 0) ||
                (ltinfo->prev_trans_utc > 0) )
            {
                TIMEMGR_ERR("Invalid ltinfo for non-DST region!\n");
                ret = -1;
            }
            cur_offset = another_offset = ltinfo->offset;
        }
        else if(1==ltinfo->is_dst || 0==ltinfo->is_dst)
        {
            cur_offset = ltinfo->offset;
            if(ltinfo->next_trans_utc > 0)
            {
                another_offset = ltinfo->next_offset;
            }
            else if(ltinfo->prev_trans_utc > 0)
            {
                another_offset = ltinfo->prev_offset;
            }
            else
            {
                TIMEMGR_ERR("Invalid ltinfo for DST-used region!\n");
                ret = -1;
            }
        }
        else
        {
            TIMEMGR_ERR("Invalid is_dst sign!\n");
            ret = -1;
        }

        if(0==ret)
        {
            if( (abs(cur_offset) > MAX_OFFSET) ||
                (abs(another_offset) > MAX_OFFSET) )
            {
                TIMEMGR_ERR("Invalid time_offset!\n");
                ret = -1;
            }
        }

        if((0==ret) && (1==ltinfo->is_dst || 0==ltinfo->is_dst))
        {
            if( abs(cur_offset - another_offset) > MAX_OFFSET_DIFF )
            {
                TIMEMGR_ERR("Invalid time_offset for DST-used region! offset1=%d(hour) offset2=%d(hour)\n",
                    cur_offset/HOUR_SEC,
                    another_offset/HOUR_SEC);
                ret = -1;
            }
        }
    }
    return ret;
}

/**
 * @brief   Process time offset data
 *
 * @param   type    [in] Time source class where the time offset data is collected from
 * @param   remote_utc [in] Time data collected from time source
 * @param   offset  [in] Time offset collected from time source
 *
 * @retval  0       Success
 * @retval  non-0   Failure
 *
 * @pre
 * @post
 *
 */
static int timemgr_process_time_offset(
    o_time_source_type_t    type,
    time_t                  remote_utc,
    o_time_offset_t *       offset)
{
    int ret = -1;
    o_local_time_info_t     ltinfo;

    if(timemgr_should_patch_zone_info())
    {
        ret = timemgr_convert_timeoffset_to_ltinfo(remote_utc, offset, &ltinfo);
        ENSURE(0==ret);

        if(0==ret)
        {
            ret = timemgr_is_valid_ltinfo(&ltinfo);
            if(0==ret)
            {
                ret= 0; // timemgr_check_and_patch_zoneInfo(&ltinfo);
                TIMEMGR_TRC("timemgr_check_and_patch_zoneInfo() returns %d!\n", ret);
            }
            else
            {
                TIMEMGR_TRC("Ignore an invalid TOT local time offset descriptor!\n");
                ret = 0;
            }
        }
    }
    else
    {
        /* Time zone information:
            1. local time offset
            2. DST changing time
           are decided by the zone information file.
           So we ignore the local time offset descriptor in TOT. */
        ret = 0;
    }
    return ret;
}

/**
 * @brief   Process time data
 *
 * @param   type      [in] Time source class where the time offset data is collected from
 * @param   time_data [in] Pointer to time data.
 *
 * @retval  0       Success to update the system time by time_data
 * @retval  non-0   System time haven't been updated
 *
 * @pre
 * @post
 *
 */
static int timemgr_process_time_data(o_time_source_type_t src_type, o_time_data_t *time_data)
{
    int ret = -1;
    o_time_source_t * src = NULL;

    REQUIRE(src_type < TIME_SRC_MAX);
    src = &(s_timesrc[src_type]);

    ret = timemgr_check_time_data(src_type, time_data);
    if(0==ret)
    {
        if(0==timemgr_check_source(src_type))
        {
            /* If the source is in the position to update system time */
            if(0==timemgr_rebase_sys_time(src_type, time_data))
            {
                src->rebase_cnt++;
                memcpy(&src->prev_update_data, time_data, sizeof(o_time_data_t));
                clock_gettime(CLOCK_MONOTONIC, &src->prev_sync_timestamp);
                s_rebasing_src = src;
                s_rebasing_cnt++;
                ret = TIMEMGR_RET_SUCCESS;
            }
            else
            {
                ret = TIMEMGR_RET_FAIL;
            }
        }
        else
        {
            ret = TIMEMGR_RET_ERR_SRC_PERM;
            TIMEMGR_DBG("Timemgr cannot accept time update from source (type:%s)!\n", timemgr_src_name(src_type));
        }
    }
    else
    {
        TIMEMGR_DBG("Invalid time data! Time source should re-collect!\n");
    }
    return ret;
}

/**
 * @brief   Wake up dst notifier thread to recheck DST_ON|OFF event
 *
 * @param   user_data           NULL
 */
static gboolean timemgr_dst_notifier_schedule(gpointer user_data)
{
    pthread_mutex_lock(&s_dst_notifier.mutex);
    s_dst_notifier.pending_request = 1;
    pthread_cond_signal(&s_dst_notifier.cond);
    pthread_mutex_unlock(&s_dst_notifier.mutex);
    return FALSE;
}

static void timemgr_dst_notifier_reschedule(void)
{
    g_idle_add( timemgr_dst_notifier_schedule, NULL);
}

/**********************************************************
 *                Time Manager Public Function            *
 **********************************************************/

/**
 * @brief   Init static members
 *
 * @param   None
 *
 * @retval  None
 *
 * @pre     This function should be called before any other ones in this module
 * @post    Static members are initialized and other function can be called
 *
 */
void timemgr_init_static_variables(void)
{
    int ret = -1;
    time_t cur_utc = (time_t)0;
    pthread_t pid;

    pthread_mutex_lock(&s_mutex);
    if(!s_initialized)
    {
        cur_utc = time(NULL);
        if(0==timemgr_is_rtc_available())
        {
            s_time_is_valid = 1;
        }
        else
        {
            TIMEMGR_ERR("RTC is not available in system!\n");
            s_time_is_valid = 0;
        }

        TIMEMGR_STA("[System Time]         : %s", asctime(gmtime(&cur_utc)));
        TIMEMGR_STA("[Initial time status] : %s", s_time_is_valid ? "Valid":"Invalid");

        ret = timemgr_update_status(s_time_is_valid);
        ENSURE(0 == ret);

        pthread_mutex_lock(&s_dst_notifier.mutex);
        s_dst_notifier.pending_request = 0;
        s_dst_notifier.running = 1;
        s_dst_notifier.thread = pthread_create(&pid, NULL, timemgr_dst_notifier_thread, &s_dst_notifier);
        ENSURE(s_dst_notifier.thread);
        pthread_mutex_unlock(&s_dst_notifier.mutex);

        if(s_time_is_valid)
        {
            /* Reschedule DST notifier if time is valid */
            timemgr_dst_notifier_reschedule();
        }

        s_initialized = 1;
    }
    pthread_mutex_unlock(&s_mutex);
}

/**
 * @brief   Register a time source
 *
 * @param   src_type [in] class of the source to be registerred
 * @param   start [in] start function of the source to be registerred
 * @param   stop [in] stop function of the source to be registerred
 *
 * @retval  0     Success
 * @retval  non-0 Failure
 *
 * @pre     None
 * @post    None
 *
 */
int timemgr_register(
    o_time_source_type_t    src_type,
    o_time_source_start_t   start,
    o_time_source_stop_t    stop )
{
    int ret = -1;
    o_time_source_t * src = NULL;

    REQUIRE(src_type < TIME_SRC_MAX);

    src = &(s_timesrc[src_type]);
    if(!src->registerred)
    {
        ret = pthread_mutex_init(&src->mutex, NULL);
        ENSURE(0 == ret);

        pthread_mutex_lock(&src->mutex);
        src->start = start;
        src->stop = stop;
        src->priority = 0;
        src->registerred = 1;
        src->running = 0;
        src->rebase_cnt = 0;
        memset(&src->prev_update_data, 0, sizeof(o_time_data_t));
        memset(&src->prev_sync_timestamp, 0, sizeof(struct timespec));
        src->is_glitching = 0;
        src->glitching_delta = 0;
        pthread_mutex_unlock(&src->mutex);
        ret = 0;
    }
    else
    {
        TIMEMGR_ERR("Time source (type:%s) already been registerred!\n", timemgr_src_name(src_type));
    }
    return ret;
}

/**
 * @brief   Start time source
 *
 * @param   src_type        [in] Which source to start
 * @param   priority        [in] The priority of the source to start.
 * @param   sleep_period_s  [in] The interval in seconds between each collection.
 *
 * @retval  0     Success
 * @retval  non-0 Failure
 *
 * @pre     The time source is registerred and not running
 * @post    The time source is running if success
 *
 */
int timemgr_start(
    o_time_source_type_t src_type,
    int priority,
    unsigned int sleep_period_s)
{
    int ret = -1;
    o_time_source_t *src = NULL;

    REQUIRE(src_type < TIME_SRC_MAX);

    src = &s_timesrc[src_type];

    pthread_mutex_lock(&src->mutex);
    if(src->registerred)
    {
        if(!src->running)
        {
            ret = src->start(sleep_period_s);
            if(ret==0)
            {
                src->running = 1;
                src->priority = priority;
                src->sleep_period_s = sleep_period_s;
                TIMEMGR_DBG("Succeed to start time source (type:%s).\n", timemgr_src_name(src_type));
            }
            else
            {
                TIMEMGR_ERR("Failed to start time source (type:%s).\n", timemgr_src_name(src_type));
            }
        }
        else
        {
            TIMEMGR_ERR("Time source (type:%s) haven't been registerred!\n", timemgr_src_name(src_type));
        }
    }
    pthread_mutex_unlock(&src->mutex);
    return ret;
}

/**
 * @brief   Stop the time source
 *
 * @param   src_type [in] Time source to stop
 *
 * @retval  0     Success
 * @retval  non-0 Failure
 *
 * @pre     The time source is registerred and running
 * @post    The time source stopped if success
 *
 */
int timemgr_stop(o_time_source_type_t src_type)
{
    int ret = -1;
    o_time_source_t *src = NULL;

    ENSURE(src_type < TIME_SRC_MAX);

    src = &s_timesrc[src_type];
    pthread_mutex_lock(&src->mutex);
    if(src->registerred)
    {
        if(src->running)
        {
            ret = src->stop();
            if(ret==0)
            {
                src->running = 0;
                TIMEMGR_DBG("Succeed to stop time source (type:%s).\n", timemgr_src_name(src_type));
            }
            else
            {
                TIMEMGR_ERR("Failed to stop time source (type:%s).\n", timemgr_src_name(src_type));
            }
        }
    }
    else
    {
        TIMEMGR_ERR("Time source (type:%s) haven't been registerred!\n", timemgr_src_name(src_type));
    }
    pthread_mutex_unlock(&src->mutex);
    return ret;
}

/**
 * @brief   Process time update from specific time source.
 *
 * @param   src_type [in] Time source class indicate time source causing the rebasing
 * @param   time_data [in] Pointer to time data.
 * @param   time_offset [in] Pointer to time offset data. NULL indicates no time offset data.
 *
 * @retval  TIMEMGR_RET_SUCCESS         Success
 * @retval  TIMEMGR_RET_FAIL            Failure, Time source will re-collect time data again immediately
 * @retval  TIMEMGR_RET_ERR_PARAM       Invalid parameter
 * @retval  TIMEMGR_RET_ERR_SRC         Time source is not registerred
 * @retval  TIMEMGR_RET_ERR_SRC_STATE   Time source is not started
 * @retval  TIMEMGR_RET_ERR_PERM        Higher priority source has recently updated the time
 * @retval  TIMEMGR_RET_GLITCH          The time change is a glitch, the source should recollect time again
 *
 * @pre
 * @post    The system time is updated and TIME_CHANGED event was broadcasted if success.
 *
 */
int timemgr_update(
    o_time_source_type_t    src_type,
    o_time_data_t           *time_data,
    o_time_offset_t         *time_offset)
{
    int ret = 0;
    o_time_source_t *src = NULL;
    time_t remote_utc = (time_t)0;

    ENSURE(src_type < TIME_SRC_MAX);

    src = &s_timesrc[src_type];

    pthread_mutex_lock(&src->mutex);
    if(src->registerred)
    {
        if(src->running)
        {
            if(time_data)
            {
                remote_utc = time_data->remote.tv_sec;
                ret = timemgr_process_time_data(src_type, time_data);
            }

            if(0==ret && time_offset)
            {
                /* time_data is accepted. Then process time_offset together with time_data. */
                ret = timemgr_process_time_offset(src_type, remote_utc, time_offset);
            }
        }
        else
        {
            ret = TIMEMGR_RET_ERR_SRC_STATE;
            TIMEMGR_ERR("Time source (type:%s) haven't been started!\n", timemgr_src_name(src_type));
        }
    }
    else
    {
        ret = TIMEMGR_RET_ERR_SRC;
        TIMEMGR_ERR("Time source (type:%s) haven't been registerred!\n", timemgr_src_name(src_type));
    }
    pthread_mutex_unlock(&src->mutex);
    return ret;
}

/**
 * @brief   Broadcast time manager event on DBus.
 *
 * @param   evt [in] Event to send
 *
 * @retval  none
 *
 * @pre
 * @post    The event was braodcast through DBUS
 *
 */
void timemgr_send_event(timemgr_event_t evt)
{
    REQUIRE(evt < TIMEMGR_EVENT_ENDDEF);

    if(s_timemgr_signal_created)
    {
        otvdbus_controller_send_signal_on_all_connections(
            (OtvdbusController *)timemgr_controller_get_default(),
            TIMEMGR_PATH,
            s_timemgr_signals[evt],
            0,
            evt);
        TIMEMGR_DBG("Signal %d (%s) has been sent!\n", s_timemgr_signals[evt], g_time_event_names[evt]);
    }
    else
    {
        TIMEMGR_DBG("Event (%s) has not been sent because nobody connected to time manager!\n", g_time_event_names[evt]);
    }

    timemgr_dst_notifier_reschedule();
}
