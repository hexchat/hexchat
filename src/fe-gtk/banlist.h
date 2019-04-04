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

#ifndef HEXCHAT_BANLIST_H
#define HEXCHAT_BANLIST_H

#include "../common/hexchat.h"
void banlist_opengui (session *sess);

#ifndef RPL_BANLIST
/* Where's that darn header file, that would have all these defines ? */
#define RPL_BANLIST 367
#define RPL_ENDOFBANLIST 368
#define RPL_INVITELIST 346
#define RPL_ENDOFINVITELIST 347
#define RPL_EXCEPTLIST 348
#define RPL_ENDOFEXCEPTLIST 349
#define RPL_QUIETLIST 728
#define RPL_ENDOFQUIETLIST 729
#endif

typedef enum banlist_modes_e {
	MODE_BAN,
	MODE_EXEMPT,
	MODE_INVITE,
	MODE_QUIET,
	MODE_CT
} banlist_modes;

typedef struct banlist_info_s {
	session *sess;
	int capable;	/* MODE bitmask */
	int readable;	/* subset of capable if not op */
	int writeable;	/* subset of capable if op */
	int checked;	/* subset of (op? writeable: readable) */
	int pending;	/* subset of checked */
	int current;	/* index of currently processing mode */
	int line_ct;	/* count of presented lines */
	int select_ct;	/* count of selected lines */
	GtkWidget *window;
	GtkWidget *treeview;
	GtkWidget *checkboxes[MODE_CT];
	GtkWidget *but_remove;
	GtkWidget *but_crop;
	GtkWidget *but_clear;
	GtkWidget *but_refresh;
} banlist_info;

typedef struct mode_info_s {
	char *name;		/* Checkbox name, e.g. "Bans" */
	char *type;		/* Type for type column, e.g. "Ban" */
	char letter;	/* /mode-command letter, e.g. 'b' for MODE_BAN */
	int code;		/* rfc RPL_foo code, e.g. 367 for RPL_BANLIST */
	int endcode;	/* rfc RPL_ENDOFfoo code, e.g. 368 for RPL_ENDOFBANLIST */
	int bit;			/* Mask bit, e.g., 1<<MODE_BAN  */
	void (*tester)(banlist_info *, int);	/* Function returns true to set bit into checkable */
} mode_info;

#endif /* HEXCHAT_BANLIST_H */
