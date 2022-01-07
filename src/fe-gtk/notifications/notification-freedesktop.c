/* HexChat
 * Copyright (C) 2021 Patrick Griffis.
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

static GDBusProxy *fdo_notifications;
static gboolean strip_markup;

static void
on_notify_ready (GDBusProxy *proxy, GAsyncResult *res, gpointer user_data)
{
    GError *error = NULL;
    guint32 notification_id;
    GVariant *response = g_dbus_proxy_call_finish (proxy, res, &error);
    if (error)
    {
        g_info ("Failed to send notification: %s", error->message);
        g_error_free (error);
        return;
    }

    g_variant_get (response, "(u)", &notification_id);
    g_info ("Notification sent. ID=%u", notification_id);

    g_variant_unref (response);
}

void
notification_backend_show (const char *title, const char *text)
{
    GVariantBuilder params;

    g_assert (fdo_notifications);

    if (strip_markup)
        text = g_markup_escape_text (text, -1);

    g_variant_builder_init (&params, G_VARIANT_TYPE ("(susssasa{sv}i)"));
    g_variant_builder_add (&params, "s", "hexchat"); /* App name */
    g_variant_builder_add (&params, "u", 0); /* ID, 0 means don't replace */
    g_variant_builder_add (&params, "s", "io.github.Hexchat"); /* App icon */
    g_variant_builder_add (&params, "s", title);
    g_variant_builder_add (&params, "s", text);
    g_variant_builder_add (&params, "as", NULL); /* Actions */

    /* Hints */
    g_variant_builder_open (&params, G_VARIANT_TYPE ("a{sv}"));
    g_variant_builder_open (&params, G_VARIANT_TYPE ("{sv}"));
    g_variant_builder_add (&params, "s", "desktop-entry");
    g_variant_builder_add (&params, "v", g_variant_new_string ("io.github.Hexchat"));
    g_variant_builder_close (&params);
    g_variant_builder_close (&params);

    g_variant_builder_add (&params, "i", -1); /* Expiration */

    g_dbus_proxy_call (fdo_notifications,
                       "Notify",
                       g_variant_builder_end (&params),
                       G_DBUS_CALL_FLAGS_NONE,
                       1000,
                       NULL,
                       (GAsyncReadyCallback)on_notify_ready,
                       NULL);

    if (strip_markup)
        g_free ((char*)text);
}

int
notification_backend_init (const char **error)
{
    GError *err = NULL;
    GVariant *response;
    char **capabilities;
    guint i;

    fdo_notifications = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                       G_DBUS_PROXY_FLAGS_NONE,
                                                       NULL,
                                                       "org.freedesktop.Notifications",
                                                       "/org/freedesktop/Notifications",
                                                       "org.freedesktop.Notifications",
                                                       NULL,
                                                       &err);

    if (err)
        goto return_error;

    response = g_dbus_proxy_call_sync (fdo_notifications,
                                       "GetCapabilities",
                                       NULL,
                                       G_DBUS_CALL_FLAGS_NONE,
                                       30,
                                       NULL,
                                       &err);

    if (err)
    {
        g_clear_object (&fdo_notifications);
        goto return_error;
    }

    g_variant_get (response, "(^a&s)", &capabilities);
    for (i = 0; capabilities[i]; i++)
    {
        if (strcmp (capabilities[i], "body-markup") == 0)
            strip_markup = TRUE;
    }

    g_free (capabilities);
    g_variant_unref (response);
    return 1;

return_error:
    *error = g_strdup (err->message);
    g_error_free (err);
    return 0;
}

void
notification_backend_deinit (void)
{
    g_clear_object (&fdo_notifications);
}

int
notification_backend_supported (void)
{
    return fdo_notifications != NULL;
}
