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

#include <dbus/dbus-glib.h>

#define DBUS_SERVICE "org.xchat.service"
#define DBUS_OBJECT "/org/xchat/RemoteObject"
#define DBUS_INTERFACE "org.xchat.interface"

static DBusGProxy *remote_object = NULL;
static gchar *opt_open_url = NULL;
static gchar *opt_command = NULL;
static gchar *opt_print = NULL;
static gchar *opt_channel = NULL;
static gchar *opt_server = NULL;

static GOptionEntry entries[] = {
  {"url", 'u', 0, G_OPTION_ARG_STRING, &opt_open_url, "Open an irc:// url", "irc://server:port/channel"},
  {"command", 'c', 0, G_OPTION_ARG_STRING, &opt_command, "Execute a xchat command", "\"Command to execute\""},
  {"print", 'p', 0, G_OPTION_ARG_STRING, &opt_print, "Prints some text to the current tab/window", "\"Text to print\""},
  {"channel", 0, 0, G_OPTION_ARG_STRING, &opt_channel, "Change the context to the channel", "channel"},
  {"server", 0, 0, G_OPTION_ARG_STRING, &opt_server, "Change the context to the server", "server"},
};

static void
send_command (gchar *command)
{
  GError *error = NULL;
  if (!dbus_g_proxy_call (remote_object, "command", &error,
                          G_TYPE_STRING, command, G_TYPE_INVALID,
                          G_TYPE_INVALID))
  {
    g_printerr ("Failed to complete command : %s\n", error->message);
    g_error_free (error);
  }
}

int
main (int argc, char **argv)
{
  DBusGConnection *bus = NULL;
  GError *error = NULL;
  GOptionContext *context = NULL;

  context = g_option_context_new ("");
  /* FIXME - set translation domain */
  g_option_context_add_main_entries (context, entries, NULL);
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
    g_printerr ("Couldn't connect to session bus : %s\n", error->message);
    return 1;
  }
  
  remote_object = dbus_g_proxy_new_for_name (bus,
                                             DBUS_SERVICE,
                                             DBUS_OBJECT,
                                             DBUS_INTERFACE);

  if (opt_command)
    send_command (opt_command);
  if (opt_open_url)
  {
    char *command;
    command = g_strdup_printf ("newserver %s", opt_open_url);
    send_command (command);
    g_free (command);
  }
  if (opt_print)
  {
    if (!dbus_g_proxy_call (remote_object, "print", &error,
                            G_TYPE_STRING, opt_print, G_TYPE_INVALID,
                            G_TYPE_INVALID))
    {
      g_printerr ("Failed to complete print : %s\n", error->message);
      g_error_free (error);
    }
  }
  if (opt_channel || opt_server)
  {
    if (!dbus_g_proxy_call (remote_object, "context", &error,
                            G_TYPE_STRING, opt_server,
                            G_TYPE_STRING, opt_channel, G_TYPE_INVALID,
                            G_TYPE_INVALID))
    {
      g_printerr ("Failed to complete context : %s\n", error->message);
      g_error_free (error);
    }
  }

  g_object_unref (G_OBJECT (remote_object));
  return 0;
}
