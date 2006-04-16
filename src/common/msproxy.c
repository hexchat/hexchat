/* X-Chat
 * Copyright (C) 1998 Peter Zelezny.
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
 * MS Proxy (ISA server) support is (c) 2006 Pavel Fedin <sonic_amiga@rambler.ru>
 * based on Dante source code
 * Copyright (c) 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006
 *      Inferno Nettverk A/S, Norway.  All rights reserved.
 */

/*#define DEBUG_MSPROXY*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define WANTSOCKET
#define WANTARPA
#include "inet.h"

#include "xchat.h"
#include "network.h"
#include "xchatc.h"
#include "server.h"
#include "msproxy.h"


#ifdef USE_MSPROXY
#include <ntlm.h>

static int
send_msprequest(s, state, request, end)
	int s;
	struct msproxy_state_t *state;
	struct msproxy_request_t *request;
	char *end;
{
	ssize_t w;
	size_t l;

	request->magic25 = htonl(MSPROXY_VERSION);
	request->serverack = state->seq_recv;
	/* don't start incrementing sequence until we are acking packet #2. */
	request->sequence	= (unsigned char)(request->serverack >= 2 ? state->seq_sent + 1 : 0);

	memcpy(request->RWSP, "RWSP", sizeof(request->RWSP));

	l = end - (char *)request;
	/* all requests must be atleast MSPROXY_MINLENGTH it seems. */
	if (l < MSPROXY_MINLENGTH) {
		bzero(end, (size_t)(MSPROXY_MINLENGTH - l));
		l = MSPROXY_MINLENGTH;
	}

	if ((w = send(s, request, l, 0)) != l) {
#ifdef DEBUG_MSPROXY
		printf ("send_msprequest(): send() failed (%d bytes sent instead of %d\n", w, l);
		perror ("Error is");
#endif
		return -1;
	}
	state->seq_sent = request->sequence;

	return w;
}

static int
recv_mspresponse(s, state, response)
	int s;
	struct msproxy_state_t *state;
	struct msproxy_response_t *response;
{
	ssize_t r;

	do {
		if ((r = recv (s, response, sizeof (*response), 0)) < MSPROXY_MINLENGTH) {
#ifdef DEBUG_MSPROXY
			printf ("recv_mspresponse(): expected to read atleast %d, read %d\n", MSPROXY_MINLENGTH, r);
#endif
			return -1;
		}
		if (state->seq_recv == 0)
			break; /* not started incrementing yet. */
#ifdef DEBUG_MSPROXY
		if (response->sequence == state->seq_recv)
			printf ("seq_recv: %d, dup response, seqnumber: 0x%x\n", state->seq_recv, response->sequence);
#endif
	} while (response->sequence == state->seq_recv);

	state->seq_recv = response->sequence;

	return r;
}

int
traverse_msproxy (int sok, char *serverAddr, int port, struct msproxy_state_t *state, netstore *ns_proxy, int csok4, int csok6, int *csok, char bound)
{
	struct msproxy_request_t req;
	struct msproxy_response_t res;
	char *data, *p;
	char hostname[NT_MAXNAMELEN];
	char ntdomain[NT_MAXNAMELEN];
	char challenge[8];
	netstore *ns_client;
	int clientport;
	guint32 destaddr;
	guint32 flags;

	if (!prefs.proxy_auth || !prefs.proxy_user[0] || !prefs.proxy_pass[0] )
		return 1;

	/* MS proxy protocol implementation currently doesn't support IPv6 */
	destaddr = net_getsockaddr_v4 (ns_proxy);
	if (!destaddr)
		return 1;

	state->seq_recv = 0;
	state->seq_sent = 0;

#ifdef DEBUG_MSPROXY
	printf ("Connecting to %s:%d via MS proxy\n", serverAddr, port);
#endif

	gethostname (hostname, NT_MAXNAMELEN);
	p = strchr (hostname, '.');
        if (p)
        	*p = '\0';

	bzero (&req, sizeof(req));
	req.clientid		= htonl(0x0a000000);	/* Initial client ID is always 0x0a		*/
	req.command		= htons(MSPROXY_HELLO);	/* HELLO command					*/
	req.packet.hello.magic5	= htons(0x4b00);	/* Fill in magic values				*/
	req.packet.hello.magic10	= htons(0x1400);
	req.packet.hello.magic15	= htons(0x0400);
	req.packet.hello.magic20	= htons(0x5704);
	req.packet.hello.magic25	= htons(0x0004);
	req.packet.hello.magic30	= htons(0x0100);
	req.packet.hello.magic35	= htons(0x4a02);
	req.packet.hello.magic40	= htons(0x3000);
	req.packet.hello.magic45	= htons(0x4400);
	req.packet.hello.magic50	= htons(0x3900);
	data = req.packet.hello.data;
	strcpy (data, prefs.proxy_user);		/* Append a username				*/
	data += strlen (prefs.proxy_user)+2;		/* +2 automatically creates second empty string	*/
	strcpy (data, MSPROXY_EXECUTABLE);		/* Append an application name			*/
	data += strlen (MSPROXY_EXECUTABLE)+1;
	strcpy (data, hostname);				/* Append a hostname				*/
	data += strlen (hostname)+1;

	if (send_msprequest(sok, state, &req, data) == -1)
		return 1;

	if (recv_mspresponse(sok, state, &res) == -1)
		return 1;

	if (strcmp(res.RWSP, "RWSP") != 0) {
#ifdef DEBUG_MSPROXY
		printf ("Received mailformed packet (no RWSP signature)\n");
#endif
		return 1;
	}

	if (ntohs(res.command) >> 8 != 0x10) {
#ifdef DEBUG_MSPROXY
		printf ("expected res.command = 10??, is %x", ntohs(res.command));
#endif
		return 1;
	}

	state->clientid	= htonl(rand());
	state->serverid	= res.serverid;

#ifdef DEBUG_MSPROXY
	printf ("clientid: 0x%x, serverid: 0x%0x\n", state->clientid, state->serverid);
	printf ("packet #2\n");
#endif

	/* almost identical. */
	req.clientid	= state->clientid;
	req.serverid	= state->serverid;

	if (send_msprequest(sok, state, &req, data) == -1)
		return 1;

	if (recv_mspresponse(sok, state, &res) == -1)
		return 1;

	if (res.serverid != state->serverid) {
#ifdef DEBUG_MSPROXY
		printf ("expected serverid = 0x%x, is 0x%x\n",state->serverid, res.serverid);
#endif
		return 1;
	}

	if (res.sequence != 0x01) {
#ifdef DEBUG_MSPROXY
		printf ("expected res.sequence = 0x01, is 0x%x\n", res.sequence);
#endif
		return 1;
	}

	if (ntohs(res.command) != MSPROXY_USERINFO_ACK) {
#ifdef DEBUG_MSPROXY
		printf ("expected res.command = 0x%x, is 0x%x\n", MSPROXY_USERINFO_ACK, ntohs(res.command));
#endif
		return 1;
	}

#ifdef DEBUG_MSPROXY
	printf ("packet #3\n");
#endif

	bzero(&req, sizeof(req));
	req.clientid			= state->clientid;
	req.serverid			= state->serverid;
	req.command			= htons(MSPROXY_AUTHENTICATE);
	memcpy(req.packet.auth.NTLMSSP, "NTLMSSP", sizeof("NTLMSSP"));
	req.packet.auth.bindaddr	= htonl(0x02000000);
	req.packet.auth.msgtype		= htonl(0x01000000);
	/* NTLM flags:	0x80000000 Negotiate LAN Manager key
			0x10000000 Negotiate sign
			0x04000000 Request target
			0x02000000 Negotiate OEM
			0x00800000 Always sign
			0x00020000 Negotiate NTLM
	*/
	req.packet.auth.flags		= htonl(0x06020000);

	if (send_msprequest(sok, state, &req, &req.packet.auth.data) == -1)
		return 1;

	if (recv_mspresponse(sok, state, &res) == -1)
		return 1;

	if (res.serverid != state->serverid) {
#ifdef DEBUG_MSPROXY
		printf ("expected serverid = 0x%x, is 0x%x\n", state->serverid, res.serverid);
#endif
		return 1;
	}

	if (ntohs(res.command) != MSPROXY_AUTHENTICATE_ACK) {
#ifdef DEBUG_MSPROXY
		printf ("expected res.command = 0x%x, is 0x%x\n", MSPROXY_AUTHENTICATE_ACK, ntohs(res.command));
#endif
		return 1;
	}

	flags = res.packet.auth.flags & htonl(0x00020000);			/* Remember if the server supports NTLM */
	memcpy(challenge, &res.packet.auth.challenge, sizeof(challenge));
	memcpy(ntdomain, &res.packet.auth.NTLMSSP[res.packet.auth.target.offset], res.packet.auth.target.len);
	ntdomain[res.packet.auth.target.len] = 0;

#ifdef DEBUG_MSPROXY
	printf ("ntdomain: \"%s\"\n", ntdomain);
	printf ("packet #4\n");
#endif

	bzero(&req, sizeof(req));
	req.clientid			= state->clientid;
	req.serverid			= state->serverid;
	req.command			= htons(MSPROXY_AUTHENTICATE_2);	/* Authentication response			*/
	req.packet.auth2.magic3		= htons(0x0200);			/* Something					*/
	memcpy(req.packet.auth2.NTLMSSP, "NTLMSSP", sizeof("NTLMSSP"));		/* Start of NTLM message				*/
	req.packet.auth2.msgtype		= htonl(0x03000000);			/* Message type 2				*/
	req.packet.auth2.flags		= flags | htonl(0x02000000);		/* Choose authentication method			*/
	data = req.packet.auth2.data;
	if (flags) {
		req.packet.auth2.lm_resp.len	= 0;				/* We are here if NTLM is supported,		*/
		req.packet.auth2.lm_resp.alloc	= 0;				/* Do not fill in insecure LM response		*/
		req.packet.auth2.lm_resp.offset	= data - req.packet.auth2.NTLMSSP;
		req.packet.auth2.ntlm_resp.len = 24;				/* Fill in NTLM response security buffer	*/
		req.packet.auth2.ntlm_resp.alloc = 24;
		req.packet.auth2.ntlm_resp.offset = data - req.packet.auth2.NTLMSSP;
		ntlm_smb_nt_encrypt(prefs.proxy_pass, challenge, data);		/* Append an NTLM response			*/
		data += 24;	
	} else {
		req.packet.auth2.lm_resp.len	= 24;				/* Fill in LM response security buffer		*/
		req.packet.auth2.lm_resp.alloc	= 24;
		req.packet.auth2.lm_resp.offset	= data - req.packet.auth2.NTLMSSP;
		ntlm_smb_encrypt(prefs.proxy_pass, challenge, data);		/* Append an LM response			*/
		data += 24;
		req.packet.auth2.ntlm_resp.len = 0;				/* NTLM response is empty			*/
		req.packet.auth2.ntlm_resp.alloc = 0;
		req.packet.auth2.ntlm_resp.offset = data - req.packet.auth2.NTLMSSP;
	}
	req.packet.auth2.ntdomain_buf.len = strlen(ntdomain);			/* Domain name					*/
	req.packet.auth2.ntdomain_buf.alloc = req.packet.auth2.ntdomain_buf.len;
	req.packet.auth2.ntdomain_buf.offset = data - req.packet.auth2.NTLMSSP;
	strcpy(data, ntdomain);
	data += req.packet.auth2.ntdomain_buf.len;
	req.packet.auth2.username_buf.len = strlen(prefs.proxy_user);		/* Username					*/
	req.packet.auth2.username_buf.alloc = req.packet.auth2.username_buf.len;
	req.packet.auth2.username_buf.offset = data - req.packet.auth2.NTLMSSP;
	strcpy(data, prefs.proxy_user);
	data += req.packet.auth2.username_buf.len;
	req.packet.auth2.clienthost_buf.len = strlen(hostname);			/* Hostname					*/
	req.packet.auth2.clienthost_buf.alloc = req.packet.auth2.clienthost_buf.len;
	req.packet.auth2.clienthost_buf.offset = data - req.packet.auth2.NTLMSSP;
	strcpy(data, hostname);
	data += req.packet.auth2.clienthost_buf.len;
	req.packet.auth2.sessionkey_buf.len = 0;				/* Session key (we don't use it)		*/
	req.packet.auth2.sessionkey_buf.alloc = 0;
	req.packet.auth2.sessionkey_buf.offset = data - req.packet.auth2.NTLMSSP;

	if (send_msprequest(sok, state, &req, data) == -1)
		return 1;

	if (recv_mspresponse(sok, state, &res) == -1)
		return 1;

	if (res.serverid != state->serverid) {
#ifdef DEBUG_MSPROXY
		printf ("expected res.serverid = 0x%x, is 0x%x\n", state->serverid, res.serverid);
#endif
		return 1;
	}

	if (res.clientack != 0x01) {
#ifdef DEBUG_MSPROXY
		printf ("expected res.clientack = 0x01, is 0x%x\n", res.clientack);
#endif
		return 1;
	}

	if (ntohs(res.command) >> 8 != 0x47) {
#ifdef DEBUG_MSPROXY
		printf ("expected res.command = 47??, is 0x%x\n", ntohs(res.command));
#endif
		return 1;
	}

	if (ntohs(res.command) ==  MSPROXY_AUTHENTICATE_2_NAK) {
#ifdef DEBUG_MSPROXY
		printf ("Authentication failed\n");
#endif
		return -1;
	}

#ifdef DEBUG_MSPROXY
	printf ("packet #5\n");
#endif

	bzero(&req, sizeof(req));
	req.clientid			= state->clientid;
	req.serverid			= state->serverid;
	req.command			= htons(MSPROXY_CONNECT);
	req.packet.connect.magic2	= htons(0x0200);
	req.packet.connect.magic6	= htons(0x0200);
	req.packet.connect.destport	= htons(port);
	req.packet.connect.destaddr	= destaddr;
	data = req.packet.connect.executable;
	strcpy(data, MSPROXY_EXECUTABLE);
	data += strlen(MSPROXY_EXECUTABLE) + 1;

	/*
	 * need to tell server what port we will connect from, so we bind our sockets.
	 */
	ns_client = net_store_new ();
	if (!bound) {
		net_store_fill_any (ns_client);
		net_bind(ns_client, csok4, csok6);
#ifdef DEBUG_MSPROXY
		perror ("bind() result");
#endif
	}
 	clientport = net_getsockport(csok4, csok6);
	if (clientport == -1) {
#ifdef DEBUG_MSPROXY
		printf ("Unable to obtain source port\n");
#endif
		return 1;
	}
	req.packet.connect.srcport = clientport;

	if (send_msprequest(sok, state, &req, data) == -1)
		return 1;

	if (recv_mspresponse(sok, state, &res) == -1)
		return 1;

	if (ntohs(res.command) != MSPROXY_CONNECT_ACK) {
#ifdef DEBUG_MSPROXY
		printf ("expected res.command = 0x%x, is 0x%x\n",MSPROXY_CONNECT_ACK, ntohs(res.command));
#endif
		return 1;
	}

	net_store_fill_v4 (ns_client, res.packet.connect.clientaddr, res.packet.connect.clientport);

#ifdef DEBUG_MSPROXY
	printf ("Connecting...\n");
#endif
	if (net_connect (ns_client, csok4, csok6, csok) != 0) {
#ifdef DEBUG_MSPROXY
		printf ("Failed to connect to port %d\n", htons(res.packet.connect.clientport));
#endif
		net_store_destroy (ns_client);
		return 1;
	}
	net_store_destroy (ns_client);
#ifdef DEBUG_MSPROXY
	printf ("packet #6\n");
#endif

	req.clientid	= state->clientid;
	req.serverid	= state->serverid;
	req.command	= htons(MSPROXY_USERINFO_ACK);		

	if (send_msprequest(sok, state, &req, req.packet.connack.data) == -1)
		return 1;

	return 0;
}

void
msproxy_keepalive (void)
{
	server *serv;
	GSList *list = serv_list;
	struct msproxy_request_t req;
	struct msproxy_response_t res;

	while (list)
	{
		serv = list->data;
		if (serv->connected && (serv->proxy_sok != -1))
		{
#ifdef DEBUG_MSPROXY
			printf ("sending MS proxy keepalive packet\n");
#endif

			bzero(&req, sizeof(req));
			req.clientid	= serv->msp_state.clientid;
			req.serverid	= serv->msp_state.serverid;
			req.command	= htons(MSPROXY_HELLO);
			
			if (send_msprequest(serv->proxy_sok, &serv->msp_state, &req, req.packet.hello.data) == -1)
				continue;

			recv_mspresponse(serv->proxy_sok, &serv->msp_state, &res);

#ifdef DEBUG_MSPROXY
			if (ntohs(res.command) != MSPROXY_USERINFO_ACK)
				printf ("expected res.command = 0x%x, is 0x%x\n", MSPROXY_USERINFO_ACK, ntohs(res.command));
#endif
		}
		list = list->next;
	}
}

#endif
