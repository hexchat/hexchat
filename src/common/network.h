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
#ifdef NETWORK_PRIVATE
	struct addrinfo *ip6_hostent;
#else
	int _dummy;	/* some compilers don't like empty structs */
#endif
} netstore;

#define MAX_HOSTNAME 128

netstore *net_store_new (void);
void net_store_destroy (netstore *ns);
int net_connect (netstore *ns, int sok4, int sok6, int *sok_return);
char *net_resolve (netstore *ns, char *hostname, int port, char **real_host);
void net_bind (netstore *tobindto, int sok4, int sok6);
char *net_ip (guint32 addr);
void net_sockets (int *sok4, int *sok6);

#endif
