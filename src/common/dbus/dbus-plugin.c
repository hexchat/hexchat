/* dbus-plugin.c - xchat plugin for remote access using D-Bus
 * Copyright (C) 2006 Claessens Xavier
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 * Claessens Xavier
 * xclaesse@gmail.com
 */

#define DBUS_API_SUBJECT_TO_CHANGE

#include <config.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <glib/gi18n.h>
#include "../xchat-plugin.h"

#define PNAME _("remote access")
#define PDESC _("plugin for remote access using DBUS")
#define PVERSION ""

#define DBUS_SERVICE "org.xchat.service"
#define DBUS_OBJECT_PATH "/org/xchat"

static xchat_plugin *ph;
static guint last_context_id = 0;
static GList *contexts = NULL;
static GHashTable *clients = NULL;
static DBusGConnection *connection;

typedef struct RemoteObject RemoteObject;
typedef struct RemoteObjectClass RemoteObjectClass;

GType Remote_object_get_type (void);

struct RemoteObject
{
	GObject parent;

	guint last_hook_id;
	guint last_list_id;
	xchat_context *context;
	char *dbus_path;
	char *filename;
	GHashTable *hooks;
	GHashTable *lists;
	void *handle;
};

struct RemoteObjectClass
{
	GObjectClass parent;
};

typedef struct 
{
	guint id;
	int return_value;
	xchat_hook *hook;
	RemoteObject *obj;
} HookInfo;

typedef struct
{
	guint id;
	xchat_context *context;
} ContextInfo;

enum
{
	SERVER_SIGNAL,
	COMMAND_SIGNAL,
	PRINT_SIGNAL,
	UNLOAD_SIGNAL,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

#define REMOTE_TYPE_OBJECT              (remote_object_get_type ())
#define REMOTE_OBJECT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), REMOTE_TYPE_OBJECT, RemoteObject))
#define REMOTE_OBJECT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), REMOTE_TYPE_OBJECT, RemoteObjectClass))
#define REMOTE_IS_OBJECT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), REMOTE_TYPE_OBJECT))
#define REMOTE_IS_OBJECT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), REMOTE_TYPE_OBJECT))
#define REMOTE_OBJECT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), REMOTE_TYPE_OBJECT, RemoteObjectClass))
#define REMOTE_OBJECT_ERROR (remote_object_error_quark ())
#define REMOTE_TYPE_ERROR (remote_object_error_get_type ()) 

G_DEFINE_TYPE (RemoteObject, remote_object, G_TYPE_OBJECT)

/* Available services */

static gboolean		remote_object_connect		(RemoteObject *obj,
							 const char *filename,
							 const char *name,
							 const char *desc,
							 const char *version,
							 DBusGMethodInvocation *context);

static gboolean		remote_object_disconnect	(RemoteObject *obj,
							 DBusGMethodInvocation *context);

static gboolean		remote_object_command		(RemoteObject *obj,
							 const char *command,
							 GError **error);

static gboolean		remote_object_print		(RemoteObject *obj,
							 const char *text,
							 GError **error);

static gboolean		remote_object_find_context	(RemoteObject *obj,
							 const char *server,
							 const char *channel,
							 guint *ret_id,
							 GError **error);

static gboolean		remote_object_get_context	(RemoteObject *obj,
							 guint *ret_id,
							 GError **error);

static gboolean		remote_object_set_context	(RemoteObject *obj,
							 guint id,
							 gboolean *ret,
							 GError **error);

static gboolean		remote_object_print		(RemoteObject *obj,
							 const char *text,
							 GError **error);

static gboolean		remote_object_get_info		(RemoteObject *obj,
							 const char *id,
							 char **ret_info,
							 GError **error);

static gboolean		remote_object_get_prefs		(RemoteObject *obj,
							 const char *name,
							 int *ret_type,
							 char **ret_str,
							 int *ret_int,
							 GError **error);

static gboolean		remote_object_hook_command	(RemoteObject *obj,
							 const char *name,
							 int pri,
							 const char *help_text,
							 int return_value,
							 guint *ret_id,
							 GError **error);

static gboolean		remote_object_hook_server	(RemoteObject *obj,
							 const char *name,
							 int pri,
							 int return_value,
							 guint *ret_id,
							 GError **error);

static gboolean		remote_object_hook_print	(RemoteObject *obj,
							 const char *name,
							 int pri,
							 int return_value,
							 guint *ret_id,
							 GError **error);

static gboolean		remote_object_unhook		(RemoteObject *obj,
							 guint id,
							 GError **error);

static gboolean		remote_object_list_get		(RemoteObject *obj,
							 const char *name,
							 guint *ret_id,
							 GError **error);

static gboolean		remote_object_list_next		(RemoteObject *obj,
							 guint id,
							 gboolean *ret,
							 GError **error);

static gboolean		remote_object_list_str		(RemoteObject *obj,
							 guint id,
							 const char *name,
							 char **ret_str,
							 GError **error);

static gboolean		remote_object_list_int		(RemoteObject *obj,
							 guint id,
							 const char *name,
							 int *ret_int,
							 GError **error);

static gboolean		remote_object_list_time		(RemoteObject *obj,
							 guint id,
							 const char *name,
							 guint64 *ret_time,
							 GError **error);

static gboolean		remote_object_list_fields	(RemoteObject *obj,
							 const char *name,
							 char ***ret,
							 GError **error);

static gboolean		remote_object_list_free		(RemoteObject *obj,
							 guint id,
							 GError **error);

static gboolean		remote_object_emit_print	(RemoteObject *obj,
							 const char *event_name,
							 const char *args[],
							 gboolean *ret,
							 GError **error);

static gboolean		remote_object_nickcmp		(RemoteObject *obj,
							 const char *nick1,
							 const char *nick2,
							 int *ret,
							 GError **error);

static gboolean		remote_object_strip		(RemoteObject *obj,
							 const char *str,
							 int len,
							 int flag,
							 char **ret_str,
							 GError **error);

static gboolean		remote_object_send_modes	(RemoteObject *obj,
							 const char *targets[],
							 int modes_per_line,
							 char sign,
							 char mode,
							 GError **error);

#include "remote-object-glue.h"
#include "marshallers.h"

/* Useful functions */

static char**		build_list			(char *word[]);
static guint		context_list_find_id		(xchat_context *context);
static xchat_context*	context_list_find_context	(guint id);

/* Remote Object */

static void
hook_info_destroy (gpointer data)
{
	HookInfo *info = (HookInfo*)data;

	if (info == NULL) {
		return;
	}
	xchat_unhook (ph, info->hook);
	g_free (info);
}

static void
list_info_destroy (gpointer data)
{
	xchat_list_free (ph, (xchat_list*)data);
}

static void
remote_object_finalize (GObject *obj)
{
	RemoteObject *self = (RemoteObject*)obj;

	g_hash_table_destroy (self->lists);
	g_hash_table_destroy (self->hooks);
	g_free (self->dbus_path);
	g_free (self->filename);
	xchat_plugingui_remove (ph, self->handle);

	G_OBJECT_CLASS (remote_object_parent_class)->finalize (obj);
}

static void
remote_object_init (RemoteObject *obj)
{
	obj->hooks =
		g_hash_table_new_full (g_int_hash,
				       g_int_equal,
				       NULL,
				       hook_info_destroy);

	obj->lists =
		g_hash_table_new_full (g_int_hash,
				       g_int_equal,
				       g_free,
				       list_info_destroy);
	obj->dbus_path = NULL;
	obj->filename = NULL;
	obj->last_hook_id = 0;
	obj->last_list_id = 0;
	obj->context = xchat_get_context (ph);
}

static void
remote_object_class_init (RemoteObjectClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->finalize = remote_object_finalize;

	signals[SERVER_SIGNAL] =
		g_signal_new ("server_signal",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      g_cclosure_user_marshal_VOID__POINTER_POINTER_UINT,
			      G_TYPE_NONE,
			      3, G_TYPE_STRV, G_TYPE_STRV, G_TYPE_UINT);

	signals[COMMAND_SIGNAL] =
		g_signal_new ("command_signal",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      g_cclosure_user_marshal_VOID__POINTER_POINTER_UINT,
			      G_TYPE_NONE,
			      3, G_TYPE_STRV, G_TYPE_STRV, G_TYPE_UINT);

	signals[PRINT_SIGNAL] =
		g_signal_new ("print_signal",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      g_cclosure_user_marshal_VOID__POINTER_POINTER_UINT,
			      G_TYPE_NONE,
			      2, G_TYPE_STRV, G_TYPE_UINT);

	signals[UNLOAD_SIGNAL] =
		g_signal_new ("unload_signal",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}

/* Implementation of services */

static gboolean
remote_object_connect (RemoteObject *obj,
		       const char *filename,
		       const char *name,
		       const char *desc,
		       const char *version,
		       DBusGMethodInvocation *context)
{
	static guint count = 0;
	char *sender, *path;
	RemoteObject *remote_object;
	
	sender = dbus_g_method_get_sender (context);
	remote_object = g_hash_table_lookup (clients, sender);
	if (remote_object != NULL) {
		dbus_g_method_return (context, remote_object->dbus_path);
		g_free (sender);
		return TRUE;
	}
	path = g_strdup_printf (DBUS_OBJECT_PATH"/%d", count++);
	remote_object = g_object_new (REMOTE_TYPE_OBJECT, NULL);
	remote_object->dbus_path = path;
	remote_object->filename = g_path_get_basename (filename);
	remote_object->handle = xchat_plugingui_add (ph,
						     remote_object->filename,
						     name,
						     desc,
						     version, NULL);
	dbus_g_connection_register_g_object (connection,
					     path,
					     G_OBJECT (remote_object));

	g_hash_table_insert (clients,
			     sender,
			     remote_object);

	dbus_g_method_return (context, path);
	return TRUE;
}

static gboolean
remote_object_disconnect (RemoteObject *obj,
			  DBusGMethodInvocation *context)
{
	char *sender;
	
	sender = dbus_g_method_get_sender (context);
	g_hash_table_remove (clients, sender);
	g_free (sender);

	dbus_g_method_return (context);
	return TRUE;
}

static gboolean
remote_object_command (RemoteObject *obj,
		       const char *command,
		       GError **error)
{
	if (xchat_set_context (ph, obj->context)) {
		xchat_command (ph, command);
	}
	return TRUE;
}

static gboolean
remote_object_print (RemoteObject *obj,
		     const char *text,
		     GError **error)
{
	if (xchat_set_context (ph, obj->context)) {
		xchat_print (ph, text);
	}
	return TRUE;
}

static gboolean
remote_object_find_context (RemoteObject *obj,
			    const char *server,
			    const char *channel,
			    guint *ret_id,
			    GError **error)
{
	xchat_context *context;

	if (*server == '\0') {
		server = NULL;
	}
	if (*channel == '\0') {
		channel = NULL;
	}
	context = xchat_find_context (ph, server, channel);
	*ret_id = context_list_find_id (context);

	return TRUE;
}

static gboolean
remote_object_get_context (RemoteObject *obj,
			   guint *ret_id,
			   GError **error)
{
	*ret_id = context_list_find_id (obj->context);
	return TRUE;
}

static gboolean
remote_object_set_context (RemoteObject *obj,
			   guint id,
			   gboolean *ret,
			   GError **error)
{
	xchat_context *context;
	
	context = context_list_find_context (id);
	if (context == NULL) {
		*ret = FALSE;
		return TRUE;
	}
	obj->context = context;
	*ret = TRUE;

	return TRUE;
}

static gboolean
remote_object_get_info (RemoteObject *obj,
			const char *id,
			char **ret_info,
			GError **error)
{
	if (!xchat_set_context (ph, obj->context)) {
		*ret_info = NULL;
		return TRUE;
	}
	*ret_info = g_strdup (xchat_get_info (ph, id));
	return TRUE;
}

static gboolean
remote_object_get_prefs (RemoteObject *obj,
			 const char *name,
			 int *ret_type,
			 char **ret_str,
			 int *ret_int,
			 GError **error)
{
	const char *str;

	if (!xchat_set_context (ph, obj->context)) {
		*ret_type = 0;
		return TRUE;
	}
	*ret_type = xchat_get_prefs (ph, name, &str, ret_int);
	*ret_str = g_strdup (str);

	return TRUE;
}

static int
server_hook_cb (char *word[],
		char *word_eol[],
		void *userdata)
{
	HookInfo *info = (HookInfo*)userdata;
	char **arg1;
	char **arg2;

	arg1 = build_list (word + 1);
	arg2 = build_list (word_eol + 1);
	g_signal_emit (info->obj,
		       signals[SERVER_SIGNAL],
		       0,
		       arg1, arg2, info->id);
	g_strfreev (arg1);
	g_strfreev (arg2);
  
	return info->return_value;
}

static int
command_hook_cb (char *word[],
		 char *word_eol[],
		 void *userdata)
{
	HookInfo *info = (HookInfo*)userdata;
	char **arg1;
	char **arg2;

	arg1 = build_list (word + 1);
	arg2 = build_list (word_eol + 1);
	g_signal_emit (info->obj,
		       signals[COMMAND_SIGNAL],
		       0,
		       arg1, arg2, info->id);
	g_strfreev (arg1);
	g_strfreev (arg2);
  
	return info->return_value;
}

static int
print_hook_cb (char *word[],
	       void *userdata)
{
	HookInfo *info = (HookInfo*)userdata;
	char **arg1;

	arg1 = build_list (word + 1);
	g_signal_emit (info->obj,
		       signals[PRINT_SIGNAL],
		       0,
		       arg1, info->id);
	g_strfreev (arg1);
  
	return info->return_value;
}

static gboolean
remote_object_hook_command (RemoteObject *obj,
			    const char *name,
			    int priority,
			    const char *help_text,
			    int return_value,
			    guint *ret_id,
			    GError **error)
{
	HookInfo *info;

	info = g_new0 (HookInfo, 1);
	info->obj = obj;
	info->return_value = return_value;
	info->id = obj->last_hook_id++;
	info->hook = xchat_hook_command (ph,
					 name,
					 priority,
					 command_hook_cb,
					 help_text,
					 info);
	g_hash_table_insert (obj->hooks, &info->id, info);
	*ret_id = info->id;

	return TRUE;
}

static gboolean
remote_object_hook_server (RemoteObject *obj,
			   const char *name,
			   int priority,
			   int return_value,
			   guint *ret_id,
			   GError **error)
{
	HookInfo *info;

	info = g_new0 (HookInfo, 1);
	info->obj = obj;
	info->return_value = return_value;
	info->id = obj->last_hook_id++;
	info->hook = xchat_hook_server (ph,
					name,
					priority,
					server_hook_cb,
					info);
	g_hash_table_insert (obj->hooks, &info->id, info);
	*ret_id = info->id;

	return TRUE;
}

static gboolean
remote_object_hook_print (RemoteObject *obj,
			  const char *name,
			  int priority,
			  int return_value,
			  guint *ret_id,
			  GError **error)
{
	HookInfo *info;

	info = g_new0 (HookInfo, 1);
	info->obj = obj;
	info->return_value = return_value;
	info->id = obj->last_hook_id++;
	info->hook = xchat_hook_print (ph,
				       name,
				       priority,
				       print_hook_cb,
				       info);
	g_hash_table_insert (obj->hooks, &info->id, info);
	*ret_id = info->id;

	return TRUE;
}

static gboolean
remote_object_unhook (RemoteObject *obj,
		      guint id,
		      GError **error)
{
	g_hash_table_remove (obj->hooks, &id);
	return TRUE;
}

static gboolean
remote_object_list_get (RemoteObject *obj,
			const char *name,
			guint *ret_id,
			GError **error)
{
	xchat_list *xlist;
	guint *id;

	if (!xchat_set_context (ph, obj->context)) {
		*ret_id = 0;
		return TRUE;
	}
	xlist = xchat_list_get (ph, name);
	if (xlist == NULL) {
		*ret_id = 0;
		return TRUE;
	}
	id = g_new0 (guint, 1);
	*id = obj->last_list_id++;
	*ret_id = *id;
	g_hash_table_insert (obj->lists,
			     id,
			     xlist);

	return TRUE;
}

static gboolean
remote_object_list_next	(RemoteObject *obj,
			 guint id,
			 gboolean *ret,
			 GError **error)
{
	xchat_list *xlist;
	
	xlist = g_hash_table_lookup (obj->lists, &id);
	if (xlist == NULL) {
		*ret = FALSE;
		return TRUE;
	}
	*ret = xchat_list_next (ph, xlist);

	return TRUE;
}			 

static gboolean
remote_object_list_str (RemoteObject *obj,
			guint id,
			const char *name,
			char **ret_str,
			GError **error)
{
	xchat_list *xlist;
	
	xlist = g_hash_table_lookup (obj->lists, &id);
	if (xlist == NULL && !xchat_set_context (ph, obj->context)) {
		*ret_str = NULL;
		return TRUE;
	}
	if (g_str_equal (name, "context")) {
		*ret_str = NULL;
		return TRUE;
	}
	*ret_str = g_strdup (xchat_list_str (ph, xlist, name));

	return TRUE;
}

static gboolean
remote_object_list_int (RemoteObject *obj,
			guint id,
			const char *name,
			int *ret_int,
			GError **error)
{
	xchat_list *xlist;
	
	xlist = g_hash_table_lookup (obj->lists, &id);
	if (xlist == NULL && !xchat_set_context (ph, obj->context)) {
		*ret_int = -1;
		return TRUE;
	}
	if (g_str_equal (name, "context")) {
		xchat_context *context;
		context = (xchat_context*)xchat_list_str (ph, xlist, name);
		*ret_int = context_list_find_id (context);
	} else {
		*ret_int = xchat_list_int (ph, xlist, name);
	}

	return TRUE;
}

static gboolean
remote_object_list_time (RemoteObject *obj,
			 guint id,
			 const char *name,
			 guint64 *ret_time,
			 GError **error)
{
	xchat_list *xlist;
	
	xlist = g_hash_table_lookup (obj->lists, &id);
	if (xlist == NULL) {
		*ret_time = (guint64) -1;
		return TRUE;
	}
	*ret_time = xchat_list_time (ph, xlist, name);
	
	return TRUE;
}

static gboolean
remote_object_list_fields (RemoteObject *obj,
			   const char *name,
			   char ***ret,
			   GError **error)
{
	*ret = g_strdupv ((char**)xchat_list_fields (ph, name));
	return TRUE;
}

static gboolean
remote_object_list_free (RemoteObject *obj,
			 guint id,
			 GError **error)
{
	g_hash_table_remove (obj->lists, &id);
	return TRUE;
}

static gboolean
remote_object_emit_print (RemoteObject *obj,
			  const char *event_name,
			  const char *args[],
			  gboolean *ret,
			  GError **error)
{
	const char *argv[4] = {NULL, NULL, NULL, NULL};
	int i;
	
	for (i = 0; i < 4 && args[i] != NULL; i++) {
		argv[i] = args[i];
	}

	*ret = xchat_set_context (ph, obj->context);
	if (*ret) {
		*ret = xchat_emit_print (ph, event_name, argv[1], argv[2],
							 argv[3], argv[4]);
	}

	return TRUE;
}

static gboolean
remote_object_nickcmp (RemoteObject *obj,
		       const char *nick1,
		       const char *nick2,
		       int *ret,
		       GError **error)
{
	xchat_set_context (ph, obj->context);
	*ret = xchat_nickcmp (ph, nick1, nick2);
	return TRUE;
}

static gboolean
remote_object_strip (RemoteObject *obj,
		     const char *str,
		     int len,
		     int flag,
		     char **ret_str,
		     GError **error)
{
	*ret_str = xchat_strip (ph, str, len, flag);
	return TRUE;
}

static gboolean
remote_object_send_modes (RemoteObject *obj,
			  const char *targets[],
			  int modes_per_line,
			  char sign,
			  char mode,
			  GError **error)
{
	if (xchat_set_context (ph, obj->context)) {
		xchat_send_modes (ph, targets,
				  g_strv_length ((char**)targets),
				  modes_per_line,
				  sign, mode);
	}
	return TRUE;
}

/* DBUS stuffs */

static void
name_owner_changed (DBusGProxy *driver_proxy,
		    const char *name,
		    const char *old_owner,
		    const char *new_owner,
		    void       *user_data)
{
	if (*new_owner == '\0') {
		/* this name has vanished */
		g_hash_table_remove (clients, name);
	}
}

static gboolean
init_dbus (void)
{
	DBusGProxy *proxy;
	RemoteObject *remote;
	guint request_name_result;
	GError *error = NULL;

	dbus_g_object_type_install_info (REMOTE_TYPE_OBJECT,
					 &dbus_glib_remote_object_object_info);

	connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
	if (connection == NULL) {
		xchat_printf (ph, _("Couldn't connect to session bus: %s\n"),
			      error->message);
		g_error_free (error);
		return FALSE;
	}

	proxy = dbus_g_proxy_new_for_name (connection,
					   DBUS_SERVICE_DBUS,
					   DBUS_PATH_DBUS,
					   DBUS_INTERFACE_DBUS);

	if (!dbus_g_proxy_call (proxy, "RequestName", &error,
				G_TYPE_STRING, DBUS_SERVICE,
				G_TYPE_UINT, DBUS_NAME_FLAG_ALLOW_REPLACEMENT,
				G_TYPE_INVALID,
				G_TYPE_UINT, &request_name_result,
				G_TYPE_INVALID)) {
		xchat_printf (ph, _("Failed to acquire %s: %s\n"),
			      DBUS_SERVICE,
			      error->message);
		g_error_free (error);

		return FALSE;
	}

	dbus_g_proxy_add_signal (proxy, "NameOwnerChanged",
				 G_TYPE_STRING,
				 G_TYPE_STRING,
				 G_TYPE_STRING,
				 G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (proxy, "NameOwnerChanged", 
				     G_CALLBACK (name_owner_changed),
				     NULL, NULL);

	remote = g_object_new (REMOTE_TYPE_OBJECT, NULL);
	dbus_g_connection_register_g_object (connection,
					     DBUS_OBJECT_PATH"/Remote",
					     G_OBJECT (remote));

	return TRUE;
}

/* xchat_plugin stuffs */

static char**
build_list (char *word[])
{
	guint i;
	guint num = 0;
	char **result;

	if (word == NULL) {
		return NULL;
	}
  
	while (word[num] && word[num][0]) {
		num++;
	}
	
	result = g_new0 (char*, num + 1);
	for (i = 0; i < num; i++) {
		result[i] = g_strdup (word[i]);
	}

	return result;
}

static guint
context_list_find_id (xchat_context *context)
{
	GList *l = NULL;

	for (l = contexts; l != NULL; l = l->next) {
		if (((ContextInfo*)l->data)->context == context) {
			return ((ContextInfo*)l->data)->id;
		}
	}

	return 0;
}

static xchat_context*
context_list_find_context (guint id)
{
	GList *l = NULL;

	for (l = contexts; l != NULL; l = l->next) {
		if (((ContextInfo*)l->data)->id == id) {
			return ((ContextInfo*)l->data)->context;
		}
	}

	return NULL;
}

static int
open_context_cb (char *word[],
		 void *userdata)
{
	ContextInfo *info;
	
	info = g_new0 (ContextInfo, 1);
	info->id = ++last_context_id;
	info->context = xchat_get_context (ph);
	contexts = g_list_prepend (contexts, info);

	return XCHAT_EAT_NONE;
}

static int
close_context_cb (char *word[],
		  void *userdata)
{
	GList *l;
	xchat_context *context = xchat_get_context (ph);

	for (l = contexts; l != NULL; l = l->next) {
		if (((ContextInfo*)l->data)->context == context) {
			g_free (l->data);
			contexts = g_list_delete_link (contexts, l);
			break;
		}
	}

	return XCHAT_EAT_NONE;
}

static gboolean
clients_find_filename_foreach (gpointer key,
			       gpointer value,
			       gpointer user_data)
{
	RemoteObject *obj = REMOTE_OBJECT (value);
	return g_str_equal (obj->filename, (char*)user_data);
}

static int
unload_plugin_cb (char *word[], char *word_eol[], void *userdata)
{
	RemoteObject *obj;
	
	obj = g_hash_table_find (clients,
				 clients_find_filename_foreach,
				 word[2]);
	if (obj != NULL) {
		g_signal_emit (obj, 
			       signals[UNLOAD_SIGNAL],
			       0);
		return XCHAT_EAT_ALL;
	}
	
	return XCHAT_EAT_NONE;
}

int
dbus_plugin_init (xchat_plugin *plugin_handle,
		  char **plugin_name,
		  char **plugin_desc,
		  char **plugin_version,
		  char *arg)
{
	ph = plugin_handle;
	*plugin_name = PNAME;
	*plugin_desc = PDESC;
	*plugin_version = PVERSION;

	if (init_dbus()) {
		xchat_printf (ph, _("%s loaded successfully!\n"), PNAME);

		clients = g_hash_table_new_full (g_str_hash,
						 g_str_equal,
						 g_free,
						 g_object_unref);

		xchat_hook_print (ph, "Open Context",
				  XCHAT_PRI_NORM,
				  open_context_cb,
				  NULL);

		xchat_hook_print (ph, "Close Context",
				  XCHAT_PRI_NORM,
				  close_context_cb,
				  NULL);

		xchat_hook_command (ph, "unload",
				    XCHAT_PRI_HIGHEST,
				    unload_plugin_cb, NULL, NULL);
	}

	return TRUE; 
}
