 * ============================================================================
 */

/**
 * @file     timeproxy.h
 * @author   Lianghe Li
 * @date     Feb 15, 2011
 */
#ifndef __TIMEPROXY_H_
#define __TIMEPROXY_H_

/**
 * @brief Time manager proxy library header file
 * @defgroup TIMEPROXY Time manager proxy library header file
 * @ingroup TIMEMGR
 * This header file contains prixy interface definition for out of process time manager clients.
 *
 * @note
 * These is no need to use DBus proxy constructs with any of the calls, except for the once off
 * usage at overall library initialisation time.
 * @{
 */

/**** Includes ***************************************************************/
#include <glib.h>
#include <stdint.h>
#include <stdbool.h>
#include "timemgr.h"
// #include "otypes.h"

/**
 * @brief   Notification function for time events.
 *
 * @param[in] enEvent : The time event
 * @param[in] pCookie : Unique cookie
 *
 * @pre  None
 * @post None
 *
 * @par Description
 * Callback for time event.
 */
typedef void(*timeproxy_event_handler_t)(
    timemgr_event_t enEvent,
    void*           pCookie );

/**
 * @brief   Initialisation for the time manager proxy
 *
 * @param[in] pGMainCtxt : Pointer to GMainLoop context
 * @retval  0 indicates success
 * @retval  -1 indicates error
 *
 * @pre  None
 * @pre  Protected against multiple initialisation
 * @post Initialisation is launched
 *
 * @par Description
 * Initialises the time manager proxy, creates any internal resources, and establishes the
 * connection to the time manager server. If the context pointer is non-NULL then it will be used.
 * If it is NULL, then the default context will be used. If the pointer is NULL and the default
 * context cannot be used then the proxy will return failure. It will \b NOT create it's own
 * GMainLoop context.
 */
int timeproxy_init( GMainContext* pGMainCtxt );

/**
 * @brief   Shutdown for the time manager manager proxy
 *
 * @retval  0 indicates success
 * @retval  -1 indicates error
 *
 * @pre     None
 * @post    All resources are released
 *
 * @par Description
 * Cleanup for forced process shutdown, releases any resources. TIMEPROXY is
 * protected against multiple exit.
 */
int timeproxy_exit( void );

/**
 * @brief       Query if current system time is valid
 *
 * @retval true  System time is valid
 * @retval false System time is not yet valid
 *
 * @pre  Proxy is intitialised
 * @post None
 *
 * @par Description
 * Query time manager if current system time is valid.
 */
_Bool timeproxy_is_time_valid( void );

/**
 * @brief  Register for time events.
 *
 * @param[in] fctCallback: Event callback function
 * @param[in] pCookie    : Unique cookie
 *
 * @retval      0       SUCCESS
 * @retval      non-0   FAILURE
 *
 * @pre  None
 * @post None
 *
 * @par Description
 * This call allows a user to register for time events. Once registered, the user will receive
 * all time events. There is no need to register for individual events as they are infrequent.
 * The cookie allows multiple users in the same process to register unique callbacks.
 */
int timeproxy_register_for_events(
    timeproxy_event_handler_t fctCallback,
    void*                     pCookie );

#endif /* __TIMEPROXY_H_ */

