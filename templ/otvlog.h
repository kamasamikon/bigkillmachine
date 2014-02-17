/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __NTVLOG_H__
#define __NTVLOG_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if (defined(O_LOG_COMPNAME))
#define KMODU_NAME  O_LOG_COMPNAME
#elif (defined(PACKAGE_NAME))
#define KMODU_NAME  PACKAGE_NAME
#else
#define KMODU_NAME  "NONAME"
#endif

#include <hilda/klog.h>

/**** Includes ***************************************************************/
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <glib/gstdio.h>

#define STMT(stuff) do { stuff } while(0)

void o_log_assert_trap( void );

#if !defined(NDEBUG)
#define O_ASSERT                        kassert
#define O_WARN                          kassert
#define ASSERT                          kassert
#define O_DEBUG_ASSERT                  kassert
#define WARN                            kassert
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

#define O_LOG_FATAL                     kfatal

#define O_LOG_ERROR                     kerror
#define O_LOG_ERROR_SIMPLE              kerror

#define O_LOG_WARNING                   kwarning
#define O_LOG_WARNING_SIMPLE            kwarning

#define O_LOG_MAINSTATUS                knotice
#define O_LOG_MAINSTATUS_SIMPLE         knotice

#define O_LOG_DEBUG                     klog
#define O_LOG_DEBUG_SIMPLE              klog

#define O_LOG_TRACE                     ktrace
#define O_LOG_TRACE_SIMPLE              ktrace

#if (O_LOG_LEVEL >= O_TRACE)
#define O_LOG_ENTER(fmt, ...)           ktrace("Enter:" fmt, ##__VA_ARGS__)
#define O_LOG_EXIT(fmt, ...)            ktrace("Exit:" fmt, ##__VA_ARGS__)
#else
#define O_LOG_ENTER(msg,args...)        ((void)0)
#define O_LOG_EXIT(msg,args...)         ((void)0)
#endif


#define REQUIRE(pre)                    ((void)0)
#define ENSURE(post)                    ((void)0)
#define LOOP_INVARIANT_INIT(inv,x)      ((void)0)
#define LOOP_INVARIANT_TEST(inv,x)      ((void)0)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __NTVLOG_H__ */

