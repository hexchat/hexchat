/* HexChat
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
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
*/

#include "config.h"

#include <string.h>
#include <gio/gio.h>
#include "hexchat-plugin.h"

#define _(x) hexchat_gettext(ph,x)

static hexchat_plugin *ph;
static GSocketService *service;
static GHashTable *responses;

typedef struct ident_info
{
	GSocketConnection *conn;
	gchar *username;
	gchar read_buf[16];
} ident_info;

static int
identd_cleanup_response_cb (gpointer userdata)
{
	g_return_val_if_fail (responses != NULL, 0);

	g_hash_table_remove (responses, userdata);

	return 0;
}

static int
identd_command_cb (char *word[], char *word_eol[], void *userdata)
{
	g_return_val_if_fail (responses != NULL, HEXCHAT_EAT_ALL);

	if (service == NULL) /* If we are not running plugins can handle it */
		return HEXCHAT_EAT_HEXCHAT;

	if (word[2] && *word[2] && word[3] && *word[3])
	{
		guint64 port = g_ascii_strtoull (word[2], NULL, 0);

		if (port && port <= G_MAXUINT16)
		{
			g_hash_table_insert (responses, GINT_TO_POINTER (port), g_strdup (word[3]));
			/* Automatically remove entry after 30 seconds */
			hexchat_hook_timer (ph, 30000, identd_cleanup_response_cb, GINT_TO_POINTER (port));
		}
	}
	else
	{
		hexchat_command (ph, "HELP IDENTD");
	}

	return HEXCHAT_EAT_ALL;
}

static void
identd_write_ready (GOutputStream *stream, GAsyncResult *res, ident_info *info)
{
	g_output_stream_write_finish (stream, res, NULL);

	g_free (info->username);
	g_object_unref (info->conn);
	g_free (info);
}

static void
identd_read_ready (GInputStream *in_stream, GAsyncResult *res, ident_info *info)
{
	GSocketAddress *sok_addr;
	GOutputStream *out_stream;
	guint64 local, remote;
	gchar buf[512], *p;

	if (g_input_stream_read_finish (in_stream, res, NULL))
	{
		local = g_ascii_strtoull (info->read_buf, NULL, 0);
		p = strchr (info->read_buf, ',');
		if (!p)
			goto cleanup;

		remote = g_ascii_strtoull (p + 1, NULL, 0);

		if (!local || !remote || local > G_MAXUINT16 || remote > G_MAXUINT16)
			goto cleanup;

		info->username = g_strdup (g_hash_table_lookup (responses, GINT_TO_POINTER (local)));
		if (!info->username)
			goto cleanup;
		g_hash_table_remove (responses, GINT_TO_POINTER (local));

		if ((sok_addr = g_socket_connection_get_remote_address (info->conn, NULL)))
		{
			GInetAddress *inet_addr;
			gchar *addr;

			inet_addr = g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (sok_addr));
			addr = g_inet_address_to_string (inet_addr);

			hexchat_printf (ph, _("*\tServicing ident request from %s as %s"), addr, info->username);

			g_object_unref (sok_addr);
			g_object_unref (inet_addr);
			g_free (addr);
		}

		g_snprintf (buf, sizeof (buf), "%"G_GUINT16_FORMAT", %"G_GUINT16_FORMAT" : USERID : UNIX : %s\r\n", (guint16)local, (guint16)remote, info->username);
		out_stream = g_io_stream_get_output_stream (G_IO_STREAM (info->conn));
		g_output_stream_write_async (out_stream, buf, strlen (buf), G_PRIORITY_DEFAULT,
									NULL, (GAsyncReadyCallback)identd_write_ready, info);
	}

	return;

cleanup:
	g_object_unref (info->conn);
	g_free (info);
}

static gboolean
identd_incoming_cb (GSocketService *service, GSocketConnection *conn,
					GObject *source, gpointer userdata)
{
	GInputStream *stream;
	ident_info *info;

	info = g_new0 (ident_info, 1);

	info->conn = conn;
	g_object_ref (conn);

	stream = g_io_stream_get_input_stream (G_IO_STREAM (conn));
	g_input_stream_read_async (stream, info->read_buf, sizeof (info->read_buf), G_PRIORITY_DEFAULT,
							NULL, (GAsyncReadyCallback)identd_read_ready, info);

	return TRUE;
}

static void
identd_start_server (void)
{
	GError *error = NULL;
	int enabled, port = 113;

	if (hexchat_get_prefs (ph, "identd", NULL, &enabled) == 3)
	{
		if (!enabled)
			return;
	}
	if (hexchat_get_prefs (ph, "identd_port", NULL, &port) == 2 && (port <= 0 || port > G_MAXUINT16))
	{
		port = 113;
	}

	service = g_socket_service_new ();

	g_socket_listener_add_inet_port (G_SOCKET_LISTENER (service), port, NULL, &error);
	if (error)
	{
		hexchat_printf (ph, _("*\tError starting identd server: %s"), error->message);

		g_object_unref (service);
		service = NULL;
		return;
	}
	/*hexchat_printf (ph, "*\tIdentd listening on port: %d", port); */

	g_signal_connect (G_OBJECT (service), "incoming", G_CALLBACK(identd_incoming_cb), NULL);
	g_socket_service_start (service);
}

int
identd_plugin_init (hexchat_plugin *plugin_handle, char **plugin_name,
					char **plugin_desc, char **plugin_version, char *arg)
{
	ph = plugin_handle;
	*plugin_name = "";
	*plugin_desc = "";
	*plugin_version = "";


	responses = g_hash_table_new_full (NULL, NULL, NULL, g_free);
	hexchat_hook_command (ph, "IDENTD", HEXCHAT_PRI_NORM, identd_command_cb,
						_("IDENTD <port> <username>"), NULL);

	identd_start_server ();

	return 1; /* This must always succeed for /identd to work */
}

int
identd_plugin_deinit (void)
{
	if (service)
	{
		g_socket_service_stop (service);
		g_object_unref (service);
	}

	g_hash_table_destroy (responses);

	return 1;
}
