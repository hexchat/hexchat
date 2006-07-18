/* dbus-plugin.c - xchat plugin for remote access using DBUS
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

#define PNAME _("xchat remote access")
#define PDESC _("plugin for remote access using DBUS")
#define DOMAIN "dbus plugin"
#define PVERSION "0.9"

#define DBUS_SERVICE "org.xchat.service"
#define DBUS_OBJECT "/org/xchat/RemoteObject"

typedef struct RemoteObject RemoteObject;
typedef struct RemoteObjectClass RemoteObjectClass;

GType Remote_object_get_type (void);

struct RemoteObject
{
	GObject parent;
};

struct RemoteObjectClass
{
	GObjectClass parent;
};

typedef enum
{
	REMOTE_OBJECT_ERROR_SET_CONTEXT,
	REMOTE_OBJECT_ERROR_FIND_CONTEXT,
	REMOTE_OBJECT_ERROR_FIND_ID,
	REMOTE_OBJECT_ERROR_GET_INFO,
	REMOTE_OBJECT_ERROR_GET_PREFS
} RemoteObjectError;

enum
{
	SERVER_SIGNAL,
	COMMAND_SIGNAL,
	PRINT_SIGNAL,
	LAST_SIGNAL
};

typedef struct 
{
	guint id;
	int return_value;
	char *client;
	xchat_hook *hook;
} HookInfo;

typedef struct
{
	guint id;
	xchat_context *context;
} ContextInfo;

static xchat_plugin *ph;
static RemoteObject *remote_object;
static guint last_hook_id = 0;
static guint last_context_id = 0;
static GHashTable *hook_hash_table = NULL;
static GHashTable *client_hash_table = NULL;
static GList *context_list = NULL;
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
							 DBusGMethodInvocation *context);

static gboolean		remote_object_print		(RemoteObject *obj,
							 const char *text,
							 DBusGMethodInvocation *context);

static gboolean		remote_object_find_context	(RemoteObject *obj,
							 const char *server,
							 const char *channel,
							 guint *ret_id,
							 GError **error);

static gboolean		remote_object_get_context	(RemoteObject *obj,
							 DBusGMethodInvocation *context);

static gboolean		remote_object_set_context	(RemoteObject *obj,
							 guint id,
							 DBusGMethodInvocation *context);

static gboolean		remote_object_print		(RemoteObject *obj,
							 const char *text,
							 DBusGMethodInvocation *context);

static gboolean		remote_object_get_info		(RemoteObject *obj,
							 const char *id,
							 DBusGMethodInvocation *context);

static gboolean		remote_object_get_prefs		(RemoteObject *obj,
							 const char *name,
							 DBusGMethodInvocation *context);

static gboolean		remote_object_hook_command	(RemoteObject *obj,
							 const char *name,
							 int pri,
							 const char *help_text,
							 int return_value,
							 DBusGMethodInvocation *context);

static gboolean		remote_object_hook_server	(RemoteObject *obj,
							 const char *name,
							 int pri,
							 int return_value,
							 DBusGMethodInvocation *context);

static gboolean		remote_object_hook_print	(RemoteObject *obj,
							 const char *name,
							 int pri,
							 int return_value,
							 DBusGMethodInvocation *context);

static gboolean		remote_object_unhook		(RemoteObject *obj,
							 guint id,
							 GError **error);

#include "dbus-plugin-glue.h"
#include "marshallers.h"

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
			{ 0, 0, 0 }
		};
		etype = g_enum_register_static ("RemoteObjectError", values);
	}

	return etype;
}

static void
remote_object_init (RemoteObject *obj)
{
}

static void
remote_object_class_init (RemoteObjectClass *klass)
{
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

	for (l = context_list; l != NULL; l = l->next) {
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

	for (l = context_list; l != NULL; l = l->next) {
		if (((ContextInfo*)l->data)->id == id) {
			return ((ContextInfo*)l->data)->context;
		}
	}

	return NULL;
}

static int
switch_context (DBusGMethodInvocation *context)
{
	xchat_context *xcontext;
	char *sender;

	sender = dbus_g_method_get_sender (context);
	xcontext = g_hash_table_lookup (client_hash_table, sender);
	g_free (sender);
	
	if (!xchat_set_context (ph, xcontext)) {
		GError *error;

		error = g_error_new (REMOTE_OBJECT_ERROR,
				     REMOTE_OBJECT_ERROR_SET_CONTEXT,
				     _("switch to an invalide xchat context"));
		dbus_g_method_return_error (context, error);
		g_error_free (error);

		return FALSE;
	}

	return TRUE;
}

static gboolean
remote_object_command (RemoteObject *obj,
		       const char *command,
		       DBusGMethodInvocation *context)
{
	if (!switch_context (context)) {
		return FALSE;
	}
	xchat_command (ph, command);

	dbus_g_method_return (context);
	return TRUE;
}

static gboolean
remote_object_print (RemoteObject *obj,
		     const char *text,
		     DBusGMethodInvocation *context)
{
	if (!switch_context (context)) {
		return FALSE;
	}
	xchat_print (ph, text);

	dbus_g_method_return (context);
	return TRUE;
}

static gboolean
remote_object_find_context (RemoteObject *obj,
			    const char *server,
			    const char *channel,
			    guint *ret_id,
			    GError **error)
{
	xchat_context *xcontext = NULL;

	if (*server == '\0') {
		server = NULL;
	}
	if (*channel == '\0') {
		channel = NULL;
	}

	xcontext = xchat_find_context (ph, server, channel);
	*ret_id = context_list_find_id (xcontext);
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
			   DBusGMethodInvocation *context)
{
	xchat_context *xcontext;
	guint id;

	if (!switch_context (context)) {
		return FALSE;
	}
	xcontext = xchat_get_context (ph);
	id = context_list_find_id (xcontext);

	dbus_g_method_return (context, id);
	return TRUE;
}

static gboolean
remote_object_set_context (RemoteObject *obj,
			   guint id,
			   DBusGMethodInvocation *context)
{
	xchat_context *xcontext;
	char *sender;
	
	xcontext = context_list_find_context (id);
	if (xcontext == NULL) {
		GError *error;
		
		error = g_error_new (REMOTE_OBJECT_ERROR,
				     REMOTE_OBJECT_ERROR_FIND_ID,
				     _("xchat context ID not found"));
		dbus_g_method_return_error (context, error);
		g_error_free (error);

		return FALSE;
	}
	sender = dbus_g_method_get_sender (context);
	g_hash_table_insert (client_hash_table,
			     sender,
			     xcontext);
	
	dbus_g_method_return (context);
	return TRUE;
}

static gboolean
remote_object_get_info (RemoteObject *obj,
			const char *id,
			DBusGMethodInvocation *context)
{
	const char *ret_info;

	if (!switch_context (context)) {
		return FALSE;
	}
	ret_info = xchat_get_info (ph, id);
	if (ret_info == 0) {
		GError *error;
		
		error = g_error_new (REMOTE_OBJECT_ERROR,
				     REMOTE_OBJECT_ERROR_GET_INFO,
				     _("info \"%s\" does not exist"),
				     id);
		dbus_g_method_return_error (context, error);
		g_error_free (error);

		return FALSE;
	}
	
	dbus_g_method_return (context, ret_info);
	return TRUE;
}

static gboolean
remote_object_get_prefs (RemoteObject *obj,
			 const char *name,
			 DBusGMethodInvocation *context)
{
	const char *ret_str;
	int ret_int;
	int ret_type;

	if (!switch_context (context)) {
		return FALSE;
	}
	ret_type = xchat_get_prefs (ph, name, &ret_str, &ret_int);
	if (ret_type == 0) {
		GError *error;
		
		error = g_error_new (REMOTE_OBJECT_ERROR,
				     REMOTE_OBJECT_ERROR_GET_PREFS,
				     _("preference \"%s\" does not exist"),
				     name);
		dbus_g_method_return_error (context, error);
		g_error_free (error);

		return FALSE;
	}

	dbus_g_method_return (context, ret_type, ret_str, ret_int);
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

static gboolean
check_hook_info (HookInfo *info)
{
	if (g_hash_table_lookup (client_hash_table, info->client) == NULL) {
		g_hash_table_remove (hook_hash_table, &info->id);
		return FALSE;
	}

	g_hash_table_insert (client_hash_table,
			     g_strdup (info->client),
			     xchat_get_context (ph));

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

	if (!check_hook_info (info)) {
		return XCHAT_EAT_NONE;
	}

	arg1 = build_list (word + 1);
	arg2 = build_list (word_eol + 1);
	g_signal_emit (remote_object,
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

	if (!check_hook_info (info)) {
		return XCHAT_EAT_NONE;
	}
  
	arg1 = build_list (word + 1);
	arg2 = build_list (word_eol + 1);
	g_signal_emit (remote_object,
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

	if (!check_hook_info (info)) {
		return XCHAT_EAT_NONE;
	}

	arg1 = build_list (word + 1);
	g_signal_emit (remote_object,
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
			    DBusGMethodInvocation *context)
{
	HookInfo *info;

	info = g_new0 (HookInfo, 1);
	info->client = dbus_g_method_get_sender (context);
	info->return_value = return_value;
	info->id = last_hook_id++;
	info->hook = xchat_hook_command (ph,
					 name,
					 priority,
					 command_hook_cb,
					 help_text,
					 info);
	g_hash_table_insert (hook_hash_table, &info->id, info);

	dbus_g_method_return (context, info->id);
	return TRUE;
}

static gboolean
remote_object_hook_server (RemoteObject *obj,
			   const char *name,
			   int priority,
			   int return_value,
			   DBusGMethodInvocation *context)
{
	HookInfo *info;

	info = g_new0 (HookInfo, 1);
	info->client = dbus_g_method_get_sender (context);
	info->return_value = return_value;
	info->id = last_hook_id++;
	info->hook = xchat_hook_server (ph,
					name,
					priority,
					server_hook_cb,
					info);
	g_hash_table_insert (hook_hash_table, &info->id, info);

	dbus_g_method_return (context, info->id);
	return TRUE;
}

static gboolean
remote_object_hook_print (RemoteObject *obj,
			  const char *name,
			  int priority,
			  int return_value,
			  DBusGMethodInvocation *context)
{
	HookInfo *info;

	info = g_new0 (HookInfo, 1);
	info->client = dbus_g_method_get_sender (context);
	info->return_value = return_value;
	info->id = last_hook_id++;
	info->hook = xchat_hook_print (ph,
				       name,
				       priority,
				       print_hook_cb,
				       info);
	g_hash_table_insert (hook_hash_table, &info->id, info);

	dbus_g_method_return (context, info->id);
	return TRUE;
}

static gboolean
remote_object_unhook (RemoteObject *obj,
		      guint id,
		      GError **error)
{
	if (!g_hash_table_remove (hook_hash_table, &id)) {
		g_set_error (error,
			     REMOTE_OBJECT_ERROR,
			     REMOTE_OBJECT_ERROR_FIND_ID,
			     _("xchat context ID not found"));

		return FALSE;
	}
	return TRUE;
}

static void
hook_info_destroy (gpointer data)
{
	HookInfo *info = (HookInfo*)data;

	if (info == NULL) {
		return;
	}
	xchat_unhook (ph, info->hook);
	g_free (info->client);
	g_free (info);
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
		g_hash_table_remove (client_hash_table, name);
	} else if (*old_owner == '\0') {
		/* this name has been added */
		g_hash_table_insert (client_hash_table,
				     g_strdup (name),
				     xchat_get_context (ph));
	}
}

static gboolean
init_dbus (void)
{
	DBusGConnection *connection;
	DBusGProxy *proxy;
	guint request_name_result;
	GError *error = NULL;

	dbus_g_object_type_install_info (REMOTE_TYPE_OBJECT,
					 &dbus_glib_remote_object_object_info);

	dbus_g_error_domain_register (REMOTE_OBJECT_ERROR,
				      NULL,
				      REMOTE_TYPE_ERROR);

	connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
	if (connection == NULL) {
		xchat_printf (ph, _("Couldn't connect to session bus : %s\n"),
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

	remote_object = g_object_new (REMOTE_TYPE_OBJECT, NULL);
	dbus_g_connection_register_g_object (connection,
					     DBUS_OBJECT,
					     G_OBJECT (remote_object));

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
	context_list = g_list_prepend (context_list, info);

	return XCHAT_EAT_NONE;
}

static int
close_context_cb (char *word[],
		  void *userdata)
{
	GList *l;
	xchat_context *xcontext = xchat_get_context (ph);

	for (l = context_list; l != NULL; l = l->next) {
		if (((ContextInfo*)l->data)->context == xcontext) {
			g_free (l->data);
			context_list = g_list_delete_link (context_list, l);
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

		hook_hash_table =
			g_hash_table_new_full (g_int_hash,
					       g_int_equal,
					       NULL,
					       hook_info_destroy);
		client_hash_table =
			g_hash_table_new_full (g_str_hash,
					       g_str_equal,
					       g_free,
					       NULL);

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
