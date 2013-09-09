/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __NTVLOG_H_
#define __NTVLOG_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if (defined(O_LOG_COMPNAME))
#define KMODU_NAME  O_LOG_COMPNAME
#elif (defined(PACKAGE_NAME))
#define KMODU_NAME  PACKAGE_NAME
#else
#define KMODU_NAME  "?"
#endif

#include <klog.h>

/**** Includes ***************************************************************/
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib/gstdio.h>

void o_log_assert_trap( void );

#define O_ASSERT                kassert
#define O_WARN                  kerror
#define ASSERT                  kassert
#define O_DEBUG_ASSERT          kassert
#define WARN                    kerror

#define O_FATAL                 kfatal
#define O_ERROR                 kerror
#define O_WARNING               klog
#define O_MAINSTATUS            klog
#define O_DEBUG                 klog
#define O_TRACE                 ktrace

#define O_LOG_FATAL             kfatal

#define O_LOG_ERROR             kerror
#define O_LOG_ERROR_SIMPLE      kerror

#define O_LOG_WARNING           klog
#define O_LOG_WARNING_SIMPLE    klog

#define O_LOG_MAINSTATUS        ktrace
#define O_LOG_MAINSTATUS_SIMPLE ktrace

#define O_LOG_DEBUG             klog
#define O_LOG_DEBUG_SIMPLE      klog

#define O_LOG_TRACE             ktrace
#define O_LOG_TRACE_SIMPLE      ktrace

#define O_LOG_ENTER             ktrace
#define O_LOG_EXIT              ktrace

#define REQUIRE(pre)                ((void)0)
#define ENSURE(post)                ((void)0)
#define LOOP_INVARIANT_INIT(inv,x)  ((void)0)
#define LOOP_INVARIANT_TEST(inv,x)  ((void)0)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __NTVLOG_H_ */
