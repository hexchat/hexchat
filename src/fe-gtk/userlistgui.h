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

#ifndef HEXCHAT_USERLISTGUI_H
#define HEXCHAT_USERLISTGUI_H

void userlist_set_value (GtkWidget *treeview, gfloat val);
gfloat userlist_get_value (GtkWidget *treeview);
GtkWidget *userlist_create (GtkWidget *box);
GtkListStore *userlist_create_model (session *sess);
void userlist_show (session *sess);
void userlist_select (session *sess, char *name);
char **userlist_selection_list (GtkWidget *widget, int *num_ret);
GdkPixbuf *get_user_icon (server *serv, struct User *user);

#endif
