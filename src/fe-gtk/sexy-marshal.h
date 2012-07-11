
#ifndef __sexy_marshal_MARSHAL_H__
#define __sexy_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* BOOLEAN:STRING (./marshal.list:1) */
extern void sexy_marshal_BOOLEAN__STRING (GClosure     *closure,
                                          GValue       *return_value,
                                          guint         n_param_values,
                                          const GValue *param_values,
                                          gpointer      invocation_hint,
                                          gpointer      marshal_data);

/* OBJECT:OBJECT,OBJECT (./marshal.list:2) */
extern void sexy_marshal_OBJECT__OBJECT_OBJECT (GClosure     *closure,
                                                GValue       *return_value,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint,
                                                gpointer      marshal_data);

G_END_DECLS

#endif /* __sexy_marshal_MARSHAL_H__ */

