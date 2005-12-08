/* example - program to demonstrate some D-BUS stuffs.
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
#include <unistd.h>
#include "marshallers.h"

#define DBUS_SERVICE "org.xchat.service"
#define DBUS_OBJECT "/org/xchat/RemoteObject"
#define DBUS_INTERFACE "org.xchat.interface"

int command_id, server_id;

static void
write_error (gchar *message, GError *err)
{
  if (!err)
    return;
  g_printerr ("%s : %s\n", message, err->message);
  g_error_free (err);
}

static void
test_command_cb (DBusGProxy *proxy, gchar *word[], gchar *word_eol[], guint id, gpointer user_data)
{
  GError *error = NULL;
  gint i = 0;
  
  g_print ("signal received: id=%d\n", id);
  while ((word[i] && word_eol[i]))
  {
    g_print ("word=%s ; word_eol=%s\n", word[i], word_eol[i]);
    i++;
  }
  
  if (id == command_id)
  {
    if (!dbus_g_proxy_call (proxy, "unhook", &error,
                            G_TYPE_INT, id,
                            G_TYPE_INVALID, G_TYPE_INVALID))
      write_error ("Failed to complete unhook", error);
    /* Now if you write again "/test blah" in the xchat window you'll
     * get a "Unknown command" error message */
  }
}

int
main (int argc, char **argv)
{
  DBusGConnection *bus = NULL;
  GError *error = NULL;
  DBusGProxy *remote_object = NULL;
  GMainLoop *mainloop;

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

  if (!dbus_g_proxy_call (remote_object, "HookCommand", &error,
                          G_TYPE_STRING, "test",
                          G_TYPE_INT, 0,
                          G_TYPE_STRING, "Simple D-BUS example",
                          G_TYPE_INT, 1, G_TYPE_INVALID,
                          G_TYPE_INT, &command_id, G_TYPE_INVALID))
    write_error ("Failed to complete HookCommand", error);
  g_print ("Command hook id=%d\n", command_id);

  if (!dbus_g_proxy_call (remote_object, "HookServer", &error,
                          G_TYPE_STRING, "RAW LINE",
                          G_TYPE_INT, 0,
                          G_TYPE_INT, 0, G_TYPE_INVALID,
                          G_TYPE_INT, &server_id, G_TYPE_INVALID))
    write_error ("Failed to complete HookServer", error);
  g_print ("Server hook id=%d\n", server_id);

  dbus_g_object_register_marshaller (g_cclosure_user_marshal_VOID__POINTER_POINTER_INT,
				     G_TYPE_NONE,
				     G_TYPE_STRV, G_TYPE_STRV, G_TYPE_INT,
				     G_TYPE_INVALID);
  dbus_g_proxy_add_signal (remote_object, "CommandSignal", G_TYPE_STRV, G_TYPE_STRV, G_TYPE_INT, G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (remote_object, "CommandSignal", G_CALLBACK (test_command_cb),
			       NULL, NULL);
  dbus_g_proxy_add_signal (remote_object, "ServerSignal", G_TYPE_STRV, G_TYPE_STRV, G_TYPE_INT, G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (remote_object, "ServerSignal", G_CALLBACK (test_command_cb),
			       NULL, NULL);

  /* Now you can write on the xchat windows: "/test arg1 arg2" */
  mainloop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (mainloop);
  
  g_object_unref (G_OBJECT (remote_object));
  return 0;
}
