 * ============================================================================
 */

/**
 * @file     timemgr_utils.c
 * @author   Lianghe Li
 * @date     Oct 15, 2012
 * @brief    Timemgr utility functions
 * @ingroup  TIMEMGR
 * This file implement utility functions used by timemgr.
 * @{
 */
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "timemgr_controller.h"
#include "timemgr.h"
#include "timemgr_internal.h"

/********************************
 *        Static members        *
 ********************************/

static char s_TZ_buf[256];
static pthread_mutex_t s_TZ_mutex = PTHREAD_MUTEX_INITIALIZER;
static int s_TZ_locked = 0;

static void timemgr_set_lock_TZ(const char * TZ)
{
    pthread_mutex_lock(&s_TZ_mutex);
    O_ASSERT(0==s_TZ_locked);
    s_TZ_locked = 1;
    if(TZ)
    {
        sprintf(s_TZ_buf, "TZ=%s", TZ);

        /* Use putenv instead of setenv because uclibc always create new 
         * string in setenv() and never free it. It will cause memory leak. */
        putenv(s_TZ_buf);
    }
    else
    {
        unsetenv("TZ");
    }
    tzset();
}

static void timemgr_clear_unlock_TZ()
{
    unsetenv("TZ");
    tzset();
    s_TZ_locked = 0;
    pthread_mutex_unlock(&s_TZ_mutex);
}

static pthread_mutex_t s_localtime_file_mutex = PTHREAD_MUTEX_INITIALIZER;

/* If the query UTC's DST status. */
static int timemgr_is_dst(time_t t)
{
    struct tm tm_t;
    
    tzset();
    localtime_r(&t, &tm_t);

    if(tm_t.tm_isdst > 0)
    {
        return 1;
    }
    else if(tm_t.tm_isdst == 0)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

/* Compute next dst changing time according to a query UTC. */
static int timemgr_get_next_dst_change_time(
    time_t cur, 
    time_t *next_dst_change_time, 
    int *on)
{
    int ret = -1;
    int i, j;
    int step[] = {30*DAY_SEC, DAY_SEC,  HOUR_SEC, MINUTE_SEC,  1    };
    int cnt[] =  {13,         31,       24,       60,          60   };
    time_t t = cur;
    int cur_dst, dst, prev_dst;
    int found = 0;

    cur_dst = timemgr_is_dst(cur);
    if(-1==cur_dst)
    {
        return -1;
    }

    j = 0;
    do
    {
        prev_dst = timemgr_is_dst(t);
        found = 0;
        for(i=0;i<cnt[j];i++)
        {
            if(0==(j%2))
            {
                t += step[j];
            }
            else
            {
                t -= step[j];
            }
            dst = timemgr_is_dst(t);
            if(dst != prev_dst)
            {
                found = 1;
                break;
            }
        }
        j++;
    }while(found && j<5);
    
    if(found && (timemgr_is_dst(t-1) == cur_dst) && (timemgr_is_dst(t) != cur_dst))
    {
        *next_dst_change_time = t;
        *on = cur_dst ? 0 : 1;
        ret = 0;
    }
    else
    {
        TIMEMGR_TRC("Failed to find DST changing time at %s", asctime(gmtime(&cur)));
        ret = -1;
    }
    return ret;
}

/* Compute two break-down time's offset in seconds. */
static int timemgr_tm_offset(struct tm * tm_a, struct tm *tm_b)
{
    time_t t_a;
    time_t t_b;
    int offset_in_sec;
    
    /* Covert the break-down time back to time_t as UTC */
    timemgr_set_lock_TZ("GMT+0");
    t_a = mktime(tm_a);
    t_b = mktime(tm_b);
    timemgr_clear_unlock_TZ();
    
    offset_in_sec = (int)t_a - (int)t_b;
    return offset_in_sec;
}

/********************************
 *        Public members        *
 ********************************/

/**
 * @brief   Get local time info according to current zone info file at a specific time
 *
 * @param   utc_time    UTC time used to query the local time info
 * @param   ltinfo      Pointer to a local time info struct to store the result
 * @retval  0       Success
 * @retval  non-0   Failure
 *
 * @pre     None
 * @post    None
 *
 * @par Description
 *  
 */
int timemgr_get_ltinfo(
    time_t                  utc_time,
    o_local_time_info_t *   ltinfo )
{
    int ret = -1;
    int next_dst_on = 0;

    if(NULL==ltinfo)
    {
        return -1;
    }

    /* Lock the localtime file */
    pthread_mutex_lock(&s_localtime_file_mutex);

    /* Clear and lock the TZ env var */
    timemgr_set_lock_TZ(NULL);

    localtime_r(&utc_time, &ltinfo->tm_local);
    gmtime_r(&utc_time, &ltinfo->tm_gmt);

    ltinfo->cur_utc = utc_time;
    
    ret = timemgr_get_next_dst_change_time(utc_time, &ltinfo->next_trans_utc, &next_dst_on);
    if(0 == ret)
    {
        if(1==next_dst_on)
        {
            ltinfo->is_dst = 0;
        }
        else if(0==next_dst_on)
        {
            ltinfo->is_dst = 1;
        }
        else
        {
            ltinfo->is_dst = -1;
        }
    }
    else
    {
        ltinfo->is_dst = -1;
    }

    /* Compute next_offset */
    if(1==ltinfo->is_dst || 0==ltinfo->is_dst)
    {
        localtime_r(&ltinfo->next_trans_utc, &ltinfo->tm_local_next_change);
        gmtime_r(&ltinfo->next_trans_utc, &ltinfo->tm_gmt_next_change);
        
        ltinfo->prev_trans_utc = 0;
        ltinfo->prev_offset = 0;
    }
    else
    {
        ltinfo->next_trans_utc = 0;
        ltinfo->next_offset = 0;
        ltinfo->prev_trans_utc = 0;
        ltinfo->prev_offset = 0;
    }

    /* Unlock the TZ */
    timemgr_clear_unlock_TZ();

    /* Unlock the localtime file */
    pthread_mutex_unlock(&s_localtime_file_mutex);

    /* !!! timemgr_tm_offset() can't be called with s_TZ_mutex locked !!! */
    ltinfo->offset = timemgr_tm_offset(&ltinfo->tm_local, &ltinfo->tm_gmt);
    if(1==ltinfo->is_dst || 0==ltinfo->is_dst)
    {
        ltinfo->next_offset = timemgr_tm_offset(&ltinfo->tm_local_next_change, &ltinfo->tm_gmt_next_change);
    }

    return 0;
}

/**
 * @brief   Get GMT time value according to a specific date
 *
 * @param   year    Year since B.C.
 * @param   month   Month value (1-12)
 * @param   day     Day of month value (1-28, 29, 30, 31)
 * @param   hour    Hour of day value (0 - 23)
 * @param   minute  Minute of hour value (0 - 59)
 * @param   second  Second of minute value (0 - 59)
 *
 * @retval  0       Success
 * @retval  non-0   Failure
 *
 * @pre     None
 * @post    None
 *
 * @par Description
 *  
 */
time_t timemgr_get_gmt_by_date(int year, int mon, int mday, int hour, int min, int sec)
{
    struct tm tm_date;
    time_t t_date = 0;
    
    tm_date.tm_sec = sec;
    tm_date.tm_min = min;
    tm_date.tm_hour = hour;
    tm_date.tm_mday = mday;
    tm_date.tm_mon = mon - 1;
    tm_date.tm_year = year - 1900;
    tm_date.tm_wday = -1;
    tm_date.tm_yday = -1;
    tm_date.tm_isdst = -1;

    timemgr_set_lock_TZ("GMT+0");
    t_date = mktime(&tm_date);
    timemgr_clear_unlock_TZ();

    return t_date;    
}

/**
 * @brief   Print out content of memory
 *
 * @param   buf     [in]    Buffer to print
 * @param   len     [out]   Size to print
 *
 * @retval  0       Success
 * @retval  non-0   Failure
 *
 * @pre     None
 * @post    None
 *
 * @par Description
 *  
 */
void timemgr_print_memory(unsigned char* buf, int len)
{
    int i, j=0;
    unsigned int start_addr;
    unsigned int line_cnt;
    unsigned char * p;
    unsigned char * pEnd;
    
    if (len <= 0 || NULL == buf)
    {
        TIMEMGR_ERR("Invalid parameter! buf=%08x len=%d\n", (unsigned int)buf, len);
        return;
    }

    start_addr = ((unsigned int)(buf) / 16) * 16;
    line_cnt = (((unsigned int)(buf) % 16) + len + 15) / 16;
    pEnd = buf+len;
    for(i=0; i<line_cnt; i++)
    {
        printf("[%08X] ", (unsigned int)(start_addr+16*i) );
        for(j=0; j < 16; j++)
        {
            p = (unsigned char *)start_addr + (16 * i) + j;
            if(p >= buf && p < pEnd)
            {
                printf("%02x ", *p);
            }
            else
            {
                printf("   ");
            }
        }
        printf("\t");
        for(j=0; j < 16; j++)
        {
            p = (unsigned char *)start_addr + (16 * i) + j;
            if(p >= buf && p < pEnd)
            {
                printf("%c",  isprint(*p) ? (*p) : '.');
            }
            else
            {
                printf(" ");
            }
        }
        printf("\n");
    }
}

/**
 * @brief   Copy file
 *
 * @param   src_path    [in]    Source file path
 * @param   dest_path   [out]   Destination file path
 *
 * @retval  0       Success
 * @retval  non-0   Failure
 *
 * @pre     None
 * @post    None
 *
 * @par Description
 *  
 */
int timemgr_copy_file(
    const char* src_path,
    const char* dest_path )
{
    int ret = -1;
    FILE* src_fp = NULL;
    FILE* dest_fp = NULL;
    size_t read_cnt;
    size_t write_cnt;
#define FILE_COPY_BUF_SIZE     (4096)
    unsigned char buf[FILE_COPY_BUF_SIZE];
    
    src_fp = fopen(src_path, "rb");
    dest_fp = fopen(dest_path, "wb");

    if(src_fp && dest_fp)
    {
        while ( 1 )
        {
            read_cnt = fread(buf, 1, FILE_COPY_BUF_SIZE, src_fp);
            if(0==read_cnt)
            {
                ret = 0;
                break;
            }
            else
            {
                write_cnt = fwrite(buf, 1, read_cnt, dest_fp);
                if(write_cnt!=read_cnt)
                {
                    break;
                }
            }
        }
    }
    if(src_fp)
    {
        fclose(src_fp);
    }
    if(dest_fp)
    {
        fclose(dest_fp);
    }
    return ret;
}

/**
 * @brief   Create the symbol link
 *
 * @param   src_path    [in] Symbol link file path
 * @param   dest_path   [in] Real file path to be linked
 *
 * @retval  None
 *
 */
static int timemgr_create_symlink(
    const char *src_path,
    const char *dest_path )
{
    int ret = -1;
    if(0==access(dest_path, F_OK))
    {
        if(0==access(src_path, F_OK))
        {
            if(0!=remove(src_path))
            {
                TIMEMGR_ERR("Failed to remove source file %s!\n", src_path);
            }
        }

        if(0==symlink(dest_path, src_path))
        {
            TIMEMGR_DBG("%s was linked to %s!\n", src_path, dest_path);
            ret = 0;
        }
        else
        {
            TIMEMGR_ERR("Failed to create symbol link from %s to %s!\n", src_path, dest_path);
        }
    }
    else
    {
        TIMEMGR_ERR("Destination file %s doesn't exist!\n", dest_path);
    }
    return ret;
}

#define LOCALTIME_PATH                  "/tmp/localtime"

/**
 * @brief   Link /tmp/localtime to a zone info file
 *
 * @param   zoneInfo_file_path   [in] Path of zone info file
 *
 * @retval  None
 *
 */
int timemgr_link_localtime_to(const char * zoneInfo_file_path)
{
    int ret = -1;
    pthread_mutex_lock(&s_localtime_file_mutex);
    ret = timemgr_create_symlink(LOCALTIME_PATH, zoneInfo_file_path);
    pthread_mutex_unlock(&s_localtime_file_mutex);
    return ret;
}


/**
 * @brief   Parse time string into UTC time
 *
 * @param   srcTime     [in] Time in format YYYY-MM-DD_hh:mm:ss
 *
 * @retval  UTC time, 0 indicate incorrect parameters
 *
 */
time_t timemgr_parse_utc(const char * strTime)
{
    int ret = -1;
    int year, month, day, hour, minute, second;
    
    ret = sscanf(strTime, "%04d-%02d-%02d_%02d:%02d:%02d", &year,&month,&day,&hour,&minute,&second);
    if( 6==ret &&
        year>0 && 
        month>=1 && month<=12 && 
        day>=1 && day<=31 && 
        hour>=0 && hour<=23 && 
        minute>=0 && minute<=59 &&
        second>=0 && second<=59 )
    {
        return timemgr_get_gmt_by_date(year, month, day, hour, minute, second);
    }
    else
    {
        TIMEMGR_ERR("Invalid parameter!\n");
        return 0;
    }
    return 0;
}

/**
 * @brief   Split a string into string vectors
 *
 * @param   s             [in] Input string to split
 * @param   separator     [in] All the separator characters
 * @param   argv          [out] String vector to hold the output strings
 * @param   argv_count    [in] String vector size
 * @param   max_argv_size [in] Maximun charater every output string can hold
 *
 * @retval  Return the parsed number of string
 *
 */
int timemgr_split_string(
    const char * s, 
    const char * separator,
    char *argv[], 
    size_t argv_count, 
    size_t max_arg_size)
{
    int argc = 0;
    const char *p=s;
    const char *start=NULL;
    size_t vsize = 0;

    for(p=s; argc<argv_count; p++)
    {
        if(isspace(*p) || !(*p) || strchr(separator, *p) )
        {
            if(start)
            {
                vsize = p-start;
                strncpy(argv[argc++], start, vsize<max_arg_size?vsize:max_arg_size);
                start = NULL;
            }
        }
        else if(start==NULL)
        {
            start = p;
        }
        if(!(*p))
        {
            break;
        }
    }
    return argc;
}

/**
 * @brief   Convert a integer value to string
 *
 * @param   map           [in] Map
 * @param   value         [in] Value to look for
 *
 * @retval  Return the string or NULL if not found
 *
 */
const char * timemgr_val2str(const val2str_t map[], int val)
{
    int i=0;
    while(VALID_V(map[i].value))
    {
        if(map[i].value==val)
        {
            return map[i].str;
        }
        i++;
    }
    return "invalid_value";
}

/**
 * @brief   Convert a string to integer value
 *
 * @param   map           [in] Value-String Map
 * @param   str           [in] String to look for
 *
 * @retval  Return the integer value or INVALID_VALUE if not found
 *
 */
int timemgr_str2val(const val2str_t map[], const char *str)
{
    int i=0;
    while(VALID_V(map[i].value))
    {
        if(0==strcasecmp(map[i].str, str))
        {
            return map[i].value;
        }
        i++;
    }
    return INVALID_VALUE;
}

/*********************************************
            Connection Info Array
 *********************************************/ 

#ifdef DEBUG_CONNSET
static char s_connset_content[1024];
static char * connset_content(connset_t * cset)
{
    char content[16];
    int i = 0;
    sprintf(s_connset_content, "cset[%d]: ", cset->size);
    for(i=0;i<cset->size;i++)
    {
        snprintf(content, 16, "%d ", cset->data[i]);
        strcat(s_connset_content, content);
    }
    return s_connset_content;
}
#endif

/* Return the index of conn or -1 if it doesn't exist in the set. */
int connset_index(connset_t * cset, unsigned int conn_id)
{
    int i=0;
    for(i=0;i<cset->size;i++)
    {
        if(cset->data[i].id==conn_id)
        {
            return i;
        }
    }
    return -1;
}

/* Return 1 if it exist. Return 0 if it doesn't exist. */
int connset_in(connset_t * cset, unsigned int conn_id)
{
    return (-1==connset_index(cset, conn_id))?0:1;
}

/* Add connection to connection set. */
int connset_add(connset_t * cset, conn_info_t *info, const char *caller_name)
{
    int ret = -1;
    REQUIRE(cset);
    REQUIRE(info);
    if(NULL==cset || NULL==info)
    {
        TIMEMGR_ERR("Invalid parameter!\n");
        return ret;
    }
    
    if(-1==connset_index(cset, info->id) && cset->size<MAX_CONNECTION)
    {
        memcpy(&cset->data[cset->size], info, sizeof(conn_info_t));
        cset->size++;
#ifdef DEBUG_CONNSET
        TIMEMGR_DBG("%s add conn=%d into connset! ===%s\n", caller_name, info->id, connset_content(cset));
#endif
        ret = 0;
    }
    else
    {
        ret = -1;
        TIMEMGR_ERR("Failed to add conn=%d into connset!\n", info->id);
    }
    return ret;
}

/* Remove connection from connection set. */
int connset_del(connset_t * cset, unsigned int conn_id, const char *caller_name)
{
    int ret = -1;
    int i = -1;
    i = connset_index(cset, conn_id);
    if(-1!=i)
    {
        if( (i+1) <cset->size )
        {
            memcpy(&cset->data[i], &cset->data[i+1], (cset->size-i-1)*sizeof(conn_info_t));
        }
        else
        {
            memset(&cset->data[i], 0, sizeof(conn_info_t));
        }
        cset->size--;
        ret = 0;
#ifdef DEBUG_CONNSET
        TIMEMGR_DBG("%s del conn=%d from connset! ===%s\n", caller_name, conn_id, connset_content(cset));
#endif
    }
    else
    {
        ret = -1;
        TIMEMGR_ERR("%s failed to remove conn=%d from connset!\n", caller_name, conn_id);
    }
    return ret;
}

#ifndef NDEBUG
/*********************************************
            Code for Self-Test
 *********************************************/  

/**
 * @brief   Test to get DST changing time according to current zone info after specific UTC time
 *
 * @param   strTime     UTC time after which the next DST changing time should be searched
 *
 * @retval  0 for Success, non-0 for failure.
 *
 * @pre     
 * @post    
 *
 */
int timemgr_selftest_find_dst(const char * strTime)
{
    int ret = -1;
    time_t t, dst_t;
    int dst_on = -1;
    
    t = timemgr_parse_utc(strTime);
    printf("Query time:\n");
    printf("        UTC         :   %s", asctime(gmtime(&t)));
        
    ret = timemgr_get_next_dst_change_time(t, &dst_t, &dst_on);
    if(ret==0)
    {
        printf("DST is turning to %s at\n", dst_on ? "ON" : "OFF");
        printf("        UTC         :   %s", asctime(gmtime(&dst_t)));
        printf("        Local Time  :   %s", asctime(localtime(&dst_t)));
    }
    else
    {
        printf("DST is not used in this zone!\n");
    }
    return 0;    
}

/**
 * @brief   Test to get DST changing time according to current zone info in a year
 *
 * @param   year     The year DST switching time should be searched
 *
 * @retval  0 for Success, non-0 for failure.
 *
 * @pre     
 * @post    
 *
 */
int timemgr_selftest_find_dst_in_year(int year)
{
    char strDate[64];

    if(year>=1970 && year<=2038)
    {
        sprintf(strDate, "%04d-%2d-01_00:00:00", year, 1);
        timemgr_selftest_find_dst(strDate);

        sprintf(strDate, "%04d-%2d-01_00:00:00", year, 7);
        timemgr_selftest_find_dst(strDate);

        return 0;
    }
    else
    {
        printf("Invalid year %d!\n", year);
        return -1;
    }
}

/**
 * @brief   Test to wait until a UTC time
 *
 * @param   strTime     UTC time to wait
 *
 * @retval  0 for Success, non-0 for failure.
 *
 * @pre     
 * @post    
 *
 */
int timemgr_selftest_waittime(const char *strTime)
{
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    struct timespec abstime;
    struct timespec t0;
    struct timespec t1;
    time_t end_time;
    int ret = -1;
    int err = 0;
    
    ret = pthread_cond_init(&cond, NULL);
    if(ret)
    {
        TIMEMGR_ERR("Failed to init cond!\n");
        return -1;
    }
    
    ret = pthread_mutex_init(&mutex, NULL);
    if(ret)
    {
        TIMEMGR_ERR("Failed to init mutex!\n");
        return -1;
    }

    clock_gettime( CLOCK_MONOTONIC, &t0);

    clock_gettime( CLOCK_REALTIME,  &abstime );
    abstime.tv_sec = timemgr_parse_utc(strTime);
    abstime.tv_nsec = 0;
    pthread_mutex_lock(&mutex);
    TIMEMGR_DBG("Start waiting to %s", asctime(gmtime(&abstime.tv_sec)));
    ret = pthread_cond_timedwait(&cond, &mutex, &abstime);
    err = errno;

    clock_gettime( CLOCK_MONOTONIC, &t1);
    end_time = time(NULL);
    
    TIMEMGR_DBG("Wait completed with ret=%d errno=%d. Waited for %ld seconds. Current time=%s", 
        ret, err, (long int)(t1.tv_sec-t0.tv_sec), asctime(gmtime(&end_time)));

    pthread_mutex_unlock(&mutex);
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
    
    return 0;
}

/**
 * @brief   Test to set time by stime()
 *
 * @param   strTime     UTC time to set
 *
 * @retval  0 for Success, non-0 for failure.
 *
 * @pre     
 * @post    
 *
 */
int timemgr_selftest_settime(const char *strTime)
{
    time_t abstime;
    int ret = -1;
    int err = 0;
    
    abstime = timemgr_parse_utc(strTime);
    
    ret = stime(&abstime);
    err = errno;
        
    TIMEMGR_DBG("stime() return %d errno=%d.\n", ret, err);    
    return 0;
}

/**
 * @brief   Test to set time by adjtime()
 *
 * @param   strTime     UTC time to set
 *
 * @retval  0 for Success, non-0 for failure.
 *
 * @pre     
 * @post    
 *
 */
int timemgr_selftest_adjtime(const char *strTime)
{
    time_t abstime;
    time_t t;
    struct timeval delta = {0,0};
    struct timeval old_delta = {0, 0};
    int ret = -1;
    int err = 0;
    int just_query = 0;
    
    if(0==strcasecmp(strTime, "NULL"))
    {
        just_query = 1;
    }
    
    if(just_query)
    {
        delta.tv_sec = 0;
    }
    else
    {
        abstime = timemgr_parse_utc(strTime);
        t = time(NULL);

        delta.tv_sec = abstime - t;
    }
    
    ret = adjtime(&delta, &old_delta);
    err = errno;

    TIMEMGR_DBG("adjtime() return %d errno=%d.\n", ret, err);
    TIMEMGR_DBG("    old_delta.tv_sec=%ld \n", (long int)old_delta.tv_sec);
    TIMEMGR_DBG("    old_delta.tv_usec=%ld \n", (long int)old_delta.tv_usec);
    return 0;
}

#endif

