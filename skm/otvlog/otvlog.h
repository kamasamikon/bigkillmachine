/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __NTVLOG_H_
#define __NTVLOG_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if (defined(O_LOG_COMPNAME))
#define NSULOG_MODU_NAME O_LOG_COMPNAME
#elif (defined(PACKAGE_NAME))
#define NSULOG_MODU_NAME PACKAGE_NAME
#else
#define NSULOG_MODU_NAME "NONAME"
#endif

/* +++ EMBED NSULOG +++ */
#include <nsulog.h>
/* --- EMBED NSULOG --- */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <glib/gstdio.h>

/* +++ JUNK PART +++ */
/* +++ JUNK PART +++ */
/* +++ JUNK PART +++ */
#define STMT(stuff) do { stuff } while(0)

#define SHOW_LOG_FILE "/tmp/.show_log"
void o_log_assert_trap(void);
/* --- JUNK PART --- */
/* --- JUNK PART --- */
/* --- JUNK PART --- */


#if !defined(NDEBUG)
#define O_ASSERT                        nsulog_assert
#define O_WARN                          nsulog_assert
#define ASSERT                          nsulog_assert
#define O_DEBUG_ASSERT                  nsulog_assert
#define WARN                            nsulog_assert
#else
#define O_WARN(cond)                    ((void)0)
#define O_ASSERT(cond)                  ((void)0)
#define WARN(cond)                      ((void)0)
#define ASSERT(cond)                    ((void)0)
#define O_DEBUG_ASSERT(cond)            ((void)0)
#endif

#define O_FATAL                         0
#define O_ERROR                         3
#define O_WARNING                       4
#define O_MAINSTATUS                    6
#define O_DEBUG                         7
#define O_TRACE                         8

/* Reset the O_LOG_LEVEL */
#ifdef O_LOG_LEVEL
#undef O_LOG_LEVEL
#define O_LOG_LEVEL                     O_TRACE
#endif

/* Set the LOG_LEVEL */
#ifndef LOG_LEVEL
#define LOG_LEVEL                       O_FATAL
#endif

#ifdef MY_FATAL
#define O_LOG_FATAL(...)                ((void)0)
#else
#define O_LOG_FATAL                     nsulog_fatal
#endif

#ifdef MY_ERROR
#define O_LOG_ERROR(...)                ((void)0)
#define O_LOG_ERROR_SIMPLE(...)         ((void)0)
#else
#define O_LOG_ERROR                     nsulog_error
#define O_LOG_ERROR_SIMPLE              nsulog_error
#endif

#ifdef MY_WARNING
#define O_LOG_WARNING(...)              ((void)0)
#define O_LOG_WARNING_SIMPLE(...)       ((void)0)
#else
#define O_LOG_WARNING                   nsulog_warning
#define O_LOG_WARNING_SIMPLE            nsulog_warning
#endif

#ifdef MY_MAINSTATUS
#define O_LOG_MAINSTATUS(...)           ((void)0)
#define O_LOG_MAINSTATUS_SIMPLE(...)    ((void)0)
#else
#define O_LOG_MAINSTATUS                nsulog_notice
#define O_LOG_MAINSTATUS_SIMPLE         nsulog_notice
#endif

#ifdef MY_DEBUG
#define O_LOG_DEBUG(...)                ((void)0)
#define O_LOG_DEBUG_SIMPLE(...)         ((void)0)
#else
#define O_LOG_DEBUG                     nsulog_info
#define O_LOG_DEBUG_SIMPLE              nsulog_info
#endif

#ifdef MY_TRACE
#define O_LOG_TRACE(...)                ((void)0)
#define O_LOG_TRACE_SIMPLE(...)         ((void)0)
#else
#define O_LOG_TRACE                     nsulog_debug
#define O_LOG_TRACE_SIMPLE              nsulog_debug
#endif

#ifdef MY_E_E
#define O_LOG_ENTER(...)                ((void)0)
#define O_LOG_EXIT(...)                 ((void)0)
#else
#define O_LOG_ENTER(fmt, ...)           nsulog_debug("Enter:" fmt, ##__VA_ARGS__)
#define O_LOG_EXIT(fmt, ...)            nsulog_debug("Exit:" fmt, ##__VA_ARGS__)
#endif

#define REQUIRE(...)                    ((void)0)
#define ENSURE(...)                     ((void)0)
#define LOOP_INVARIANT_INIT(...)        ((void)0)
#define LOOP_INVARIANT_TEST(...)        ((void)0)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __NTVLOG_H_ */
