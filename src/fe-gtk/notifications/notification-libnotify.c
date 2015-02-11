/* HexChat
 * Copyright (C) 2015 Patrick Griffis.
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
#include <glib.h>
#include <libnotify/notify.h>

static gboolean strip_markup = FALSE;

void
notification_backend_show (const char *title, const char *text)
{
	NotifyNotification *notification;

	if (strip_markup)
		text = g_markup_escape_text (text, -1);

	notification = notify_notification_new (title, text, "hexchat");
	notify_notification_set_hint (notification, "desktop-entry", g_variant_new_string ("hexchat"));

	notify_notification_show (notification, NULL);

	g_object_unref (notification);
	if (strip_markup)
		g_free ((char*)text);
}

int
notification_backend_init (void)
{
	GList* server_caps;

	if (!NOTIFY_CHECK_VERSION (0, 7, 0))
		return 0;

	if (!notify_init (PACKAGE_NAME))
		return 0;

	server_caps = notify_get_server_caps ();
	if (g_list_find_custom (server_caps, "body-markup", (GCompareFunc)g_strcmp0))
		strip_markup = TRUE;
	g_list_free_full (server_caps, g_free);

	return 1;
}

void
notification_backend_deinit (void)
{
	notify_uninit ();
}

int
notification_backend_supported (void)
{
	return notify_is_initted ();
}
