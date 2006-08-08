/* example.c - program to demonstrate some D-BUS stuffs.
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Claessens Xavier
 * xclaesse@gmail.com
 */

#include <config.h>
#include <dbus/dbus-glib.h>
#include <stdlib.h>
#include "marshallers.h"

#define DBUS_SERVICE "org.xchat.service"
#define DBUS_REMOTE "/org/xchat/Remote"
#define DBUS_REMOTE_CONNECTION_INTERFACE "org.xchat.connection"
#define DBUS_REMOTE_PLUGIN_INTERFACE "org.xchat.plugin"

guint command_id;
guint server_id;

static void
write_error (char *message,
	     GError **error)
{
	if (error == NULL || *error == NULL) {
		return;
	}
	g_printerr ("%s: %s\n", message, (*error)->message);
	g_clear_error (error);
}

static void
test_server_cb (DBusGProxy *proxy,
		char *word[],
		char *word_eol[],
		guint id,
		gpointer user_data)
{
	if (id == server_id) {
		g_print ("message: %s\n", word_eol[0]);
	}
}

static void
test_command_cb (DBusGProxy *proxy,
		 char *word[],
		 char *word_eol[],
		 guint id,
		 gpointer user_data)
{
	GError *error = NULL;

	if (id == command_id) {
		if (!dbus_g_proxy_call (proxy, "Unhook",
					&error,
					G_TYPE_UINT, id,
					G_TYPE_INVALID, G_TYPE_INVALID)) {
			write_error ("Failed to complete unhook", &error);
		}
		/* Now if you write "/test blah" again in the xchat window
		 * you'll get a "Unknown command" error message */
		g_print ("test command received: %s\n", word_eol[1]);
		if (!dbus_g_proxy_call (proxy, "Print",
					&error,
					G_TYPE_STRING, "test command succeed",
					G_TYPE_INVALID,
					G_TYPE_INVALID)) {
			write_error ("Failed to complete Print", &error);
		}
	}
}

int
main (int argc, char **argv)
{
	DBusGConnection *connection;
	DBusGProxy *remote_object;
	GMainLoop *mainloop;
	gchar *path;
	GError *error = NULL;

	g_type_init ();

	connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
	if (connection == NULL) {
		write_error ("Couldn't connect to session bus", &error);
		return EXIT_FAILURE;
	}
  
	remote_object = dbus_g_proxy_new_for_name (connection,
						   DBUS_SERVICE,
						   DBUS_REMOTE,
						   DBUS_REMOTE_CONNECTION_INTERFACE);
	if (!dbus_g_proxy_call (remote_object, "Connect",
				&error,
				G_TYPE_INVALID,
				G_TYPE_STRING, &path, G_TYPE_INVALID)) {
		write_error ("Failed to complete Connect", &error);
		return EXIT_FAILURE;
	}
	g_object_unref (remote_object);

	remote_object = dbus_g_proxy_new_for_name (connection,
						   DBUS_SERVICE,
						   path,
						   DBUS_REMOTE_PLUGIN_INTERFACE);
	g_free (path);

	if (!dbus_g_proxy_call (remote_object, "HookCommand",
				&error,
				G_TYPE_STRING, "test",
				G_TYPE_INT, 0,
				G_TYPE_STRING, "Simple D-BUS example",
				G_TYPE_INT, 1, G_TYPE_INVALID,
				G_TYPE_UINT, &command_id, G_TYPE_INVALID)) {
		write_error ("Failed to complete HookCommand", &error);
		return EXIT_FAILURE;
	}
	g_print ("Command hook id=%d\n", command_id);

	if (!dbus_g_proxy_call (remote_object, "HookServer",
				&error,
				G_TYPE_STRING, "RAW LINE",
				G_TYPE_INT, 0,
				G_TYPE_INT, 0, G_TYPE_INVALID,
				G_TYPE_UINT, &server_id, G_TYPE_INVALID)) {
		write_error ("Failed to complete HookServer", &error);
		return EXIT_FAILURE;
	}
	g_print ("Server hook id=%d\n", server_id);

	dbus_g_object_register_marshaller (
		g_cclosure_user_marshal_VOID__POINTER_POINTER_UINT,
		G_TYPE_NONE,
		G_TYPE_STRV, G_TYPE_STRV, G_TYPE_UINT,
		G_TYPE_INVALID);

	dbus_g_proxy_add_signal (remote_object, "CommandSignal",
				 G_TYPE_STRV,
				 G_TYPE_STRV,
				 G_TYPE_UINT,
				 G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (remote_object, "CommandSignal",
				     G_CALLBACK (test_command_cb),
				     NULL, NULL);

	dbus_g_proxy_add_signal (remote_object, "ServerSignal",
				 G_TYPE_STRV,
				 G_TYPE_STRV,
				 G_TYPE_UINT,
				 G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (remote_object, "ServerSignal",
				     G_CALLBACK (test_server_cb),
				     NULL, NULL);

	/* Now you can write on the xchat windows: "/test arg1 arg2 ..." */
	mainloop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (mainloop);

	return EXIT_SUCCESS;
}
