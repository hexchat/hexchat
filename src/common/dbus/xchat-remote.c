/* xchat-remote.c - program for remote access xchat using DBUS
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
#include <glib/gi18n.h>

#define DBUS_SERVICE "org.xchat.service"
#define DBUS_OBJECT "/org/xchat/RemoteObject"
#define DBUS_INTERFACE "org.xchat.interface"

static gchar *opt_open_url = NULL;
static gchar *opt_command = NULL;
static gchar *opt_print = NULL;
static gchar *opt_channel = NULL;
static gchar *opt_server = NULL;
static gchar *opt_info = NULL;
static gchar *opt_prefs = NULL;

static GOptionEntry entries[] = {
	{"url",     'u', 0, G_OPTION_ARG_STRING, &opt_open_url,
			    N_("Open an irc:// url"),
			    N_("irc://server:port/channel")},
	{"command", 'c', 0, G_OPTION_ARG_STRING, &opt_command,
			    N_("Execute a xchat command"),
			    N_("\"Command to execute\"")},
	{"print",   'p', 0, G_OPTION_ARG_STRING, &opt_print,
			    N_("Prints some text to the current tab/window"),
			    N_("\"Text to print\"")},
	{"channel", 'h', 0, G_OPTION_ARG_STRING, &opt_channel,
			    N_("Change the context to the channel"),
			    N_("channel")},
	{"server",  's', 0, G_OPTION_ARG_STRING, &opt_server,
			    N_("Change the context to the server"),
			    N_("server")},
	{"info",    'i', 0, G_OPTION_ARG_STRING, &opt_info,
			    N_("Get some informations from xchat"),
			    N_("id")},
	{"prefs",   'r', 0, G_OPTION_ARG_STRING, &opt_prefs,
			    N_("Get settings from xchat"),
			    N_("name")},
	{ NULL }
};

static void
write_error (char *message,
	     GError *error)
{
	if (error == NULL) {
		return;
	}
	g_printerr ("%s: %s\n", message, error->message);
	g_error_free (error);
}

int
main (int argc, char **argv)
{
	DBusGConnection *connection;
	DBusGProxy *remote_object;
	GOptionContext *context;
	GError *error = NULL;
  
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
	g_option_context_parse (context, &argc, &argv, &error);

	if (error != NULL) {
		g_printerr (_("%s: %s\nTry `%s --help' for more information\n"),
			    argv[0],
			    error->message,
			    argv[0]);

		return EXIT_FAILURE;
	}

	g_type_init ();

	connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
	if (connection == NULL) {
		write_error (_("Couldn't connect to session bus"), error);
		return EXIT_FAILURE;
	}
  
	remote_object = dbus_g_proxy_new_for_name (connection,
						   DBUS_SERVICE,
						   DBUS_OBJECT,
						   DBUS_INTERFACE);

	if (opt_open_url != NULL) {
		char *command;
		command = g_strdup_printf ("url %s", opt_open_url);
		if (!dbus_g_proxy_call (remote_object, "Command",
					&error,
					G_TYPE_STRING, command,
					G_TYPE_INVALID,G_TYPE_INVALID)) {
			if (error->domain == DBUS_GERROR &&
			    error->code == DBUS_GERROR_SERVICE_UNKNOWN) {
				/* No xchat running... */

				g_free (command);
				g_clear_error (&error);

				command = g_strdup_printf (
					PREFIX"/bin/"PACKAGE_TARNAME
					" -a --url=%s", opt_open_url);
				if (!g_spawn_command_line_async (command,
								 &error)) {
					write_error (_("Failed to start "
						     PACKAGE_NAME), error);
					return EXIT_FAILURE;
				}
			} else {
				write_error (_("Failed to complete command"),
					     error);
				return EXIT_FAILURE;
			}
		}
		g_free (command);
	}

	if (opt_channel != NULL || opt_server != NULL) {
		guint id;
		if (!dbus_g_proxy_call (remote_object, "FindContext",
					&error,
					G_TYPE_STRING, opt_server,
					G_TYPE_STRING, opt_channel,
					G_TYPE_INVALID,
					G_TYPE_UINT, &id,
					G_TYPE_INVALID)) {
			write_error (_("Failed to complete FindContext"), error);
			return EXIT_FAILURE;
		}
		if (!dbus_g_proxy_call (remote_object, "SetContext",
					&error,
					G_TYPE_UINT, id,
					G_TYPE_INVALID, G_TYPE_INVALID)) {
			write_error (_("Failed to complete SetContext"), error);
			return EXIT_FAILURE;
		}
	}

	if (opt_command != NULL) {
		if (!dbus_g_proxy_call (remote_object, "Command",
					&error,
					G_TYPE_STRING, opt_command,
					G_TYPE_INVALID, G_TYPE_INVALID)) {
			write_error (_("Failed to complete command"), error);
			return EXIT_FAILURE;
		}
	}

	if (opt_print != NULL) {
		if (!dbus_g_proxy_call (remote_object, "Print",
					&error,
					G_TYPE_STRING, opt_print,
					G_TYPE_INVALID, G_TYPE_INVALID)) {
			write_error (_("Failed to complete print"), error);
			return EXIT_FAILURE;
		}
	}

	if (opt_info != NULL) {
		char *info;
		if (!dbus_g_proxy_call (remote_object, "GetInfo",
					&error,
					G_TYPE_STRING, opt_info,
					G_TYPE_INVALID,
					G_TYPE_STRING, &info,
					G_TYPE_INVALID)) {
			write_error (_("Failed to complete GetInfo"), error);
			return EXIT_FAILURE;
		} else {
			g_print ("%s = %s\n", opt_info, info);
		}
	}

	if (opt_prefs != NULL) {
		char *str;
		int type;
		int i;
		if (!dbus_g_proxy_call (remote_object, "GetPrefs",
					&error,
					G_TYPE_STRING, opt_prefs,
					G_TYPE_INVALID,
					G_TYPE_INT, &type,
					G_TYPE_STRING, &str,
					G_TYPE_INT, &i,
					G_TYPE_INVALID)) {
			write_error (_("Failed to complete GetPrefs"), error);
			return EXIT_FAILURE;
		} else {
			switch (type) {
			case 0:
				g_print (_("%s doesn't exist\n"), opt_prefs);
				break;
			case 1:
				g_print ("%s = %s\n", opt_prefs, str);
				break;
			case 2:
				g_print ("%s = %d\n", opt_prefs, i);
				break;
			default:
				g_print ("%s = %s\n", opt_prefs,
					 i ? "TRUE" : "FALSE");
				break;
			}
		}
	}

	g_object_unref (G_OBJECT (remote_object));

	return EXIT_SUCCESS;
}
