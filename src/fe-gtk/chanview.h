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

#ifndef HEXCHAT_CHANVIEW_H
#define HEXCHAT_CHANVIEW_H

typedef struct _chanview chanview;
typedef struct _chan chan;

chanview *chanview_new (int type, int trunc_len, gboolean sort, gboolean use_icons, GtkStyle *style);
void chanview_set_callbacks (chanview *cv,
	void (*cb_focus) (chanview *, chan *, int tag, void *userdata),
	void (*cb_xbutton) (chanview *, chan *, int tag, void *userdata),
	gboolean (*cb_contextmenu) (chanview *, chan *, int tag, void *userdata, GdkEventButton *),
	int (*cb_compare) (void *a, void *b));
void chanview_set_impl (chanview *cv, int type);
chan *chanview_add (chanview *cv, char *name, void *family, void *userdata, gboolean allow_closure, int tag, GdkPixbuf *icon);
int chanview_get_size (chanview *cv);
GtkWidget *chanview_get_box (chanview *cv);
void chanview_move_focus (chanview *cv, gboolean relative, int num);
GtkOrientation chanview_get_orientation (chanview *cv);
void chanview_set_orientation (chanview *cv, gboolean vertical);

int chan_get_tag (chan *ch);
void *chan_get_userdata (chan *ch);
void chan_focus (chan *ch);
void chan_move (chan *ch, int delta);
void chan_move_family (chan *ch, int delta);
void chan_set_color (chan *ch, PangoAttrList *list);
void chan_rename (chan *ch, char *new_name, int trunc_len);
gboolean chan_remove (chan *ch, gboolean force);
gboolean chan_is_collapsed (chan *ch);
chan * chan_get_parent (chan *ch);

#define FOCUS_NEW_ALL 1
#define FOCUS_NEW_ONLY_ASKED 2
#define FOCUS_NEW_NONE 0

#endif
