 * ============================================================================
 */

/**
 * @file     timemgr_zone_patcher.c
 * @author   Lianghe Li
 * @date     Oct 15, 2012
 * @brief    Timemgr zone info file patcher
 * @ingroup  TIMEMGR
 * This file implement patch zone info files according to local time offset descriptor.
 * @{
 */
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include "timemgr.h"
#include "timemgr_internal.h"
#include "outils.h"

/**********************************************************
 *                      Definition                        *
 **********************************************************/
#define MAX_OFFSET          (12*HOUR_SEC)
#define MAX_M_STR           (32)
#define MAX_OFFSET_STR      (32)
#define MAX_TZ              (64)
#define MAX_ABBR_NAME       (16)
#define TZI_HEADER_SIZE     (44)
#define TZI_TYPE_SIZE       (6)         /* 4 byte offset + 1 byte is_dst + 1 byte abbr_index */
#define TZI_LEAP_SIZE       (8)         /* 4 byte utc + 4 byte leap_sec */
#define TZI_LEAP_V2_SIZE    (12)        /* 8 byte utc + 4 byte leap_sec */

#define MAX_TRANS           (1500)      /* 1200 in zic.c    */
#define MAX_TYPE            (256)       
#define MAX_ABBR            (128)       /* 50 in zic.c      */
#define MAX_LEAP            (128)       /* 50 in zic.c  */

typedef struct
{
    uint8_t     byte[4];     
}b4_int_t;

typedef struct
{
    uint8_t     byte[8];
}b8_int_t;

#define TZI_UTC_SIZE    (4)
#define TZI_UTC_V2_SIZE (8)

typedef enum
{
    TZI_HEADER,
    TZI_UTC,
    TZI_TYPE_INDEX,
    TZI_TYPE,
    TZI_ABBR,
    TZI_LEAP,
    TZI_IS_STD,
    TZI_IS_UTC,
    TZI_END    
}tzi_item_t;

typedef struct
{
    int32_t     utc_offset;
    int8_t      is_dst;
    int8_t      abbr_index;
}tzi_type_t;

typedef struct
{
    int32_t utc;
    int32_t leap_sec;
}tzi_leap_t;

typedef struct
{
    int64_t utc;
    int32_t leap_sec;
}tzi_leap_v2_t;

typedef struct
{
    char        signature[4];   /* TZif */
    char        version;        /* '2' */
    char        rfu[15];
    int32_t     is_utc_cnt;     /* The number of UTC/local indicators stored in the file. */
    int32_t     is_std_cnt;     /* The number of standard/wall indicators stored in the file. */
    int32_t     leap_cnt;       /* The number of leap seconds for which data is stored in the file. */
    int32_t     trans_cnt;      /* The number of "transition times" for which data is stored in the file. */
    int32_t     type_cnt;       /* The number of "local time types" for which data is stored in the file (must not be zero). */
    int32_t     abbr_cnt;       /* The number of characters of "timezone abbreviation strings" stored in the file.*/
    
}tzi_header_t;

typedef struct
{
    int32_t             utc[MAX_TRANS];
    int8_t              type_index[MAX_TRANS];
    tzi_type_t          type[MAX_TYPE];
    char                abbr[MAX_ABBR];
    tzi_leap_t          leap[MAX_LEAP];
    int8_t              is_std[MAX_TYPE];
    int8_t              is_utc[MAX_TYPE];
}tzi_content_t;

typedef struct
{
    int64_t             utc[MAX_TRANS];
    int8_t              type_index[MAX_TRANS];
    tzi_type_t          type[MAX_TYPE];
    char                abbr[MAX_ABBR];
    tzi_leap_v2_t       leap[MAX_LEAP];
    int8_t              is_std[MAX_TYPE];
    int8_t              is_utc[MAX_TYPE];
}tzi_content_v2_t;

typedef struct
{
    FILE *              fp;
    void *              mapped_addr;
    size_t              mapped_size;
    size_t              file_size;

    int                 version;    /* 1 or 2 */

    tzi_header_t        header;
    tzi_content_t       content;

    tzi_header_t        header_v2;
    tzi_content_v2_t    content_v2;

    char                TZ[MAX_TZ];

}tzi_t;

#define POSSIBLE_VER(v)     (1==v||2==v)
#define VALID_VER(tzi, v)   (POSSIBLE_VER(v) && (v <= tzi->version))

static void b4_to_int32(b4_int_t * a, int32_t *b)
{
    int32_t tmp = a->byte[0];
    int i = 0;
    for(i=1;i<4;i++)
    {
        tmp <<= 8;
        tmp |= a->byte[i];
    }
    *b = tmp;
}

static void b8_to_int64(b8_int_t * a, int64_t *b)
{
    int64_t tmp = a->byte[0];
    int i = 0;
    for(i=1;i<8;i++)
    {
        tmp <<= 8;
        tmp |= a->byte[i];
    }
    *b = tmp;
}

static void int32_to_b4(int32_t *a, b4_int_t *b)
{
    b->byte[0] = (((*a) & 0xFF000000) >> 24);
    b->byte[1] = (((*a) & 0x00FF0000) >> 16);
    b->byte[2] = (((*a) & 0x0000FF00) >> 8);
    b->byte[3] = (((*a) & 0x000000FF));
}

static void int64_to_b8(int64_t *a, b8_int_t *b)
{
    int32_t hi, lo;
    hi = (*a) >> 32;
    lo = (*a) & 0xFFFFFFFF;

    b->byte[0] = ((hi & 0xFF000000) >> 24);
    b->byte[1] = ((hi & 0x00FF0000) >> 16);
    b->byte[2] = ((hi & 0x0000FF00) >> 8);
    b->byte[3] = ((hi & 0x000000FF));

    b->byte[4] = ((lo & 0xFF000000) >> 24);
    b->byte[5] = ((lo & 0x00FF0000) >> 16);
    b->byte[6] = ((lo & 0x0000FF00) >> 8);
    b->byte[7] = ((lo & 0x000000FF));
}

typedef struct
{
    int     version;
    int32_t trans_index;
    int32_t trans_cnt;

    time_t  utc;
    int32_t utc32;
    int64_t utc64;

	int8_t  type_index;
    int32_t offset;
    int8_t  is_dst;
    int8_t  abbr_index;
    char    abbr[MAX_ABBR_NAME];
    int8_t  is_std;
    int8_t  is_utc;

    time_t  next_utc;
    
}tzi_trans_t;

#ifndef NDEBUG
#define TZI_LOG_PATH        "/tmp/tzi_debug.log"
static int s_tzi_debug_cnt = 0;
static void tzi_debug(tzi_t * tzi, int ret, const char *func, int line)
{
    FILE * fp = NULL;
    fp = fopen(TZI_LOG_PATH, "a");
    if(fp)
    {
        fprintf(fp, "[DBG_POINT %03d] ret=%d\t%s : %d !\n", s_tzi_debug_cnt, ret, func, line);
        fclose(fp);
    }
    s_tzi_debug_cnt++;
}

static void tzi_debug_init(
    const char * original_path, 
    const char * patched_path,
    o_local_time_info_t *ltinfo)
{
    FILE * fp = NULL;
    fp = fopen(TZI_LOG_PATH, "w");
    if(fp)
    {
        if(original_path && ltinfo)
        {
            fprintf(fp, "Patch %s with following info to %s:\n", original_path, patched_path);
            fprintf(fp, "   cur_utc        = %s",   asctime(gmtime(&ltinfo->cur_utc)));
            fprintf(fp, "   offset         = %d\n", ltinfo->offset);
            fprintf(fp, "   is_dst         = %d\n", ltinfo->is_dst);
            fprintf(fp, "   prev_trans_utc = %s",   asctime(gmtime(&ltinfo->prev_trans_utc)));
            fprintf(fp, "   prev_offset    = %d\n", ltinfo->prev_offset);
            fprintf(fp, "   next_trans_utc = %s",   asctime(gmtime(&ltinfo->next_trans_utc)));
            fprintf(fp, "   next_offset    = %d\n", ltinfo->next_offset);
        }
        fclose(fp);
    }
}

#define TZI_DEBUG(tzi, ret)     tzi_debug(tzi, ret, __FUNCTION__, __LINE__)
#else
#define TZI_DEBUG(tzi, ret)
static void tzi_debug_init(
    const char * original_path, 
    const char * patched_path,
    o_local_time_info_t *ltinfo)
{
    return;
}
#endif

static void tzi_print_header(tzi_header_t * hdr)
{
    if(hdr)
    {
        printf("signature               :   %s\n", hdr->signature);
        printf("is_utc_cnt              :   %u\n", hdr->is_utc_cnt);
        printf("is_std_cnt              :   %u\n", hdr->is_std_cnt);
        printf("leap_cnt                :   %u\n", hdr->leap_cnt);
        printf("trans_cnt               :   %u\n", hdr->trans_cnt);
        printf("type_cnt                :   %u\n", hdr->type_cnt);
        printf("abbr_cnt                :   %u\n", hdr->abbr_cnt);
    }
}

static int tzi_serialize_header(
    void *addr,
    tzi_header_t *header,
    int is_save)
{
    uint8_t *p = (uint8_t *)addr;
    b4_int_t * pCnt = NULL;
    REQUIRE(addr);
    REQUIRE(header);

    if(NULL==addr||NULL==header)
    {
        return -1;
    }

    if(is_save)
    {
        if( (0!=memcmp(header->signature, "TZif", 4)) || 
            (0!=header->version && '2'!=header->version) ||
            (header->trans_cnt > MAX_TRANS) ||
            (header->type_cnt > MAX_TYPE)   ||
            (header->is_utc_cnt > MAX_TYPE) ||
            (header->is_std_cnt > MAX_TYPE) ||
            (header->leap_cnt > MAX_LEAP)   ||
            (header->abbr_cnt > MAX_ABBR) )
        {
            TIMEMGR_ERR("ZoneInfo header error!\n");
            tzi_print_header(header);
            return -1;
        }
    }

    if(is_save)
    {
        memcpy(p, header->signature, 4);
    }
    else
    {
        memcpy(header->signature, p, 4);
    }

    if(is_save)
    {
        p[4] = header->version;
    }
    else
    {
        header->version = p[4];
    }

    if(is_save)
    {
        memcpy(p+5, header->rfu, 15);
    }
    else
    {
        memcpy(header->rfu, p+5, 15);
    }

    pCnt = (b4_int_t *)(p + 20);
    if(is_save)
    {
        int32_to_b4(&header->is_utc_cnt, pCnt);
    }
    else
    {
        b4_to_int32(pCnt, &header->is_utc_cnt);
    }
    pCnt++;

    if(is_save)
    {
        int32_to_b4(&header->is_std_cnt, pCnt);
    }
    else
    {
        b4_to_int32(pCnt, &header->is_std_cnt);
    }
    pCnt++;

    if(is_save)
    {
        int32_to_b4(&header->leap_cnt, pCnt);
    }
    else
    {
        b4_to_int32(pCnt, &header->leap_cnt);
    }
    pCnt++;

    if(is_save)
    {
        int32_to_b4(&header->trans_cnt, pCnt);
    }
    else
    {
        b4_to_int32(pCnt, &header->trans_cnt);
    }
    pCnt++;
    
    if(is_save)
    {
        int32_to_b4(&header->type_cnt, pCnt);
    }
    else
    {
        b4_to_int32(pCnt, &header->type_cnt);
    }
    pCnt++;

    if(is_save)
    {
        int32_to_b4(&header->abbr_cnt, pCnt);
    }
    else
    {
        b4_to_int32(pCnt, &header->abbr_cnt);
    }

    if(!is_save)
    {
        if( (0!=memcmp(header->signature, "TZif", 4)) || 
            (0!=header->version && '2'!=header->version) ||
            (header->trans_cnt > MAX_TRANS) ||
            (header->type_cnt > MAX_TYPE)   ||
            (header->is_utc_cnt > MAX_TYPE) ||
            (header->is_std_cnt > MAX_TYPE) ||
            (header->leap_cnt > MAX_LEAP)   ||
            (header->abbr_cnt > MAX_ABBR) )
        {
            TIMEMGR_ERR("ZoneInfo header error!\n");
            tzi_print_header(header);
            return -1;
        }
    }

    return 0;
}

static size_t tzi_offset(tzi_header_t *header, int version, tzi_item_t item)
{
    size_t s = 0;
    REQUIRE(header);
    REQUIRE(POSSIBLE_VER(version));
    REQUIRE(item>=TZI_HEADER&&item<=TZI_END);
    if(item>TZI_HEADER)
    {
        s += TZI_HEADER_SIZE;
    }
    if(item>TZI_UTC)
    {
        s += (header->trans_cnt * (version<2 ? TZI_UTC_SIZE : TZI_UTC_V2_SIZE));
    }
    if(item>TZI_TYPE_INDEX)
    {
        s += (header->trans_cnt * sizeof(int8_t));
    }
    if(item>TZI_TYPE)
    {
        s += (header->type_cnt * TZI_TYPE_SIZE);
    }
    if(item>TZI_ABBR)
    {
        s += (header->abbr_cnt * sizeof(int8_t));
    }
    if(item>TZI_LEAP)
    {
        s += (header->leap_cnt * ((version<2) ? TZI_LEAP_SIZE : TZI_LEAP_V2_SIZE));
    }
    if(item>TZI_IS_STD)
    {
        s += (header->is_std_cnt * sizeof(int8_t));
    }
    if(item>TZI_IS_UTC)
    {
        s += (header->is_utc_cnt * sizeof(int8_t));
    }
    if(item>TZI_END)
    {
        TIMEMGR_ERR("Invalid item!\n");
        s = 0;
    }
    return s;
}

static int tzi_serialize_content(
    void * addr,
    tzi_header_t *hdr,
    tzi_content_t * content,
    int is_save)
{
    int i = 0;
    uint8_t *p = (uint8_t *)addr + tzi_offset(hdr, 1, TZI_UTC);

    REQUIRE(addr);
    REQUIRE(hdr);
    REQUIRE(content);

    if(NULL==addr||NULL==hdr||NULL==content)
    {
        return -1;
    }

    /* Trans */
    for(i=0; i<hdr->trans_cnt; i++)
    {
        if(is_save)
        {
            int32_to_b4(&content->utc[i], (b4_int_t *)p);
        }
        else
        {
            b4_to_int32((b4_int_t *)p, &content->utc[i]);
        }
        p += TZI_UTC_SIZE;
    }
    
    /* Type Index */
    if(is_save)
    {
        memcpy( p, content->type_index, hdr->trans_cnt);
    }
    else
    {
        memcpy( content->type_index, p, hdr->trans_cnt);
    }
    p += hdr->trans_cnt;

    /* Type */
    for(i=0; i<hdr->type_cnt; i++)
    {
        if(is_save)
        {
            int32_to_b4( &content->type[i].utc_offset, (b4_int_t *)p);
        }
        else
        {
            b4_to_int32( (b4_int_t *)p, &content->type[i].utc_offset);
        }
        p += sizeof(b4_int_t);

        if(is_save)
        {
            *p = content->type[i].is_dst;
        }
        else
        {
            content->type[i].is_dst = *p;
        }
        p++;

        if(is_save)
        {
            *p = content->type[i].abbr_index;
        }
        else
        {
            content->type[i].abbr_index = *p;
        }
        p++;
    }

    /* abbr */
    if(is_save)
    {
        memcpy(p, content->abbr, hdr->abbr_cnt);
    }
    else
    {
        memcpy(content->abbr, p, hdr->abbr_cnt);
    }
    p += hdr->abbr_cnt;

    /* leap */
    for(i=0; i<hdr->leap_cnt; i++)
    {
        if(is_save)
        {
            int32_to_b4(&content->leap[i].utc, (b4_int_t *)p);
        }
        else
        {
            b4_to_int32( (b4_int_t *)p, &content->leap[i].utc);
        }
        p += TZI_UTC_SIZE;

        if(is_save)
        {
            int32_to_b4( &content->leap[i].leap_sec, (b4_int_t *)p);
        }
        else
        {
            b4_to_int32( (b4_int_t *)p, &content->leap[i].leap_sec);
        }
        p += sizeof(b4_int_t);
    }

    /* is_std */
    if(is_save)
    {
        memcpy(p, content->is_std, hdr->is_std_cnt);
    }
    else
    {
        memcpy(content->is_std, p, hdr->is_std_cnt);
    }
    p += hdr->is_std_cnt;

    /* is_utc */
    if(is_save)
    {
        memcpy(p, content->is_utc, hdr->is_utc_cnt);
    }
    else
    {
        memcpy(content->is_utc, p, hdr->is_utc_cnt); 
    }
    p += hdr->is_utc_cnt;
    
    return 0;
}

static int tzi_serialize_content_v2(
    void * addr,
    tzi_header_t *hdr,
    tzi_content_v2_t * content,
    int is_save)
{
    int i = 0;
    uint8_t *p = (uint8_t *)addr + tzi_offset(hdr, 2, TZI_UTC);

    REQUIRE(addr);
    REQUIRE(hdr);
    REQUIRE(content);

    if(NULL==addr||NULL==hdr||NULL==content)
    {
        return -1;
    }

    /* Trans */
    for(i=0; i<hdr->trans_cnt; i++)
    {
        if(is_save)
        {
            int64_to_b8( &content->utc[i], (b8_int_t *)p);
        }
        else
        {
            b8_to_int64( (b8_int_t *)p, &content->utc[i]);
        }
        p += TZI_UTC_V2_SIZE;
    }
    
    /* Type Index */
    if(is_save)
    {
        memcpy( p, content->type_index, hdr->trans_cnt);
    }
    else
    {
        memcpy( content->type_index, p, hdr->trans_cnt);
    }
    p += hdr->trans_cnt;

    /* Type */
    for(i=0; i<hdr->type_cnt; i++)
    {
        if(is_save)
        {
            int32_to_b4( &content->type[i].utc_offset, (b4_int_t *)p);
        }
        else
        {
            b4_to_int32( (b4_int_t *)p, &content->type[i].utc_offset);
        }
        p += sizeof(b4_int_t);

        if(is_save)
        {
            *p = content->type[i].is_dst;
        }
        else
        {
            content->type[i].is_dst = *p;
        }
        p++;

        if(is_save)
        {
            *p = content->type[i].abbr_index;
        }
        else
        {
            content->type[i].abbr_index = *p;
        }
        p++;
    }

    /* abbr */
    if(is_save)
    {
        memcpy(p, content->abbr, hdr->abbr_cnt);
    }
    else
    {
        memcpy(content->abbr, p, hdr->abbr_cnt);
    }
    p += hdr->abbr_cnt;

    /* leap */
    for(i=0; i<hdr->leap_cnt; i++)
    {
        if(is_save)
        {
            int64_to_b8( &content->leap[i].utc, (b8_int_t *)p);
        }
        else
        {
            b8_to_int64( (b8_int_t *)p, &content->leap[i].utc);
        }
        p += TZI_UTC_V2_SIZE;

        if(is_save)
        {
            int32_to_b4( &content->leap[i].leap_sec, (b4_int_t *)p);
        }
        else
        {
            b4_to_int32( (b4_int_t *)p, &content->leap[i].leap_sec);
        }
        p += sizeof(b4_int_t);
    }

    /* is_std */
    if(is_save)
    {
        memcpy(p, content->is_std, hdr->is_std_cnt);
    }
    else
    {
        memcpy(content->is_std, p, hdr->is_std_cnt);
    }
    p += hdr->is_std_cnt;

    /* is_utc */
    if(is_save)
    {
        memcpy(p, content->is_utc, hdr->is_utc_cnt); 
    }
    else
    {
        memcpy(content->is_utc, p, hdr->is_utc_cnt); 
    }
    p += hdr->is_utc_cnt;

    return 0;
}

static tzi_t * tzi_open(
    const char *    zoneInfo_file_path )
{
    int ret = 0;
    tzi_t * tzi = NULL;
    FILE * fp = NULL;
    size_t file_size = 0;
    int    truncated = 0;
    size_t mapped_size = 0;
    void * mapped_addr = MAP_FAILED;
    size_t page_size = sysconf(_SC_PAGE_SIZE);
    tzi_header_t hdr, hdr_v2;
    uint8_t *p1 = NULL;
    uint8_t *p2 = NULL;
    
    char * newline1 = NULL;
    char * newline2 = NULL;
    size_t TZ_size = 0;

    REQUIRE(zoneInfo_file_path);

    if(strstr(zoneInfo_file_path, "/usr/share/zoneinfo"))
    {
        TIMEMGR_ERR("Can't write file under /usr/share/zoneinfo!\n");
        return NULL;
    }

    fp = fopen(zoneInfo_file_path, "r+");
    if(NULL==fp)
    {
        TIMEMGR_ERR("Failed to open zoneInfo file %s!\n", zoneInfo_file_path);
        ret = -1;
    }

    if(0==ret)
    {
        fseek(fp, 0, SEEK_END);
        file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        /* normal page-aligned size */
        mapped_size = (( file_size + page_size -1 ) / page_size ) * page_size;

        /* Add one page to allow extending the file */
        mapped_size += page_size;      
        ret = ftruncate(fileno(fp), mapped_size);
        if(ret)
        {
            TIMEMGR_ERR("Failed to truncate file!\n");
        }
        else
        {
            truncated = 1;
        }
    }

    if(0==ret)
    {
        mapped_addr = mmap(NULL, mapped_size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_LOCKED, fileno(fp), 0);
        if(MAP_FAILED==mapped_addr)
        {
            TIMEMGR_ERR("Failed to map zoneInfo file %s!\n", zoneInfo_file_path);
            ret = -1;
            TZI_DEBUG(NULL, ret);
        }
    }

    if(0==ret)
    {
        p1 = (uint8_t *)mapped_addr;
        ret = tzi_serialize_header(p1, &hdr, 0);
        TZI_DEBUG(NULL, ret);
    }

    if(0==ret && hdr.version=='2')
    {
        p2 = p1 + tzi_offset(&hdr, 1, TZI_END);
        ret = tzi_serialize_header(p2, &hdr_v2, 0);
        TZI_DEBUG(NULL, ret);
    }

    if(0==ret)
    {
        tzi = (tzi_t *)malloc(sizeof(tzi_t));
        if(NULL==tzi)
        {
            TIMEMGR_ERR("Failed to allocate memory!\n");
            ret = -1;
            TZI_DEBUG(NULL, ret);
        }
    }

    if(0==ret)
    {
        memset(tzi, 0, sizeof(tzi_t));
        tzi->fp          = fp;
        tzi->file_size   = file_size;
        tzi->mapped_addr = mapped_addr;
        tzi->mapped_size = mapped_size;
        tzi->version     = 1;
        
        memcpy(&tzi->header, &hdr, sizeof(tzi_header_t));
        memcpy(&tzi->header_v2, &hdr_v2, sizeof(tzi_header_t));

        ret = tzi_serialize_content(p1, &hdr, &tzi->content, 0);
        TZI_DEBUG(NULL, ret);
    }
    
    if(0==ret && p2)
    {
        tzi->version = 2;
        ret = tzi_serialize_content_v2( p2, &hdr_v2, &tzi->content_v2, 0);
        TZI_DEBUG(NULL, ret);

        if(0==ret)
        {
            newline1 = (char *)p2 + tzi_offset(&hdr_v2, 2, TZI_END);
            if('\n'==(*newline1))
            {
                newline2 = strstr( newline1 + 1, "\n");
                if(newline2 && newline2 > (newline1 + 1))
                {
                    TZ_size = newline2 - newline1 - 1;
                    if(TZ_size<MAX_TZ)
                    {
                        memcpy(tzi->TZ, newline1+1, TZ_size);
                    }
                    else
                    {
                        TIMEMGR_ERR("Invalid TZ size %u!\n", TZ_size);
                    }
                }
            }
        }
    }
    
    if(ret)
    {
        if(MAP_FAILED != mapped_addr)
        {
            if(0!=munmap(mapped_addr, mapped_size))
            {
                TIMEMGR_ERR("Failed to unmap the mapped memory!\n");
            }
            /* Truncate the file back to its original size. */
            if(truncated && fp && file_size>0)
            {
                if(0!=ftruncate(fileno(fp), file_size))
                {
                    TIMEMGR_ERR("Failed to truncate file!\n");
                }
            }
        }
        if(fp)
        {
            fclose(fp);
        }
        if(tzi)
        {
            free(tzi);
            tzi = NULL;
        }
        TIMEMGR_ERR("Failed to open zoneInfo file %s!\n", zoneInfo_file_path);
    }
    return tzi;
}

static int tzi_save(
    tzi_t * tzi )
{
    int ret = -1;
    uint8_t *p1 = NULL;
    uint8_t *p2 = NULL;
    char * newline = NULL;
    REQUIRE(tzi);
    if(tzi)
    {
        p1 = (uint8_t *)tzi->mapped_addr;
        if(tzi->version >= 1)
        {
            ret = tzi_serialize_header( p1, &tzi->header, 1);
            TZI_DEBUG(tzi, ret);
            
            if(0==ret)
            {
                ret = tzi_serialize_content( p1, &tzi->header, &tzi->content, 1);
                TZI_DEBUG(tzi,ret);
            }
        }

        if(0==ret && tzi->version >= 2)
        {
            p2 = p1 + tzi_offset(&tzi->header, 1, TZI_END);
            ret = tzi_serialize_header( p2, &tzi->header_v2, 1);
            TZI_DEBUG(tzi, ret);
            
            if(0==ret)
            {
                ret = tzi_serialize_content_v2( p2, &tzi->header_v2, &tzi->content_v2, 1);
                TZI_DEBUG(tzi,ret);
            }

            if(0==ret)
            {
                newline = (char *)p2 + tzi_offset(&tzi->header_v2, 2, TZI_END);

                *newline = '\n';
                newline++;

                memcpy(newline, tzi->TZ, strlen(tzi->TZ));
                newline += strlen(tzi->TZ); 

                *newline = '\n';
                newline++;

                ENSURE(newline == ((char *)tzi->mapped_addr + tzi->file_size));
            }
        }
    }
    return ret;
}


static void tzi_close(
    tzi_t * tzi )
{
    REQUIRE(tzi);
    if(tzi)
    {
        if(MAP_FAILED!=tzi->mapped_addr)
        {
            if(0!=munmap(tzi->mapped_addr, tzi->mapped_size))
            {
                TIMEMGR_ERR("Failed to unmap zoneInfo file!\n");
            }
        }
       
        if(tzi->fp)
        {
            if(0!=ftruncate(fileno(tzi->fp), tzi->file_size))
            {
                TIMEMGR_ERR("Failed to truncate file!\n");
            }
            fclose(tzi->fp);
            tzi->fp = NULL;
        }
        free(tzi);
    }    
}

static int tzi_get_trans_cnt(
    tzi_t * tzi,
    int version )
{
    REQUIRE(tzi);
    REQUIRE(VALID_VER(tzi, version));

    if(tzi && VALID_VER(tzi, version))
    {
        return (1==version) ? tzi->header.trans_cnt : tzi->header_v2.trans_cnt;
    }
    return 0;
}

static int tzi_get_trans_utc (
    tzi_t *         tzi, 
    int             version,
    int             trans_index, 
    time_t *        utc)
{
    int ret = -1;
    int trans_cnt = 0;

    REQUIRE(tzi);
    REQUIRE(utc);
    REQUIRE(VALID_VER(tzi, version));

    if(tzi && utc && VALID_VER(tzi, version))
    {
        trans_cnt = (1==version) ? tzi->header.trans_cnt : tzi->header_v2.trans_cnt;
        if(trans_index>=0 && trans_index<trans_cnt)
        {
            *utc = (1==version) ? 
                (time_t)tzi->content.utc[trans_index] : 
                (time_t)tzi->content_v2.utc[trans_index];
            ret = 0;
        }
        else
        {
            TIMEMGR_ERR("Invalid trans_index %d! trans_cnt=%d\n", trans_index, trans_cnt);
        }
    }
    return ret;
}

static tzi_type_t * tzi_get_type(
    tzi_t * tzi,
    int version,
    int8_t type_index )
{
    int type_cnt;

    REQUIRE(tzi);
    REQUIRE(VALID_VER(tzi, version));

    if(tzi && VALID_VER(tzi, version))
    {
        type_cnt = (1==version) ? tzi->header.type_cnt : tzi->header_v2.type_cnt;
        if(type_index>=0 && type_index<type_cnt)
        {
            return (1==version) ? &tzi->content.type[type_index] : &tzi->content_v2.type[type_index];
        }
        else
        {
            TIMEMGR_ERR("Invalid type_index %d! type_cnt=%d\n", type_index, type_cnt);
        }
    }
    return NULL;
}

static int tzi_get_trans(
    tzi_t *         tzi, 
    int             version,
    int             trans_index, 
    tzi_trans_t *   trans)
{
    int ret = -1;
    tzi_type_t * type = NULL;
    tzi_header_t * hdr = NULL;
    char * abbr = NULL;
    int8_t * is_std = NULL;
    int8_t * is_utc = NULL;
    
    REQUIRE(tzi);
    REQUIRE(trans);
    REQUIRE(VALID_VER(tzi, version));
    
    if(tzi && trans && VALID_VER(tzi, version))
    {
        hdr = (1==version) ? &tzi->header : &tzi->header_v2;
        if(trans_index >= 0 && trans_index < hdr->trans_cnt)
        {
            trans->version = version;            
            trans->trans_index = trans_index;
            trans->trans_cnt = hdr->trans_cnt;

            trans->utc32 = (1==version) ? tzi->content.utc[trans_index] : 0;
            trans->utc64 = (1==version) ? 0 : tzi->content_v2.utc[trans_index];
            trans->utc = (1==version) ? (time_t)trans->utc32 : (time_t)trans->utc64;
            trans->type_index = (1==version) ? 
                tzi->content.type_index[trans_index] : 
                tzi->content_v2.type_index[trans_index];
            if((trans_index + 1) < hdr->trans_cnt)
            {
                trans->next_utc = (1==version) ? 
                    (time_t)tzi->content.utc[trans_index + 1] : 
                    (time_t)tzi->content_v2.utc[trans_index + 1];
            }
            ret = 0;
        }
        else
        {
            TIMEMGR_ERR("Invalid trans_index %d!\n", trans_index);
        }

        if(0==ret)
        {
            type = tzi_get_type(tzi, version, trans->type_index);
            ENSURE(NULL!=type);
            if(type)
            {
                trans->offset = type->utc_offset;
                trans->is_dst = type->is_dst;
                trans->abbr_index = type->abbr_index;
                is_std = (1==version) ? tzi->content.is_std : tzi->content_v2.is_std;
                is_utc = (1==version) ? tzi->content.is_utc : tzi->content_v2.is_utc;
                trans->is_std = (trans->type_index < hdr->is_std_cnt) ? is_std[trans->type_index] : 1;
                trans->is_utc = (trans->type_index < hdr->is_utc_cnt) ? is_utc[trans->type_index] : 1;
                if(type->abbr_index>=0 && type->abbr_index<hdr->abbr_cnt)
                {
                    abbr = (1==version) ? tzi->content.abbr : tzi->content_v2.abbr;
                    strncpy(trans->abbr, &abbr[type->abbr_index], MAX_ABBR_NAME-1);
                }
                ret = 0;
            }
            else
            {
                ret = -1;
            }
        }
    }
    return ret;
}

static int tzi_find_trans_index_by_utc(
    tzi_t * tzi,
    int version,
    time_t t, 
    int * trans_index)
{
    int ret = -1;
    int i = 0;
    time_t trans_utc = 0;
    int trans_cnt = 0;

    REQUIRE(tzi);
    REQUIRE(VALID_VER(tzi, version));

    if(tzi && VALID_VER(tzi, version))
    {
        trans_cnt = tzi_get_trans_cnt(tzi, version);
        for(i=0; i<trans_cnt; i++)
        {
            ret = tzi_get_trans_utc(tzi, version, i, &trans_utc);
            if(t<trans_utc)
            {
                break;
            }
        }
        if(i>0)
        {
            *trans_index = i-1;
            ret = 0;
        }
        else
        {
            TIMEMGR_ERR("Failed to find trans_index for UTC %s", asctime(gmtime(&t)));
            ret = -1;
        }
    }
    return ret;
}

static int tzi_find_trans_by_utc(
    tzi_t *tzi, 
    int version, 
    time_t t, 
    tzi_trans_t *trans)
{
    int ret = -1;
    int trans_index = 0;
    ret = tzi_find_trans_index_by_utc(tzi, version, t, &trans_index);
    TZI_DEBUG(tzi, ret);
    if(0==ret)
    {
        ret = tzi_get_trans(tzi, version, trans_index, trans);
        TZI_DEBUG(tzi, ret);
    }
    return ret;
}

static int tzi_add_trans_item(
    tzi_t *tzi,
    int version, 
    time_t utc,
    int8_t type_index)
{
    int ret = -1;
    tzi_header_t * hdr = NULL;
    size_t size_to_add = 0;
    time_t last_utc;

    REQUIRE(tzi);
    REQUIRE(VALID_VER(tzi, version));
    
    if(tzi && VALID_VER(tzi, version))
    {
        hdr = (1==version) ? &tzi->header : &tzi->header_v2;

        REQUIRE(type_index>=0 && type_index<hdr->type_cnt);
        if(type_index>=0 && type_index<hdr->type_cnt)
        {
            size_to_add = (1==version) ? (TZI_UTC_SIZE+1) : (TZI_UTC_V2_SIZE+1);
            last_utc = (1==version) ? 
                (time_t)tzi->content.utc[hdr->trans_cnt-1] : 
                (time_t)tzi->content_v2.utc[hdr->trans_cnt-1];

            if(hdr->trans_cnt<MAX_TRANS && (tzi->file_size + size_to_add) <= tzi->mapped_size)
            {
                if(utc > last_utc)
                {
                    if(1==version)
                    {
                        tzi->content.utc[hdr->trans_cnt] = (int32_t)utc;
                        tzi->content.type_index[hdr->trans_cnt] = type_index;
                    }
                    else
                    {
                        tzi->content_v2.utc[hdr->trans_cnt] = (int64_t)utc;
                        tzi->content_v2.type_index[hdr->trans_cnt] = type_index;
                    }
                    hdr->trans_cnt++;
                    tzi->file_size += size_to_add;
                    TIMEMGR_DBG("trans %d was added! utc=%d type_index=%d\n", 
                        hdr->trans_cnt-1,
                        (int)utc,
                        type_index);
                    ret = 0;
                }
            }
        }
    }
    TZI_DEBUG(tzi, ret);
    if(ret)
    {
        TIMEMGR_ERR("Failed to add trans item!\n");
    }
    return ret;
}

static int tzi_add_type(
    tzi_t *tzi,
    int version,
    int offset, int8_t is_dst, int8_t abbr_index,
    int8_t *type_index)
{
    int ret = -1;
    tzi_header_t * hdr = NULL;
    tzi_type_t * type = NULL;
    int8_t * is_std = NULL;
    int8_t * is_utc = NULL;
    size_t size_to_add = (TZI_TYPE_SIZE + 1 + 1);

    REQUIRE(tzi);
    REQUIRE(VALID_VER(tzi, version));
    REQUIRE(type_index);
    
    if(tzi && VALID_VER(tzi, version) && type_index)
    {
        hdr = (1==version) ? &tzi->header : &tzi->header_v2;
        type = (1==version) ? tzi->content.type : tzi->content_v2.type;
        is_std = (1==version) ? tzi->content.is_std: tzi->content_v2.is_std;
        is_utc = (1==version) ? tzi->content.is_utc: tzi->content_v2.is_utc;
        if( hdr->type_cnt<MAX_TYPE &&
            hdr->is_std_cnt<MAX_TYPE &&
            hdr->is_utc_cnt<MAX_TYPE &&
            ((tzi->file_size + size_to_add)<=tzi->mapped_size))
        {
            type[tzi->header.type_cnt].utc_offset = offset;
            type[tzi->header.type_cnt].is_dst = is_dst;
            type[tzi->header.type_cnt].abbr_index = abbr_index;
            hdr->type_cnt++;

            is_std[hdr->is_std_cnt] = 1;/* Assume stanrdard */
            hdr->is_std_cnt++;

            is_utc[hdr->is_utc_cnt] = 1;/* Assume utc */
            hdr->is_utc_cnt++;

            *type_index = tzi->header.type_cnt-1;

            tzi->file_size += size_to_add;

            TIMEMGR_DBG("type %d (offset=%d, is_dst=%d, abbr_index=%d) was created!\n",
                *type_index,
                offset,
                is_dst,
                abbr_index);

            ret = 0;
        }
    }
    TZI_DEBUG(tzi, ret);
    return ret;
}

static int tzi_add_abbr(tzi_t * tzi, int version, const char *abbr_name, int *abbr_index)
{
    int ret = -1;
    char * abbr = NULL;
    tzi_header_t * hdr = NULL;
    size_t size_to_add;

    REQUIRE(tzi);
    REQUIRE(VALID_VER(tzi, version));
    REQUIRE(abbr_name);
    REQUIRE(strlen(abbr_name)<MAX_ABBR_NAME);

    if(tzi && VALID_VER(tzi, version) && strlen(abbr_name)<MAX_ABBR_NAME)
    {
        size_to_add = strlen(abbr_name) + 1;
        hdr = (1==version) ? &tzi->header : &tzi->header_v2;
        abbr = (1==version) ? tzi->content.abbr : tzi->content_v2.abbr;
        if( (tzi->file_size + size_to_add) <= tzi->mapped_size &&
            (hdr->abbr_cnt + size_to_add) <= MAX_ABBR )
        {
            *abbr_index = hdr->abbr_cnt;

            memcpy(abbr+hdr->abbr_cnt, abbr_name, size_to_add);
            hdr->abbr_cnt += size_to_add;
            tzi->file_size += size_to_add;
            
            TIMEMGR_DBG("abbr %s was added! abbr_index=%d\n", abbr_name, *abbr_index);
            ret = 0;
        }
        else
        {
            TIMEMGR_ERR("Not enough space to add abbr!\n");
        }
    }
    else
    {
        TIMEMGR_ERR("Invalid parameter!\n");
    }
    return ret;
}

static int tzi_decrease_trans_item(tzi_t *tzi, int version, int trans_cnt)
{
    int ret = -1;
    tzi_header_t * hdr = NULL;
    size_t trans_item_size = 0;
    
    REQUIRE(tzi);
    REQUIRE(VALID_VER(tzi, version));
    REQUIRE(trans_cnt>=0 && trans_cnt<MAX_TRANS);

    if(tzi && VALID_VER(tzi, version) && trans_cnt>=0 && trans_cnt<MAX_TRANS)
    {
        hdr = (1==version) ? &tzi->header : &tzi->header_v2;
        if(trans_cnt <= hdr->trans_cnt)
        {
            trans_item_size = (1==version) ? (TZI_UTC_SIZE + 1) : (TZI_UTC_V2_SIZE + 1);
            tzi->file_size -= (hdr->trans_cnt - trans_cnt) * trans_item_size;
            TIMEMGR_DBG("Trans cnt decrease from %d to %d!\n", hdr->trans_cnt, trans_cnt);
            hdr->trans_cnt = trans_cnt;
            ret = 0;
        }
        else
        {
            TIMEMGR_ERR("New trans_cnt is bigger than old one!\n");
        }
    }
    else
    {
        TIMEMGR_ERR("Invalid parameter!\n");
    }
    return ret;
}

static int tzi_find_and_create_type(
    tzi_t *tzi, 
    int version,
    int trans_index, 
    int offset, int is_dst, const char * abbr_for_create,
    int8_t *type_index)
{
    int ret = -1;
    tzi_trans_t trans;
    int trans_cnt = tzi_get_trans_cnt(tzi, version);
    int i;
    int found = 0;
    int abbr_index = 0;
  
    for(i=trans_index;i<trans_cnt;i++)
    {
        ret = tzi_get_trans(tzi, version, i, &trans);
        ENSURE(0==ret);

        if(trans.offset==offset && trans.is_dst==is_dst)
        {
            found = 1;
            *type_index = trans.type_index;
            ret = 0;
            break;
        }
    }

    if(0==found)
    {
        ret = tzi_add_abbr(tzi, version, abbr_for_create, &abbr_index);
        ENSURE(0==ret);

        if(0==ret)
        {
            ret = tzi_add_type(tzi, version, offset, is_dst, abbr_index, type_index);
            ENSURE(0==ret);
        }
    }
    return ret;
}

#ifndef NDEBUG
static int tzi_print_trans(tzi_t *tzi, int version)
{
    int ret = 0;
    int i = 0;
    tzi_trans_t trans;
    int trans_cnt;

    trans_cnt = tzi_get_trans_cnt(tzi, version);
    
    for(i=0; i<trans_cnt; i++)
    {
        ret = tzi_get_trans(tzi, version, i, &trans);
        if(ret)
        {
            TIMEMGR_ERR("Failed to trans %d!\n", i);
            ret = -1;
            break;
        }
        printf("offset=%d (%d hour) is_dst=%d abbr=%s is_std=%d is_utc=%d utc=%s", 
            trans.offset, 
            trans.offset / HOUR_SEC, 
            trans.is_dst,
            trans.abbr,
            trans.is_std,
            trans.is_utc,
            asctime(gmtime(&trans.utc)));
    }
    return ret;
}

static int tzi_print(tzi_t *tzi)
{
    if(tzi->version>=1)
    {
        printf("mapped_addr             :   %08X\n", (uint32_t)tzi->mapped_addr);
        printf("mapped_size             :   %u\n", tzi->mapped_size);
        printf("file_size               :   %u\n", tzi->file_size);
        printf("================= Version 1 Header =======================\n");
        tzi_print_header(&tzi->header);
    }
    if(tzi->version>=2)
    {
        printf("================= Version 2 Header =======================\n");
        tzi_print_header(&tzi->header_v2);
        printf("TZ                      :   %s\n", tzi->TZ);
    }
    return 0;
}
#endif

/************************************************************/
/*          Patch the Posix TZ string in zone info file     */
/************************************************************/

#define PATCH_VERSION   1


/* Patcher function for non-dst-used-region */
static int tzi_ndur_patch(
    tzi_t *tzi,
    time_t utc, 
    int offset )
{
    int ret = -1;
    int ver;
    tzi_trans_t trans;
    int8_t type_index;
    int trans_cnt;

    for(ver=1; ver<=PATCH_VERSION; ver++)
    {
        ret = tzi_find_trans_by_utc(tzi, ver, utc, &trans);
        ENSURE(0==ret);

        if(0==ret)
        {
            ret = tzi_find_and_create_type(tzi, ver, trans.trans_index, 
                offset, 0, "STDT", &type_index);
            ENSURE(0==ret);
        }

        if(0==ret)
        {
            if(utc==trans.utc)
            {
                trans_cnt = trans.trans_index;
            }
            else
            {
                trans_cnt = trans.trans_index + 1;
            }   
            ret = tzi_decrease_trans_item(tzi, ver, trans_cnt);
            ENSURE(0==ret);
        }

        if(0==ret)
        {
            ret = tzi_add_trans_item(tzi, ver, utc, type_index);
            ENSURE(0==ret);
        }

        if(ret)
        {
            break;
        }
    }
#ifndef NDEBUG
    if(0==ret)
    {
        for(ver=1;ver<=PATCH_VERSION;ver++)
        {
            ret = tzi_find_trans_by_utc(tzi, ver, utc, &trans);
            ENSURE(0==ret);
            ENSURE(trans.utc==utc);
            ENSURE(trans.offset==offset);
            ENSURE(trans.is_dst==0);
            ENSURE((trans.trans_index+1)==trans.trans_cnt);
        }
        ret = 0;
    }
#endif
    return ret;    
}

typedef struct
{
    time_t  trans_utc;
    int     offset;
    int     is_dst;
}tzi_patch_t;

static int tzi_dur_patch(tzi_t *tzi, tzi_patch_t patch[2])
{
    int ret = -1;
    int ver = 0;
    tzi_trans_t trans;
    int trans_cnt;
    int8_t type_index[2];

    REQUIRE(tzi);
    REQUIRE(patch[0].offset != patch[1].offset);
    REQUIRE(1==(patch[0].is_dst + patch[1].is_dst));

    for(ver=1;ver<=PATCH_VERSION;ver++)
    {
        ret = tzi_find_trans_by_utc(tzi, ver, patch[0].trans_utc, &trans);
        ENSURE(0==ret);

        if(0==ret)
        {
            ret = tzi_find_and_create_type(
                tzi, 
                ver, 
                trans.trans_index,
                patch[0].offset, patch[0].is_dst, patch[0].is_dst ? "DST" : "STDT", 
                &type_index[0]);
            ENSURE(0==ret);
        }

        if(0==ret)
        {
            ret = tzi_find_and_create_type(
                tzi, 
                ver, 
                trans.trans_index,
                patch[1].offset, patch[1].is_dst, patch[1].is_dst ? "DST" : "STDT", 
                &type_index[1]);
            ENSURE(0==ret);
        }

        if(0==ret)
        {
            if(trans.utc == patch[0].trans_utc)
            {
                trans_cnt = trans.trans_index;
            }
            else
            {
                trans_cnt = trans.trans_index + 1;
            }
            ret = tzi_decrease_trans_item(tzi, ver, trans_cnt);
            ENSURE(0==ret);
        }

        if(0==ret)
        {
            ret = tzi_add_trans_item(tzi, ver, patch[0].trans_utc, type_index[0]);
            ENSURE(0==ret);
        }

        if(0==ret)
        {
            ret = tzi_add_trans_item(tzi, ver, patch[1].trans_utc, type_index[1]);
            ENSURE(0==ret);
        }

        if(ret)
        {
            break;
        }
    }

#ifndef NDEBUG
    if(0==ret)
    {
        for(ver=1;ver<=PATCH_VERSION;ver++)
        {
            ret = tzi_find_trans_by_utc(tzi, ver, patch[0].trans_utc, &trans);
            ENSURE(0==ret);
            ENSURE(trans.utc==patch[0].trans_utc);
            ENSURE(trans.offset==patch[0].offset);
            ENSURE(trans.is_dst==patch[0].is_dst);

            ret = tzi_find_trans_by_utc(tzi, ver, patch[1].trans_utc, &trans);
            ENSURE(0==ret);
            ENSURE(trans.utc==patch[1].trans_utc);
            ENSURE(trans.offset==patch[1].offset);
            ENSURE(trans.is_dst==patch[1].is_dst);
        }
        ret = 0;
    }
#endif
    return ret;
}

/************************************************************/
/*          Patch the Posix TZ string in zone info file     */
/************************************************************/

static void tzi_get_offset_string(int offset, char str[16])
{
    int hour, minute, second;
    int less_than_zero = 0;
    
    /* The offset in TZ string is the time value you must add 
       to the local time to get a Coordinated Universal Time value.
       So we must reverse the offset! */
    offset = -offset;

    if(offset<0)
    {
        less_than_zero = 1;
        offset = -offset;
    }

    hour = offset / 3600;
    minute = (offset % 3600) / 60;
    second = (offset % 3600) % 60;

    REQUIRE(hour>=0 && hour<12);
    
    if(offset % 3600)
    {
        if(second>0)
        {
            sprintf(str, "%s%d:%d:%d", less_than_zero?"-":"", hour, minute, second);
        }
        else
        {
            sprintf(str, "%s%d:%d", less_than_zero?"-":"", hour, minute);
        }
    }
    else
    {
        sprintf(str, "%s%d", less_than_zero?"-":"", hour);
    }
}

static int is_leap_year( int year )
{
    if(year%4)
    {
        return 0;
    }
    else
    {
        if((0==(year%100)) && (year%400))
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }
}

static int days_in_month(int year, int month)
{
    switch(month)
    {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
        return 31;
    case 4:
    case 6:
    case 9:
    case 11:
        return 30;
    case 2:
        if( is_leap_year(year))
        {
            return 29;
        }
        else
        {
            return 28;
        }
    default:
        return 0;
    }
    return 0;
}

static void get_M_date( 
    time_t utc, 
    int local_offset, 
    int *month, int *week, int *wday, int *hour, int *minu, int *seco)
{
    struct tm tm_switch;
    time_t utc_local = utc+local_offset;
    int w;
    
    gmtime_r(&utc_local, &tm_switch);
    w = ((tm_switch.tm_mday-1) / 7) + 1;
    if(w==4)
    {
        if(tm_switch.tm_mday + 7 > days_in_month(1900+tm_switch.tm_year, 1+tm_switch.tm_mon))
        {
            /* Last weekday in that month although w==4 */
            w = 5;
        }
    }
    *month = tm_switch.tm_mon+1;
    *week = w;
    *wday = tm_switch.tm_wday;
    *hour = tm_switch.tm_hour;
    *minu = tm_switch.tm_min;
    *seco = tm_switch.tm_sec;
}

static void get_M_string( time_t utc, int local_offset, char M_str[MAX_M_STR])
{
    int month, week, wday, hour, minu, seco;
    get_M_date(utc, local_offset, &month, &week, &wday, &hour, &minu, &seco);
    if(0==minu && 0==seco)
    {
        sprintf(M_str, "M%d.%d.%d/%d", month, week, wday, hour);
    }
    else if(0==seco)
    {
        sprintf(M_str, "M%d.%d.%d/%d:%d", month, week, wday, hour, minu);
    }
    else
    {
        sprintf(M_str, "M%d.%d.%d/%d:%d:%d", month, week, wday, hour, minu, seco);
    }
}

static int tzi_generate_TZ_at_time(
    tzi_t *tzi,
    time_t utc,
    char TZ[MAX_TZ] )
{
    int ret = -1;
    tzi_trans_t trans1;
    tzi_trans_t trans2;
    int dst_used = 0;
    int std_offset = 0;
    int dst_offset = 0;
    char *std_abbr = NULL;
    char *dst_abbr = NULL;
    time_t dst_on_utc = (time_t)0;
    time_t dst_off_utc = (time_t)0;
    char std_offset_str[MAX_OFFSET_STR];
    char dst_offset_str[MAX_OFFSET_STR];
    char dst_on_M_str[MAX_M_STR];
    char dst_off_M_str[MAX_M_STR];

    ret = tzi_find_trans_by_utc(tzi, 1, utc, &trans1);
    ENSURE(0==ret);
    if(ret)
    {
        return ret;
    }

    if((trans1.trans_index + 1) < trans1.trans_cnt)
    {
        ret = tzi_get_trans(tzi, 1, trans1.trans_index+1, &trans2);
        ENSURE(0==ret);
        ENSURE(trans1.offset!=trans2.offset);
        ENSURE(1==(trans1.is_dst + trans2.is_dst));
        if( (0==ret) && 
            (trans1.offset!=trans2.offset) && 
            (1==(trans1.is_dst + trans2.is_dst)))
        {
            dst_used = 1;
        }
    }
    
    if(0==dst_used)
    {
        std_offset = trans1.offset;
        std_abbr = trans1.abbr; 
    }
    else
    {
        if(trans1.is_dst)
        {
            dst_off_utc = trans2.utc;
            std_offset  = trans2.offset;
            std_abbr    = trans2.abbr;
            dst_on_utc  = trans1.utc;
            dst_offset  = trans1.offset;
            dst_abbr    = trans1.abbr;
        }
        else
        {
            dst_off_utc = trans1.utc;
            std_offset  = trans1.offset;
            std_abbr    = trans1.abbr;
            dst_on_utc  = trans2.utc;   
            dst_offset  = trans2.offset;
            dst_abbr    = trans2.abbr;
        }
    }
    
    if(0==ret)
    {
        tzi_get_offset_string(std_offset, std_offset_str);
        if(dst_used)
        {
            get_M_string(dst_on_utc, std_offset, dst_on_M_str);
            get_M_string(dst_off_utc, dst_offset, dst_off_M_str);
            if(dst_offset == (std_offset + 3600))
            {
                sprintf(TZ, "%s%s%s,%s,%s", 
                    std_abbr, 
                    std_offset_str,
                    dst_abbr,
                    dst_on_M_str,
                    dst_off_M_str);
                ret = 0;
            }
            else
            {
                tzi_get_offset_string(dst_offset, dst_offset_str);
                sprintf(TZ, "%s%s%s%s,%s,%s", 
                    std_abbr,
                    std_offset_str,
                    dst_abbr,
                    dst_offset_str,
                    dst_on_M_str,
                    dst_off_M_str );
                ret = 0;
            }
        }
        else
        {
            sprintf(TZ, "%s%s", std_abbr, std_offset_str);
            ret = 0;
        }
    }
    return ret;
}

static int tzi_update_TZ_at_time(
    tzi_t *tzi,
    o_local_time_info_t *   ltinfo )
{
    int ret = -1;
    char TZ[MAX_TZ];
    size_t old_size;

    REQUIRE(tzi);

    if(tzi)
    {
        ret = tzi_generate_TZ_at_time(tzi, ltinfo->cur_utc, TZ);
        ENSURE(0==ret);
        if(0==ret)
        {
            old_size = strlen(tzi->TZ);
            strcpy(tzi->TZ, TZ);
            tzi->file_size = tzi->file_size - old_size + strlen(tzi->TZ);
            TIMEMGR_DBG("Patched TZ: %s\n", tzi->TZ);    
        }
    }
    return ret;    
}
    
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
    o_local_time_info_t *   ltinfo )
{
    int ret = -1;
    tzi_t * tzi = NULL;
    tzi_patch_t patch[2];
    
    REQUIRE(original_path);
    REQUIRE(patched_path);
    REQUIRE(ltinfo);

    tzi_debug_init(original_path, patched_path, ltinfo);
    
    ret = 0;
    if(NULL==original_path||NULL==patched_path||NULL==ltinfo)
    {
        ret = -1;
    }

    if(0==ret)
    {
        ret = timemgr_copy_file(original_path, patched_path);
        ENSURE(0==ret);
    }

    if(0==ret)
    {
        tzi = tzi_open(patched_path);
        ENSURE(tzi);
        ret = tzi ? 0 : (-1);
    }
    
    if(0==ret)
    {
        if(-1 == ltinfo->is_dst)
        {
            /* There is no DST used */
            ret = tzi_ndur_patch(
                tzi, 
                ltinfo->cur_utc, ltinfo->offset);
            ENSURE(0==ret);
        }
        else if(0 == ltinfo->is_dst || 1 == ltinfo->is_dst)
        {
            patch[0].offset = ltinfo->offset;
            patch[0].is_dst = ltinfo->is_dst;
            patch[1].is_dst = 1 - ltinfo->is_dst;
            if(ltinfo->prev_trans_utc>0 && ltinfo->next_trans_utc==0)
            {
                patch[1].offset = ltinfo->prev_offset;
                patch[0].trans_utc = ltinfo->prev_trans_utc;
                patch[1].trans_utc = ((patch[0].trans_utc + (180 * DAY_SEC)) / HOUR_SEC) * HOUR_SEC;
                if(ltinfo->cur_utc >= patch[1].trans_utc)
                {
                    patch[1].trans_utc = ((ltinfo->cur_utc / HOUR_SEC) + 1) * HOUR_SEC;
                }
            }
            else if(ltinfo->next_trans_utc>0 && ltinfo->prev_trans_utc==0)
            {
                patch[1].offset = ltinfo->next_offset;
                patch[1].trans_utc = ltinfo->next_trans_utc;
                patch[0].trans_utc = ((patch[1].trans_utc - (180 * DAY_SEC)) / HOUR_SEC) * HOUR_SEC;
                if(ltinfo->cur_utc < patch[0].trans_utc)
                {
                    patch[0].trans_utc = ((ltinfo->cur_utc / HOUR_SEC) - 1) * HOUR_SEC;
                }
            }
            else
            {
                ret = -1;
                TIMEMGR_ERR("Invalid parameter!\n");
            }

            if(0==ret)
            {
                ret = tzi_dur_patch(tzi, patch);
                ENSURE(0==ret);
            }
        }
        else
        {
            ret = -1;
            TIMEMGR_ERR("Invalid parameter!\n");
        }
    }

    if(0==ret)
    {
        ret = tzi_update_TZ_at_time(tzi, ltinfo);
        ENSURE(0==ret);
    }

    if(0==ret)
    {
        ret = tzi_save(tzi);
        ENSURE(0==ret);
    }

    if(tzi)
    {
        tzi_close(tzi);
    }
    return ret;
}

#ifndef NDEBUG
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
int timemgr_selftest_print_zoneInfo( const char * zoneInfo_path)
{
    int ret = -1;
    tzi_t * tzi = tzi_open(zoneInfo_path);
    if(tzi)
    {
        ret = tzi_print(tzi);
        tzi_close(tzi);
    }
    return ret;
}

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
int timemgr_selftest_print_zoneInfo_trans( const char * zoneInfo_path, int version)
{
    int ret = -1;
    tzi_t * tzi = tzi_open(zoneInfo_path);
    if(tzi)
    {
        ret = tzi_print_trans(tzi, version);
        tzi_close(tzi);
    }
    return ret;
}

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
int timemgr_selftest_generate_TZ( const char * zoneInfo_path, const char *strTime)
{
    int ret = -1;
    tzi_t * tzi = tzi_open(zoneInfo_path);
    time_t t;
    char TZ[MAX_TZ];
    
    if(tzi)
    {
        if(tzi->version==2)
        {
            printf("ZoneInfo contained TZ string\n");
            printf("    %s\n", tzi->TZ);
        }
        if(strTime)
        {
            t = timemgr_parse_utc(strTime);
        }
        else
        {
            time(&t);
        }
        ret = tzi_generate_TZ_at_time(tzi, t, TZ);
        if(0==ret)
        {
            printf("Timemgr generated TZ string at UTC %s", asctime(gmtime(&t)) );
            printf("    %s\n", TZ);
        }
        tzi_close(tzi);
    }
    return ret;
}

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

    ret= timemgr_patch_zoneInfo_file(original_path, patched_path, &ltinfo);
    if(ret)
    {
        TIMEMGR_ERR("Failed to patch file %s!\n", patched_path);
    }
            
    return ret;
}

#endif

