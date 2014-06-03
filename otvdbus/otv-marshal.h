
#ifndef __otv_marshal_MARSHAL_H__
#define __otv_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* BOOLEAN:ULONG (otv-marshal.list:1) */
extern void otv_marshal_BOOLEAN__ULONG (GClosure     *closure,
                                        GValue       *return_value,
                                        guint         n_param_values,
                                        const GValue *param_values,
                                        gpointer      invocation_hint,
                                        gpointer      marshal_data);

/* VOID:VOID (otv-marshal.list:2) */
#define otv_marshal_VOID__VOID	g_cclosure_marshal_VOID__VOID

/* VOID:POINTER (otv-marshal.list:3) */
#define otv_marshal_VOID__POINTER	g_cclosure_marshal_VOID__POINTER

G_END_DECLS

#endif /* __otv_marshal_MARSHAL_H__ */

