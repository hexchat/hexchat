/* HexChat
 * Copyright (C) 2013 Berke Viktor.
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

#ifndef HEXCHAT_PROFILE_H
#define HEXCHAT_PROFILE_H

#include "servlist.h"	/* for profile definition */

extern GSList *profile_list;

void profile_init (void);
int profile_save (void);
profile *profile_add (char *name, char *nick1, char *nick2, char *nick3, char *user, char *real);
profile *profile_find_default (void);
profile *profile_find_for_net (ircnet *net);
profile *profile_find_for_serv (server *serv);
gboolean profile_remove (profile *prof);

#endif
