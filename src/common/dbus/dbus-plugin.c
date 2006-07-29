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

/* TODO:
 * implement full C plugin API. Functions remaining:
 *	- xchat_nickcmp
 *	- xchat_list_fields
 *	- xchat_list_time
 *	- xchat_plugingui_add
 *	- xchat_plugingui_remove
 *	- xchat_emit_print (is it possible to send valist through D-Bus ?)
 *	- xchat_send_modes
 *	- xchat_strip
 *	- xchat_free
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

typedef struct ManagerObject ManagerObject;
typedef struct ManagerObjectClass ManagerObjectClass;

GType Manager_object_get_type (void);

struct ManagerObject
{
	GObject parent;
};

struct ManagerObjectClass
{
	GObjectClass parent;
};

#define MANAGER_TYPE_OBJECT              (manager_object_get_type ())
#define MANAGER_OBJECT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), MANAGER_TYPE_OBJECT, ManagerObject))
#define MANAGER_OBJECT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), MANAGER_TYPE_OBJECT, ManagerObjectClass))
#define MANAGER_IS_OBJECT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MANAGER_TYPE_OBJECT))
#define MANAGER_IS_OBJECT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), MANAGER_TYPE_OBJECT))
#define MANAGER_OBJECT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MANAGER_TYPE_OBJECT, ManagerObjectClass))

G_DEFINE_TYPE (ManagerObject, manager_object, G_TYPE_OBJECT)

static gboolean		manager_object_connect		(ManagerObject *obj,
							 DBusGMethodInvocation *context);

static gboolean		manager_object_disconnect	(ManagerObject *obj,
							 DBusGMethodInvocation *context);

typedef struct RemoteObject RemoteObject;
typedef struct RemoteObjectClass RemoteObjectClass;

GType Remote_object_get_type (void);

struct RemoteObject
{
	GObject parent;

	guint last_hook_id;
	guint last_list_id;
	xchat_context *context;
	char *path;
	GHashTable *hooks;
	GHashTable *lists;
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

typedef enum
{
	REMOTE_OBJECT_ERROR_SET_CONTEXT,
	REMOTE_OBJECT_ERROR_FIND_CONTEXT,
	REMOTE_OBJECT_ERROR_FIND_ID,
	REMOTE_OBJECT_ERROR_GET_INFO,
	REMOTE_OBJECT_ERROR_GET_PREFS,
	REMOTE_OBJECT_ERROR_LIST_NAME,
} RemoteObjectError;

enum
{
	SERVER_SIGNAL,
	COMMAND_SIGNAL,
	PRINT_SIGNAL,
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

static gboolean		remote_object_list_free		(RemoteObject *obj,
							 guint id,
							 GError **error);

#include "manager-object-glue.h"
#include "remote-object-glue.h"
#include "marshallers.h"

/* Manager Object */

static void
manager_object_init (ManagerObject *obj)
{
}

static void
manager_object_class_init (ManagerObjectClass *klass)
{
}

static gboolean
manager_object_connect (ManagerObject *obj,
			DBusGMethodInvocation *context)
{
	static guint count = 0;
	char *sender, *path;
	RemoteObject *remote_object;
	
	sender = dbus_g_method_get_sender (context);
	remote_object = g_hash_table_lookup (clients, sender);
	if (remote_object != NULL) {
		dbus_g_method_return (context, remote_object->path);
		g_free (sender);
		return TRUE;
	}
	path = g_strdup_printf (DBUS_OBJECT_PATH"/%d", count++);
	remote_object = g_object_new (REMOTE_TYPE_OBJECT, NULL);
	remote_object->path = path;
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
manager_object_disconnect (ManagerObject *obj,
			   DBusGMethodInvocation *context)
{
	char *sender;
	
	sender = dbus_g_method_get_sender (context);
	g_hash_table_remove (clients, sender);
	g_free (sender);

	dbus_g_method_return (context);
	return TRUE;
}

/* Remote Object */

GQuark
remote_object_error_quark (void)
{
	static GQuark quark = 0;
	if (!quark) {
		quark = g_quark_from_static_string ("remote_object_error");
	}

	return quark;
}

#define ENUM_ENTRY(NAME, DESC) { NAME, "" #NAME "", DESC }

GType
remote_object_error_get_type (void)
{
	static GType etype = 0;

	if (etype == 0) {
		static const GEnumValue values[] = {
			ENUM_ENTRY (REMOTE_OBJECT_ERROR_SET_CONTEXT, "SetContext"),
			ENUM_ENTRY (REMOTE_OBJECT_ERROR_FIND_CONTEXT, "FindContext"),
			ENUM_ENTRY (REMOTE_OBJECT_ERROR_FIND_ID, "FindID"),
			ENUM_ENTRY (REMOTE_OBJECT_ERROR_GET_INFO, "GetInfo"),
			ENUM_ENTRY (REMOTE_OBJECT_ERROR_GET_PREFS, "GetPrefs"),
			ENUM_ENTRY (REMOTE_OBJECT_ERROR_LIST_NAME, "ListName"),
			{ 0, 0, 0 }
		};
		etype = g_enum_register_static ("RemoteObjectError", values);
	}

	return etype;
}

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
	g_free (self->path);

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
}
/* Implementation of services */

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

static gboolean
remote_object_switch_context (RemoteObject *obj, GError **error)
{
	if (!xchat_set_context (ph, obj->context)) {
		obj->context = xchat_get_context (ph);
		g_set_error (error,
			     REMOTE_OBJECT_ERROR,
			     REMOTE_OBJECT_ERROR_SET_CONTEXT,
			     _("switch to an invalide xchat context"));

		return FALSE;
	}

	return TRUE;
}

static gboolean
remote_object_command (RemoteObject *obj,
		       const char *command,
		       GError **error)
{
	if (!remote_object_switch_context(obj, error)) {
		return FALSE;
	}
	xchat_command (ph, command);
	return TRUE;
}

static gboolean
remote_object_print (RemoteObject *obj,
		     const char *text,
		     GError **error)
{
	if (!remote_object_switch_context(obj, error)) {
		return FALSE;
	}
	xchat_print (ph, text);
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
	if (*ret_id == 0) {
		g_set_error (error,
			     REMOTE_OBJECT_ERROR,
			     REMOTE_OBJECT_ERROR_FIND_CONTEXT,
			     _("xchat context not found"));

		return FALSE;
	}

	return TRUE;
}

static gboolean
remote_object_get_context (RemoteObject *obj,
			   guint *ret_id,
			   GError **error)
{
	*ret_id = context_list_find_id (obj->context);
	if (*ret_id == 0) {
		obj->context = xchat_get_context (ph);
		g_set_error (error,
			     REMOTE_OBJECT_ERROR,
			     REMOTE_OBJECT_ERROR_FIND_CONTEXT,
			     _("xchat context not found"));

		return FALSE;
	}

	return TRUE;
}

static gboolean
remote_object_set_context (RemoteObject *obj,
			   guint id,
			   GError **error)
{
	xchat_context *context;
	
	context = context_list_find_context (id);
	if (context == NULL) {
		g_set_error (error,
			     REMOTE_OBJECT_ERROR,
			     REMOTE_OBJECT_ERROR_FIND_ID,
			     _("xchat context ID not found"));

		return FALSE;
	}
	obj->context = context;

	return TRUE;
}

static gboolean
remote_object_get_info (RemoteObject *obj,
			const char *id,
			char **ret_info,
			GError **error)
{
	if (!remote_object_switch_context(obj, error)) {
		return FALSE;
	}
	*ret_info = g_strdup (xchat_get_info (ph, id));
	if (*ret_info == NULL) {
		g_set_error (error,
			     REMOTE_OBJECT_ERROR,
			     REMOTE_OBJECT_ERROR_GET_INFO,
			     _("info \"%s\" does not exist"),
			     id);

		return FALSE;
	}
	
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
	if (!remote_object_switch_context(obj, error)) {
		return FALSE;
	}
	*ret_type = xchat_get_prefs (ph, name, &str, ret_int);
	if (*ret_type == 0) {
		g_set_error (error,
			     REMOTE_OBJECT_ERROR,
			     REMOTE_OBJECT_ERROR_GET_PREFS,
			     _("preference \"%s\" does not exist"),
			     name);

		return FALSE;
	}
	*ret_str = g_strdup (str);

	return TRUE;
}

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
	if (!g_hash_table_remove (obj->hooks, &id)) {
		g_set_error (error,
			     REMOTE_OBJECT_ERROR,
			     REMOTE_OBJECT_ERROR_FIND_ID,
			     _("xchat hook ID not found"));

		return FALSE;
	}
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

	xlist = xchat_list_get (ph, name);
	if (xlist == NULL) {
		g_set_error (error,
			     REMOTE_OBJECT_ERROR,
			     REMOTE_OBJECT_ERROR_LIST_NAME,
			     _("\"%s\" list name not found"),
			     name);

		return FALSE;
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
		g_set_error (error,
			     REMOTE_OBJECT_ERROR,
			     REMOTE_OBJECT_ERROR_FIND_ID,
			     _("xchat list ID not found"));

		return FALSE;
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
	if (xlist == NULL) {
		g_set_error (error,
			     REMOTE_OBJECT_ERROR,
			     REMOTE_OBJECT_ERROR_FIND_ID,
			     _("xchat list ID not found"));

		return FALSE;
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
	if (xlist == NULL) {
		g_set_error (error,
			     REMOTE_OBJECT_ERROR,
			     REMOTE_OBJECT_ERROR_FIND_ID,
			     _("xchat list ID not found"));

		return FALSE;
	}
	*ret_int = xchat_list_int (ph, xlist, name);

	return TRUE;
}

static gboolean
remote_object_list_free (RemoteObject *obj,
			 guint id,
			 GError **error)
{
	if (!g_hash_table_remove (obj->lists, &id)) {
		g_set_error (error,
			     REMOTE_OBJECT_ERROR,
			     REMOTE_OBJECT_ERROR_FIND_ID,
			     _("xchat list ID not found"));

		return FALSE;
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
	} else if (*old_owner == '\0') {
		/* this name has been added */
	}
}

static gboolean
init_dbus (void)
{
	DBusGProxy *proxy;
	ManagerObject *manager;
	guint request_name_result;
	GError *error = NULL;

	dbus_g_object_type_install_info (REMOTE_TYPE_OBJECT,
					 &dbus_glib_remote_object_object_info);

	dbus_g_object_type_install_info (MANAGER_TYPE_OBJECT,
					 &dbus_glib_manager_object_object_info);

	dbus_g_error_domain_register (REMOTE_OBJECT_ERROR,
				      NULL,
				      REMOTE_TYPE_ERROR);

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

	manager = g_object_new (MANAGER_TYPE_OBJECT, NULL);
	dbus_g_connection_register_g_object (connection,
					     DBUS_OBJECT_PATH"/Manager",
					     G_OBJECT (manager));

	return TRUE;
}

/* xchat_plugin stuffs */

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

		clients =
			g_hash_table_new_full (g_str_hash,
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
	}

	return TRUE; 
}
