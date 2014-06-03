
/**
 * @file     timemgr_internal.h
 * @author   Lianghe Li
 * @date     Feb 15, 2011
 */
#ifndef __TIMEMGR_INTERNAL_H_
#define __TIMEMGR_INTERNAL_H_

/**
 * @brief       Time manager common header file
 * @defgroup    TIMEMGR
 * @ingroup     TIMEMGR
 * This header file contains debug macro definition for time manager related code.
 * It should be included in both time manager and clients.
 * @{
 */
#include <sys/time.h>
#include <ntvlog.h>
#include <glib.h>
#include <dbus/dbus-glib.h>
#include <glib-object.h>

#if 1
#define TIMEMGR_DBG(fmt, arg...) O_LOG_DEBUG(fmt, ##arg)
#define TIMEMGR_ERR(fmt, arg...) O_LOG_ERROR(fmt, ##arg)
#define TIMEMGR_TRC(fmt, arg...) O_LOG_TRACE(fmt, ##arg)
#define TIMEMGR_STA(fmt, arg...) O_LOG_MAINSTATUS(fmt, ##arg)
#else
#ifdef NDEBUG
#undef NDEBUG
#endif
#define TIMEMGR_DBG(fmt, arg...) printf("[timemgr][DBG] "fmt, ##arg )
#define TIMEMGR_ERR(fmt, arg...) printf("[timemgr][ERR] "fmt, ##arg )
#define TIMEMGR_TRC(fmt, arg...) printf("[timemgr][TRC] "fmt, ##arg )
#define TIMEMGR_STA(fmt, arg...) printf("[timemgr][STA] "fmt, ##arg )
#endif

GType timemgr_get_type(void);

GType app_time_source_get_type(void);


/*************************************************************
 *   Macro definition
 *************************************************************/
#define MINUTE_SEC                  (60)
#define HOUR_SEC                    (3600)
#define DAY_SEC                     (86400)
#define WEEK_SEC                    (604800)
#define HOUR_MIN                    (60)
#define DEFAULT_SLEEP_PERIOD        (1*DAY_SEC)

/* SNTP servers */
#define MAX_HOST        (512)
#define MAX_PORT        (32)
#define MAX_SERVER      (MAX_HOST+MAX_PORT+1)
#define MAX_SERVER_CNT  (8)

#define TIMEMGR_CONFIG_FOLDER       "/system/nemotv/timemgr"
/************************************************************** 
                    Timemgr Utility
 **************************************************************/
typedef struct
{
    time_t                  cur_utc;
    int                     offset;
    int                     is_dst;
    time_t                  next_trans_utc;
    int                     next_offset;
    time_t                  prev_trans_utc;
    int                     prev_offset;

    struct tm               tm_local;
    struct tm               tm_gmt;
    struct tm               tm_local_next_change;
    struct tm               tm_gmt_next_change;

}o_local_time_info_t;


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
    o_local_time_info_t *   ltinfo );


/**
 * @brief   Get GMT time value according to a specific date
 *
 * @param   year    Year since BC
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
time_t timemgr_get_gmt_by_date(int year, int mon, int mday, int hour, int min, int sec);

/**
 * @brief   Print out content of memory
 *
 * @param   buf     [in]    Buffer to print
 * @param   len     [out]   Size to print
 *
 * @retval  none
 *
 * @pre     None
 * @post    None
 *
 * @par Description
 *  
 */
void timemgr_print_memory(unsigned char* buf, int len);

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
    const char* dest_path );

/**
 * @brief   Link /tmp/localtime to a zone info file
 *
 * @param   zoneInfo_file_path   [in] Path of zone info file
 *
 * @retval  None
 *
 */
int timemgr_link_localtime_to(const char * zoneInfo_file_path);


/**
 * @brief   Parse time string into UTC time
 *
 * @param   srcTime     [in] Time in format YYYY-MM-DD_hh:mm:ss
 *
 * @retval  UTC time, 0 indicate incorrect parameters
 *
 */
time_t timemgr_parse_utc(const char * strTime);

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
    size_t max_arg_size);

typedef struct
{
    int        value;
    const char *str;
}val2str_t;

#define INVALID_VALUE           (-2)
#define INVALID_V(x)            (INVALID_VALUE==((int)x))
#define VALID_V(x)              (INVALID_VALUE!=((int)x))
#define INVALID_PAIR            {INVALID_VALUE, "invalid_value" }

/**
 * @brief   Convert a integer value to string
 *
 * @param   map           [in] Map
 * @param   value         [in] Value to look for
 *
 * @retval  Return the string or NULL if not found
 *
 */
const char * timemgr_val2str(const val2str_t map[], int val);

/**
 * @brief   Convert a string to integer value
 *
 * @param   map           [in] Value-String Map
 * @param   str           [in] String to look for
 *
 * @retval  Return the integer value or INVALID_VALUE if not found
 *
 */
int timemgr_str2val(const val2str_t map[], const char *str);


/*****************************************************************************/

#define MAX_CONNECTION  32
typedef struct
{
    unsigned int    id;
    int             type;
    int             tuner_type;
    unsigned short  on_id;      /* Original network ID */
    unsigned short  ts_id;      /* Transponder stram ID */
    unsigned short  svc_id;     /* Service ID */
    unsigned long   id_mask;    
    
}conn_info_t;

typedef struct
{
    conn_info_t data[MAX_CONNECTION];
    int         size;
}connset_t;

/**
 * @brief   Query the index of connection in the connection set
 *
 * @param   cset          [in] Connection info set pointer
 * @param   str           [in] Connection ID to query
 *
 * @retval  index of the connection info
 *          -1 indicate the connection is NOT in the set
 *
 */
int connset_index(connset_t * cset, unsigned int conn_id);

/**
 * @brief   Query if the connection is in the connection set
 *
 * @param   cset          [in] Connection info set pointer
 * @param   str           [in] Connection ID to query
 *
 * @retval  Non-0 indicate the connection is in the set
 *          0 indicate the connection is NOt in the set
 *
 */
int connset_in(connset_t * cset, unsigned int conn_id);

/**
 * @brief   Add a connection into a connection set
 *
 * @param   cset          [in] Connection info set pointer
 * @param   info          [in] Connection info pointer
 * @param   caller_name   [in] Name of caller for debug
 *
 * @retval  0 indicate succeed to add. Non-0 indicate failure.
 *
 */
int connset_add(connset_t * cset, conn_info_t *info, const char *caller_name);

/**
 * @brief   Remove a connection from a connection set
 *
 * @param   cset          [in] Connection info set pointer
 * @param   info          [in] Connection info pointer
 * @param   caller_name   [in] Name of caller for debug
 *
 * @retval  0 indicate succeed to add. Non-0 indicate failure.
 *
 */
int connset_del(connset_t * cset, unsigned int conn_id, const char *caller_name);


/*************************************************************
 *   Locale watcher
 *************************************************************/

/**
 * @brief   Init the locale watcher 
 *
 * @pre  None
 * @post None
 *
 * @par Description
 * To be called when timemgr init.
 */
int timemgr_locale_watcher_init(void);

/**
 * @brief   Time source start function type
 *
 * @param[out]   countryCode : Char array to store the country code.
 * @param[out]   regionID : Pointer to store the region ID.
 * @retval      0     Success
 * @return      non-0 Failure
 *
 * @pre  None
 * @post None
 *
 * @par Description
 * To be called by DVB time source to parse TOT.
 */
int timemgr_get_locale(char countryCode[4], int *regionID);

/**
 * @brief   Check the remote local time info and patch the zone info file if they are in conflict
 *
 * @param[in]   ltinfo_remote       Remote local time info
 *
 * @retval      0     Success
 * @return      non-0 Failure
 *
 * @pre  None
 * @post Remote and local info should match to each other
 *
 * @par Description
 */
int timemgr_check_and_patch_zoneInfo(o_local_time_info_t * ltinfo_remote);


/*************************************************************
 *   Definitions and functions to implement time source
 *************************************************************/

/**
 * Time.
 */
typedef struct timeval o_time_t;

/**
 * Substract two timeval, just use the seconds for now. 
 */
#define TIME_SUB(a,b)    ((a).tv_sec - (b).tv_sec)

/**
 * Time zone.
 */
typedef struct timezone o_time_zone_t;

/**
 * Time source class.
 */
typedef enum 
{
    TIME_SRC_SNTP,
    TIME_SRC_DVB,
    TIME_SRC_ARIB,
    TIME_SRC_ATSC,
    TIME_SRC_TEST,
	TIME_SRC_APP,
    TIME_SRC_MAX
    
}o_time_source_type_t;

/**
 * Time data. It contains the time of source and local system time
 * stamp when collecting the source time.
 */
typedef struct
{
    o_time_t remote;        /*!< Time of the source. */
    o_time_t local;         /*!< Local system time when collecting from source. */
    
}o_time_data_t;

/* Return delta in seconds which is contained in o_time_data_t */
#define DELTA_SEC(time_data)    TIME_SUB(time_data->remote, time_data->local)

/**
 * Time offset data. It contains data for delay applied time change.
 */
typedef struct
{
    int             offset1_to_utc;  /*!< In seconds. UTC + offset1_to_utc = Local time before time_of_change */

    o_time_t        time_of_change; /*!< UTC time at which the offset (e.g. daylight savings) 
                                     * will next be applied. */

    int             offset2_to_utc; /*!< In seconds. UTC + offset2_to_utc = Local time after time_of_change */

}o_time_offset_t;

/**
 * @brief   Time source start function type
 *
 * @param[in]   sleep_period : The interval between each time collection, in seconds.
 * @retval      0     Success
 * @return      non-0 Failure
 *
 * @pre  None
 * @post None
 *
 * @par Description
 * To be called by time updater to start a time source collecting.
 */
typedef int (* o_time_source_start_t)(unsigned int sleep_period_s);

/**
 * @brief   Time source stop function type
 *
 * @param   None
 * @retval  0     Success
 * @return  non-0 Failure
 *
 * @pre     None
 * @post    None
 *
 * @par Description
 * To be called by time updater to start a time source collecting.
 */
typedef int (* o_time_source_stop_t)(void);

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
void timemgr_init_static_variables(void);

/**
 * @brief   Register a time source
 *
 * @param   src_type [in] class of the source to be registerred
 * @param   start [in] start function of the source to be registerred
 * @param   stop [in] stop function of the source to be registerred
 *
 * @retval  0     Success
 * @return  non-0 Failure
 *
 * @pre     None
 * @post    None
 *
 */
int timemgr_register(
    o_time_source_type_t    src_type,
    o_time_source_start_t   start,
    o_time_source_stop_t    stop );

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
    unsigned int sleep_period_s);


/**
 * @brief   Stop the time source
 *
 * @param   src_type [in] Time source to stop
 *
 * @retval  0     Success
 * @return  non-0 Failure
 *
 * @pre     The time source is registerred and running
 * @post    The time source stopped if success
 *
 */
int timemgr_stop(o_time_source_type_t src_type);

#define TIMEMGR_RET_SUCCESS         (0)
#define TIMEMGR_RET_FAIL            (-1)
#define TIMEMGR_RET_ERR_PARAM       (-2)
#define TIMEMGR_RET_ERR_SRC         (-3)
#define TIMEMGR_RET_ERR_SRC_STATE   (-4)
#define TIMEMGR_RET_ERR_SRC_PERM    (-5)
#define TIMEMGR_RET_GLITCH          (-6)

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
    o_time_offset_t         *time_offset);

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
void timemgr_send_event(timemgr_event_t evt);


/***************************************************** 
                    Zone Info File Patcher 
 *****************************************************/

/**
  * @brief   Patch zone info file by the given local time info
  *
  * @param[in]   original_path      Original zone info file path to be patched
  * @param[in]   patched_path       Patched zone info file path 
  * @param[in]   ltinfo             Pointer to a local time info
  *
  * @retval      0     Success
  * @return      non-0 Failure
  *
  * @pre  None
  * @post Remote and local info should match to each other
  *
  * @par Description
  */
int timemgr_patch_zoneInfo_file(
    const char *            original_path,
    const char *            patched_path,
    o_local_time_info_t *   ltinfo );


/***************************************************** 
                    Time Source 
 *****************************************************/

/**
 * @brief   Init SNTP client for specific SNTP server.
 *
 * @param   servers [in] A string specify SNTP servers. 
 *                       Servers are seperated by ';'
 *                       host name and port is seperated by ':'
 *
 * @retval  0 for Success, non-0 for failure.
 *
 * @pre     
 * @post    
 *
 */
int timesrc_sntp_init(const char * servers);

/**
 * @brief   Init time source to collect time date from DVB.
 *
 * @retval  0 for Success, non-0 for failure.
 *
 * @pre     
 * @post    
 *
 */
int timesrc_dvb_init(void);

/**
 * @brief   Init app time source.
 *
 * @retval  0 for Success, non-0 for failure.
 *
 * @pre     
 * @post    
 *
 */
int timesrc_app_init(void);

/**
 * @brief   Init time source for test.
 *
 * @retval  0 for Success, non-0 for failure.
 *
 * @pre     
 * @post    
 *
 */
int timesrc_test_init(void);

/*************************************************************
 *   Exported from timemgr.c
 *************************************************************/

/**
 * @brief   Query if it is need to patch the POSIX zone info file
 *
 * @retval      0     No need to patch zone info file.
 * @retval      non-0 Should patch the zone info file if the LTOD confict with zone info file.
 *
 * @par Description
 * To be called when timemgr init.
 */
int timemgr_should_patch_zone_info(void);

/**
 * @brief   Query time source name
 *
 * @param[in]   src_type : Time source ID.
 *
 * @retval      Return source name
 */
const char * timemgr_src_name(o_time_source_type_t src_type);

/**
 * @brief   Query the preempt time in seconds. 
 *          Timemgr will accept lower priority time source to update system time
 *          if there is no time update from higher priority sources for a period of time.
 *          This period of time is preempt_time.
 *
 * @retval  Preempt time in seconds.
 */
int timemgr_get_preempt_time(void);

#ifndef NDEBUG
                  
/***************************************************** 
                  Code for Selft Test
 *****************************************************/

/**
 * @brief   Parse the TS stream for TDT&TOT 
 * 
 * @param   pid_20_file_path    TS file path to parse
 *
 * @retval  0 for Success, non-0 for failure.
 */
int timemgr_selftest_parse_tdt_tot(char * pid_20_file_path);

/**
 * @brief   Check rule with rule string
 * 
 * @param   rule_str    String represent the rule
 * @param   id          ID to check if it satisfy the rule
 *
 * @retval  0 for Success, non-0 for failure.
 */
int timemgr_selftest_check_id(const char * rule_str, int id);

/**
 * @brief   Get zoneinfo from zonemap file by countryCode and regionID
 *
 * @param   strCountryCode [in] CountryCode.
 * @param   strRegionID    [in] RegionID.
 *
 * @retval  0 for Success, non-0 for failure.
 *
 * @pre     
 * @post    
 *
 */
int timemgr_selftest_lookup_zoneInfo(const char * countryCode, int regionID);

/**
 * @brief   Test localtime() when change the countryCode and regionID
 *
 * @param   ccode   [in] Country code
 * @param   rid     [in] Region ID
 *
 * @retval  0 for Success, non-0 for failure.
 *
 * @pre     
 * @post    
 *
 */
int timemgr_selftest_set_zone(char * ccode, int rid);

/**
 * @brief   Test timemgr_check_and_patch_zoneInfo()
 *
 * @param   strTime         [in] The current UTC
 * @param   offset          [in] offset to GMT before next_change
 * @param   strNextChange   [in] The UTC of next_change
 * @param   next_offset     [in] offset to GMT after next_change
 *
 * @retval  0 for Success, non-0 for failure.
 *
 * @pre     
 * @post      
 *
 */
int timemgr_selftest_check_and_patch_zoneInfo(
    const char * strTime,
    int offset,
    const char * strNextChange,
    int next_offset );

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
int timemgr_selftest_find_dst(const char * strTime);

/**
 * @brief   Test to get DST switching time according to current zone info in a year
 *
 * @param   year     The year DST switching time should be searched
 *
 * @retval  0 for Success, non-0 for failure.
 *
 * @pre     
 * @post    
 *
 */
int timemgr_selftest_find_dst_in_year(int year);

/**
 * @brief   Test to get DST changing time according to current zone info after specific UTC time
 *
 * @param   strTime     UTC time to wait
 *
 * @retval  0 for Success, non-0 for failure.
 *
 * @pre     
 * @post    
 *
 */
int timemgr_selftest_waittime(const char *strTime);

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
int timemgr_selftest_settime(const char *strTime);

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
int timemgr_selftest_adjtime(const char *strTime);

/**
 * @brief   Test to parse sntp host:port from the string
 *
 * @param   str         String contain the servers
 *
 * @retval  0 for Success, non-0 for failure.
 *
 * @pre     
 * @post    
 *
 */
int timemgr_selftest_parse_sntp_servers(const char *str);

/**
 * @brief   Test to print out information of a given zone info file
 *
 * @param   zoneInfo_path   [in] Zone info file path to use
 *
 * @retval  0 for Success, non-0 for failure.
 *
 * @pre     
 * @post    
 *
 */
int timemgr_selftest_print_zoneInfo( const char * zoneInfo_path);

/**
 * @brief   Test to print out all transition types of a given zone info file
 *
 * @param   zoneInfo_path   [in] Zone info file path to use
 * @param   version         [in] version of the info. It can be 1 or 2.
 *
 * @retval  0 for Success, non-0 for failure.
 *
 * @pre     
 * @post    
 *
 */
int timemgr_selftest_print_zoneInfo_trans( const char * zoneInfo_path, int version);

/**
 * @brief   Test to generate a posix TZ at specific time point according to a given zone info file
 *
 * @param   zoneInfo_path   [in] Zone info file path to use
 * @param   strTime         [in] UTC time to generate the posix TZ string
 *
 * @retval  0 for Success, non-0 for failure.
 *
 * @pre     
 * @post    
 *
 */
int timemgr_selftest_generate_TZ( const char * zoneInfo_path, const char *strTime);

/**
 * @brief   Test to patch a zone info file by local time info comes from TOT
 *
 * @param   original_path   [in] Original zone info file to be patched
 * @param   patched_path    [in] Patched zone info file path
 * @param   strTime         [in] The current UTC
 * @param   offset          [in] offset to GMT before next_change
 * @param   strNextChange   [in] The UTC of next_change
 * @param   next_offset     [in] offset to GMT after next_change
 *
 * @retval  0 for Success, non-0 for failure.
 *
 * @pre     
 * @post    
 *
 */
int timemgr_selftest_patch_zoneInfo_file(
    const char * original_path,
    const char * patched_path,
    const char * strTime,
    int offset,
    const char * strNextChange,
    int next_offset );


#endif

#endif

