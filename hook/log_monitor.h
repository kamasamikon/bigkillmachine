/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __NH_LOG_MONITOR_H__
#define __NH_LOG_MONITOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/inotify.h>

#define EVENT_MASK (IN_MODIFY)

void klogmon_init();

#ifdef __cplusplus
}
#endif
#endif /* __NH_LOG_MONITOR_H__ */

