/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __NTVLOG_H_
#define __NTVLOG_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if (defined(O_LOG_COMPNAME))
#define NHL_MODU_NAME  O_LOG_COMPNAME
#elif (defined(PACKAGE_NAME))
#define NHL_MODU_NAME  PACKAGE_NAME
#else
#define NHL_MODU_NAME  "NONAME"
#endif

#include <nhlog.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <glib/gstdio.h>

#define STMT(stuff) do { stuff } while(0)

void o_log_assert_trap(void);

#if !defined(NDEBUG)
#define O_ASSERT                        nhassert
#define O_WARN                          nhassert
#define ASSERT                          nhassert
#define O_DEBUG_ASSERT                  nhassert
#define WARN                            nhassert
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
#define O_LOG_FATAL                     nhfatal
#endif

#ifdef MY_ERROR
#define O_LOG_ERROR(...)                ((void)0)
#define O_LOG_ERROR_SIMPLE(...)         ((void)0)
#else
#define O_LOG_ERROR                     nherror
#define O_LOG_ERROR_SIMPLE              nherror
#endif

#ifdef MY_WARNING
#define O_LOG_WARNING(...)              ((void)0)
#define O_LOG_WARNING_SIMPLE(...)       ((void)0)
#else
#define O_LOG_WARNING                   nhwarning
#define O_LOG_WARNING_SIMPLE            nhwarning
#endif

#ifdef MY_MAINSTATUS
#define O_LOG_MAINSTATUS(...)           ((void)0)
#define O_LOG_MAINSTATUS_SIMPLE(...)    ((void)0)
#else
#define O_LOG_MAINSTATUS                nhnotice
#define O_LOG_MAINSTATUS_SIMPLE         nhnotice
#endif

#ifdef MY_DEBUG
#define O_LOG_DEBUG(...)                ((void)0)
#define O_LOG_DEBUG_SIMPLE(...)         ((void)0)
#else
#define O_LOG_DEBUG                     nhlog
#define O_LOG_DEBUG_SIMPLE              nhlog
#endif

#ifdef MY_TRACE
#define O_LOG_TRACE(...)                ((void)0)
#define O_LOG_TRACE_SIMPLE(...)         ((void)0)
#else
#define O_LOG_TRACE                     nhtrace
#define O_LOG_TRACE_SIMPLE              nhtrace
#endif

#ifdef MY_E_E
#define O_LOG_ENTER(...)                ((void)0)
#define O_LOG_EXIT(...)                 ((void)0)
#else
#define O_LOG_ENTER(fmt, ...)           nhtrace("Enter:" fmt, ##__VA_ARGS__)
#define O_LOG_EXIT(fmt, ...)            nhtrace("Exit:" fmt, ##__VA_ARGS__)
#endif

#define REQUIRE(...)                    ((void)0)
#define ENSURE(...)                     ((void)0)
#define LOOP_INVARIANT_INIT(...)        ((void)0)
#define LOOP_INVARIANT_TEST(...)        ((void)0)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __NTVLOG_H_ */

