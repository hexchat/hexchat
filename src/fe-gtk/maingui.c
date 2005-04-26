/* X-Chat
 * Copyright (C) 2002 Peter Zelezny.
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
 */

#define GTK_DISABLE_DEPRECATED

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <gtk/gtkarrow.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkhpaned.h>
#include <gtk/gtkframe.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkprogressbar.h>
#include <gtk/gtkstock.h>
#include <gtk/gtktable.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtkcheckmenuitem.h>
#include <gtk/gtkvscrollbar.h>

#include "../common/xchat.h"
#include "../common/fe.h"
#include "../common/server.h"
#include "../common/text.h"
#include "../common/xchatc.h"
#include "../common/outbound.h"
#include "../common/inbound.h"
#include "../common/plugin.h"
#include "../common/modes.h"
#include "fe-gtk.h"
#include "banlist.h"
#include "gtkutil.h"
#include "palette.h"
#include "maingui.h"
#include "menu.h"
#include "fkeys.h"
#include "userlistgui.h"
#include "tabs.h"
#include "xtext.h"

static void mg_create_entry (session *sess, GtkWidget *box);
static void mg_link_irctab (session *sess, int focus);

static session_gui static_mg_gui;
static session_gui *mg_gui = NULL;	/* the shared irc tab */
static int ignore_chanmode = FALSE;
static const char chan_flags[] = { 't', 'n', 's', 'i', 'p', 'm', 'l', 'k' };

static GtkWidget *active_tab = NULL;	/* active tab - toggle button */
GtkWidget *parent_window = NULL;			/* the master window */

GtkStyle *input_style;

static PangoAttrList *newdata_list;
static PangoAttrList *nickseen_list;
static PangoAttrList *newmsg_list;
static PangoAttrList *plain_list = NULL;


static PangoAttrList *
mg_attr_list_create (GdkColor *col)
{
	PangoAttribute *attr;
	PangoAttrList *list;

	list = pango_attr_list_new ();

	if (col)
	{
		attr = pango_attr_foreground_new (col->red, col->green, col->blue);
		attr->start_index = 0;
		attr->end_index = 0xffff;
		pango_attr_list_insert (list, attr);
	}

	if (prefs.tab_small)
	{
#ifdef WIN32
		attr = pango_attr_scale_new (PANGO_SCALE_SMALL);
#else
		attr = pango_attr_scale_new (PANGO_SCALE_X_SMALL);
#endif
		attr->start_index = 0;
		attr->end_index = 0xffff;
		pango_attr_list_insert (list, attr);
	}

	return list;
}

static void
mg_create_tab_colors (void)
{
	if (plain_list)
	{
		pango_attr_list_unref (plain_list);
		pango_attr_list_unref (newmsg_list);
		pango_attr_list_unref (newdata_list);
		pango_attr_list_unref (nickseen_list);
	}

	plain_list = mg_attr_list_create (NULL);
	newdata_list = mg_attr_list_create (&colors[COL_NEW_DATA]);
	nickseen_list = mg_attr_list_create (&colors[COL_HILIGHT]);
	newmsg_list = mg_attr_list_create (&colors[COL_NEW_MSG]);
}

#ifdef WIN32
#define WINVER 0x0501	/* needed for vc6? */
#include <windows.h>
#include <gdk/gdkwin32.h>

/* Flash the taskbar button on Windows when there's a highlight event. */

static void
flash_window (GtkWidget *win)
{
	FLASHWINFO fi;
	static HMODULE user = NULL;
	static BOOL (*flash) (PFLASHWINFO) = NULL;

	if (!user)
	{
		user = GetModuleHandleA ("USER32");
		if (!user)
			return;	/* this should never fail */
	}

	if (!flash)
	{
		flash = (void *)GetProcAddress (user, "FlashWindowEx");
		if (!flash)
			return;	/* this fails on NT4.0 and Win95 */
	}

	fi.cbSize = sizeof (fi);
	fi.hwnd = GDK_WINDOW_HWND (win->window);
	fi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
	fi.uCount = 0;
	fi.dwTimeout = 500;
	flash (&fi);
	/*FlashWindowEx (&fi);*/
}
#endif

/* set a tab plain, red, light-red, or blue */

void
fe_set_tab_color (struct session *sess, int col, int flash)
{
	if (sess->gui->is_tab && (col == 0 || sess != current_tab))
	{
		switch (col)
		{
		case 0:	/* no particular color (theme default) */
			sess->new_data = FALSE;
			sess->msg_said = FALSE;
			sess->nick_said = FALSE;
			tab_set_attrlist (sess->res->tab, plain_list);
			break;
		case 1:	/* new data has been displayed (dark red) */
			sess->new_data = TRUE;
			sess->msg_said = FALSE;
			sess->nick_said = FALSE;
			tab_set_attrlist (sess->res->tab, newdata_list);
			break;
		case 2:	/* new message arrived in channel (light red) */
			sess->new_data = FALSE;
			sess->msg_said = TRUE;
			sess->nick_said = FALSE;
			tab_set_attrlist (sess->res->tab, newmsg_list);
			break;
		case 3:	/* your nick has been seen (blue) */
			sess->new_data = FALSE;
			sess->msg_said = FALSE;
			sess->nick_said = TRUE;
			tab_set_attrlist (sess->res->tab, nickseen_list);
			break;
		}
	}

#ifdef WIN32
	if (flash && prefs.flash_hilight)
		flash_window (sess->gui->window);
#endif
}

/* change the little icon to the left of your nickname */

void
mg_set_access_icon (session_gui *gui, GdkPixbuf *pix)
{
	if (gui->op_xpm)
	{
		gtk_widget_destroy (gui->op_xpm);
		gui->op_xpm = 0;
	}

	if (pix)
	{
		gui->op_xpm = gtk_image_new_from_pixbuf (pix);
		gtk_box_pack_start (GTK_BOX (gui->nick_box), gui->op_xpm, 0, 0, 0);
		gtk_widget_show (gui->op_xpm);
	}
}

static gboolean
mg_inputbox_focus (GtkWidget *widget, GdkEventFocus *event, session_gui *gui)
{
	GSList *list;
	session *sess;

	if (gui->is_tab)
		return FALSE;

	list = sess_list;
	while (list)
	{
		sess = list->data;
		if (sess->gui == gui)
		{
			current_sess = sess;
			if (!sess->server->server_session)
				sess->server->server_session = sess;
			break;
		}
		list = list->next;
	}

	return FALSE;
}

static void
mg_inputbox_cb (GtkWidget *igad, session_gui *gui)
{
	char *cmd = GTK_ENTRY (igad)->text;
	static int ignore = FALSE;
	GSList *list;
	session *sess = NULL;

	if (ignore)
		return;

	if (cmd[0] == 0)
		return;

	cmd = strdup (cmd);

	/* avoid recursive loop */
	ignore = TRUE;
	gtk_entry_set_text (GTK_ENTRY (igad), "");
	ignore = FALSE;

	/* where did this event come from? */
	if (gui->is_tab)
	{
		sess = current_tab;
	} else
	{
		list = sess_list;
		while (list)
		{
			sess = list->data;
			if (sess->gui == gui)
				break;
			list = list->next;
		}
		if (!list)
			sess = NULL;
	}

	if (sess)
		handle_multiline (sess, cmd, TRUE, FALSE);
	free (cmd);
}

void
fe_set_title (session *sess)
{
	char tbuf[256];
	int type;

	if (sess->gui->is_tab && sess != current_tab)
		return;

	type = sess->type;

	if (sess->server->connected == FALSE && sess->type != SESS_DIALOG)
		goto def;

	switch (type)
	{
	case SESS_DIALOG:
		snprintf (tbuf, sizeof (tbuf), "X-Chat ["VERSION"]: %s %s @ %s",
					 _("Dialog with"), sess->channel, sess->server->servername);
		break;
	case SESS_SERVER:
		snprintf (tbuf, sizeof (tbuf), "X-Chat ["VERSION"]: %s @ %s",
					 sess->server->nick, sess->server->servername);
		break;
	case SESS_CHANNEL:
		snprintf (tbuf, sizeof (tbuf),
					 "X-Chat ["VERSION"]: %s @ %s / %s (%s)",
					 sess->server->nick, sess->server->servername,
					 sess->channel, sess->current_modes ? sess->current_modes : "");
		break;
	case SESS_NOTICES:
	case SESS_SNOTICES:
		snprintf (tbuf, sizeof (tbuf), "X-Chat ["VERSION"]: %s @ %s (notices)",
					 sess->server->nick, sess->server->servername);
		break;
	default:
	def:
		gtk_window_set_title (GTK_WINDOW (sess->gui->window), "X-Chat ["VERSION"]");
		return;
	}

	gtk_window_set_title (GTK_WINDOW (sess->gui->window), tbuf);
}

static gboolean
mg_windowstate_cb (GtkWidget *wid, GdkEvent *event, gpointer userdata)
{
	prefs.gui_win_state = 0;
	if (gdk_window_get_state (wid->window) & GDK_WINDOW_STATE_MAXIMIZED)
		prefs.gui_win_state = 1;

	return FALSE;
}

static gboolean
mg_configure_cb (GtkWidget *wid, GdkEventConfigure *event, session *sess)
{
	if (sess == NULL)			/* for the main_window */
	{
		if (mg_gui)
		{
			if (prefs.mainwindow_save)
			{
				sess = current_sess;
				gtk_window_get_position (GTK_WINDOW (wid), &prefs.mainwindow_left,
												 &prefs.mainwindow_top);
				gtk_window_get_size (GTK_WINDOW (wid), &prefs.mainwindow_width,
											&prefs.mainwindow_height);
			}
		}
	}

	if (sess)
	{
		if (sess->type == SESS_DIALOG && prefs.mainwindow_save)
		{
			gtk_window_get_position (GTK_WINDOW (wid), &prefs.dialog_left,
											 &prefs.dialog_top);
			gtk_window_get_size (GTK_WINDOW (wid), &prefs.dialog_width,
										&prefs.dialog_height);
		}

		if (((GtkXText *) sess->gui->xtext)->transparent)
			gtk_widget_queue_draw (sess->gui->xtext);
	}

	return FALSE;
}

/* move to a non-irc tab */

static void
mg_show_generic_tab (GtkWidget *button)
{
	GtkWidget *box;
	int num;
	GtkWidget *f = NULL;

	if (current_sess && GTK_WIDGET_HAS_FOCUS (current_sess->gui->input_box))
		f = current_sess->gui->input_box;

	box = g_object_get_data (G_OBJECT (button), "box");
	num = gtk_notebook_page_num (GTK_NOTEBOOK (mg_gui->note_book), box);
	gtk_notebook_set_current_page (GTK_NOTEBOOK (mg_gui->note_book), num);
	gtk_window_set_title (GTK_WINDOW (mg_gui->window),
								 g_object_get_data (G_OBJECT (button), "title"));
	gtk_widget_set_sensitive (mg_gui->menu, FALSE);

	if (f)
		gtk_widget_grab_focus (f);
}

/* a channel has been focused */

static void
mg_focus (session *sess)
{
	if (sess->gui->is_tab)
		current_tab = sess;
	current_sess = sess;

	/* dirty trick to avoid auto-selection */
	GTK_ENTRY (sess->gui->input_box)->editable = 0;
	gtk_widget_grab_focus (sess->gui->input_box);
	gtk_editable_set_editable (GTK_EDITABLE (sess->gui->input_box), TRUE);

	sess->server->front_session = sess;

	if (sess->server->server_session != NULL)
	{
		if (sess->server->server_session->type != SESS_SERVER)
			sess->server->server_session = sess;
	} else
	{
		sess->server->server_session = sess;
	}

	if (sess->new_data || sess->nick_said || sess->msg_said)
	{
		sess->nick_said = FALSE;
		sess->msg_said = FALSE;
		sess->new_data = FALSE;
		fe_set_tab_color (sess, 0, FALSE);
	}
}

static int
mg_progressbar_update (GtkWidget *bar)
{
	static int type = 0;
	static float pos = 0;

	pos += 0.05;
	if (pos >= 0.99)
	{
		if (type == 0)
		{
			type = 1;
			gtk_progress_bar_set_orientation ((GtkProgressBar *) bar,
														 GTK_PROGRESS_RIGHT_TO_LEFT);
		} else
		{
			type = 0;
			gtk_progress_bar_set_orientation ((GtkProgressBar *) bar,
														 GTK_PROGRESS_LEFT_TO_RIGHT);
		}
		pos = 0.05;
	}
	gtk_progress_bar_set_fraction ((GtkProgressBar *) bar, pos);
	return 1;
}

void
mg_progressbar_create (session_gui *gui)
{
	gui->bar = gtk_progress_bar_new ();
	gtk_box_pack_start (GTK_BOX (gui->nick_box), gui->bar, 0, 0, 0);
	gtk_widget_show (gui->bar);
	gui->bartag = fe_timeout_add (50, mg_progressbar_update, gui->bar);
}

void
mg_progressbar_destroy (session_gui *gui)
{
	fe_timeout_remove (gui->bartag);
	gtk_widget_destroy (gui->bar);
	gui->bar = 0;
	gui->bartag = 0;
}

/* switching tabs away from this one, so remember some info about it! */

static void
mg_unpopulate (session *sess)
{
	restore_gui *res;
	session_gui *gui;
	int i;

	gui = sess->gui;
	res = sess->res;

	if (gui->pane && sess->type == SESS_CHANNEL)
		prefs.paned_pos = gui->pane_pos = gtk_paned_get_position (GTK_PANED (gui->pane));

	res->input_text = strdup (GTK_ENTRY (gui->input_box)->text);
	res->topic_text = strdup (GTK_ENTRY (gui->topic_entry)->text);
	res->limit_text = strdup (GTK_ENTRY (gui->limit_entry)->text);
	res->key_text = strdup (GTK_ENTRY (gui->key_entry)->text);
	if (gui->laginfo)
		res->lag_text = strdup (gtk_label_get_text (GTK_LABEL (gui->laginfo)));
	if (gui->throttleinfo)
		res->queue_text = strdup (gtk_label_get_text (GTK_LABEL (gui->throttleinfo)));

	for (i = 0; i < NUM_FLAG_WIDS - 1; i++)
		res->flag_wid_state[i] = GTK_TOGGLE_BUTTON (gui->flag_wid[i])->active;

	res->old_ul_value = userlist_get_value (gui->user_tree);
	if (gui->lagometer)
		res->lag_value = gtk_progress_bar_get_fraction (
													GTK_PROGRESS_BAR (gui->lagometer));
	if (gui->throttlemeter)
		res->queue_value = gtk_progress_bar_get_fraction (
													GTK_PROGRESS_BAR (gui->throttlemeter));

	if (gui->bar)
	{
		res->c_graph = TRUE;	/* still have a graph, just not visible now */
		mg_progressbar_destroy (gui);
	}
}

static void
mg_restore_label (GtkWidget *label, char **text)
{
	if (!label)
		return;

	if (*text)
	{
		gtk_label_set_text (GTK_LABEL (label), *text);
		free (*text);
		*text = NULL;
	} else
	{
		gtk_label_set_text (GTK_LABEL (label), "");
	}
}

static void
mg_restore_entry (GtkWidget *entry, char **text)
{
	if (*text)
	{
		gtk_entry_set_text (GTK_ENTRY (entry), *text);
		free (*text);
		*text = NULL;
	} else
	{
		gtk_entry_set_text (GTK_ENTRY (entry), "");
	}
	gtk_editable_set_position (GTK_EDITABLE (entry), -1);
}

void
mg_set_topic_tip (session *sess)
{
	char buf[512];

	switch (sess->type)
	{
	case SESS_CHANNEL:
		if (sess->topic)
		{
			snprintf (buf, sizeof (buf), _("Topic for %s is: %s"), sess->channel,
						 sess->topic);
			add_tip (sess->gui->topic_entry, buf);
		} else
			add_tip (sess->gui->topic_entry, _("No topic is set"));
		break;
	default:
		if (GTK_ENTRY (sess->gui->topic_entry)->text &&
			 GTK_ENTRY (sess->gui->topic_entry)->text[0])
			add_tip (sess->gui->topic_entry, GTK_ENTRY (sess->gui->topic_entry)->text);
		else
			add_tip (sess->gui->topic_entry, ""); /* hSP: set topic to empty in case it isn't set.. */
	}
}

static int ul_tag = 0;

static gboolean
mg_populate_userlist (session *sess)
{
	session_gui *gui;

	if (!sess)
		sess = current_tab;

	if (is_session (sess))
	{
		gui = sess->gui;
		if (sess->type == SESS_DIALOG)
			mg_set_access_icon (sess->gui, NULL);
		else
			mg_set_access_icon (sess->gui, get_user_icon (sess->server, sess->res->myself));
		userlist_show (sess);
		userlist_set_value (sess->gui->user_tree, sess->res->old_ul_value);
	}

	ul_tag = 0;
	return 0;
}

/* fill the irc tab with a new channel */

static void
mg_populate (session *sess)
{
	session_gui *gui = sess->gui;
	restore_gui *res = sess->res;
	int i, vis, render = TRUE;

	if (gui->pane)
		vis = gui->ul_hidden;
	else
		vis = GTK_WIDGET_VISIBLE (gui->user_box);

	switch (sess->type)
	{
	case SESS_DIALOG:
		/* show the dialog buttons */
		gtk_widget_show (gui->dialogbutton_box);
		/* hide the chan-mode buttons */
		gtk_widget_hide (gui->topicbutton_box);
		/* hide the userlist */
		mg_userlist_showhide (sess, FALSE);
		/* shouldn't edit the topic */
		gtk_editable_set_editable (GTK_EDITABLE (gui->topic_entry), FALSE);
		break;
	case SESS_SERVER:
		if (prefs.chanmodebuttons)
			gtk_widget_show (gui->topicbutton_box);
		/* hide the dialog buttons */
		gtk_widget_hide (gui->dialogbutton_box);
		/* hide the userlist */
		mg_userlist_showhide (sess, FALSE);
		/* shouldn't edit the topic */
		gtk_editable_set_editable (GTK_EDITABLE (gui->topic_entry), FALSE);
		break;
	default:
		/* hide the dialog buttons */
		gtk_widget_hide (gui->dialogbutton_box);
		if (prefs.chanmodebuttons)
			gtk_widget_show (gui->topicbutton_box);
		/* show the userlist */
		mg_userlist_showhide (sess, TRUE);
		/* let the topic be editted */
		gtk_editable_set_editable (GTK_EDITABLE (gui->topic_entry), TRUE);
	}

	/* move to THE irc tab */
	if (gui->is_tab)
		gtk_notebook_set_current_page (GTK_NOTEBOOK (gui->note_book), 0);

	if (gui->pane)
	{
		/* no change? then there will be no expose, so render now */
		if (vis != gui->ul_hidden && gui->user_box->allocation.width > 1)
			render = FALSE;
	} else
	{
		/* userlist CHANGED? Let the pending exposure draw the xtext */
		if (vis && !GTK_WIDGET_VISIBLE (gui->user_box))
			render = FALSE;
		if (!vis && GTK_WIDGET_VISIBLE (gui->user_box))
			render = FALSE;
	}

	gtk_xtext_buffer_show (GTK_XTEXT (gui->xtext), res->buffer, render);
	GTK_XTEXT (gui->xtext)->color_paste = sess->color_paste;

	if (gui->is_tab)
		gtk_widget_set_sensitive (gui->menu, TRUE);

	/* restore all the GtkEntry's */
	mg_restore_entry (gui->topic_entry, &res->topic_text);
	mg_restore_entry (gui->input_box, &res->input_text);
	mg_restore_entry (gui->key_entry, &res->key_text);
	mg_restore_entry (gui->limit_entry, &res->limit_text);
	mg_restore_label (gui->laginfo, &res->lag_text);
	mg_restore_label (gui->throttleinfo, &res->queue_text);

	mg_focus (sess);
	fe_set_title (sess);

	/* this one flickers, so only change if necessary */
	if (strcmp (sess->server->nick, gtk_button_get_label (GTK_BUTTON (gui->nick_label))) != 0)
		gtk_button_set_label (GTK_BUTTON (gui->nick_label), sess->server->nick);

	/* this is slow, so make it a timeout event */
	if (!gui->is_tab)
	{
		mg_populate_userlist (sess);
	} else
	{
		if (ul_tag == 0)
			ul_tag = g_idle_add ((GSourceFunc)mg_populate_userlist, NULL);
	}

	fe_userlist_numbers (sess);

	/* restore all the channel mode buttons */
	ignore_chanmode = TRUE;
	for (i = 0; i < NUM_FLAG_WIDS - 1; i++)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gui->flag_wid[i]),
												res->flag_wid_state[i]);
	ignore_chanmode = FALSE;

	if (gui->lagometer)
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (gui->lagometer),
												 res->lag_value);
	if (gui->throttlemeter)
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (gui->throttlemeter),
												 res->queue_value);

	/* did this tab have a connecting graph? restore it.. */
	if (res->c_graph)
	{
		res->c_graph = FALSE;
		mg_progressbar_create (gui);
	}

	GTK_CHECK_MENU_ITEM (gui->away_item)->active = sess->server->is_away;

	mg_set_topic_tip (sess);

	plugin_emit_dummy_print (sess, "Focus Tab");
}

void
mg_bring_tofront (GtkWidget *tab)
{
	session *sess;

	sess = g_object_get_data (G_OBJECT (tab), "sess");
	if (!sess)
	{
		/* non-irc tab */
		if (GTK_IS_WINDOW (tab))
			gtk_window_present (GTK_WINDOW (tab));
		else
			tab_focus (tab);
	} else
	{
		if (sess && !sess->gui->is_tab)
		{
			gtk_window_present (GTK_WINDOW (sess->gui->window));
			return;
		}
		tab_focus (tab);
	}
}

void
mg_switch_page (int relative, int num)
{
	if (mg_gui)
		tab_group_switch (mg_gui->tabs_box, relative, num);
}

static void
mg_find_replacement_focus (GtkWidget *tab)
{
	mg_switch_page (TRUE, -1);
}

static void
mg_gendestroy (GtkWidget *tab)
{
	char *title;

	title = g_object_get_data (G_OBJECT (tab), "title");
/*	printf("enter mg_gendestroy (title=%s)\n", title);*/
	free (title);

	if (!xchat_is_quitting)
		gtk_widget_destroy (g_object_get_data (G_OBJECT (tab), "box"));
}

/* a toplevel IRC window was destroyed */

static void
mg_topdestroy_cb (GtkWidget *win, session *sess)
{
/*	printf("enter mg_topdestroy. sess %p was destroyed\n", sess);*/

	/* kill the user list */
	g_object_unref (G_OBJECT (sess->res->user_model));

	kill_session_callback (sess);	/* tell xchat.c about it */
}

/* an IRC tab was destroyed */

static void
mg_ircdestroy (GtkWidget *tab, session *sess)
{
	GSList *list;

	/* kill the text buffer */
	gtk_xtext_buffer_free (sess->res->buffer);
	/* kill the user list */
	g_object_unref (G_OBJECT (sess->res->user_model));

	kill_session_callback (sess);	/* tell xchat.c about it */

	if (mg_gui == NULL)
	{
/*		puts("-> mg_gui is already NULL");*/
		return;
	}

	list = sess_list;
	while (list)
	{
		sess = list->data;
		if (sess->gui->is_tab)
		{
/*			puts("-> some tabs still remain");*/
			return;
		}
		list = list->next;
	}

/*	puts("-> no tabs left, killing main tabwindow");*/
	gtk_widget_destroy (mg_gui->window);
	active_tab = NULL;
	mg_gui = NULL;
	parent_window = NULL;
}

/* a tab has been destroyed */

static void
mg_tabdestroy_cb (GtkWidget *tab, session *sess)
{
	if (tab == active_tab && !xchat_is_quitting)
		mg_find_replacement_focus (tab);

	if (sess == NULL)
		mg_gendestroy (tab);
	else
	{
		if (!xchat_is_quitting)
			tab_group_cleanup (sess->gui->tabs_box);
		mg_ircdestroy (tab, sess);
	}
}

static void
mg_tree_cb (GtkWidget *item, session *sess)
{
	if (is_session (sess))
	{
		if (sess->gui->is_tab)
			mg_bring_tofront (sess->res->tab);
		else
			gtk_window_present (GTK_WINDOW (sess->gui->window));
	}
}

static void
mg_colorpaste_cb (GtkCheckMenuItem *item, session *sess)
{
	sess->color_paste = FALSE;
	if (item->active)
		sess->color_paste = TRUE;
	GTK_XTEXT (sess->gui->xtext)->color_paste = sess->color_paste;
}

static void
mg_beepmsg_cb (GtkCheckMenuItem *item, session *sess)
{
	sess->beep = FALSE;
	if (item->active)
		sess->beep = TRUE;
}

static void
mg_hidejp_cb (GtkCheckMenuItem *item, session *sess)
{
	sess->hide_join_part = TRUE;
	if (item->active)
		sess->hide_join_part = FALSE;
}

static void
mg_create_sess_tree (GtkWidget *menu)
{
	GtkWidget *top_item, *item, *submenu;
	GSList *list, *ilist;
	server *serv;
	session *sess;

	list = serv_list;
	while (list)
	{
		serv = list->data;

		if (serv->servername[0] == 0)
			top_item = gtk_menu_item_new_with_label (_("<none>"));
		else
			top_item = gtk_menu_item_new_with_label (server_get_network (serv, TRUE));

		gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), top_item);
		gtk_widget_show (top_item);

		submenu = gtk_menu_new ();

		ilist = sess_list;
		while (ilist)
		{
			sess = ilist->data;
			if (sess->server == serv)
			{
				if (sess->channel[0] == 0)
					item = gtk_menu_item_new_with_label (_("<none>"));
				else
					item = gtk_menu_item_new_with_label (sess->channel);
				g_signal_connect (G_OBJECT (item), "activate",
										G_CALLBACK (mg_tree_cb), sess);
				gtk_menu_shell_prepend (GTK_MENU_SHELL (submenu), item);
				gtk_widget_show (item);
			}
			ilist = ilist->next;
		}

		gtk_menu_item_set_submenu (GTK_MENU_ITEM (top_item), submenu);

		list = list->next;
	}
}

static void
mg_menu_destroy (GtkMenuShell *menushell, GtkWidget *menu)
{
	gtk_widget_destroy (menu);
}

static void
mg_create_icon_item (char *label, char *stock, GtkWidget *menu,
							void *callback, void *userdata)
{
	GtkWidget *item;

	item = create_icon_menu (label, stock, TRUE);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (callback),
							userdata);
	gtk_widget_show (item);
}

static void
mg_quit_cb (GtkDialog *dialog, gint arg1, gpointer userdata)
{
	gtk_widget_destroy (GTK_WIDGET (dialog));
	if (arg1 == GTK_RESPONSE_YES)
		mg_safe_quit ();
}

static void
mg_close_sess (session *sess)
{
	GtkWidget *dialog;

	if (sess_list->next == NULL)
	{
		dialog = gtk_message_dialog_new (NULL, 0,
										GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO,
										_("No other tabs open, quit xchat?"));
		g_signal_connect (G_OBJECT (dialog), "response",
								G_CALLBACK (mg_quit_cb), NULL);
		gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
		gtk_widget_show (dialog);
		return;
	}

	fe_close_window (sess);
}

static void
mg_detach_tab_cb (GtkWidget *item, session *sess)
{
	mg_link_irctab (sess, 0);
}

static void
mg_destroy_tab_cb (GtkWidget *item, GtkWidget *tab)
{
	session *sess;

	sess = g_object_get_data (G_OBJECT (tab), "sess");
	if (sess)
		mg_close_sess (sess);
	else
		gtk_widget_destroy (tab);
}

static void
mg_color_insert (GtkWidget *item, gpointer userdata)
{
	char buf[32];

	sprintf (buf, "\003%02d", GPOINTER_TO_INT (userdata));
	key_action_insert (current_sess->gui->input_box, 0, buf, 0, 0);
}

static void
mg_create_color_menu (GtkWidget *menu, session *sess)
{
	GtkWidget *item;
	GtkWidget *submenu;
	char buf[256];
	int i;

	item = gtk_menu_item_new_with_label (_("Insert color code"));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	gtk_widget_show (item);

	submenu = gtk_menu_new ();
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);
	gtk_widget_show (submenu);

	for (i = 0; i < 16; i++)
	{
		item = gtk_menu_item_new_with_label ("");
		sprintf (buf, "<sup>%02d</sup> <span background=\"#%02x%02x%02x\">"
					"            </span>",
				i, colors[i].red >> 8, colors[i].green >> 8, colors[i].blue >> 8);
		gtk_label_set_markup (GTK_LABEL (GTK_BIN (item)->child), buf);
		g_signal_connect (G_OBJECT (item), "activate",
								G_CALLBACK (mg_color_insert), GINT_TO_POINTER (i));
		gtk_menu_shell_append (GTK_MENU_SHELL (submenu), item);
		gtk_widget_show (item);
	}
}

static gboolean
mg_tab_press_cb (GtkWidget *wid, GdkEventButton *event, session *sess)
{
	GtkWidget *menu, *submenu, *item;

	/* shift-click to close a tab */
	if (event->state & GDK_SHIFT_MASK)
	{
		if (sess)
			mg_close_sess (sess);
		else
			gtk_widget_destroy (wid);
		return FALSE;
	}

	if (event->button != 3)
		return FALSE;

	menu = gtk_menu_new ();

	if (sess)
	{
		item = gtk_menu_item_new_with_label (sess->channel[0] ? sess->channel : _("<none>"));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

		submenu = gtk_menu_new ();

		menu_toggle_item (_("Beep on message"), submenu, mg_beepmsg_cb, sess,
								sess->beep);
		if (sess->type == SESS_CHANNEL)
			menu_toggle_item (_("Show join/part messages"), submenu, mg_hidejp_cb,
									sess, !sess->hide_join_part);
		menu_toggle_item (_("Color paste"), submenu, mg_colorpaste_cb, sess,
								sess->color_paste);

		gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);
		gtk_widget_show (item);
	}

	if (sess)
		mg_create_color_menu (menu, sess);

	item = gtk_menu_item_new_with_label (_("Go to"));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	submenu = gtk_menu_new ();
	mg_create_sess_tree (submenu);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);
	gtk_widget_show (item);

	/* separator */
	item = gtk_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	gtk_widget_show (item);

	mg_create_icon_item (_("Close Tab"), GTK_STOCK_CLOSE, menu,
								mg_destroy_tab_cb, wid);
	if (sess)
		mg_create_icon_item (_("Detach Tab"), GTK_STOCK_REDO, menu,
									mg_detach_tab_cb, sess);

	if (sess && tabmenu_list)
		menu_create (menu, tabmenu_list, sess->channel, FALSE);

	g_signal_connect (G_OBJECT (menu), "selection-done",
							G_CALLBACK (mg_menu_destroy), menu);
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, event->time);
	return TRUE;
}

/* add a tabbed channel */

static void
mg_add_chan (session *sess)
{
	char *name = _("<none>");

	if (sess->channel[0])
		name = sess->channel;

	sess->res->tab = tab_group_add (sess->gui->tabs_box, name, sess->server,
											  sess, mg_tab_press_cb, mg_link_cb,
											  prefs.truncchans, prefs.tab_dnd);
	if (plain_list == NULL)
		mg_create_tab_colors ();

	tab_set_attrlist (sess->res->tab, plain_list);
	g_object_set_data (G_OBJECT (sess->res->tab), "sess", sess);

	g_signal_connect (G_OBJECT (sess->res->tab), "destroy",
					      G_CALLBACK (mg_tabdestroy_cb), sess);

	if (sess->res->buffer == NULL)
	{
		sess->res->buffer = gtk_xtext_buffer_new (GTK_XTEXT (sess->gui->xtext));
		gtk_xtext_set_time_stamp (sess->res->buffer, prefs.timestamp);
		sess->res->user_model = userlist_create_model ();
	}
}

static void
mg_userlist_button (GtkWidget * box, char *label, char *cmd,
						  int a, int b, int c, int d)
{
	GtkWidget *wid = gtk_button_new_with_label (label);
	g_signal_connect (G_OBJECT (wid), "clicked",
							G_CALLBACK (userlist_button_cb), cmd);
	gtk_table_attach_defaults (GTK_TABLE (box), wid, a, b, c, d);
	show_and_unfocus (wid);
}

static GtkWidget *
mg_create_userlistbuttons (GtkWidget *box)
{
	struct popup *pop;
	GSList *list = button_list;
	int a = 0, b = 0;
	GtkWidget *tab;

	tab = gtk_table_new (5, 2, FALSE);
	gtk_box_pack_end (GTK_BOX (box), tab, FALSE, FALSE, 0);

	while (list)
	{
		pop = list->data;
		if (pop->cmd[0])
		{
			mg_userlist_button (tab, pop->name, pop->cmd, a, a + 1, b, b + 1);
			a++;
			if (a == 2)
			{
				a = 0;
				b++;
			}
		}
		list = list->next;
	}

	return tab;
}

void
mg_chanmodebuttons_showhide (session *sess, int show)
{
	switch (sess->type)
	{
	case SESS_SERVER:
	case SESS_DIALOG:
		show = FALSE;
	}

	if (show)
		gtk_widget_show (sess->gui->topicbutton_box);
	else
		gtk_widget_hide (sess->gui->topicbutton_box);
}

void
mg_userlist_showhide (session *sess, int show)
{
	session_gui *gui = sess->gui;

	if (!show || prefs.hideuserlist)
	{
		if (gui->pane)
			gtk_paned_set_position (GTK_PANED (gui->pane), 9999);
		gtk_widget_hide (gui->user_box);
		sess->gui->ul_hidden = 1;
	} else
	{
		if (gui->pane)
			gtk_paned_set_position (GTK_PANED (gui->pane), gui->pane_pos);
		gtk_widget_show (gui->user_box);
		sess->gui->ul_hidden = 0;
	}
}

static void
mg_userlist_toggle_cb (GtkWidget *button, gpointer userdata)
{
	session_gui *gui = current_sess->gui;

	if (GTK_WIDGET_VISIBLE (gui->user_box))
	{
		prefs.hideuserlist = 1;
		mg_userlist_showhide (current_sess, FALSE);
	} else
	{
		prefs.hideuserlist = 0;
		mg_userlist_showhide (current_sess, TRUE);
	}

	gtk_widget_grab_focus (gui->input_box);
}

static void
mg_topic_cb (GtkWidget *entry, gpointer userdata)
{
	session *sess = current_sess;
	char *text;

	if (sess->channel[0] && sess->server->connected && sess->type == SESS_CHANNEL)
	{
		text = GTK_ENTRY (entry)->text;
		if (text[0] == 0)
			text = NULL;
		sess->server->p_topic (sess->server, sess->channel, text);
	} else
		gtk_entry_set_text (GTK_ENTRY (entry), "");
	/* restore focus to the input widget, where the next input will most
likely be */
	gtk_widget_grab_focus (sess->gui->input_box);
}

static void
mg_dcc_cb (GtkDialog *dialog, gint arg1, gpointer userdata)
{
	gtk_widget_destroy (GTK_WIDGET (dialog));
	if (arg1 == GTK_RESPONSE_YES)
		xchat_exit ();
}

static int
mg_dcc_active (void)
{
	GSList *list;
	struct DCC *dcc;

	list = dcc_list;
	while (list)
	{
		dcc = list->data;
		if (dcc->dccstat == STAT_ACTIVE)
			return 1;
		list = list->next;
	}

	return 0;
}

void
mg_safe_quit (void)
{
	GtkWidget *dialog;

	if (mg_dcc_active ())
	{
		dialog = gtk_message_dialog_new (NULL, 0,
									GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO,
								_("Some file transfers still active, quit xchat?"));
		g_signal_connect (G_OBJECT (dialog), "response",
								G_CALLBACK (mg_dcc_cb), NULL);
		gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
		gtk_widget_show (dialog);
	} else
	{
/*		printf("no active DCCs, quiting\n");*/
		xchat_exit ();
	}
}

void
mg_x_click_cb (GtkWidget *button, gpointer userdata)
{
	int page;

	/* this, to handle CTRL-W while in non-irc tab */
	if (current_sess && current_sess->gui->is_tab && mg_gui)
	{
		page = gtk_notebook_get_current_page (GTK_NOTEBOOK (mg_gui->note_book));
		if (page != 0)
		{
			/* destroy non-irc tab (when called from menu.c) */
			gtk_widget_destroy (tab_group_get_focused (mg_gui->tabs_box));
			return;
		}
	}

	/* is it a non-irc tab? */
	if (userdata)
	{
		gtk_widget_destroy (userdata);
		return;
	}

	mg_close_sess (current_sess);
}

static void
mg_changui_destroy (session *sess)
{
	if (sess->gui->is_tab)
	{
		/* avoid calling the "destroy" callback */
		g_signal_handlers_disconnect_by_func (G_OBJECT (sess->res->tab),
														  mg_tabdestroy_cb, sess);
		tab_remove (sess->res->tab);
		/* any tabs left? */
		if (tab_group_get_size (sess->gui->tabs_box) < 1)
			/* if not, destroy the main tab window */
			gtk_widget_destroy (mg_gui->window);
	} else
	{
		/* avoid calling the "destroy" callback */
		g_signal_handlers_disconnect_by_func (G_OBJECT (sess->gui->window),
														  mg_topdestroy_cb, sess);
		gtk_widget_destroy (sess->gui->window);
		free (sess->gui);
		sess->gui = NULL;
	}
}

static void
mg_link_irctab (session *sess, int focus)
{
	if (sess->gui->is_tab)
	{
		mg_changui_destroy (sess);
		mg_changui_new (sess, sess->res, 0, focus);
		mg_populate (sess);
		xchat_is_quitting = FALSE;
		return;
	}

	mg_unpopulate (sess);
	mg_changui_destroy (sess);
	mg_changui_new (sess, sess->res, 1, focus);
}

static void
mg_link_gentab (GtkWidget *tab)
{
#if 0
	GtkWidget *win, *box, *vbox;

	box = g_object_get_data (G_OBJECT (tab), "box");
	gtk_widget_reparent (box, vbox);
#endif
}

void
mg_link_cb (GtkWidget *but, gpointer userdata)
{
	if (userdata)
	{
		mg_link_gentab (userdata);
		return;
	}

	mg_link_irctab (current_sess, 0);
}

static int
check_is_number (char *t)
{
	while (*t)
	{
		if (*t < '0' || *t > '9')
			return FALSE;
		t++;
	}
	return TRUE;
}

static void
mg_change_flag (GtkWidget * wid, session *sess, char flag)
{
	server *serv = sess->server;
	char mode[3];

	mode[1] = flag;
	mode[2] = '\0';
	if (serv->connected && sess->channel[0])
	{
		if (GTK_TOGGLE_BUTTON (wid)->active)
			mode[0] = '+';
		else
			mode[0] = '-';
		serv->p_mode (serv, sess->channel, mode);
		serv->p_join_info (serv, sess->channel);
		sess->ignore_mode = TRUE;
		sess->ignore_date = TRUE;
	}
}

static void
flagl_hit (GtkWidget * wid, struct session *sess)
{
	char modes[512];
	const char *limit_str;
	server *serv = sess->server;

	if (GTK_TOGGLE_BUTTON (wid)->active)
	{
		if (serv->connected && sess->channel[0])
		{
			limit_str = gtk_entry_get_text (GTK_ENTRY (sess->gui->limit_entry));
			if (check_is_number ((char *)limit_str) == FALSE)
			{
				gtkutil_simpledialog (_("User limit must be a number!\n"));
				gtk_entry_set_text (GTK_ENTRY (sess->gui->limit_entry), "");
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wid), FALSE);
				return;
			}
			snprintf (modes, sizeof (modes), "+l %d", atoi (limit_str));
			serv->p_mode (serv, sess->channel, modes);
			serv->p_join_info (serv, sess->channel);
		}
	} else
		mg_change_flag (wid, sess, 'l');
}

static void
flagk_hit (GtkWidget * wid, struct session *sess)
{
	char modes[512];
	server *serv = sess->server;

	if (serv->connected && sess->channel[0])
	{
		snprintf (modes, sizeof (modes), "-k %s", 
			  gtk_entry_get_text (GTK_ENTRY (sess->gui->key_entry)));

		if (GTK_TOGGLE_BUTTON (wid)->active)
			modes[0] = '+';

		serv->p_mode (serv, sess->channel, modes);
	}
}

static void
mg_flagbutton_cb (GtkWidget *but, char *flag)
{
	session *sess;
	char mode;

	if (ignore_chanmode)
		return;

	sess = current_sess;
	mode = tolower (flag[0]);

	switch (mode)
	{
	case 'l':
		flagl_hit (but, sess);
		break;
	case 'k':
		flagk_hit (but, sess);
		break;
	case 'b':
		ignore_chanmode = TRUE;
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sess->gui->flag_b), FALSE);
		ignore_chanmode = FALSE;
		banlist_opengui (sess);
		break;
	default:
		mg_change_flag (but, sess, mode);
	}
}

static GtkWidget *
mg_create_flagbutton (char *tip, GtkWidget *box, char *face)
{
	GtkWidget *wid;

	wid = gtk_toggle_button_new_with_label (face);
	gtk_widget_set_size_request (wid, 18, 0);
	add_tip (wid, tip);
	gtk_box_pack_start (GTK_BOX (box), wid, 0, 0, 0);
	g_signal_connect (G_OBJECT (wid), "toggled",
							G_CALLBACK (mg_flagbutton_cb), face);
	show_and_unfocus (wid);

	return wid;
}

static void
mg_key_entry_cb (GtkWidget * igad, gpointer userdata)
{
	char modes[512];
	session *sess = current_sess;
	server *serv = sess->server;

	if (serv->connected && sess->channel[0])
	{
		snprintf (modes, sizeof (modes), "+k %s",
				gtk_entry_get_text (GTK_ENTRY (igad)));
		serv->p_mode (serv, sess->channel, modes);
		serv->p_join_info (serv, sess->channel);
	}
}

static void
mg_limit_entry_cb (GtkWidget * igad, gpointer userdata)
{
	char modes[512];
	session *sess = current_sess;
	server *serv = sess->server;

	if (serv->connected && sess->channel[0])
	{
		if (check_is_number ((char *)gtk_entry_get_text (GTK_ENTRY (igad))) == FALSE)
		{
			gtk_entry_set_text (GTK_ENTRY (igad), "");
			gtkutil_simpledialog (_("User limit must be a number!\n"));
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sess->gui->flag_l), FALSE);
			return;
		}
		snprintf (modes, sizeof(modes), "+l %d", 
				atoi (gtk_entry_get_text (GTK_ENTRY (igad))));
		serv->p_mode (serv, sess->channel, modes);
		serv->p_join_info (serv, sess->channel);
	}
}

static void
mg_apply_entry_style (GtkWidget *entry)
{
	gtk_widget_modify_base (entry, GTK_STATE_NORMAL, &colors[COL_BG]);
	gtk_widget_modify_text (entry, GTK_STATE_NORMAL, &colors[COL_FG]);
	gtk_widget_modify_font (entry, input_style->font_desc);
}

static void
mg_create_chanmodebuttons (session_gui *gui, GtkWidget *box)
{
	gui->flag_t = mg_create_flagbutton (_("Topic Protection"), box, "T");
	gui->flag_n = mg_create_flagbutton (_("No outside messages"), box, "N");
	gui->flag_s = mg_create_flagbutton (_("Secret"), box, "S");
	gui->flag_i = mg_create_flagbutton (_("Invite Only"), box, "I");
	gui->flag_p = mg_create_flagbutton (_("Private"), box, "P");
	gui->flag_m = mg_create_flagbutton (_("Moderated"), box, "M");
	gui->flag_b = mg_create_flagbutton (_("Ban List"), box, "B");

	gui->flag_k = mg_create_flagbutton (_("Keyword"), box, "K");
	gui->key_entry = gtk_entry_new ();
	gtk_widget_set_name (gui->key_entry, "xchat-inputbox");
	gtk_entry_set_max_length (GTK_ENTRY (gui->key_entry), 16);
	gtk_widget_set_size_request (gui->key_entry, 30, -1);
	gtk_box_pack_start (GTK_BOX (box), gui->key_entry, 0, 0, 0);
	g_signal_connect (G_OBJECT (gui->key_entry), "activate",
							G_CALLBACK (mg_key_entry_cb), NULL);

	if (prefs.style_inputbox)
		mg_apply_entry_style (gui->key_entry);

	gui->flag_l = mg_create_flagbutton (_("User Limit"), box, "L");
	gui->limit_entry = gtk_entry_new ();
	gtk_widget_set_name (gui->limit_entry, "xchat-inputbox");
	gtk_entry_set_max_length (GTK_ENTRY (gui->limit_entry), 10);
	gtk_widget_set_size_request (gui->limit_entry, 30, -1);
	gtk_box_pack_start (GTK_BOX (box), gui->limit_entry, 0, 0, 0);
	g_signal_connect (G_OBJECT (gui->limit_entry), "activate",
							G_CALLBACK (mg_limit_entry_cb), NULL);

	if (prefs.style_inputbox)
		mg_apply_entry_style (gui->limit_entry);
}

static void
mg_create_link_buttons (GtkWidget *box, gpointer userdata)
{
	gtkutil_button (box, GTK_STOCK_CLOSE, _("Close this tab/window"),
						 mg_x_click_cb, userdata, 0);

	/*if (!userdata)
	gtkutil_button (box, GTK_STOCK_REDO, _("Attach/Detach this tab"),
						 mg_link_cb, userdata, 0);*/
}

static void
mg_dialog_button_cb (GtkWidget *wid, char *cmd)
{
	/* the longest cmd is 12, and the longest nickname is 64 */
	char buf[128];
	char *host = "";
	char *topic;

	if (!current_sess)
		return;

	topic = (char *)(GTK_ENTRY (current_sess->gui->topic_entry)->text);
	topic = strrchr (topic, '@');
	if (topic)
		host = topic + 1;

	auto_insert (buf, sizeof (buf), cmd, 0, 0, "", "", "", host, "", current_sess->channel);

	handle_command (current_sess, buf, TRUE);

	/* dirty trick to avoid auto-selection */
	GTK_ENTRY (current_sess->gui->input_box)->editable = 0;
	gtk_widget_grab_focus (current_sess->gui->input_box);
	gtk_editable_set_editable (GTK_EDITABLE (current_sess->gui->input_box), TRUE);
}

static void
mg_dialog_button (GtkWidget *box, char *name, char *cmd)
{
	GtkWidget *wid;

	wid = gtk_button_new_with_label (name);
	gtk_box_pack_start (GTK_BOX (box), wid, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (wid), "clicked",
							G_CALLBACK (mg_dialog_button_cb), cmd);
	gtk_widget_set_size_request (wid, -1, 0);
}

static void
mg_create_dialogbuttons (GtkWidget *box)
{
	struct popup *pop;
	GSList *list = dlgbutton_list;

	while (list)
	{
		pop = list->data;
		if (pop->cmd[0])
			mg_dialog_button (box, pop->name, pop->cmd);
		list = list->next;
	}
}

static void
mg_create_topicbar (session *sess, GtkWidget *box, char *name)
{
	GtkWidget *hbox, *topic, *bbox;
	session_gui *gui = sess->gui;

	gui->topic_bar = hbox = gtk_hbox_new (FALSE, 1);
	gtk_box_pack_start (GTK_BOX (box), hbox, 0, 0, 0);

	mg_create_link_buttons (hbox, NULL);

	if (!gui->is_tab)
	{
		sess->res->tab = gtk_toggle_button_new_with_label (name);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sess->res->tab), TRUE);
		gtk_box_pack_start (GTK_BOX (hbox), sess->res->tab, 0, 0, 0);
	}

	gui->topic_entry = topic = gtk_entry_new ();
	gtk_widget_set_name (topic, "xchat-inputbox");
	gtk_container_add (GTK_CONTAINER (hbox), topic);
	g_signal_connect (G_OBJECT (topic), "activate",
							G_CALLBACK (mg_topic_cb), 0);

	if (prefs.style_inputbox)
		mg_apply_entry_style (topic);

	gui->topicbutton_box = bbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), bbox, 0, 0, 0);
	mg_create_chanmodebuttons (gui, bbox);

	gui->dialogbutton_box = bbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), bbox, 0, 0, 0);
	mg_create_dialogbuttons (bbox);

	if (!prefs.paned_userlist)
		gtkutil_button (hbox, GTK_STOCK_GOTO_LAST, _("Show/Hide userlist"),
							 mg_userlist_toggle_cb, 0, 0);
}

/* check if a word is clickable */

static int
mg_word_check (GtkWidget * xtext, char *word)
{
	return text_word_check (word);	/* common/text.c */
}

/* mouse click inside text area */

static void
mg_word_clicked (GtkWidget *xtext, char *word, GdkEventButton *even,
					  session *sess)
{
	sess = current_sess;

	if (even->button == 1)			/* left button */
	{
		if (word == NULL)
		{
			mg_focus (sess);
			return;
		}

		if (even->state & GDK_CONTROL_MASK)
		{
			switch (mg_word_check (xtext, word))
			{
			case WORD_URL:
			case WORD_HOST:
				fe_open_url (word);
			}
		}
		return;
	}

	if (even->button == 2)
	{
		if (sess->type == SESS_DIALOG)
			menu_middlemenu (sess, even);
		else if (even->type == GDK_2BUTTON_PRESS)
			userlist_select (sess, word);
		return;
	}

	switch (mg_word_check (xtext, word))
	{
	case 0:
		menu_middlemenu (sess, even);
		break;
	case WORD_URL:
	case WORD_HOST:
		menu_urlmenu (even, word);
		break;
	case WORD_NICK:
		menu_nickmenu (sess, even, (word[0]=='@' || word[0]=='+') ?
			word+1 : word, FALSE);
		break;
	case WORD_CHANNEL:
		if (*word == '@' || *word == '+' || *word=='^' || *word=='%' || *word=='*')
			word++;
		menu_chanmenu (sess, even, word);
		break;
	case WORD_EMAIL:
		{
			char *newword = malloc (strlen (word) + 10);
			if (*word == '~')
				word++;
			sprintf (newword, "mailto:%s", word);
			menu_urlmenu (even, newword);
			free (newword);
		}
		break;
	case WORD_DIALOG:
		menu_nickmenu (sess, even, sess->channel, FALSE);
		break;
	}
}

void
mg_update_xtext (GtkWidget *wid)
{
	GtkXText *xtext = GTK_XTEXT (wid);

	gtk_xtext_set_palette (xtext, colors);
	gtk_xtext_set_max_lines (xtext, prefs.max_lines);
	gtk_xtext_set_tint (xtext, prefs.tint_red, prefs.tint_green, prefs.tint_blue);
	gtk_xtext_set_background (xtext, channelwin_pix, prefs.transparent, prefs.tint);
	gtk_xtext_set_wordwrap (xtext, prefs.wordwrap);
	gtk_xtext_set_show_marker (xtext, prefs.show_marker);
	gtk_xtext_set_show_separator (xtext, prefs.indent_nicks ? prefs.show_separator : 0);
	gtk_xtext_set_indent (xtext, prefs.indent_nicks);
	if (!gtk_xtext_set_font (xtext, prefs.font_normal))
	{
		fe_message ("Failed to open any font. I'm out of here!", TRUE);
		exit (1);
	}

	gtk_xtext_refresh (xtext, FALSE);
}

/* handle errors reported by xtext */

static void
mg_xtext_error (int type)
{
	switch (type)
	{
	case 0:
		gtkutil_simpledialog (_("Unable to set transparent background!\n\n"
										"You may be using a non-compliant window\n"
										"manager that is not currently supported.\n"));
		prefs.transparent = 0;
		/* no others exist yet */
	}
}

static void
mg_create_textarea (session_gui *gui, GtkWidget *box)
{
	GtkWidget *inbox, *hbox, *frame;
	GtkXText *xtext;

	hbox = gtk_vbox_new (FALSE, 1);
	gtk_container_add (GTK_CONTAINER (box), hbox);

	inbox = gtk_hbox_new (FALSE, 1);
	gtk_container_add (GTK_CONTAINER (hbox), inbox);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (inbox), frame);

	gui->xtext = gtk_xtext_new (colors, TRUE);
	xtext = GTK_XTEXT (gui->xtext);
	gtk_xtext_set_max_indent (xtext, prefs.max_auto_indent);
	gtk_xtext_set_thin_separator (xtext, prefs.thin_separator);
	gtk_xtext_set_error_function (xtext, mg_xtext_error);
	gtk_xtext_set_urlcheck_function (xtext, mg_word_check);
	gtk_xtext_set_max_lines (xtext, prefs.max_lines);
	gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (xtext));
	mg_update_xtext (GTK_WIDGET (xtext));

	g_signal_connect (G_OBJECT (xtext), "word_click",
							G_CALLBACK (mg_word_clicked), NULL);

	gui->vscrollbar = gtk_vscrollbar_new (GTK_XTEXT (xtext)->adj);
	gtk_box_pack_start (GTK_BOX (inbox), gui->vscrollbar, FALSE, TRUE, 1);
}

static GtkWidget *
mg_create_infoframe (GtkWidget *box)
{
	GtkWidget *frame, *label, *hbox;

	frame = gtk_frame_new (0);
	gtk_frame_set_shadow_type ((GtkFrame*)frame, GTK_SHADOW_OUT);
	gtk_container_add (GTK_CONTAINER (box), frame);

	hbox = gtk_hbox_new (0, 0);
	gtk_container_add (GTK_CONTAINER (frame), hbox);

	label = gtk_label_new (NULL);
	gtk_container_add (GTK_CONTAINER (hbox), label);

	return label;
}

static void
mg_create_meters (session_gui *gui, GtkWidget *box)
{
	GtkWidget *infbox = box, *wid;

	if ((prefs.lagometer & 2) || (prefs.throttlemeter & 2))
	{
		infbox = gtk_hbox_new (0, 0);
		gtk_box_pack_start (GTK_BOX (box), infbox, 0, 0, 0);
	}

	if (prefs.lagometer & 1)
	{
		gui->lagometer = wid = gtk_progress_bar_new ();
		gtk_widget_set_size_request (wid, 1, 8);
		gtk_box_pack_start (GTK_BOX (box), wid, 0, 0, 0);
	}
	if (prefs.lagometer & 2)
	{
		gui->laginfo = wid = mg_create_infoframe (infbox);
		gtk_label_set_text ((GtkLabel *) wid, "Lag");
	}

	if (prefs.throttlemeter & 1)
	{
		gui->throttlemeter = wid = gtk_progress_bar_new ();
		gtk_widget_set_size_request (wid, 1, 8);
		gtk_box_pack_start (GTK_BOX (box), wid, 0, 0, 0);
	}
	if (prefs.throttlemeter & 2)
	{
		gui->throttleinfo = wid = mg_create_infoframe (infbox);
		gtk_label_set_text ((GtkLabel *) wid, "Throttle");
	}
}

static void
mg_create_userlist (session_gui *gui, GtkWidget *box, int pack)
{
	GtkWidget *frame, *ulist, *vbox;

	vbox = gtk_vbox_new (0, 2);
	if (pack)
		gtk_box_pack_start (GTK_BOX (box), vbox, 0, 0, 0);
	else
		gtk_container_add (GTK_CONTAINER (box), vbox);

	frame = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (vbox), frame, 0, 0, 0);

	gui->namelistinfo = gtk_label_new (NULL);
	gtk_container_add (GTK_CONTAINER (frame), gui->namelistinfo);

	gui->user_box = vbox;

	gui->user_tree = ulist = userlist_create (vbox);
	gtk_widget_set_size_request (ulist, 86, -1);

	if (prefs.style_namelistgad)
	{
		gtk_widget_set_style (ulist, input_style);
		gtk_widget_modify_base (ulist, GTK_STATE_NORMAL, &colors[COL_BG]);
	}

	mg_create_meters (gui, vbox);

	gui->button_box_parent = vbox;
	gui->button_box = mg_create_userlistbuttons (vbox);
}

static void
mg_create_center (session *sess, session_gui *gui, GtkWidget *box)
{
	GtkWidget *vbox, *hbox, *paned;

	hbox = gtk_hbox_new (FALSE, 3);

	if (prefs.paned_userlist)
	{
		gui->pane = paned = gtk_hpaned_new ();
		gtk_paned_pack1 (GTK_PANED (paned), hbox, TRUE, TRUE);

		vbox = gtk_vbox_new (FALSE, 1);
		gtk_container_add (GTK_CONTAINER (hbox), vbox);
		mg_create_topicbar (sess, vbox, NULL);

		gtk_container_add (GTK_CONTAINER (box), paned);

		mg_create_textarea (gui, vbox);
		mg_create_entry (sess, vbox);

		hbox = gtk_hbox_new (FALSE, 1);
		gtk_paned_pack2 (GTK_PANED (paned), hbox, FALSE, TRUE);

		mg_create_userlist (gui, hbox, FALSE);

		if (gui->pane)
		{
			if (gui->pane_pos == 0)
				gui->pane_pos = prefs.paned_pos;
			if (gui->pane_pos == 0)
				gui->pane_pos = prefs.mainwindow_width - 140;
			gtk_paned_set_position (GTK_PANED (gui->pane), gui->pane_pos);
		}

	} else
	{
		gtk_container_add (GTK_CONTAINER (box), hbox);

		vbox = gtk_vbox_new (FALSE, 1);
		gtk_container_add (GTK_CONTAINER (hbox), vbox);
		mg_create_topicbar (sess, vbox, NULL);

		mg_create_textarea (gui, vbox);
		mg_create_userlist (gui, hbox, TRUE);
		mg_create_entry (sess, vbox);
	}
}

static void
mg_change_nick (int cancel, char *text, gpointer userdata)
{
	char buf[256];

	if (!cancel)
	{
		snprintf (buf, sizeof (buf), "nick %s", text);
		handle_command (current_sess, buf, FALSE);
	}
}

static void
mg_nickclick_cb (GtkWidget *button, gpointer userdata)
{
	fe_get_str (_("Enter new nickname:"), current_sess->server->nick,
					mg_change_nick, NULL);
}

static void
mg_set_tabs_pos (session_gui *gui, int pos)
{
	GtkOrientation orientation;
	GtkWidget *new_tabs_box;

	if (!gui)
	{
		gui = mg_gui;
		if (!gui)
			return;
	}

	gtk_widget_ref (gui->tabs_box);

	if (pos != 4)
		gtk_container_remove (GTK_CONTAINER (gui->main_table), gui->tabs_box);

	orientation = tab_group_get_orientation (gui->tabs_box);
	if ((pos == 0 || pos == 1) && orientation == GTK_ORIENTATION_VERTICAL)
	{
		new_tabs_box = tab_group_set_orientation (gui->tabs_box, FALSE);
		gtk_widget_unref (gui->tabs_box);
		gui->tabs_box = new_tabs_box;
	} else if((pos == 2 || pos == 3) && orientation == GTK_ORIENTATION_HORIZONTAL) {
		new_tabs_box = tab_group_set_orientation (gui->tabs_box, TRUE);
		gtk_widget_unref (gui->tabs_box);
		gui->tabs_box = new_tabs_box;
	}

	gtk_widget_show (gui->tabs_box);

	switch (pos)
	{
	case 0: /* bottom */
		gtk_table_attach (GTK_TABLE (gui->main_table), gui->tabs_box,
							1, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
		break;
	case 1: /* top */
		gtk_table_attach (GTK_TABLE (gui->main_table), gui->tabs_box,
							1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
		break;
	case 2: /* left */
		gtk_table_attach (GTK_TABLE (gui->main_table), gui->tabs_box,
							0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
		break;
	case 3: /* right */
		gtk_table_attach (GTK_TABLE (gui->main_table), gui->tabs_box,
							2, 3, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
		break;
	case 4: /* hidden */
		gtk_widget_hide (gui->tabs_box);
		break;
	}

	if (orientation == tab_group_get_orientation (gui->tabs_box))
		gtk_widget_unref (gui->tabs_box);
}

static void
mg_create_entry (session *sess, GtkWidget *box)
{
	GtkWidget *hbox, *but, *entry;
	session_gui *gui = sess->gui;

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (box), hbox, 0, 0, 0);

	gui->nick_box = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), gui->nick_box, 0, 0, 0);

	gui->nick_label = but = gtk_button_new_with_label (sess->server->nick);
	gtk_button_set_relief (GTK_BUTTON (but), GTK_RELIEF_NONE);
	GTK_WIDGET_UNSET_FLAGS (but, GTK_CAN_FOCUS);
	gtk_box_pack_end (GTK_BOX (gui->nick_box), but, 0, 0, 0);
	g_signal_connect (G_OBJECT (but), "clicked",
							G_CALLBACK (mg_nickclick_cb), NULL);

	gui->input_box = entry = gtk_entry_new ();
	gtk_widget_set_name (entry, "xchat-inputbox");
	gtk_entry_set_max_length (GTK_ENTRY (gui->input_box), 2048);
	g_signal_connect (G_OBJECT (entry), "key_press_event",
							G_CALLBACK (key_handle_key_press), NULL);
	g_signal_connect (G_OBJECT (entry), "focus_in_event",
							G_CALLBACK (mg_inputbox_focus), gui);
	g_signal_connect (G_OBJECT (entry), "activate",
							G_CALLBACK (mg_inputbox_cb), gui);
	gtk_container_add (GTK_CONTAINER (hbox), entry);
	gtk_widget_grab_focus (entry);

	if (prefs.style_inputbox)
		mg_apply_entry_style (entry);
}

static void
mg_switch_tab_cb (GtkWidget *tab, session *sess, gpointer family)
{
	GtkWidget *old;

	old = active_tab;
	active_tab = GTK_WIDGET (tab);

	/* sess of NULL is reserved for non-irc tabs */
	if (sess)
	{
		if (active_tab != old)
		{
			if (old && current_tab)
				mg_unpopulate (current_tab);
			mg_populate (sess);
		}
	} else if (old != active_tab)
		mg_show_generic_tab (active_tab);
}

/* compare two tabs (for tab sorting function) */

static int
mg_tabs_compare (session *a, session *b)
{
	/* this is for "Open Utilities in: Tabs" (i.e. sess is NULL) */
	if (!a)
		return 1;
	if (!b)
		return -1;

	/* server tabs always go first */
	if (a->type == SESS_SERVER)
		return -1;

	/* then channels */
	if (a->type == SESS_CHANNEL && b->type != SESS_CHANNEL)
		return -1;
	if (a->type != SESS_CHANNEL && b->type == SESS_CHANNEL)
		return 1;

	return strcasecmp (a->channel, b->channel);
}

static void
mg_create_tabs (session_gui *gui, GtkWidget *box)
{
	gboolean vert = FALSE;

	if (prefs.tabs_position == 2 || prefs.tabs_position == 3)
		vert = TRUE;

	gui->tabs_box = tab_group_new (mg_switch_tab_cb, mg_tabs_compare,
											 vert, prefs.tab_sort);
	gtk_table_attach (GTK_TABLE (gui->main_table), gui->tabs_box,
						1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
}

static gboolean
mg_tabwin_focus_cb (GtkWindow * win, GdkEventFocus *event, gpointer userdata)
{
	current_sess = current_tab;
	if (current_sess)
	{
		gtk_xtext_check_marker_visibility (GTK_XTEXT (current_sess->gui->xtext));
		plugin_emit_dummy_print (current_sess, "Focus Window");
	}
	return FALSE;
}

static gboolean
mg_topwin_focus_cb (GtkWindow * win, GdkEventFocus *event, session *sess)
{
	current_sess = sess;
	if (!sess->server->server_session)
		sess->server->server_session = sess;
	gtk_xtext_check_marker_visibility(GTK_XTEXT (current_sess->gui->xtext));
	plugin_emit_dummy_print (sess, "Focus Window");
	return FALSE;
}

static void
mg_create_menu (session_gui *gui, GtkWidget *box, int away_state)
{
	GtkAccelGroup *accel_group;

	accel_group = gtk_accel_group_new ();
	gtk_window_add_accel_group (GTK_WINDOW (gtk_widget_get_toplevel (box)),
										 accel_group);
	g_object_unref (accel_group);

	gui->menu = menu_create_main (accel_group, TRUE, away_state, !gui->is_tab,
											&gui->away_item, &gui->user_menu);
	gtk_box_pack_start (GTK_BOX (box), gui->menu, 0, 0, 0);
	gtk_box_reorder_child (GTK_BOX (box), gui->menu, 0);
}

static void
mg_create_topwindow (session *sess)
{
	GtkWidget *win;
	GtkWidget *vbox;
	GtkWidget *vvbox;

	if (sess->type == SESS_DIALOG)
		win = gtkutil_window_new ("X-Chat ["VERSION"]", NULL,
										  prefs.dialog_width, prefs.dialog_height, 0);
	else
		win = gtkutil_window_new ("X-Chat ["VERSION"]", NULL,
										  prefs.mainwindow_width,
										  prefs.mainwindow_height, 0);
	sess->gui->window = win;
	gtk_container_set_border_width (GTK_CONTAINER (win), 2);
	g_signal_connect (G_OBJECT (win), "focus_in_event",
							G_CALLBACK (mg_topwin_focus_cb), sess);
	g_signal_connect (G_OBJECT (win), "destroy",
							G_CALLBACK (mg_topdestroy_cb), sess);
	g_signal_connect (G_OBJECT (win), "configure_event",
							G_CALLBACK (mg_configure_cb), sess);

	palette_alloc (win);

	vbox = gtk_vbox_new (FALSE, 1);
	gtk_container_add (GTK_CONTAINER (win), vbox);

	vvbox = gtk_vbox_new (FALSE, 3);
	gtk_container_add (GTK_CONTAINER (vbox), vvbox);

	mg_create_center (sess, sess->gui, vvbox);
	mg_create_menu (sess->gui, vbox, sess->server->is_away);

	if (sess->res->buffer == NULL)
	{
		sess->res->buffer = gtk_xtext_buffer_new (GTK_XTEXT (sess->gui->xtext));
		gtk_xtext_buffer_show (GTK_XTEXT (sess->gui->xtext), sess->res->buffer, TRUE);
		gtk_xtext_set_time_stamp (sess->res->buffer, prefs.timestamp);
		sess->res->user_model = userlist_create_model ();
	}

	userlist_show (sess);

	gtk_widget_show_all (win);

	if (prefs.hidemenu)
		gtk_widget_hide (sess->gui->menu);

	if (sess->type == SESS_DIALOG)
	{
		/* hide the chan-mode buttons */
		gtk_widget_hide (sess->gui->topicbutton_box);
		/* hide the userlist */
		mg_userlist_showhide (sess, FALSE);
	} else
	{
		gtk_widget_hide (sess->gui->dialogbutton_box);

		if (prefs.hideuserlist)
			mg_userlist_showhide (sess, FALSE);

		if (!prefs.chanmodebuttons)
			gtk_widget_hide (sess->gui->topicbutton_box);
	}

	if (!prefs.userlistbuttons)
		gtk_widget_hide (sess->gui->button_box);

	if (!prefs.topicbar)
		gtk_widget_hide (sess->gui->topic_bar);
}

static void
mg_create_irctab (session *sess, GtkWidget *book)
{
	GtkWidget *vbox;
	session_gui *gui = sess->gui;

	vbox = gtk_vbox_new (FALSE, 3);
	gtk_notebook_append_page (GTK_NOTEBOOK (book), vbox, NULL);

	mg_create_center (sess, gui, vbox);
}

static void
mg_tabwindow_kill_cb (GtkWidget *win, gpointer userdata)
{
	GSList *list;
	session *sess;

/*	puts("enter mg_tabwindow_kill_cb");*/
	xchat_is_quitting = TRUE;

	/* see if there's any non-tab windows left */
	list = sess_list;
	while (list)
	{
		sess = list->data;
		if (!sess->gui->is_tab)
		{
			xchat_is_quitting = FALSE;
/*			puts("-> will not exit, some toplevel windows left");*/
			break;
		}
		list = list->next;
	}

	current_tab = NULL;
	active_tab = NULL;
	mg_gui = NULL;
	parent_window = NULL;
}

static gboolean
mg_tabwindow_de_cb (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	GSList *list;
	session *sess;

	/* check for remaining toplevel windows */
	list = sess_list;
	while (list)
	{
		sess = list->data;
		if (!sess->gui->is_tab)
		{
/*			printf("toplevel's remain, not quiting\n");*/
			return FALSE;
		}
		list = list->next;
	}

	mg_safe_quit ();
	return TRUE;
}

static void
mg_create_tabwindow (session *sess)
{
	GtkWidget *win;
	GtkWidget *vbox;
	GtkWidget *table;
	GtkWidget *book;

	win = gtkutil_window_new ("X-Chat ["VERSION"]", NULL, prefs.mainwindow_width,
									  prefs.mainwindow_height, 0);
	sess->gui->window = win;
	gtk_window_move (GTK_WINDOW (win), prefs.mainwindow_left,
						  prefs.mainwindow_top);
	if (prefs.gui_win_state)
		gtk_window_maximize (GTK_WINDOW (win));
	gtk_container_set_border_width (GTK_CONTAINER (win), 2);

	g_signal_connect (G_OBJECT (win), "delete_event",
						   G_CALLBACK (mg_tabwindow_de_cb), 0);
	g_signal_connect (G_OBJECT (win), "destroy",
						   G_CALLBACK (mg_tabwindow_kill_cb), 0);
	g_signal_connect (G_OBJECT (win), "focus_in_event",
							G_CALLBACK (mg_tabwin_focus_cb), NULL);
	g_signal_connect (G_OBJECT (win), "configure_event",
							G_CALLBACK (mg_configure_cb), NULL);
	g_signal_connect (G_OBJECT (win), "window_state_event",
							G_CALLBACK (mg_windowstate_cb), NULL);

	palette_alloc (win);

	sess->gui->main_vbox = vbox = gtk_vbox_new (FALSE, 1);
	gtk_container_add (GTK_CONTAINER (win), vbox);

	sess->gui->main_table = table = gtk_table_new (3, 3, FALSE);
	gtk_container_add (GTK_CONTAINER (vbox), table);

	sess->gui->note_book = book = gtk_notebook_new ();
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (book), FALSE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (book), FALSE);
	gtk_table_attach (GTK_TABLE (table), book, 1, 2, 1, 2,
						GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

	mg_create_irctab (sess, book);
	mg_create_tabs (sess->gui, vbox);
	mg_create_menu (sess->gui, vbox, sess->server->is_away);

	mg_focus (sess);

	if (prefs.tabs_position != 1)
		mg_set_tabs_pos (sess->gui, prefs.tabs_position);

	if (!prefs.hidemenu)
		gtk_widget_show (sess->gui->menu);

	gtk_widget_show (vbox);
	gtk_widget_show (table);
	gtk_widget_show_all (book);

	if (prefs.hideuserlist)
		mg_userlist_showhide (sess, FALSE);

	if (!prefs.topicbar)
		gtk_widget_hide (sess->gui->topic_bar);

	if (!prefs.chanmodebuttons)
		gtk_widget_hide (sess->gui->topicbutton_box);

	if (!prefs.userlistbuttons)
		gtk_widget_hide (sess->gui->button_box);

	gtk_widget_show (win);
}

void
mg_apply_setup (void)
{
	GSList *list = sess_list;
	session *sess;

	mg_create_tab_colors ();

	while (list)
	{
		sess = list->data;
		gtk_xtext_set_time_stamp (sess->res->buffer, prefs.timestamp);
		((xtext_buffer *)sess->res->buffer)->needs_recalc = TRUE;
		list = list->next;
	}

	if (mg_gui)
		mg_set_tabs_pos (mg_gui, prefs.tabs_position);
}

static GtkWidget *
mg_add_generic_tab (char *name, char *title, void *family, GtkWidget *box)
{
	GtkWidget *but;

	gtk_notebook_append_page (GTK_NOTEBOOK (mg_gui->note_book), box, NULL);
	gtk_widget_show (box);

	but = tab_group_add (mg_gui->tabs_box, name, family, NULL,
								mg_tab_press_cb, mg_link_cb, prefs.truncchans,
								prefs.tab_dnd);
	tab_set_attrlist (but, plain_list);
	g_object_set_data (G_OBJECT (but), "title", strdup (title));
	g_object_set_data (G_OBJECT (but), "box", box);
	g_object_set_data (G_OBJECT (but), "sess", NULL);

	g_signal_connect (G_OBJECT (but), "destroy",
					      G_CALLBACK (mg_tabdestroy_cb), 0);

	if (prefs.newtabstofront)
		tab_focus (but);

	return but;
}

void
fe_buttons_update (session *sess)
{
	session_gui *gui = sess->gui;

	gtk_widget_destroy (gui->button_box);
	gui->button_box = mg_create_userlistbuttons (gui->button_box_parent);

	if (prefs.userlistbuttons)
		gtk_widget_show (sess->gui->button_box);
	else
		gtk_widget_hide (sess->gui->button_box);
}

void
fe_clear_channel (session *sess)
{
	char tbuf[CHANLEN+6];
	session_gui *gui = sess->gui;

	if (sess->waitchannel[0])
	{
		if (prefs.truncchans > 2 && g_utf8_strlen (sess->waitchannel, -1) > prefs.truncchans)
		{
			/* truncate long channel names */
			tbuf[0] = '(';
			strcpy (tbuf + 1, sess->waitchannel);
			g_utf8_offset_to_pointer(tbuf, prefs.truncchans)[0] = 0;
			strcat (tbuf, "..)");
		} else
		{
			sprintf (tbuf, "(%s)", sess->waitchannel);
		}
	}
	else
		strcpy (tbuf, _("<none>"));
	tab_rename (sess->res->tab, tbuf, prefs.truncchans);

	if (!sess->gui->is_tab || sess == current_tab)
	{
		gtk_entry_set_text (GTK_ENTRY (gui->topic_entry), "");

		if (gui->op_xpm)
		{
			gtk_widget_destroy (gui->op_xpm);
			gui->op_xpm = 0;
		}
	} else
	{
		if (sess->res->topic_text)
		{
			free (sess->res->topic_text);
			sess->res->topic_text = NULL;
		}
	}
}

void
fe_set_nonchannel (session *sess, int state)
{
}

void
fe_dlgbuttons_update (session *sess)
{
	GtkWidget *box;
	session_gui *gui = sess->gui;

	gtk_widget_destroy (gui->dialogbutton_box);

	gui->dialogbutton_box = box = gtk_hbox_new (0, 0);
	gtk_box_pack_start (GTK_BOX (gui->topic_bar), box, 0, 0, 0);
	gtk_box_reorder_child (GTK_BOX (gui->topic_bar), box, 3);
	mg_create_dialogbuttons (box);

	gtk_widget_show_all (box);

	if (current_tab && current_tab->type != SESS_DIALOG)
		gtk_widget_hide (current_tab->gui->dialogbutton_box);
}

void
fe_update_mode_buttons (session *sess, char mode, char sign)
{
	int state, i;

	if (sign == '+')
		state = TRUE;
	else
		state = FALSE;

	for (i = 0; i < NUM_FLAG_WIDS - 1; i++)
	{
		if (chan_flags[i] == mode)
		{
			if (!sess->gui->is_tab || sess == current_tab)
			{
				ignore_chanmode = TRUE;
				if (GTK_TOGGLE_BUTTON (sess->gui->flag_wid[i])->active != state)
					gtk_toggle_button_set_active (
							GTK_TOGGLE_BUTTON (sess->gui->flag_wid[i]), state);
				ignore_chanmode = FALSE;
			} else
			{
				sess->res->flag_wid_state[i] = state;
			}
			return;
		}
	}
}

void
fe_set_nick (server *serv, char *newnick)
{
	GSList *list = sess_list;
	session *sess;

	while (list)
	{
		sess = list->data;
		if (sess->server == serv)
		{
			if (current_tab == sess || !sess->gui->is_tab)
				gtk_button_set_label (GTK_BUTTON (sess->gui->nick_label), newnick);
		}
		list = list->next;
	}
}

void
fe_set_away (server *serv)
{
	GSList *list = sess_list;
	session *sess;

	while (list)
	{
		sess = list->data;
		if (sess->server == serv)
		{
			if (!sess->gui->is_tab || sess == current_tab)
				GTK_CHECK_MENU_ITEM (sess->gui->away_item)->active = serv->is_away;
		}
		list = list->next;
	}
}

void
fe_set_channel (session *sess)
{
	tab_rename (sess->res->tab, sess->channel, prefs.truncchans);
}

void
mg_changui_new (session *sess, restore_gui *res, int tab, int focus)
{
	int first_run = FALSE;
	session_gui *gui;
	struct User *user = NULL;

	if (!res)
	{
		res = malloc (sizeof (restore_gui));
		memset (res, 0, sizeof (restore_gui));
	}

	sess->res = res;

	if (!sess->server->front_session)
		sess->server->front_session = sess;

	if (!is_channel (sess->server, sess->channel))
		user = find_name_global (sess->server, sess->channel);

	if (!tab)
	{
		gui = malloc (sizeof (session_gui));
		memset (gui, 0, sizeof (session_gui));
		gui->is_tab = FALSE;
		sess->gui = gui;
		mg_create_topwindow (sess);
		fe_set_title (sess);
		if (user && user->hostname)
			set_topic (sess, user->hostname);
		return;
	}

	if (mg_gui == NULL)
	{
		first_run = TRUE;
		gui = &static_mg_gui;
		memset (gui, 0, sizeof (session_gui));
		gui->is_tab = TRUE;
		sess->gui = gui;
		mg_create_tabwindow (sess);
		mg_gui = gui;
		parent_window = gui->window;
	} else
	{
		sess->gui = gui = mg_gui;
		gui->is_tab = TRUE;
	}

	if (user && user->hostname)
		set_topic (sess, user->hostname);

	mg_add_chan (sess);

	if (first_run || (prefs.newtabstofront == FOCUS_NEW_ONLY_ASKED && focus)
			|| prefs.newtabstofront == FOCUS_NEW_ALL )
		tab_focus (res->tab);

/*	while (g_main_pending ())
		g_main_iteration (TRUE);*/
	/*g_idle_add ((GSourceFunc)tab_group_resize, mg_gui->tabs_box);*/
}

GtkWidget *
mg_create_generic_tab (char *name, char *title, int force_toplevel,
							  int link_buttons,
							  void *close_callback, void *userdata,
							  int width, int height, GtkWidget **vbox_ret,
							  void *family)
{
	GtkWidget *tab, *vbox, *hbox, *win;

	if (force_toplevel || !prefs.windows_as_tabs)
	{
		win = gtkutil_window_new (title, name, width, height, 3);
		vbox = gtk_vbox_new (0, 0);
		*vbox_ret = vbox;
		gtk_container_add (GTK_CONTAINER (win), vbox);
		gtk_widget_show (vbox);
		if (close_callback)
			g_signal_connect (G_OBJECT (win), "destroy",
									G_CALLBACK (close_callback), userdata);
		g_object_set_data (G_OBJECT (win), "box", vbox);
		return win;
	}

	vbox = gtk_vbox_new (0, 0);
	*vbox_ret = vbox;

	if (close_callback)
		g_signal_connect (G_OBJECT (vbox), "destroy",
								G_CALLBACK (close_callback), userdata);

	tab = mg_add_generic_tab (name, title, family, vbox);

	if (link_buttons)
	{
		hbox = gtk_hbox_new (FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), hbox, 0, 0, 0);
		mg_create_link_buttons (hbox, tab);
		gtk_widget_show (hbox);
	}

	return tab;
}

void
mg_topic_showhide (session *sess)
{
	if (GTK_WIDGET_VISIBLE (sess->gui->topic_bar))
	{
		gtk_widget_hide (sess->gui->topic_bar);
		prefs.topicbar = 0;
	} else
	{
		gtk_widget_show (sess->gui->topic_bar);
		prefs.topicbar = 1;
	}
}

void
mg_move_tab (GtkWidget *button, int delta)
{
	tab_move (button, delta);
}

void
mg_move_tab_family (GtkWidget *button, int delta)
{
	tab_family_move (button, delta);
}

void
mg_set_title (GtkWidget *button, char *title)
{
	char *old;

	old = g_object_get_data (G_OBJECT (button), "title");
	g_object_set_data (G_OBJECT (button), "title", strdup (title));
	if (old)
		free (old);
}

void
fe_server_callback (server *serv)
{
	if (serv->gui->chanlist_window)
		gtk_widget_destroy (serv->gui->chanlist_window);

	if (serv->gui->rawlog_window)
		gtk_widget_destroy (serv->gui->rawlog_window);

	free (serv->gui);
}

/* called when a session is being killed */

void
fe_session_callback (session *sess)
{
	if (sess->res->input_text)
		free (sess->res->input_text);

	if (sess->res->topic_text)
		free (sess->res->topic_text);

	if (sess->res->limit_text)
		free (sess->res->limit_text);

	if (sess->res->key_text)
		free (sess->res->key_text);

	if (sess->res->queue_text)
		free (sess->res->queue_text);

	if (sess->res->lag_text)
		free (sess->res->lag_text);

	if (sess->gui->bartag)
		fe_timeout_remove (sess->gui->bartag);

	if (sess->gui != mg_gui)
		free (sess->gui);
	free (sess->res);
}
