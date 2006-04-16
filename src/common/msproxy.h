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

#include "network.h"

#define MSPROXY_EXECUTABLE 		"xchat.exe"	/* This probably can be used for access control on the server side */

#define MSPROXY_MINLENGTH		172		/* minimum length of packet.				*/
#define NT_MAXNAMELEN			17		/* maximum name length (domain etc), comes from NetBIOS */
#define MSPROXY_VERSION			0x00010200	/* MS Proxy v2 ?					*/

/* Commands / responses */
#define MSPROXY_HELLO			0x0500	/* packet 1 from client.			*/
#define MSPROXY_HELLO_ACK		0x1000	/* packet 1 from server.			*/

#define MSPROXY_USERINFO_ACK		0x0400	/* packet 2 from server.			*/

#define MSPROXY_AUTHENTICATE		0x4700	/* authentication request		*/
#define MSPROXY_AUTHENTICATE_ACK	0x4714	/* authentication challenge		*/

#define MSPROXY_AUTHENTICATE_2		0x4701	/* authentication response		*/
#define MSPROXY_AUTHENTICATE_2_ACK	0x4715	/* authentication passed		*/
#define MSPROXY_AUTHENTICATE_2_NAK	0x4716	/* authentication failure		*/

#define MSPROXY_CONNECT			0x071e	/* connect request.			*/
#define MSPROXY_CONNECT_ACK		0x0703	/* connect request accepted.		*/

#pragma pack(1)

struct ntlm_buffer {
	guint16	len;
	guint16	alloc;
	guint32	offset;
};

struct msproxy_request_t {
	guint32					clientid;			/* 1-4							*/
	guint32					magic25;				/* 5-8							*/
	guint32					serverid;			/* 9-12							*/
	unsigned char				serverack;			/* 13: ack of last server packet			*/
	char					pad10[3];			/* 14-16						*/
	unsigned char				sequence;			/* 17: sequence # of this packet.			*/
	char					pad11[7];			/* 18-24						*/
	char					RWSP[4];			/* 25-28: 0x52,0x57,0x53,0x50				*/
	char					pad15[8];			/* 29-36						*/
	guint16					command;				/* 37-38						*/

	/* packet specifics start at 39. */
	union {
		struct {
			char			pad1[18];			/* 39-56						*/
			guint16			magic3;				/* 57-58						*/
			char           		pad3[114];			/* 59-172						*/
			guint16			magic5;				/* 173-174: 0x4b, 0x00					*/
			char			pad5[2];			/* 175-176						*/
			guint16			magic10;				/* 177-178: 0x14, 0x00					*/
			char			pad6[2];			/* 179-180						*/
			guint16			magic15;				/* 181-182: 0x04, 0x00					*/
			char			pad10[2];			/* 183-184						*/
			guint16			magic16;				/* 185-186						*/
			char			pad11[2];			/* 187-188						*/
			guint16			magic20;				/* 189-190: 0x57, 0x04					*/
			guint16			magic25;				/* 191-192: 0x00, 0x04					*/
			guint16			magic30;				/* 193-194: 0x01, 0x00					*/
			char			pad20[2];			/* 195-196: 0x4a, 0x02					*/
			guint16			magic35;				/* 197-198: 0x4a, 0x02					*/
			char			pad30[10];			/* 199-208						*/
			guint16			magic40;				/* 209-210: 0x30, 0x00					*/
			char			pad40[2];			/* 211-212						*/
			guint16			magic45;				/* 213-214: 0x44, 0x00					*/
			char			pad45[2];			/* 215-216						*/
			guint16			magic50;				/* 217-218: 0x39, 0x00					*/
			char			pad50[2];			/* 219-220						*/
			char			data[256];			/* 221-EOP: a sequence of NULL-terminated strings:
											- username;
											- empty string (just a NULL);
											- application name;
											- hostname					*/
		} hello;

		struct {
			char			pad1[4];			/* 39-42						*/
			guint16			magic2;				/* 43-44						*/
			char			pad10[12];			/* 45-56						*/
			guint32			bindaddr;			/* 57-60: address to bind.				*/
			guint16			bindport;			/* 61-62: port to bind.					*/
			char           		pad15[2];			/* 63-64						*/
			guint16			magic3;				/* 65-66						*/
			guint16			boundport;			/* 67-68						*/
			char           		pad20[104];			/* 69-172						*/
			char			NTLMSSP[sizeof("NTLMSSP")];	/* 173-180: "NTLMSSP"					*/
			guint32			msgtype;				/* 181-184: NTLM message type = 1			*/
			guint32			flags;				/* 185-188: NTLM message flags				*/
			guint16			magic20;				/* 189-190: 0x28, 0x00					*/
			char			pad30[2];			/* 191-192						*/
			guint16			magic25;				/* 193-194: 0x96, 0x82					*/
			guint16			magic30;				/* 195-196: 0x01, 0x00					*/
			char			pad40[12];			/* 197-208						*/
			guint16			magic50;				/* 209-210: 0x30, 0x00					*/
			char			pad50[6];			/* 211-216						*/
			guint16			magic55;				/* 217-218: 0x30, 0x00					*/
			char			pad55[2];			/* 219-220						*/
			char			data[0];			/* Dummy end marker, no real data required		*/
		} auth;

		struct {
			char			pad1[4];			/* 39-42						*/
			guint16			magic1;				/* 43-44						*/
			guint32			magic2;				/* 45-48						*/
			char			pad2[8];			/* 49-56						*/
			guint16			magic3;				/* 57-58						*/
			char			pad3[6];			/* 59-64						*/
			guint16			magic4;				/* 65-66						*/
			guint16			boundport;			/* 67-68						*/
			char           		pad4[104];			/* 69-172						*/
			char			NTLMSSP[sizeof("NTLMSSP")];	/* 173-180: "NTLMSSP"					*/
			guint32			msgtype;				/* 181-184: NTLM message type = 3			*/
			struct ntlm_buffer	lm_resp;				/* 185-192: LM response security buffer			*/
			struct ntlm_buffer	ntlm_resp;			/* 193-200: NTLM response security buffer		*/
			struct ntlm_buffer	ntdomain_buf;			/* 201-208: domain name security buffer			*/
			struct ntlm_buffer	username_buf;			/* 209-216: username security buffer			*/
			struct ntlm_buffer	clienthost_buf;			/* 217-224: hostname security buffer			*/
			struct ntlm_buffer	sessionkey_buf;			/* 225-232: session key security buffer			*/
			guint32			flags;				/* 233-236: message flags				*/
			char			data[1024];			/* 237-EOP: data area					*/
		} auth2;

		struct {
			guint16			magic1;				/* 39-40						*/
			char			pad1[2];			/* 41-42						*/
			guint16			magic2;				/* 43-44						*/
			guint32			magic3;				/* 45-48						*/
			char			pad5[8];			/* 48-56						*/
			guint16			magic6;				/* 57-58: 0x0200					*/
			guint16			destport;			/* 59-60						*/
			guint32			destaddr;			/* 61-64						*/
			char			pad10[4];			/* 65-68						*/
			guint16			magic10;				/* 69-70						*/
			char			pad15[2];			/* 71-72						*/
			guint16			srcport;			/* 73-74: port client connects from			*/
			char			pad20[82];			/* 75-156						*/
			char			executable[256];		/* 76-EOP: application name				*/
		} connect;

		struct {
			guint16			magic1;				/* 39-40						*/
			char			pad5[2];			/* 41-42						*/
			guint16			magic5;				/* 43-44						*/
			guint32			magic10;				/* 45-48						*/
			char			pad10[2];			/* 49-50						*/
			guint16			magic15;				/* 51-52						*/
			guint32			magic16;				/* 53-56						*/
			guint16			magic20;				/* 57-58						*/
			guint16			clientport;			/* 59-60: forwarded port.				*/
			guint32			clientaddr;			/* 61-64: forwarded address.				*/
			guint32			magic30;				/* 65-68						*/
			guint32			magic35;				/* 69-72						*/
			guint16			serverport;			/* 73-74: port server will connect to us from.		*/
			guint16			srcport;			/* 75-76: connect request; port used on client behalf.	*/
			guint16			boundport;			/* 77-78: bind request; port used on client behalf.	*/
			guint32			boundaddr;			/* 79-82: addr used on client behalf			*/
			char			pad30[90];			/* 83-172						*/
			char			data[0];			/* End marker						*/
		} connack;

	} packet;
};

struct msproxy_response_t {
	guint32					packetid;			/* 1-4							*/
	guint32					magic5;				/* 5-8							*/
	guint32             			serverid;			/* 9-12							*/
	char					clientack;			/* 13: ack of last client packet.			*/
	char					pad5[3];			/* 14-16						*/
	unsigned char				sequence;			/* 17: sequence # of this packet.			*/
	char					pad10[7];			/* 18-24						*/
	char					RWSP[4];			/* 25-28: 0x52,0x57,0x53,0x50				*/
	char					pad15[8];			/* 29-36						*/
	guint16					command;				/* 37-38						*/

	union {
		struct {
			char			pad5[18];			/* 39-56						*/
			guint16			magic20;				/* 57-58: 0x02, 0x00					*/
			char			pad10[6];			/* 59-64						*/
			guint16			magic30;				/* 65-66: 0x74, 0x01					*/
			char			pad15[2];			/* 67-68						*/
			guint16			magic35;				/* 69-70: 0x0c, 0x00					*/
			char			pad20[6];			/* 71-76						*/
			guint16			magic50;				/* 77-78: 0x04, 0x00					*/
			char			pad30[6];			/* 79-84						*/
			guint16			magic60;				/* 85-86: 0x65, 0x05					*/
			char			pad35[2];			/* 87-88						*/
			guint16			magic65;				/* 89-90: 0x02, 0x00					*/
			char			pad40[8];			/* 91-98						*/
			guint16			udpport;			/* 99-100						*/
			guint32			udpaddr;			/* 101-104						*/
		} hello;

		struct {
			char			pad1[6];			/* 39-44						*/
			guint32			magic10;				/* 45-48						*/
			char			pad3[10];			/* 49-58						*/
			guint16			boundport;			/* 59-60: port server bound for us.			*/
			guint32			boundaddr;			/* 61-64: addr server bound for us.			*/
			char			pad10[4];			/* 65-68						*/
			guint16			magic15;				/* 69-70						*/
			char			pad15[102];			/* 70-172						*/
			char			NTLMSSP[sizeof("NTLMSSP")];	/* 173-180: "NTLMSSP"					*/
			guint32			msgtype;				/* 181-184: NTLM message type = 2			*/
			struct ntlm_buffer	target;				/* 185-192: target security buffer			*/
			guint32			flags;				/* 193-196: NTLM message flags				*/
			char			challenge[8];			/* 197-204: NTLM challenge request			*/
			char			context[8];			/* 205-212: NTLM context				*/
			char			data[1024];			/* 213-EOP: target information data			*/
		} auth;

		struct {
			guint16			magic1;				/* 39-40						*/
			char			pad5[18];			/* 41-58						*/
			guint16			clientport;			/* 59-60: forwarded port.				*/
			guint32			clientaddr;			/* 61-64: forwarded address.				*/
			guint32			magic10;				/* 65-68						*/
			guint32			magic15;				/* 69-72						*/
			guint16			serverport;			/* 73-74: port server will connect to us from.		*/
			guint16			srcport;			/* 75-76: connect request; port used on client behalf.	*/
			guint16			boundport;			/* 77-78: bind request; port used on client behalf.	*/
			guint32			boundaddr;			/* 79-82: addr used on client behalf			*/
			char			pad10[90];			/* 83-172						*/
		} connect;
	} packet;
};

#pragma pack()

int traverse_msproxy (int sok, char *serverAddr, int port, struct msproxy_state_t *state, netstore *ns_proxy, int csok4, int csok6, int *csok, char bound);
void msproxy_keepalive (void);
