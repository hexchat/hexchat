/* xchat-remote - program for remote access xchat using DBUS
 * Copyright (C) 2005 Claessens Xavier
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Claessens Xavier
 * x_claessens@skynet.be
 */

#include "../../config.h"
#include <dbus/dbus-glib.h>
#include <unistd.h>

#define DBUS_SERVICE "org.xchat.service"
#define DBUS_OBJECT "/org/xchat/RemoteObject"
#define DBUS_INTERFACE "org.xchat.interface"

static DBusGProxy *remote_object = NULL;
static gchar *opt_open_url = NULL;
static gchar *opt_command = NULL;
static gchar *opt_print = NULL;
static gchar *opt_channel = NULL;
static gchar *opt_server = NULL;
static gchar *opt_info = NULL;
static gchar *opt_prefs = NULL;

static GOptionEntry entries[] = {
  {"url", 'u', 0, G_OPTION_ARG_STRING, &opt_open_url, "Open an irc:// url", "irc://server:port/channel"},
  {"command", 'c', 0, G_OPTION_ARG_STRING, &opt_command, "Execute a xchat command", "\"Command to execute\""},
  {"print", 'p', 0, G_OPTION_ARG_STRING, &opt_print, "Prints some text to the current tab/window", "\"Text to print\""},
  {"channel", 'h', 0, G_OPTION_ARG_STRING, &opt_channel, "Change the context to the channel", "channel"},
  {"server", 's', 0, G_OPTION_ARG_STRING, &opt_server, "Change the context to the server", "server"},
  {"info", 'i', 0, G_OPTION_ARG_STRING, &opt_info, "Get some informations from xchat", "id"},
  {"prefs", 'r', 0, G_OPTION_ARG_STRING, &opt_prefs, "Get settings from xchat", "name"},
};

static void
write_error (gchar *message, GError *err)
{
  if (!err)
    return;
  g_printerr ("%s : %s\n", message, err->message);
  g_error_free (err);
}

int
main (int argc, char **argv)
{
  DBusGConnection *bus = NULL;
  GError *error = NULL;
  GOptionContext *context = NULL;

  context = g_option_context_new ("");
  g_option_context_add_main_entries (context, entries, PACKAGE);
  g_option_context_parse (context, &argc, &argv, &error);

  if (error)
  {
    g_printerr ("xchat-remote: %s\n"
                "Try `xchat-remote --help' for more information\n",
                error->message);
    return 1;
  }

  g_type_init ();

  bus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (!bus)
  {
    write_error ("Couldn't connect to session bus", error);
    return 1;
  }
  
  remote_object = dbus_g_proxy_new_for_name (bus,
                                             DBUS_SERVICE,
                                             DBUS_OBJECT,
                                             DBUS_INTERFACE);

  if (opt_open_url)
  {
    gchar *command;
    command = g_strdup_printf ("newserver %s", opt_open_url);
    if (!dbus_g_proxy_call (remote_object, "command", &error,
                            G_TYPE_STRING, command, G_TYPE_INVALID,
                            G_TYPE_INVALID))
    {
      if (error->domain == DBUS_GERROR && error->code == DBUS_GERROR_SERVICE_UNKNOWN)
      {
        /* No xchat running... */
        if (fork() == 0)
          execlp (PACKAGE, PACKAGE, "-a", opt_open_url);
      } else
        write_error ("Failed to complete command", error);
    }
    g_free (command);
  }
  if (opt_channel || opt_server)
    if (!dbus_g_proxy_call (remote_object, "SetContext", &error,
                            G_TYPE_STRING, opt_server,
                            G_TYPE_STRING, opt_channel, G_TYPE_INVALID,
                            G_TYPE_INVALID))
      write_error ("Failed to complete SetContext", error);
  if (opt_command)
    if (!dbus_g_proxy_call (remote_object, "command", &error,
                            G_TYPE_STRING, opt_command, G_TYPE_INVALID,
                            G_TYPE_INVALID))
      write_error ("Failed to complete command", error);
  if (opt_print)
    if (!dbus_g_proxy_call (remote_object, "print", &error,
                            G_TYPE_STRING, opt_print, G_TYPE_INVALID,
                            G_TYPE_INVALID))
      write_error ("Failed to complete print", error);
  if (opt_info)
  {
    gchar *info;
    if (!dbus_g_proxy_call (remote_object, "GetInfo", &error,
                            G_TYPE_STRING, opt_info, G_TYPE_INVALID,
                            G_TYPE_STRING, &info, G_TYPE_INVALID))
      write_error ("Failed to complete GetInfo", error);
    else
      g_printf ("%s = %s\n", opt_info, info);
  }
  if (opt_prefs)
  {
    gchar *str;
    int type, i;
    if (!dbus_g_proxy_call (remote_object, "GetPrefs", &error,
                            G_TYPE_STRING, opt_prefs, G_TYPE_INVALID,
                            G_TYPE_INT, &type,
                            G_TYPE_STRING, &str,
                            G_TYPE_INT, &i, G_TYPE_INVALID))
      write_error ("Failed to complete GetPrefs", error);
    else
    {
      if (type == 0)
        g_printf ("%s doesn't exist\n", opt_prefs);
      else if (type == 1)
        g_printf ("%s = %s\n", opt_prefs, str);
      else if (type == 2)
        g_printf ("%s = %d\n", opt_prefs, i);
      else
        g_printf ("%s = %s\n", opt_prefs, i ? "TRUE" : "FALSE");
    }
  }

  g_object_unref (G_OBJECT (remote_object));
  return 0;
}
