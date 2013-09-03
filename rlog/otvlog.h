/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __NTVLOG_H_
#define __NTVLOG_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * O_LOG_CRITICAL
 * O_LOG_CRITICAL_ERROR
 * O_LOG_CRITICAL_STATUS
 * O_LOG_DEBUG
 * O_LOG_DEBUG_EITS_SRC_PERF
 * O_LOG_DEBUG_SIMPLE
 * O_LOG_ENTER
 * O_LOG_ERROR
 * O_LOG_ERROR_SIMPLE
 * O_LOG_EXIT
 * O_LOG_FATAL
 * O_LOG_MAINSTATUS
 * O_LOG_MAINSTATUS_SIMPLE
 * O_LOG_METADATA
 * O_LOG_TRACE
 * O_LOG_TRACE_SIMPLE
 * O_LOG_WARNING
 * O_LOG_WARNING_SIMPLE
 *
 * O_WARN === kassert
 * O_ASSERT === kassert
 */

/**** Includes ***************************************************************/
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void o_log_assert_trap( void );

#define O_ASSERT kassert
#define O_WARN kassert
#define ASSERT kassert
#define O_DEBUG_ASSERT kassert
#define WARN(cond)  kassert

#if (defined(O_LOG_COMPNAME))
    #define COMP_NAME  O_LOG_COMPNAME
#elif (defined(PACKAGE_NAME))
    #define COMP_NAME  PACKAGE_NAME
#else
    #define COMP_NAME  "?"
#endif

#define O_FATAL
#define O_ERROR
#define O_WARNING
#define O_MAINSTATUS
#define O_DEBUG
#define O_TRACE

#define REQUIRE kassert
#define ENSURE kassert

/**
 * Macro, creates a local variable to store an initial  loop invariant test value.
 */
#define LOOP_INVARIANT_INIT(_inv_,x) size_t dbc_##_inv_ = (size_t)(x);

/**
 * Macro, Tests the current value against the expected invariant value.
 * This does not work on pointer dereferences. It is used at the end of a loop
 * or function.
 * @par Note
 * In the implementation of any function there \b MUST be (at least) a corresponding LOOP_INVARIANT_TEST
 * statement for every loop invariant listed in the header documentation.
 */
#define LOOP_INVARIANT_TEST(_inv_,x) STMT(if (dbc_##_inv_ != (size_t)(x)) {             \
    o_log_assert_trap();                                                                \
    syslog( O_FATAL, "Loop invariant changed - file:%s,func:%s,ln:%d var:%s %d->%d\n",  \
        __FILE__,__func__,__LINE__,#x,dbc_##_inv_,((size_t)(x)) );                      \
    g_print( "Loop invariant changed - file:%s,func:%s,ln:%d var:%s %d->%d\n",          \
        __FILE__,__func__,__LINE__,#x,dbc_##_inv_,((size_t)(x)) );                      \
    fflush( stdout );assert(0); })

#ifdef __cplusplus
}
#endif

#endif /* __NTVLOG_H_ */
