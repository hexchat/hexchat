/* X-Chat
 * Copyright (C) 2001 Peter Zelezny.
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

/* ipv4 and ipv6 networking functions with a common interface */

#include "config.h"

#include <string.h>
#include <glib.h>
#include <gio/gio.h>

#include "network.h"

void
net_store_destroy (netstore *ns)
{
	g_return_if_fail (ns != NULL);

	g_resolver_free_addresses (ns->addrs);
	g_free (ns);
}

netstore *
net_store_new (void)
{
	return g_new0 (netstore, 1);
}

char *
net_resolve (netstore *ns, char *hostname, char **real_host, GError **error)
{
	GResolver *res;
	GList *addrs;
	GInetAddress *addr;
	char *ipstring;

	res = g_resolver_get_default ();

	// todo: lookup by irc service?
	addrs = g_resolver_lookup_by_name (res, hostname, NULL, error);
	if (!addrs)
	{
		g_object_unref (res);
		return NULL;
	}

	ns->addrs = addrs;
	addr = G_INET_ADDRESS(addrs->data);
	ipstring = g_inet_address_to_string (addr);

	if (real_host)
	{
		if (!(*real_host = g_resolver_lookup_by_address (res, addr, NULL, NULL)))
			*real_host = g_strdup (hostname);
	}

	g_object_unref (res);

	return ipstring;
}

/* the only thing making this interface unclean, this shitty sok4, sok6 business */

GSocket *
net_connect (netstore *ns, guint16 port, GSocket *sok4, GSocket *sok6, GError **error)
{
	GSocket *sok;
	GList *addrs;
	gboolean success;

	for (addrs = ns->addrs; addrs; addrs = g_list_next (addrs))
	{
		GInetAddress *inet_addr = G_INET_ADDRESS(addrs->data);
		GSocketAddress *sok_addr;

		g_clear_error (error); /* Last failed attempt set */

		sok_addr = g_inet_socket_address_new (inet_addr, port);
		if (g_socket_address_get_family (sok_addr) == G_SOCKET_FAMILY_IPV4)
			sok = sok4;
		else
			sok = sok6;
		success = g_socket_connect (sok, sok_addr, NULL, error);
		g_object_unref (sok_addr);

		if (success)
			return sok;
	}
	return NULL;
}

gboolean
net_bind (netstore *ns, GSocket *sok4, GSocket *sok6, GError **error4, GError **error6)
{
	GInetAddress *inet_addr = G_INET_ADDRESS(ns->addrs->data);
	GSocketAddress *sok_addr;
	gboolean success;

	sok_addr = g_inet_socket_address_new (inet_addr, 0);
	success = g_socket_bind (sok4, sok_addr, TRUE, error4);
	success &= g_socket_bind (sok6, sok_addr, TRUE, error6);
	g_object_unref (sok_addr);

	return success;
}

void
net_sockets (GSocket **sok4, GSocket **sok6, GError **error4, GError **error6)
{
	*sok4 = g_socket_new (G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_TCP, error4);
	*sok6 = g_socket_new (G_SOCKET_FAMILY_IPV6, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_TCP, error6);

	if (!*sok4 || !*sok6)
	{
		g_warning ("Creating sockets failed\n");
		return;
	}

	g_socket_set_keepalive (*sok4, TRUE);
	g_socket_set_keepalive (*sok6, TRUE);
}

char *
net_resolve_proxy (const char *hostname, guint16 port, GError **error)
{
	GProxyResolver *res;
	char *uri;
	char **proxies;
	char *proxy = NULL;
	guint i;

	res = g_proxy_resolver_get_default ();
	if (!res)
		return NULL;

	// FIXME: ircs also
	uri = g_strdup_printf ("irc://%s:%d", hostname, port);
	proxies = g_proxy_resolver_lookup (res, uri, NULL, error);
	g_free (uri);
	if (g_strv_length (proxies) == 0)
		return NULL;

	for (i = 0; i < g_strv_length (proxies); i++)
	{
		int type;

		net_parse_proxy_uri (proxies[i], NULL, NULL, &type);
		
		if (type != -1)
		{
			proxy = g_strdup (proxies[i]);
			break;
		}
	}
	g_strfreev (proxies);

	if (!proxy) /* FIXME: error code */
		*error = g_error_new_literal (0, 0, "No system proxy found that is supported");

	return proxy;
}

void
net_parse_proxy_uri (const char *proxy_uri, char **host, guint16 *port, int *type)
{
	if (type)
	{
		char *scheme = g_uri_parse_scheme (proxy_uri);

		if (!strcmp (scheme, "direct"))
			*type = 0;
		else if (!strcmp (scheme, "http"))
			*type = 4;
		else if (!strcmp (scheme, "socks5"))
			*type = 3;
		else if (!strcmp (scheme, "socks"))
			*type = 2;
		else
			*type = -1;

		g_free (scheme);
	}

	if (host)
	{
		char *c1, *c2;

		c1 = strchr (proxy_uri, ':') + 3;
		if (c1)
		{
			c2 = strrchr (c1, ':');

			if (c2)
				*host = g_strndup (c1, c2 - c1);
			else
				*host = g_strdup (c1);
		}
		else
			*host = NULL;
	}

	if (port)
	{
		char *c;
		guint64 p;

		c = strrchr (proxy_uri, ':');
		if (c)
		{
			p = g_ascii_strtoull (c + 1, NULL, 0);
			if (p <= G_MAXUINT16)
				*port = (guint16)p;
		}
		else
			*port = 0;
	}
}

guint16
net_get_local_port (GSocket *sok)
{
	GSocketAddress *addr;
	guint16 port;

	addr = g_socket_get_local_address (sok, NULL);
	if (!addr)
		return 0;

	port = g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS(addr));
	g_object_unref (addr);

	return port;
}
