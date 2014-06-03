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
// #include "outils.h"

typedef struct
{
    int                     running;
    unsigned int            sleep_period_s;
    pthread_mutex_t         mutex;
    pthread_cond_t          cond;
    pthread_t               thread;

    int                     update_cnt;

}test_src_t;

/**********************************************************
 *                    Static Members                      *
 **********************************************************/
static test_src_t       s_test;
static pthread_mutex_t  s_test_mutex = PTHREAD_MUTEX_INITIALIZER;
static int              s_test_initialized = 0;


/**********************************************************
 *          Time Source Internal Functions                *
 **********************************************************/

/* Main function for TEST time source */
static void * timesrc_test_main(void *arg)
{
    int ret = 0;
    test_src_t * src = (test_src_t *)arg;
    o_time_data_t td;
    int time_updated = 0;
    o_time_offset_t to;

    while(1)
    {
        time_updated = 0;

        pthread_mutex_lock(&src->mutex);
        while(!src->running)
        {
            pthread_cond_wait(&src->cond, &src->mutex);
        }

        gettimeofday(&td.local, (struct timezone *)NULL);
        td.remote.tv_sec = timemgr_parse_utc("2012-11-07_23:59:30");
        td.remote.tv_usec = 0;

        to.offset1_to_utc = -10800;
        to.offset2_to_utc = -7200;
        to.time_of_change.tv_sec = timemgr_parse_utc("2012-11-08_00:00:00");
        to.time_of_change.tv_usec = 0;

        ret = timemgr_update(TIME_SRC_TEST, &td, &to);
        if(0==ret)
        {
            TIMEMGR_DBG("Succeed to update time!\n");
            src->update_cnt++;
            time_updated = 1;
        }
        pthread_mutex_unlock(&src->mutex);

        if(time_updated)
        {
            /* Sleep if successfully updated the time */
            TIMEMGR_TRC("TEST time source is going to sleep %d seconds........\n", src->sleep_period_s);
            sleep(src->sleep_period_s);
        }
    }

    return NULL;
}

static int timesrc_test_start(unsigned int sleep_period_s)
{
    test_src_t * src = &s_test;
    pthread_mutex_lock(&src->mutex);
    src->sleep_period_s = sleep_period_s;
    src->running = 1;
    pthread_cond_signal(&src->cond);
    pthread_mutex_unlock(&src->mutex);
    return 0;
}

static int timesrc_test_stop(void)
{
    test_src_t * src = &s_test;
    pthread_mutex_lock(&src->mutex);
    src->sleep_period_s = 0;
    src->running = 0;
    pthread_cond_signal(&src->cond);
    pthread_mutex_unlock(&src->mutex);
    return 0;
}

/**********************************************************
 *                    Public Function                     *
 **********************************************************/

/**
 * @brief   Init Test time source.
 *
 * @retval  0 for Success, non-0 for failure.
 *
 * @pre
 * @post
 *
 */
int timesrc_test_init(void)
{
    int ret = -1;
    test_src_t * src = &s_test;

    pthread_mutex_lock(&s_test_mutex);

    /* Nothing will be changed if reinit the sntp */
    if(s_test_initialized)
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

        src->thread = O_PTHREAD_CREATE_NO_EXIT(timesrc_test_main, src, 16*1024);
        ENSURE(src->thread);

        ret = timemgr_register(
            TIME_SRC_TEST,
            timesrc_test_start,
            timesrc_test_stop);
        ENSURE(0 == ret);

        s_test_initialized = 1;
        ret = 0;

        pthread_mutex_unlock(&src->mutex);
    }
    pthread_mutex_unlock(&s_test_mutex);
    return ret;
}

