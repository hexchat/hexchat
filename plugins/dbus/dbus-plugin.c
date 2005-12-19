/* dbus-plugin - xchat plugin for remote access using DBUS
 * Copyright (C) 2005 Claessens Xavier
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Claessens Xavier
 * xclaesse@gmail.com
 */

#include <config.h>
#include <dbus/dbus-glib.h>
#include "xchat-plugin.h"
#include <glib/gi18n.h>

#define PNAME _("xchat remote access")
#define PDESC _("plugin for remote access using DBUS")
#define PVERSION "0.6"

#define DBUS_SERVICE "org.xchat.service"
#define DBUS_OBJECT "/org/xchat/RemoteObject"

void xchat_plugin_get_info(char **name, char **desc, char **version, void **reserved);
int  xchat_plugin_deinit(void);
int  xchat_plugin_init(xchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg);

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
  xchat_hook *hook;
} HookInfo;

static xchat_plugin *ph;
static RemoteObject *remote_object;
static GHashTable *hook_hash_table = NULL;
static guint num_id = 0;
static guint signals[LAST_SIGNAL] = { 0 };

#define REMOTE_TYPE_OBJECT              (remote_object_get_type ())
#define REMOTE_OBJECT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), REMOTE_TYPE_OBJECT, RemoteObject))
#define REMOTE_OBJECT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), REMOTE_TYPE_OBJECT, RemoteObjectClass))
#define REMOTE_IS_OBJECT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), REMOTE_TYPE_OBJECT))
#define REMOTE_IS_OBJECT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), REMOTE_TYPE_OBJECT))
#define REMOTE_OBJECT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), REMOTE_TYPE_OBJECT, RemoteObjectClass))

G_DEFINE_TYPE(RemoteObject, remote_object, G_TYPE_OBJECT)

/* Available services */
static gboolean remote_object_command (RemoteObject *obj, const gchar *command, GError **error);
static gboolean remote_object_print (RemoteObject *obj, const gchar *text, GError **error);
static gboolean remote_object_set_context (RemoteObject *obj, const gchar *server, const gchar *channel, GError **error);
static gboolean remote_object_get_info (RemoteObject *obj, const gchar *id, gchar **ret_info, GError **error);
static gboolean remote_object_get_prefs (RemoteObject *obj, const gchar *name, int *ret_type, gchar **ret_str, int *ret_int, GError **error);
static gboolean remote_object_hook_command (RemoteObject *obj, const char *name, int pri, const char *help_text, int return_value, int *id, GError **error);
static gboolean remote_object_hook_server (RemoteObject *obj, const char *name, int pri, int return_value, int *id, GError **error);
static gboolean remote_object_hook_print (RemoteObject *obj, const char *name, int pri, int return_value, int *id, GError **error);
static gboolean remote_object_unhook (RemoteObject *obj, int id, GError **error);

#include "dbus-plugin-glue.h"
#include "marshallers.h"

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
                  g_cclosure_user_marshal_VOID__POINTER_POINTER_INT,
                  G_TYPE_NONE, 3, G_TYPE_STRV, G_TYPE_STRV, G_TYPE_INT);
  signals[COMMAND_SIGNAL] =
    g_signal_new ("command_signal",
		  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  g_cclosure_user_marshal_VOID__POINTER_POINTER_INT,
                  G_TYPE_NONE, 3, G_TYPE_STRV, G_TYPE_STRV, G_TYPE_INT);
  signals[PRINT_SIGNAL] =
    g_signal_new ("print_signal",
		  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  g_cclosure_user_marshal_VOID__POINTER_POINTER_INT,
                  G_TYPE_NONE, 2, G_TYPE_STRV, G_TYPE_INT);
}

/* Implementation of services */

static gboolean
remote_object_command (RemoteObject *obj, const gchar *command, GError **error)
{
  xchat_command (ph, command);
  return TRUE;
}

static gboolean
remote_object_print (RemoteObject *obj, const gchar *text, GError **error)
{
  xchat_print (ph, text);
  return TRUE;
}

static gboolean
remote_object_set_context (RemoteObject *obj, const gchar *server, const gchar *channel, GError **error)
{
  xchat_context *context = NULL;
  if (server[0] == '\0')
    server = NULL;
  if (channel[0] == '\0')
    channel = NULL;
  context = xchat_find_context (ph, server, channel);
  xchat_set_context (ph, context);
  return TRUE;
}

static gboolean
remote_object_get_info (RemoteObject *obj, const gchar *id, gchar **ret_info, GError **error)
{
  const gchar *info = NULL;
  info = xchat_get_info (ph, id);
  *ret_info = g_strdup (info);
  return TRUE;
}

static gboolean
remote_object_get_prefs (RemoteObject *obj, const gchar *name, int *ret_type, gchar **ret_str, int *ret_int, GError **error)
{
  const gchar *str = NULL;
  int i = 0;
  *ret_type = xchat_get_prefs (ph, name, &str, &i);
  *ret_str = g_strdup (str);
  *ret_int = i;
  return TRUE;
}

static char**
build_list (char *word[])
{
  guint i, num = 0;
  char **result;
  if (!word)
    return NULL;
  
  while (word[num] && word[num][0])
    num++;
  result = g_new0 (char*, num + 1);
  for (i = 0; i < num; i++)
    result[i] = g_strdup (word[i]);

  return result;
}

static void
free_list (char *word[])
{
  guint i = 0;
  if (!word)
    return;

  while (word[i])
  {
    g_free (word[i]);
    i++;
  }
  g_free (word);
}

static int
server_hook_cb (char *word[], char *word_eol[], void *userdata)
{
  HookInfo *info = (HookInfo*)userdata;
  char **arg1, **arg2;
  
  arg1 = build_list (word+1);
  arg2 = build_list (word_eol+1);
  g_signal_emit (remote_object, signals[SERVER_SIGNAL], 0, arg1, arg2, info->id);
  free_list (arg1);
  free_list (arg2);
  
  return info->return_value;
}

static int
command_hook_cb (char *word[], char *word_eol[], void *userdata)
{
  HookInfo *info = (HookInfo*)userdata;
  char **arg1, **arg2;
  
  arg1 = build_list (word+1);
  arg2 = build_list (word_eol+1);
  g_signal_emit (remote_object, signals[COMMAND_SIGNAL], 0, arg1, arg2, info->id);
  free_list (arg1);
  free_list (arg2);
  
  return info->return_value;
}

static int
print_hook_cb (char *word[], void *userdata)
{
  HookInfo *info = (HookInfo*)userdata;
  char **arg1;
  
  arg1 = build_list (word+1);
  g_signal_emit (remote_object, signals[PRINT_SIGNAL], 0, arg1, info->id);
  free_list (arg1);
  
  return info->return_value;
}

static gboolean
remote_object_hook_command (RemoteObject *obj, const char *name, int pri, const char *help_text, int return_value, int *id, GError **error)
{
  HookInfo *info;

  info = g_new0 (HookInfo, 1);
  info->return_value = return_value;
  info->id = num_id++;
  info->hook = xchat_hook_command (ph, name, pri, command_hook_cb, help_text, info);
  *id = info->id;
  g_hash_table_insert (hook_hash_table, &info->id, info);

  return TRUE;
}

static gboolean
remote_object_hook_server (RemoteObject *obj, const char *name, int pri, int return_value, int *id, GError **error)
{
  HookInfo *info;

  info = g_new0 (HookInfo, 1);
  info->return_value = return_value;
  info->id = num_id++;
  info->hook = xchat_hook_server (ph, name, pri, server_hook_cb, info);
  *id = info->id;
  g_hash_table_insert (hook_hash_table, &info->id, info);

  return TRUE;
}

static gboolean
remote_object_hook_print (RemoteObject *obj, const char *name, int pri, int return_value, int *id, GError **error)
{
  HookInfo *info;

  info = g_new0 (HookInfo, 1);
  info->return_value = return_value;
  info->id = num_id++;
  info->hook = xchat_hook_print (ph, name, pri, print_hook_cb, info);
  *id = info->id;
  g_hash_table_insert (hook_hash_table, &info->id, info);

  return TRUE;
}


static gboolean
remote_object_unhook (RemoteObject *obj, int id, GError **error)
{
  g_hash_table_remove (hook_hash_table, &id);
  return TRUE;
}

static void
hook_info_destroy (gpointer data)
{
  HookInfo *info = (HookInfo*)data;
  if (!info)
    return;
  xchat_unhook (ph, info->hook);
  g_free (info);
}

/* DBUS stuffs */

static gboolean
init_dbus(void)
{
  DBusGConnection *bus;
  DBusGProxy *bus_proxy;
  GError *error = NULL;
  guint request_name_result;

  dbus_g_object_type_install_info (REMOTE_TYPE_OBJECT, &dbus_glib_remote_object_object_info);

  bus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (!bus)
  {
    xchat_printf (ph, _("Couldn't connect to session bus : %s\n"), error->message);
    g_error_free (error);
    return FALSE;
  }

  bus_proxy = dbus_g_proxy_new_for_name (bus, "org.freedesktop.DBus",
                                         "/org/freedesktop/DBus",
                                         "org.freedesktop.DBus");

  if (!dbus_g_proxy_call (bus_proxy, "RequestName", &error,
                          G_TYPE_STRING, DBUS_SERVICE,
#ifdef DBUS_NAME_FLAG_PROHIBIT_REPLACEMENT
                          G_TYPE_UINT, DBUS_NAME_FLAG_PROHIBIT_REPLACEMENT,
#else	/* removed in dbus 0.60 */
                          G_TYPE_UINT, 0,
#endif
                          G_TYPE_INVALID,
                          G_TYPE_UINT, &request_name_result,
                          G_TYPE_INVALID))
  {
    xchat_printf (ph, _("Failed to acquire %s: %s\n"), DBUS_SERVICE, error->message);
    g_error_free (error);
    return FALSE;
  }

  remote_object = g_object_new (REMOTE_TYPE_OBJECT, NULL);
  dbus_g_connection_register_g_object (bus, DBUS_OBJECT, G_OBJECT (remote_object));
  return TRUE;
}

/* xchat_plugin stuffs */

void
xchat_plugin_get_info(char **name, char **desc, char **version, void **reserved)
{
  *name = PNAME;
  *desc = PDESC;
  *version = PVERSION;
}

int
xchat_plugin_deinit()
{
  /* TODO: Close all D-BUS stuff
   * This is impossible yet. D-BUS doesn't support unloading and cause xchat
   * to crash if we unload the plugin.
   */
  //g_hash_table_destroy (hook_hash_table);
  //xchat_printf (ph, _("%s unloaded successfully!\n"), PNAME);
  return 0;
}

int
xchat_plugin_init(xchat_plugin *plugin_handle,
                  char **plugin_name,
                  char **plugin_desc,
                  char **plugin_version,
                  char *arg)
{
  gboolean success;
  ph = plugin_handle;
  *plugin_name = PNAME;
  *plugin_desc = PDESC;
  *plugin_version = PVERSION;
  hook_hash_table = g_hash_table_new_full (g_int_hash, g_int_equal,
                                           NULL, hook_info_destroy);
  success = init_dbus();
  if (success)
    xchat_printf(ph, _("%s loaded successfully!\n"), PNAME);
  /* TODO: Close all D-BUS stuff if success == FALSE
   * This is impossible yet. D-BUS doesn't support unloading and cause xchat
   * to crash if we unload the plugin.
   */
  return TRUE; 
}
