 * ============================================================================
 */

/**
 * @file     ntvlog.h
 * @author   Don Ma
 * @date     March 11, 2011
 */
#ifndef __NTVLOG_H_
#define __NTVLOG_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Logging and debug utilities
 * @defgroup ODEBUG Logging and debug utilities
 * @ingroup  SYSUTILS
 * Interface for:
 * - simple debug calls
 * - design by contract statements
 * - system logging
 *
 * @section olog_sect_1 Routing debug to STDOUT
 * By default the output is routed to STDOUT. i.e. the terminal in which
 * you are executing. There are cases where this is undesirable, for instance when running
 * uPITS. The debug information interferes with the readability of the uPITS output. To disable
 * output to STDOUT add the following to the MAKE:
 * - -DNTVLOG_NO_STDOUT
 *
 * @section olog_sect_2 Routing debug to SYSLOG
 * By default the output is routed to SYSLOG. There are cases where this is may be undesirable, To disable
 * output to SYSLOG add the following to the MAKE:
 * - -DNTVLOG_NO_SYSLOG
 *
 * @section olog_sect_3 Asserts and design-by-contract.
 * These will \b always route to both SYSLOG and STDOUT, regardless of whether one of those outputs
 * is disabled for general output.
 *
 * @{
 */

/**** Includes ***************************************************************/
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

/**** Definitions ************************************************************/

/* Use STMT to implement macros with compound statements */
#define STMT(stuff) do { stuff } while(0)

/* cannot define both "no stdout" and "no syslog" */
#if !defined(NDEBUG)
    #if (defined(NTVLOG_NO_STDOUT) && defined(NTVLOG_NO_SYSLOG))
        #error "Only one of NTVLOG_NO_STDOUT and NTVLOG_NO_SYSLOG may be enabled!"
    #endif
#endif /* !defined(NDEBUG) */

/**
 * If this is defined then the trace statements will take the form of "function_name() + line number" rather than
 * "file name + line number". It is a convenience setting. It can result in significantly shorter
 * trace statements, depending on how the file name has been generated.
 */
#undef NTVLOG_FUNCTION_NOT_FILE

/*===========================================================================*/
/* GENERAL DEBUG FUNCTIONALITY                                               */
/*===========================================================================*/

/**
 * @brief   Simple debug trap
 *
 * @retval  None
 *
 * @pre     none
 * @post    none
 *
 * @par Description
 * This is simply in place to allow breakpoints to be set to trap on assertion, logger or
 * design by contract statements.
 */
void o_log_assert_trap( void );


#if !defined(NDEBUG)

/**
 * Simple macro replacement for assert. The functionality is the same as assert, but we are able
 * to insert a breakpoint in the debug trap.
 */
#define O_ASSERT(cond) STMT(if (!(cond)) {                                 \
    o_log_assert_trap();                                                   \
    syslog( LOG_EMERG, "ASSERT fail - file:%s,func:%s,ln:%d cond:%s\n",    \
    __FILE__,__func__,__LINE__,#cond );                                    \
    printf( "ASSERT fail - file:%s,func:%s,ln:%d cond:%s\n",              \
    __FILE__,__func__,__LINE__,#cond );                                    \
    fflush( stdout );assert(0); })

/**
 * Simple warning macro. Will not cause an assert, but will warn about
 * wrong non-fatal conditions
 */
#define O_WARN(cond) STMT(if (!(cond)) {                                              \
    syslog( LOG_EMERG, "WARNING condition not met - file:%s,func:%s,ln:%d cond:%s\n", \
    __FILE__,__func__,__LINE__,#cond );                                               \
    printf( "WARNING condition not met - file:%s,func:%s,ln:%d cond:%s\n",           \
    __FILE__,__func__,__LINE__,#cond ); })

/**
 * @deprecated
 * Will be phased out, preferred usage is \ref O_ASSERT
 */
#define ASSERT(cond)         O_ASSERT(cond)

/**
 * @deprecated
 * Will be phased out, preferred usage is \ref O_ASSERT
 */
#define O_DEBUG_ASSERT(cond) O_ASSERT(cond)

/**
 * @deprecated
 * Will be phased out, preferred usage is \ref O_WARN
 */
#define WARN(cond) O_WARN(cond)

#else
    /* protect against poor coding habits, like missing braces */
    #define O_WARN(cond)            ((void)0)
    #define O_ASSERT(cond)          ((void)0)
    #define WARN(cond)              ((void)0)
    #define ASSERT(cond)            ((void)0)
    #define O_DEBUG_ASSERT(cond)    ((void)0)
#endif /* !defined(NDEBUG) */


/*===========================================================================*/
/* LOGGER MACROS                                                             */
/*===========================================================================*/

/**
 * @brief Logger macros
 * @defgroup OLOG Logger macros
 * @ingroup ODEBUG
 * Series of logging macros that route all output to the syslog.
 *
 * @{
 */

/* if neither compname or pkgmane are defined then "?" is used */
#if (defined(O_LOG_COMPNAME))
    #define COMP_NAME  O_LOG_COMPNAME
#elif (defined(PACKAGE_NAME))
    #define COMP_NAME  PACKAGE_NAME
#else
    #define COMP_NAME  "?"
#endif

/* Logging level, 0-7 level are mapping to syslog's priority level, levels above 7 are nemotv specific level. */

/**
 * System is unusable (value = 0)
 */
#define O_FATAL      LOG_EMERG

/**
 * Error conditions (value = 3)
 */
#define O_ERROR      LOG_ERR

/**
 * Warning conditions (value = 4)
 */
#define O_WARNING    LOG_WARNING

/**
 * Informational (value = 6)
 */
#define O_MAINSTATUS LOG_INFO

/**
 * Debug level messages (value = 7)
 */
#define O_DEBUG      LOG_DEBUG

/**
 * Debug trace messages (value = 8)
 */
#define O_TRACE      8

/* If log level is defined by user when make, using the customised level.    */
/* TODO: Why are we doing this crap in the file. Just fix the damn makefiles */
#if defined(LOG_LEVEL)
    #if defined(O_LOG_LEVEL)
        #undef O_LOG_LEVEL
    #endif /* defined(O_LOG_LEVEL) */
    #define O_LOG_LEVEL LOG_LEVEL
#else /* !defined(LOG_LEVEL) */
    /* in case the makefile does not have the definitions */
    #if !defined(O_LOG_LEVEL)
        #if !defined(NDEBUG)
            #define O_LOG_LEVEL O_TRACE
        #else
            #define O_LOG_LEVEL O_FATAL
        #endif /* !defined(NDEBUG) */
    #endif /* !defined(O_LOG_LEVEL) */
#endif/* LOG_LEVEL */


#if !defined(NTVLOG_NO_SYSLOG)
    /* Internal macro defined for using syslog.
     * NOT FOR PUBLIC USE
     */
    #define O_LOG_SYSLOG(lvl,msg,args...) \
        syslog(lvl, "["COMP_NAME"] " msg, ## args)

    /* Internal macro defined for using syslog with level info.
     * NOT FOR PUBLIC USE
     */
    #if defined(NTVLOG_FUNCTION_NOT_FILE)
        #define O_LOG_SYSLOG_DBG(lvlname,lvl,msg,args...) \
                syslog(lvl, lvlname ": ["COMP_NAME"] %s():line %i - " msg, __func__, __LINE__, ## args);
    #else
        #define O_LOG_SYSLOG_DBG(lvlname,lvl,msg,args...) \
                syslog(lvl, lvlname ": ["COMP_NAME"] " __FILE__  ",line %i - " msg, __LINE__, ## args);
    #endif

    #define O_LOG_SYSLOG_DBG_SIMPLE(lvlname,lvl,msg,args...) \
        syslog(lvl, lvlname ": ["COMP_NAME"] "  msg, ## args);

    /* Internal macro defined for using syslog with log level info, enter or exit info.
     * NOT FOR PUBLIC USE
     */
    #define O_LOG_SYSLOG_EX(lvlname,lvl,msg,args...) \
        syslog(lvl, lvlname ": ["COMP_NAME"] " __FILE__ ",%s():line %i - " msg,__func__, __LINE__, ## args);

#else
    /* protect against poor coding habits, like missing braces */
    #define O_LOG_SYSLOG(lvl,msg,args...)                       ((void)0)
    #define O_LOG_SYSLOG_DBG(lvlname,lvl,msg,args...)           ((void)0)
    #define O_LOG_SYSLOG_DBG_SIMPLE(lvlname,lvl,msg,args...)    ((void)0)
    #define O_LOG_SYSLOG_EX(lvlname,lvl,msg,args...)            ((void)0)
#endif

#if !defined(NTVLOG_NO_STDOUT)
    /* Internal macro defined for outputting debug message in terminal with logging level name, used in debug build.
     * NOT FOR PUBLIC USE
     */
    #if defined(NTVLOG_FUNCTION_NOT_FILE)
        #define O_LOG_PRINT_DBG(lvlname, msg,args...) \
                printf(lvlname ": ["COMP_NAME"] %s():line %i - " msg,__func__,__LINE__, ## args);
    #else
        #define O_LOG_PRINT_DBG(lvlname, msg,args...) \
                printf(lvlname ": ["COMP_NAME"] " __FILE__  ",line %i - " msg,__LINE__, ## args);
    #endif

    #define O_LOG_PRINT_DBG_SIMPLE(lvlname, msg,args...) \
        printf(lvlname ": ["COMP_NAME"] " msg, ## args);

    /* Internal macro defined for outputting debug message in terminal with logging level name, used for enter or exit a function.
     * NOT FOR PUBLIC USE
     */
    #define O_LOG_PRINT_EX(lvlname, msg,args...) \
        printf(lvlname ": ["COMP_NAME"] " __FILE__ ",%s,line %i - " msg,__func__, __LINE__, ## args);
#else
    /* protect against poor coding habits, like missing braces */
    #define O_LOG_PRINT_DBG(lvlname, msg,args...)           ((void)0)
    #define O_LOG_PRINT_DBG_SIMPLE(lvlname, msg,args...)    ((void)0)
    #define O_LOG_PRINT_EX(lvlname, msg,args...)            ((void)0)
#endif

#if !defined(NDEBUG) || defined(LOG_LEVEL)
/**
 * Macros defined for logging with O_FATAL level.
 * @par Note
 * This macro will will cause the process to exit raising the SIGABRT signal.
 */
#if ( O_LOG_LEVEL >= O_FATAL )
    #define O_LOG_FATAL(msg,args...) STMT(                                                       \
        o_log_assert_trap();                                                                     \
        O_LOG_SYSLOG_DBG("FATAL",O_FATAL, msg, ## args); O_LOG_PRINT_DBG("FATAL", msg, ## args); \
        fflush( stdout );assert(0);)
#else
    #define O_LOG_FATAL(msg,args...)    ((void)0)
#endif
/**
 * Macros defined for logging with O_LOG_ERROR level.
 */
#if ( O_LOG_LEVEL >= O_ERROR )
    #define O_LOG_ERROR(msg,args...) STMT(O_LOG_SYSLOG_DBG("ERROR",O_ERROR, msg, ## args); O_LOG_PRINT_DBG("ERROR", msg, ## args);)
    #define O_LOG_ERROR_SIMPLE(msg,args...) STMT(O_LOG_SYSLOG_DBG_SIMPLE("ERROR  ",O_ERROR, msg, ## args); O_LOG_PRINT_DBG_SIMPLE("ERROR  ", msg, ## args);)
#else
    #define O_LOG_ERROR(msg,args...)        ((void)0)
    #define O_LOG_ERROR_SIMPLE(msg,args...) ((void)0)
#endif
/**
 * Macros defined for logging with O_WARNING level.
 */
#if ( O_LOG_LEVEL >= O_WARNING )
    #define O_LOG_WARNING(msg,args...) STMT(O_LOG_SYSLOG_DBG("WARNING",O_WARNING, msg, ## args); O_LOG_PRINT_DBG("WARNING", msg, ## args);)
    #define O_LOG_WARNING_SIMPLE(msg,args...) STMT(O_LOG_SYSLOG_DBG_SIMPLE("WARNING",O_WARNING, msg, ## args); O_LOG_PRINT_DBG_SIMPLE("WARNING", msg, ## args);)
#else
    #define O_LOG_WARNING(msg,args...)          ((void)0)
    #define O_LOG_WARNING_SIMPLE(msg,args...)   ((void)0)
#endif
/**
 * Macros defined for logging with O_MAINSTATUS level.
 */
#if ( O_LOG_LEVEL >= O_MAINSTATUS )
    #define O_LOG_MAINSTATUS(msg,args...) STMT(O_LOG_SYSLOG_DBG("STATUS",O_MAINSTATUS, msg, ## args); O_LOG_PRINT_DBG("STATUS", msg, ## args);)
    #define O_LOG_MAINSTATUS_SIMPLE(msg,args...) STMT(O_LOG_SYSLOG_DBG_SIMPLE("STATUS ",O_MAINSTATUS, msg, ## args); O_LOG_PRINT_DBG_SIMPLE("STATUS ", msg, ## args);)
#else
    #define O_LOG_MAINSTATUS(msg,args...)           ((void)0)
    #define O_LOG_MAINSTATUS_SIMPLE(msg,args...)    ((void)0)
#endif
/**
 * Macros defined for logging with O_DEBUG level.
 */
#if ( O_LOG_LEVEL >= O_DEBUG )
    #define O_LOG_DEBUG(msg,args...) STMT(O_LOG_SYSLOG_DBG("DEBUG",O_DEBUG, msg, ## args); O_LOG_PRINT_DBG("DEBUG", msg, ## args);)
    #define O_LOG_DEBUG_SIMPLE(msg,args...) STMT(O_LOG_SYSLOG_DBG_SIMPLE("DEBUG  ",O_DEBUG, msg, ## args); O_LOG_PRINT_DBG_SIMPLE("DEBUG  ", msg, ## args);)
#else
    #define O_LOG_DEBUG(msg,args...)        ((void)0)
    #define O_LOG_DEBUG_SIMPLE(msg,args...) ((void)0)
#endif
/**
 * Macros defined for logging with O_TRACE level.
 */
#if (O_LOG_LEVEL >= O_TRACE)
    #define O_LOG_TRACE(msg,args...) STMT(O_LOG_SYSLOG_DBG("TRACE",O_DEBUG, msg, ## args); O_LOG_PRINT_DBG("TRACE", msg, ## args);)
    #define O_LOG_TRACE_SIMPLE(msg,args...) STMT(O_LOG_SYSLOG_DBG_SIMPLE("TRACE  ",O_DEBUG, msg, ## args); O_LOG_PRINT_DBG_SIMPLE("TRACE  ", msg, ## args);)
#else
    #define O_LOG_TRACE(msg,args...)        ((void)0)
    #define O_LOG_TRACE_SIMPLE(msg,args...) ((void)0)
#endif

/**
 * Macros defined for logging of entering a function with O_TRACE level.
 */
#if (O_LOG_LEVEL >= O_TRACE)
    #define O_LOG_ENTER(msg,args...) STMT(O_LOG_SYSLOG_EX("ENTER",O_DEBUG, msg, ## args); O_LOG_PRINT_EX("ENTER", msg, ## args);)
#else
    #define O_LOG_ENTER(msg,args...) ((void)0)
#endif
/**
 * Macros defined for logging of exiting a function with O_TRACE level.
 */
#if (O_LOG_LEVEL >= O_TRACE)
    #define O_LOG_EXIT(msg,args...) STMT(O_LOG_SYSLOG_EX("EXIT",O_DEBUG, msg, ## args); O_LOG_PRINT_EX("EXIT", msg, ## args);)
#else
    #define O_LOG_EXIT(msg,args...) ((void)0)
#endif

#else
    /* protect against poor coding habits, like missing braces */
    #define O_LOG_FATAL(msg,args...)                {O_LOG_SYSLOG(O_FATAL, msg, ## args); fflush( stdout ); abort();}
    #define O_LOG_ERROR(msg,args...)                O_LOG_SYSLOG(O_ERROR, msg, ## args)
    #define O_LOG_ERROR_SIMPLE(msg,args...)         O_LOG_SYSLOG(O_ERROR, msg, ## args)
    #define O_LOG_WARNING(msg,args...)              ((void)0)
    #define O_LOG_WARNING_SIMPLE(msg,args...)       ((void)0)
    #define O_LOG_MAINSTATUS(msg,args...)           O_LOG_SYSLOG(O_MAINSTATUS, msg, ## args)
    #define O_LOG_MAINSTATUS_SIMPLE(msg,args...)    O_LOG_SYSLOG(O_MAINSTATUS, msg, ## args)
    #define O_LOG_DEBUG(msg,args...)                ((void)0)
    #define O_LOG_DEBUG_SIMPLE(msg,args...)         ((void)0)
    #define O_LOG_TRACE(msg,args...)                ((void)0)
    #define O_LOG_TRACE_SIMPLE(msg,args...)         ((void)0)
    #define O_LOG_ENTER(msg,args...)                ((void)0)
    #define O_LOG_EXIT(msg,args...)                 ((void)0)
#endif

/**
 * @}
 */


/*===========================================================================*/
/* DESIGN BY CONTRACT STATEMENTS                                             */
/*===========================================================================*/

/**
 * @brief    Design by contract (DbC) statements
 * @defgroup ODBC Design by contract (DbC) statements
 * @ingroup  ODEBUG
 * Provides support simple design by contract statements:
 * - require
 * - ensure
 * - loop invariant
 *
 * @{
 */
#if !defined(NDEBUG)

/**
 * Macro, asserts on a "require" failure, i.e. a pre-condition failure.
 * @par Note
 * In the implementation of any function there \b MUST be (at least) a corresponding REQUIRE
 * statement for every pre-condition listed in the header documentation.
 */
#define REQUIRE(pre) STMT(if (!(pre)) {                                     \
    o_log_assert_trap();                                                    \
    syslog( O_FATAL, "Precondition fail - file:%s,func:%s,ln:%d cond:%s\n", \
        __FILE__,__func__,__LINE__,#pre );                                  \
    printf( "Precondition fail - file:%s,func:%s,ln:%d cond:%s\n",         \
        __FILE__,__func__,__LINE__,#pre );                                  \
    fflush( stdout );assert(0); })

/**
 * Macro, asserts on an "ensure" failure, i.e. a post-condition failure.
 * @par Note
 * In the implementation of any function there \b MUST be (at least) a corresponding ENSURE
 * statement for every post-condition listed in the header documentation.
 */
#define ENSURE(post) STMT(if (!(post)) {                                    \
    o_log_assert_trap();                                                    \
    syslog( O_FATAL, "Postcondition fail - file:%s,func:%s,ln:%d cond:%s\n",\
        __FILE__,__func__,__LINE__,#post );                                 \
    printf( "Postcondition fail - file:%s,func:%s,ln:%d cond:%s\n",        \
        __FILE__,__func__,__LINE__,#post );                                 \
    fflush( stdout );assert(0); })

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
    printf( "Loop invariant changed - file:%s,func:%s,ln:%d var:%s %d->%d\n",          \
        __FILE__,__func__,__LINE__,#x,dbc_##_inv_,((size_t)(x)) );                      \
    fflush( stdout );assert(0); })

#else
    /* protect against poor coding habits, like missing braces */
    #define REQUIRE(pre)                ((void)0)
    #define ENSURE(post)                ((void)0)
    #define LOOP_INVARIANT_INIT(inv,x)  ((void)0)
    #define LOOP_INVARIANT_TEST(inv,x)  ((void)0)
#endif
/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __NTVLOG_H_ */
