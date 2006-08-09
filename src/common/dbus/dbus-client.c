/* dbus-client.c - XChat command-line options for D-Bus
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

#include <dbus/dbus-glib.h>
#include "dbus-client.h"
#include "../xchat.h"
#include "../xchatc.h"

#define DBUS_SERVICE "org.xchat.service"
#define DBUS_REMOTE "/org/xchat/Remote"
#define DBUS_REMOTE_INTERFACE "org.xchat.plugin"

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

void
xchat_remote (void)
/* TODO: dbus_g_connection_unref (connection) are commented because it makes
 * dbus to crash. Fixed in dbus >=0.70 ?!?
 * https://launchpad.net/distros/ubuntu/+source/dbus/+bug/54375
 */
{
	DBusGConnection *connection;
	DBusGProxy *dbus = NULL;
	DBusGProxy *remote_object = NULL;
	gboolean xchat_running;
	GError *error = NULL;

	/* GnomeVFS >=2.15 uses D-Bus and threads, so threads should be
	 * initialised before opening for the first time a D-Bus connection */
	if (!g_thread_supported ()) {
		g_thread_init (NULL);
	}
	dbus_g_thread_init ();

	/* if there is nothing to do, return now. */
	if (!arg_existing || !arg_url) {
		return;
	}

	arg_dont_autoconnect = TRUE;

	connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
	if (!connection) {
		write_error (_("Couldn't connect to session bus"), &error);
		return;
	}

	/* Checks if xchat is already running */
	dbus = dbus_g_proxy_new_for_name (connection,
					  DBUS_SERVICE_DBUS,
					  DBUS_PATH_DBUS,
					  DBUS_INTERFACE_DBUS);
	if (!dbus_g_proxy_call (dbus, "NameHasOwner", &error,
				G_TYPE_STRING, DBUS_SERVICE,
				G_TYPE_INVALID,
				G_TYPE_BOOLEAN, &xchat_running,
				G_TYPE_INVALID)) {
		write_error (_("Failed to complete NameHasOwner"), &error);
		xchat_running = FALSE;
	}
	g_object_unref (dbus);

	if (!xchat_running) {
		//dbus_g_connection_unref (connection);
		return;
	}

	remote_object = dbus_g_proxy_new_for_name (connection,
						   DBUS_SERVICE,
						   DBUS_REMOTE,
						   DBUS_REMOTE_INTERFACE);

	if (arg_url) {
		char *command = g_strdup_printf ("url %s", arg_url);
		if (!dbus_g_proxy_call (remote_object, "Command",
					&error,
					G_TYPE_STRING, command,
					G_TYPE_INVALID,G_TYPE_INVALID)) {
			write_error (_("Failed to complete Command"), &error);
		}
		g_free (command);
	}

	exit (0);
}
