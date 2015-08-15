/* vim:set noet ts=8 sw=8 sts=8 ff=unix: */

#ifndef __BKM_DALOG_SETUP_H__
#define __BKM_DALOG_SETUP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/inotify.h>

#define EVENT_MASK (IN_MODIFY)

void dalog_setup(void);

#ifdef __cplusplus
}
#endif
#endif /* __BKM_DALOG_SETUP_H__ */

