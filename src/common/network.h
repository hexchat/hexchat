/* HexChat
 * Copyright (C) 1998-2010 Peter Zelezny.
 * Copyright (C) 2009-2013 Berke Viktor.
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

#ifndef HEXCHAT_NETWORK_H
#define HEXCHAT_NETWORK_H

typedef struct netstore_
{
	GList *addrs;
} netstore;

#define MAX_HOSTNAME 128

netstore *net_store_new (void);
void net_store_destroy (netstore *ns);
GSocket *net_connect (netstore *ns, guint16 port, GSocket *sok4, GSocket *sok6, GError **error);
char *net_resolve (netstore *ns, char *hostname, char **real_host, GError **error);
char * net_resolve_proxy (const char *hostname, guint16 port, GError **error);
gboolean net_bind (netstore *ns, GSocket *sok4, GSocket *sok6, GError **error4, GError **error6);
void net_sockets (GSocket **sok4, GSocket **sok6, GError **error4, GError **error6);
void net_parse_proxy_uri (const char *proxy_uri, char **host, guint16 *port, int *type);
guint16 net_get_local_port (GSocket *sok);
#endif
