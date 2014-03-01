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

#ifndef HEXCHAT_IGNORE_H
#define HEXCHAT_IGNORE_H

extern GSList *ignore_list;

extern int ignored_ctcp;
extern int ignored_priv;
extern int ignored_chan;
extern int ignored_noti;
extern int ignored_invi;

#define IG_PRIV	1
#define IG_NOTI	2
#define IG_CHAN	4
#define IG_CTCP	8
#define IG_INVI	16
#define IG_UNIG	32
#define IG_NOSAVE	64
#define IG_DCC		128

struct ignore
{
	char *mask;
	unsigned int type;	/* one of more of IG_* ORed together */
};

struct ignore *ignore_exists (char *mask);
int ignore_add (char *mask, int type, gboolean overwrite);
void ignore_showlist (session *sess);
int ignore_del (char *mask, struct ignore *ig);
int ignore_check (char *mask, int type);
void ignore_load (void);
void ignore_save (void);
void ignore_gui_open (void);
void ignore_gui_update (int level);
int flood_check (char *nick, char *ip, server *serv, session *sess, int what);

#endif
