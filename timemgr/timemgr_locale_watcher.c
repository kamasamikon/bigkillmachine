 * ============================================================================
 */
#include <unistd.h>
#include <glib.h>
#include <limits.h>     /* Use LINE_MAX */
#include <ctype.h>
#include <dbus/dbus-glib.h>
#include <glib-object.h>
#include <pthread.h>
// #include "configman_lib.h"
#include "timemgr.h"
#include "timemgr_internal.h"

#define MAX_ZONEINFO                    (128)
#define MAX_PATH                        (260)

/* Below values stored in configman. They can be configured to change locale. */
#define LOCALE_DIR                      "/network/siconfig"
#define COUNTRY_CODE_KEYPATH            "/network/siconfig/countryCode"  /* Type: string    */
#define REGION_ID_KEYPATH               "/network/siconfig/regionID"     /* Type: int       */
#define INVALID_COUNTRY_CODE            (NULL)
#define INVALID_REGION_ID               (-1)
#define DEFAULT_COUNTRY_CODE            "GBR"
#define DEFAULT_REGION_ID               (0)
#define DEFAULT_ZONE_INFO               "Europe/London"

/* Below values are for zone info file */
#define ZONEINFO_FOLDER                 "/usr/share/zoneinfo/"
#define ZONEMAP_PATH                    "/etc/timemgr/zonemap"
#define ZONEINFO_PATCHED_PATH           "/tmp/localtime_patched"

/*********************************************************
 *                   Static Members                      *
 *********************************************************/

static char     s_countryCode[4]            = DEFAULT_COUNTRY_CODE;
static int      s_regionID                  = DEFAULT_REGION_ID;
static char     s_zoneInfo[MAX_ZONEINFO]    = DEFAULT_ZONE_INFO;
static int      s_patched                   = 0;
static int      s_patched_ok = 0;

static o_local_time_info_t s_checked_ltinfo;
static int                 s_checked = 0;

static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;
static int s_local_watcher_initialized = 0;

/* @brief Check country code and region ID */
static int check_countryCode_and_regionID(const char * countryCode, int regionID)
{
    if(countryCode && 3==strlen(countryCode) && regionID>=0 && regionID<=60 )
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

static int split_string(const char * s, char *argv[], size_t argv_count, size_t max_arg_size)
{
    int argc = 0;
    argc = timemgr_split_string(s, "", argv, argv_count, max_arg_size);
    return argc;
}

/**
* @brief   Look up zoneInfo in the zonemap file by countryCode and regionID
*
* @param   zonemap      [in] Zone map file path
* @param   countryCode  [in] Country code of the new zone
* @param   regionID     [in] Region ID of the new zone
* @param   zoneInfo     [out] Store the zone info matched to the countryCode and regionID
*
* @retval  None
*
*/
static int timemgr_get_zoneInfo_by_countryCode_and_regionID(
    const char * zonemap,
    const char * countryCode,
    int regionID,
    char zoneInfo[MAX_ZONEINFO])
{
    int ret = -1;
    FILE * fmap = NULL;
    char buf[LINE_MAX];
    char * p = NULL;
    char strCountryCode[MAX_ZONEINFO];
    char strRegionID[MAX_ZONEINFO];
    char strZoneInfo[MAX_ZONEINFO];
    char * argv[3] = {strCountryCode, strRegionID, strZoneInfo};
    int n=0;
    int argc = 0;

    memset(zoneInfo, 0, MAX_ZONEINFO);

    REQUIRE(countryCode);

    fmap = fopen(zonemap, "r");
    if(NULL==fmap)
    {
        TIMEMGR_ERR("Failed to open file %s to map countryCode and regionID to zoneInfo!\n", zonemap);
        return -1;
    }

    for(n=0; !feof(fmap); n++)
    {
        p = fgets(buf, LINE_MAX, fmap);
        if(NULL==p)
        {
            break;
        }

        if(p[0]=='#'||p[0]==';'||p[0]=='\0')
        {
            continue;
        }

        memset(strCountryCode,  0, MAX_ZONEINFO);
        memset(strRegionID,     0, MAX_ZONEINFO);
        memset(strZoneInfo,     0, MAX_ZONEINFO);
        argc = split_string(p, argv, 3, MAX_ZONEINFO);
        if(argc<3)
        {
            continue;
        }

        if(0==strcmp(strCountryCode, countryCode) && regionID==atoi(strRegionID))
        {
            strcpy(zoneInfo, strZoneInfo);
            ret = 0;
            break;
        }
    }

    if(fmap)
    {
        fclose(fmap);
    }
    return ret;
}

/**
 * @brief   Get default zone info file path
 *
 * @param   zoneinfo            [in]    Zone info
 * @param   zoneInfo_file_path  [out]   Default zone info file path
 *
 * @retval  None
 *
 */
static int timemgr_get_default_zoneInfo_file(
    const char      zoneInfo[MAX_ZONEINFO],
    char            zoneInfo_file_path[MAX_PATH] )
{
    strcpy(zoneInfo_file_path, ZONEINFO_FOLDER);
    strcat(zoneInfo_file_path, zoneInfo);
    return 0;
}

/**
 * @brief   Recreate the symbol link at /etc/localtime->/tmp/localtime->zoneInfo_file_path
 *
 * @param   localtime_file_path [in] Localtime file path
 * @param   zoneinfo            [in] Zone info
 *
 * @retval  None
 *
 */
static int timemgr_remove_zoneInfo_patch()
{
    if(s_patched)
    {
        /* localtime was relinked, so clear the patched zone info file */
        s_patched = 0;
        if(0!=remove(ZONEINFO_PATCHED_PATH))
        {
            TIMEMGR_ERR("Failed to remove %s!\n", ZONEINFO_PATCHED_PATH);
        }
    }
    s_checked = 0;/* Always re-check the ltinfo */
    return 0;
}

/**
 * @brief   Recreate the symbol link from localtime_file_path to zoneInfo's default zone info file
 *
 * @param   zoneinfo            [in] Zone info
 *
 * @retval  None
 *
 */
static int timemgr_link_default_zoneInfo_file(
    char        zoneInfo[MAX_ZONEINFO] )
{
    int ret = -1;
    char zoneInfo_file_path[MAX_PATH];
    ret = timemgr_get_default_zoneInfo_file(zoneInfo, zoneInfo_file_path);
    if(0==ret)
    {
        ret = timemgr_link_localtime_to(zoneInfo_file_path);
        if(0==ret)
        {
            /* localtime was relinked, so clear the patched zone info file */
            timemgr_remove_zoneInfo_patch();
        }
    }
    return ret;
}

/**
 * @brief   Recreate the symbol link at /etc/localtime according to the countryCode and regionID
 *
 * @param   countryCode [in] Country code of the new zone
 * @param   regionID    [in] Region ID of the new zone
 * @param   zoneInfo    [out] Store the new zone info according to the countryCode and regionID
 *
 * @retval  None
 *
 */
static int timemgr_set_zoneInfo(
    const char *countryCode,
    int regionID,
    char zoneInfo[MAX_ZONEINFO])
{
    int ret = -1;
    ret = timemgr_get_zoneInfo_by_countryCode_and_regionID(
        ZONEMAP_PATH,
        countryCode,
        regionID,
        zoneInfo);

    if(0==ret)
    {
        ret = timemgr_link_default_zoneInfo_file(zoneInfo);
        ENSURE(0==ret);
    }
	else
	{
		TIMEMGR_ERR("Failed to get zoneInfo from countryCode=%s regionID=%d!\n", countryCode, regionID);
	}
    return ret;
}

/**
 * @brief   Callback function when locale related config has changed
 *
 * @param   keypath     [in] Path of key in config manager
 * @param   value       [in] Value of the new settings
 * @param   user_data   [in] User data used as cookie
 *
 * @retval  None
 *
 */

/**
 * Decide if local time info equals to each other.
 * This function decide whether the zone info file need to be patched.
 */
static int timemgr_ltinfo_equal(o_local_time_info_t * a, o_local_time_info_t * b)
{
    int a_equals_b = 0;
    if(a && b)
    {
        if((a->next_trans_utc > 0) && (b->next_trans_utc > 0))
        {
            /* Compare 5 items if next change time are available */
            if( (a->cur_utc == b->cur_utc) &&
                (a->is_dst == b->is_dst) &&
                (a->offset == b->offset) &&
                (a->next_trans_utc == b->next_trans_utc) &&
                (a->next_offset == b->next_offset) )
            {
                a_equals_b = 1;
            }
        }
        else
        {
            /* Just compare 3 items if next change time is not available. */
            if( (a->cur_utc == b->cur_utc) &&
                (a->is_dst == b->is_dst) &&
                (a->offset == b->offset) )
            {
                a_equals_b = 1;
            }
        }
    }
    return a_equals_b;
}

/*********************************************************
 *                   Public Functions                    *
 *********************************************************/

/**
 * @brief   Init locale watcher
 *
 * @param   None
 *
 * @retval  0   SUCCESS
 * @retval  1   FAIL
 *
 */
int timemgr_locale_watcher_init(void)
{
}

/**
 * @brief   Get locale
 *
 * @param   None
 *
 * @retval
 *
 */
int timemgr_get_locale(char countryCode[4], int *regionID)
{
    int ret = -1;

    pthread_mutex_lock(&s_mutex);
    if(s_local_watcher_initialized)
    {
        strcpy(countryCode, s_countryCode);
        *regionID = s_regionID;
        ret = 0;
    }
    pthread_mutex_unlock(&s_mutex);
    return ret;
}

/* Check if the zoneInfo file need to be patched */
static int timemgr_check_if_zoneInfo_need_patch( o_local_time_info_t * ltinfo_remote )
{
    int ret = -1;
    int need_patch = 0;
    o_local_time_info_t ltinfo_sys;

    TIMEMGR_TRC("Check zone (%s:%d) with info: [%d:%d][%d:%d:%d][%d:%d]\n",
        s_countryCode, s_regionID,
        (int)ltinfo_remote->prev_trans_utc, ltinfo_remote->prev_offset,
        (int)ltinfo_remote->cur_utc, ltinfo_remote->offset, ltinfo_remote->is_dst,
        (int)ltinfo_remote->next_trans_utc, ltinfo_remote->next_offset);

    ret = timemgr_get_ltinfo(ltinfo_remote->cur_utc, &ltinfo_sys);
    ENSURE(0==ret);

    TIMEMGR_TRC("Zone (%s:%d)'s info is [%d:%d][%d:%d:%d][%d:%d] at %s",
        s_countryCode, s_regionID,
        (int)ltinfo_sys.prev_trans_utc, ltinfo_sys.prev_offset,
        (int)ltinfo_sys.cur_utc, ltinfo_sys.offset, ltinfo_sys.is_dst,
        (int)ltinfo_sys.next_trans_utc, ltinfo_sys.next_offset,
        asctime(gmtime(&ltinfo_remote->cur_utc)));

    if(0==ret)
    {
        if(timemgr_ltinfo_equal(ltinfo_remote, &ltinfo_sys))
        {
            TIMEMGR_TRC("TOT's local time info match the zone info file!\n");
            ret = 0;
        }
        else
        {
            TIMEMGR_DBG("TOT's local time info doesn't match current zone info file!\n");

            if(s_patched)
            {
                /* Resotre original zone info file and check it again. */
                ret = timemgr_link_default_zoneInfo_file(s_zoneInfo);
                ENSURE(0==ret);

                /* s_patched will be reset to 0 in timemgr_link_default_zoneInfo_file()
                 * Reset here again to make sure. */
                s_patched = 0;

                if(0==ret)
                {
                    /* Get unpatched ltinfo */
                    ret = timemgr_get_ltinfo(ltinfo_remote->cur_utc, &ltinfo_sys);
                    ENSURE(0==ret);

                    if(timemgr_ltinfo_equal(ltinfo_remote, &ltinfo_sys))
                    {
                        TIMEMGR_TRC("TOT's local time info match original info file!\n");
                        ret = 0;
                    }
                    else
                    {
                        TIMEMGR_TRC("TOT's local time info doesn't match original info file!\n");
                        need_patch = 1;
                    }
                }
                else
                {
                    TIMEMGR_ERR("Failed to resotre original zoneInfo file!\n");
                    ret = -1;
                }
            }
            else
            {
                need_patch = 1;
            }
        }
    }
    else
    {
        TIMEMGR_ERR("Failed to get ltinfo!\n");
    }
    memcpy(&s_checked_ltinfo, ltinfo_remote, sizeof(s_checked_ltinfo));
    s_checked = 1;
    return need_patch;
}


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
int timemgr_check_and_patch_zoneInfo(o_local_time_info_t * ltinfo)
{
    int ret = -1;
    char zoneInfo_file_path[MAX_PATH];
    o_local_time_info_t ltinfo_sys;

    REQUIRE(ltinfo);

    if(NULL==ltinfo)
    {
        TIMEMGR_ERR("Invalid parameter!\n");
        return -1;
    }

    pthread_mutex_lock(&s_mutex);

    /* Check if it already have been patched by same ltinfo */
    if(s_checked)
    {
        if( (ltinfo->offset == s_checked_ltinfo.offset) &&
            (ltinfo->is_dst == s_checked_ltinfo.is_dst) &&
            (ltinfo->next_trans_utc == s_checked_ltinfo.next_trans_utc) &&
            (ltinfo->next_offset == s_checked_ltinfo.next_offset) &&
            (ltinfo->prev_trans_utc == s_checked_ltinfo.prev_trans_utc) &&
            (ltinfo->prev_offset == s_checked_ltinfo.prev_offset))
        {
            TIMEMGR_TRC("Already checked same ltinfo! Patch status: %s\n",
                (s_patched ? (s_patched_ok ? "PATCH-SUCCESS" : "PATCH-FAILED") : "NO-NEED-PATCH"));
            ret = 0;
            pthread_mutex_unlock(&s_mutex);
            return ret;
        }
        else
        {
            s_checked = 0;
            TIMEMGR_TRC("Need to re-check the ltinfo!\n");
        }
    }

    if(timemgr_check_if_zoneInfo_need_patch(ltinfo))
    {
        TIMEMGR_TRC("zoneInfo needs to patch!\n");

        ret = timemgr_get_default_zoneInfo_file(s_zoneInfo, zoneInfo_file_path);
        ENSURE(0==ret);

        if(0==ret)
        {
            ret = timemgr_patch_zoneInfo_file(zoneInfo_file_path, ZONEINFO_PATCHED_PATH, ltinfo);
            if(0==ret)
            {
                ret = timemgr_link_localtime_to(ZONEINFO_PATCHED_PATH);
                ENSURE(0==ret);
                if(0==ret)
                {
                    s_patched = 1;

                    /* After patch the zoneinfo file, send out a event to notify client local time may changed.
                     * It is more accurate to send a TIME_ZONE_PATCHED event while we borrow TIME_ZONE_CHANGED for now.*/
                    timemgr_send_event(TIMEMGR_EVENT_TIME_ZONE_CHANGED);

                    /**************************************************/
                    /*             Check if it works after patch      */
                    /**************************************************/
                    ret = timemgr_get_ltinfo(ltinfo->cur_utc, &ltinfo_sys);
                    ENSURE(0==ret);

                    if(timemgr_ltinfo_equal(ltinfo, &ltinfo_sys))
                    {
                        TIMEMGR_DBG("Patch zoneInfo file succeed!\n");
                        s_patched_ok = 1;
                        ret = 0;
                    }
                    else
                    {
                        TIMEMGR_DBG("Patch zoneInfo file failed!\n");
                        TIMEMGR_DBG("At time utc=%d %s", (int)ltinfo->cur_utc, asctime(gmtime(&ltinfo->cur_utc)));
                        TIMEMGR_DBG("Remote zone info for %s:%d is [%d:%d][%d:%d:%d][%d:%d]\n",
                            s_countryCode, s_regionID,
                            (int)ltinfo->prev_trans_utc, ltinfo->prev_offset,
                            (int)ltinfo->cur_utc, ltinfo->offset, ltinfo->is_dst,
                            (int)ltinfo->next_trans_utc, ltinfo->next_offset);
                        TIMEMGR_DBG("Pathed zone info for %s:%d is [%d:%d][%d:%d:%d][%d:%d]\n",
                            s_countryCode, s_regionID,
                            (int)ltinfo_sys.prev_trans_utc, ltinfo_sys.prev_offset,
                            (int)ltinfo_sys.cur_utc, ltinfo_sys.offset, ltinfo_sys.is_dst,
                            (int)ltinfo_sys.next_trans_utc, ltinfo_sys.next_offset);
                        s_patched_ok = 0;
                        ret = -1;
                    }
                }
            }
        }
    }
    else
    {
        TIMEMGR_TRC("zoneInfo is fine! Don't need to patch!\n");
        ret = 0;
    }
    pthread_mutex_unlock(&s_mutex);
    return ret;
}

#ifndef NDEBUG

/*********************************************
            Code for Self-Test
 *********************************************/

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
int timemgr_selftest_lookup_zoneInfo(const char * countryCode, int regionID)
{
    int ret = -1;
    char zoneInfo[MAX_ZONEINFO];

    ret = timemgr_get_zoneInfo_by_countryCode_and_regionID(
        ZONEMAP_PATH,
        countryCode,
        regionID,
        zoneInfo);
    if(ret)
    {
        printf("Failed to search zoneInfo for countryCode=%s regionID=%d in %s!\n",
            countryCode,
            regionID,
            ZONEMAP_PATH);
    }
    else
    {
        printf("ZoneInfo found! %s\n", zoneInfo);
    }
    return ret;
}

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
int timemgr_selftest_set_zone(char * ccode, int rid)
{
    int ret = -1;
    time_t t = time(NULL);
    char zoneInfo[MAX_ZONEINFO];

    memset(zoneInfo, 0, MAX_ZONEINFO);

    if(rid<0)
    {
        printf("Invalid rid %d\n", rid);
        return -1;
    }

    printf("    UTC time        :   %s", asctime(gmtime(&t)));

    ret = timemgr_set_zoneInfo(ccode, rid, zoneInfo);
    if(ret)
    {
        printf("Failed to set zone for %s:%d\n", ccode, rid);
        return ret;
    }
    else
    {
        /* Update the global variable for test code after this one */
        memcpy(s_zoneInfo, zoneInfo, MAX_ZONEINFO);
        strcpy(s_countryCode, ccode);
        s_regionID = rid;
    }

    printf("%s:%d maps to zone %s\n", ccode, rid, zoneInfo);
    printf("    Local time      :   %s", asctime(localtime(&t)));

    return ret;
}

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
    int next_offset )
{
    int ret = -1;
    o_local_time_info_t ltinfo;
    time_t cur_utc = timemgr_parse_utc(strTime);
    time_t switch_utc = timemgr_parse_utc(strNextChange);

    ltinfo.cur_utc = cur_utc;

    if(offset==next_offset)
    {
        ltinfo.is_dst = -1;
        ltinfo.offset = offset;
        ltinfo.next_trans_utc = 0;
        ltinfo.next_offset = 0;
        ltinfo.prev_trans_utc = 0;
        ltinfo.prev_offset = 0;
    }
    else
    {
        if(offset < next_offset)
        {
            /* change is for DST_ON */
            ltinfo.is_dst = (cur_utc<switch_utc)?0:1;
        }
        else if(offset > next_offset)
        {
            /* The change is for DST_OFF */
            ltinfo.is_dst = (cur_utc<switch_utc)?1:0;
        }

        if(cur_utc<switch_utc)
        {
            ltinfo.offset = offset;
            ltinfo.next_trans_utc = switch_utc;
            ltinfo.next_offset = next_offset;
            ltinfo.prev_trans_utc = 0;
            ltinfo.prev_offset = 0;
        }
        else
        {
            ltinfo.offset = next_offset;
            ltinfo.next_trans_utc = 0;
            ltinfo.next_offset = 0;
            ltinfo.prev_trans_utc = switch_utc;
            ltinfo.prev_offset = offset;
        }
    }

    ret= timemgr_check_and_patch_zoneInfo(&ltinfo);
    TIMEMGR_ERR("Check and patch %s!\n", ret?"failed":"succeed");

    return ret;
}


#endif

