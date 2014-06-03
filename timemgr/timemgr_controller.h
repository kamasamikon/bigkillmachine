
#ifndef __TIMEMGR_CONTROLLER_H__
#define __TIMEMGR_CONTROLLER_H__

#include <glib-object.h>
#include <otvdbus-controller.h>

G_BEGIN_DECLS

/* convenience macros */
#define TIMEMGR_TYPE_CONTROLLER             (timemgr_controller_get_type())
#define TIMEMGR_CONTROLLER(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),TIMEMGR_TYPE_CONTROLLER, TimemgrController))
#define TIMEMGR_CONTROLLER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),TIMEMGR_TYPE_CONTROLLER,  TimemgrControllerClass))
#define TIMEMGR_IS_CONTROLLER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),TIMEMGR_TYPE_CONTROLLER))
#define TIMEMGR_IS_CONTROLLER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),TIMEMGR_TYPE_CONTROLLER))
#define TIMEMGR_CONTROLLER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),TIMEMGR_TYPE_CONTROLLER, TimemgrControllerClass))

typedef struct _TimemgrController           TimemgrController;
typedef struct _TimemgrControllerClass      TimemgrControllerClass;

struct _TimemgrController 
{
    OtvdbusController parent;
};

struct _TimemgrControllerClass 
{
    OtvdbusControllerClass parent_class;
};

/* member functions */
GType timemgr_controller_get_type(void) G_GNUC_CONST;

TimemgrController*  timemgr_controller_get_default(void);

G_END_DECLS

#endif /* __TIMEMGR_CONTROLLER_H__ */


