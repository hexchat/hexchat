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
 */

#define GTK_DISABLE_DEPRECATED

#include <stdio.h>
#include <string.h>
#include "../../config.h"
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <stdlib.h>
#include <unistd.h>

#include "fe-gtk.h"

#include <gtk/gtkmain.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkprogressbar.h>
#include <gtk/gtkbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtktogglebutton.h>

#include "../common/xchat.h"
#include "../common/fe.h"
#include "../common/util.h"
#include "../common/text.h"
#include "../common/cfgfiles.h"
#include "../common/xchatc.h"
#include "gtkutil.h"
#include "maingui.h"
#include "pixmaps.h"
#include "xtext.h"
#include "palette.h"
#include "menu.h"
#include "notifygui.h"
#include "textgui.h"
#include "fkeys.h"
#include "tabs.h"
#include "urlgrab.h"

#ifdef USE_XLIB
#include <gdk/gdkx.h>
#include <gtk/gtkinvisible.h>
#endif


GdkPixmap *channelwin_pix;


#ifdef USE_XLIB

static void
redraw_trans_xtexts (void)
{
	GSList *list = sess_list;
	session *sess;
	int done_main = FALSE;

	while (list)
	{
		sess = list->data;
#ifdef USE_ZVT
		if (sess->type == SESS_SHELL)
		{
			menu_newshell_set_palette (sess);
			gtk_widget_queue_draw (sess->gui->xtext);
		} else
#endif
		{
			if (GTK_XTEXT (sess->gui->xtext)->transparent)
			{
				if (!sess->gui->is_tab || !done_main)
					gtk_xtext_refresh (GTK_XTEXT (sess->gui->xtext), 1);
				if (sess->gui->is_tab)
					done_main = TRUE;
			}
		}
		list = list->next;
	}
}

static GdkFilterReturn
root_event_cb (GdkXEvent *xev, GdkEventProperty *event, gpointer data)
{
	Atom at = None;
	XEvent *xevent = (XEvent *)xev;

	if (at == None)
		at = XInternAtom (GDK_DISPLAY (), "_XROOTPMAP_ID", True);

	if (xevent->type == PropertyNotify && at == xevent->xproperty.atom)
		redraw_trans_xtexts ();

	return GDK_FILTER_CONTINUE;
}

#endif

int
fe_args (int argc, char *argv[])
{
	int offset = 0;

#ifdef ENABLE_NLS
	bindtextdomain (PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (PACKAGE, "UTF-8");
	textdomain (PACKAGE);
#endif

	if (argc > 1)
	{
		if (!strcasecmp (argv[1], "-v") || !strcasecmp (argv[1], "--version"))
		{
			printf (PACKAGE" "VERSION"\n");
			return 0;
		}
		if (!strcasecmp (argv[1], "-h") || !strcasecmp (argv[1], "--help"))
		{
			printf (PACKAGE" "VERSION"\n"
					"Usage: %s [OPTIONS]... [URL]\n\n", argv[0]);
			printf (_("Options:\n"
					"  -d,  --cfgdir DIRECTORY   use a different config dir\n"
					"  -a,  --no-auto            don't auto connect\n"
					"  -v,  --version            show version information\n\n"
					"URL:\n"
					"  irc://server:port/channel\n\n"));
			return 0;
		}
		if (!strcasecmp (argv[1], "-a") || !strcasecmp (argv[1], "--no-auto"))
		{
			auto_connect = 0;
			offset++;
		}
	}

	if (argc > 2 + offset)
	{
		if (!strcasecmp (argv[1+offset], "-d") || !strcasecmp (argv[1+offset], "--cfgdir"))
		{
			xdir = strdup (argv[2+offset]);
			if (xdir[strlen (xdir) - 1] == '/')
				xdir[strlen (xdir) - 1] = 0;
			offset += 2;
		}
	}

	if (argc > (offset + 1))
		connect_url = strdup (argv[offset + 1]);

	gtk_set_locale ();

	gtk_init (&argc, &argv);

#ifdef USE_XLIB
/*	XSelectInput (GDK_DISPLAY (), GDK_ROOT_WINDOW (), PropertyChangeMask);*/
	gdk_window_set_events (gdk_get_default_root_window (), GDK_PROPERTY_CHANGE_MASK);
	gdk_window_add_filter (gdk_get_default_root_window (),
								  (GdkFilterFunc)root_event_cb, NULL);
#endif

	return 1;
}

static const char rc[] =
	"style \"xc-ib-st\""
	"{"
		"GtkEntry::cursor-color=\"#%02x%02x%02x\""
	"}"
	"widget \"*.xchat-inputbox\" style : application \"xc-ib-st\"";

GtkStyle *
create_input_style (void)
{
	GtkStyle *style;
	char buf[256];
	static int done_rc = FALSE;

	style = gtk_style_new ();
	pango_font_description_free (style->font_desc);
	style->font_desc = pango_font_description_from_string (prefs.font_normal);

	/* fall back */
	if (pango_font_description_get_size (style->font_desc) == 0)
	{
		snprintf (buf, sizeof (buf), _("Failed to open font:\n\n%s"), prefs.font_normal);
		gtkutil_simpledialog (buf);
		pango_font_description_free (style->font_desc);
		style->font_desc = pango_font_description_from_string ("sans 11");
	}

	if (prefs.style_inputbox && !done_rc)
	{
		done_rc = TRUE;
		sprintf (buf, rc, (colors[18].red >> 8), (colors[18].green >> 8),
					(colors[18].blue >> 8));
		gtk_rc_parse_string (buf);
	}

	style->bg[GTK_STATE_NORMAL] = colors[18];
	style->base[GTK_STATE_NORMAL] = colors[19];
	style->text[GTK_STATE_NORMAL] = colors[18];

	return style;
}

void
fe_init (void)
{
	palette_load ();
	key_init ();
	pixmaps_init ();

	channelwin_pix = pixmap_load_from_file (prefs.background);
	input_style = create_input_style ();
}

void
fe_main (void)
{
	gtk_main ();

	/* sleep for 3 seconds so any QUIT messages are not lost. The  */
	/* GUI is closed at this point, so the user doesn't even know! */
	if (prefs.wait_on_exit)
		sleep (3);
}

void
fe_cleanup (void)
{
	palette_save ();
}

void
fe_exit (void)
{
	gtk_main_quit ();
}

int
fe_timeout_add (int interval, void *callback, void *userdata)
{
	return g_timeout_add (interval, (GSourceFunc) callback, userdata);
}

void
fe_timeout_remove (int tag)
{
	g_source_remove (tag);
}

void
fe_new_window (session *sess)
{
	int tab = FALSE;

	if (sess->type == SESS_DIALOG)
	{
		if (prefs.privmsgtab)
			tab = TRUE;
	} else
	{
		if (prefs.tabchannels)
			tab = TRUE;
	}

	mg_changui_new (sess, NULL, tab);
}

void
fe_new_server (struct server *serv)
{
	serv->gui = malloc (sizeof (struct server_gui));
	memset (serv->gui, 0, sizeof (struct server_gui));
}

static void
null_this_var (GtkWidget * unused, GtkWidget ** dialog)
{
	*dialog = 0;
}

void
fe_message (char *msg, int wait)
{
	GtkWidget *dialog;

	dialog = gtkutil_simpledialog (msg);
	if (wait)
	{
		g_signal_connect (G_OBJECT (dialog), "destroy",
								G_CALLBACK (null_this_var), &dialog);
		while (dialog)
			gtk_main_iteration ();
	}
}

void
fe_idle_add (void *func, void *data)
{
	g_idle_add (func, data);
}

void
fe_input_remove (int tag)
{
	g_source_remove (tag);
}

int
fe_input_add (int sok, int flags, void *func, void *data)
{
	int tag, type = 0;
	GIOChannel *channel;

#ifdef WIN32
	if (flags & FIA_FD)
		channel = g_io_channel_win32_new_fd (sok);
	else
		channel = g_io_channel_win32_new_socket (sok);
#else
	channel = g_io_channel_unix_new (sok);
#endif

	if (flags & FIA_READ)
		type |= G_IO_IN | G_IO_HUP | G_IO_ERR;
	if (flags & FIA_WRITE)
		type |= G_IO_OUT | G_IO_ERR;
	if (flags & FIA_EX)
		type |= G_IO_PRI;

	tag = g_io_add_watch (channel, type, (GIOFunc) func, data);
	g_io_channel_unref (channel);

	return tag;
}

void
fe_set_topic (session *sess, char *topic)
{
	if (!sess->gui->is_tab || sess == current_tab)
	{
		gtk_entry_set_text (GTK_ENTRY (sess->gui->topic_entry), topic);
		mg_set_topic_tip (sess);
	} else
	{
		if (sess->res->topic_text)
			free (sess->res->topic_text);
		sess->res->topic_text = strdup (topic);
	}
}

void
fe_set_hilight (struct session *sess)
{
	if (sess->gui->is_tab)
	{
		sess->nick_said = TRUE;
		tab_set_attrlist (sess->res->tab, nickseen_list);
	}
}

static void
fe_update_mode_entry (session *sess, GtkWidget *entry, char **text, char *new_text)
{
	if (!sess->gui->is_tab || sess == current_tab)
	{
		if (sess->gui->flag_wid[0])	/* channel mode buttons enabled? */
			gtk_entry_set_text (GTK_ENTRY (entry), new_text);
	} else
	{
		if (sess->gui->is_tab)
		{
			if (*text)
				free (*text);
			*text = strdup (new_text);
		}
	}
}

void
fe_update_channel_key (struct session *sess)
{
	fe_update_mode_entry (sess, sess->gui->key_entry,
								 &sess->res->key_text, sess->channelkey);
	fe_set_title (sess);
}

void
fe_update_channel_limit (struct session *sess)
{
	char tmp[16];

	sprintf (tmp, "%d", sess->limit);
	fe_update_mode_entry (sess, sess->gui->limit_entry,
								 &sess->res->limit_text, tmp);
	fe_set_title (sess);
}

int
fe_is_chanwindow (struct server *serv)
{
	if (!serv->gui->chanlist_window)
		return 0;
	return 1;
}

int
fe_is_banwindow (struct session *sess)
{
   if (!sess->res->banlist_window)
     return 0;
   return 1;
}

void
fe_chan_list_end (struct server *serv)
{
	gtk_widget_set_sensitive (serv->gui->chanlist_refresh, TRUE);
}

void
fe_notify_update (char *name)
{
	if (name)
		update_all_of (name);
	else
		notify_gui_update ();
}

void
fe_text_clear (struct session *sess)
{
	gtk_xtext_clear (sess->res->buffer);
}

void
fe_close_window (struct session *sess)
{
	if (sess->gui->is_tab)
		gtk_widget_destroy (sess->res->tab);
	else
		gtk_widget_destroy (sess->gui->window);
}

void
fe_progressbar_start (session *sess)
{
	if (!sess->gui->is_tab || current_tab == sess)
	/* if it's the focused tab, create it for real! */
		mg_progressbar_create (sess->gui);
	else
	/* otherwise just remember to create on when it gets focused */
		sess->res->c_graph = TRUE;
}

void
fe_progressbar_end (server *serv)
{
	GSList *list = sess_list;
	session *sess;

	while (list)				  /* check all windows that use this server and  *
									   * remove the connecting graph, if it has one. */
	{
		sess = list->data;
		if (sess->server == serv)
		{
			if (sess->gui->bar)
				mg_progressbar_destroy (sess->gui);
			sess->res->c_graph = FALSE;
		}
		list = list->next;
	}
}

void
fe_print_text (struct session *sess, char *text)
{
	PrintTextRaw (sess->res->buffer, text, prefs.indent_nicks);

/*	if (prefs.limitedtabhighlight && !sess->highlight_tab)
		return;*/

	sess->highlight_tab = FALSE;

	if (!sess->new_data && sess != current_tab &&
		 sess->gui->is_tab && !sess->nick_said)
	{
		sess->new_data = TRUE;
		if (sess->msg_said)
			tab_set_attrlist (sess->res->tab, newmsg_list);
		else
			tab_set_attrlist (sess->res->tab, newdata_list);
	}
}

void
fe_beep (void)
{
	gdk_beep ();
}

typedef struct {
	session *sess;
	char *sstr;
} fe_lastlog_info;

static void
fe_lastlog_foreach (GtkXText *xtext, unsigned char *text, void *data)
{
	fe_lastlog_info *info = data;

	if (nocasestrstr (text, info->sstr))
		PrintText (info->sess, text);
}

void
fe_lastlog (session *sess, session *lastlog_sess, char *sstr)
{
	if (gtk_xtext_is_empty (GTK_XTEXT (sess->gui->xtext)))
	{
		PrintText (lastlog_sess, _("Search buffer is empty.\n"));
	}
	else
	{
		fe_lastlog_info info;

		info.sess = lastlog_sess;
		info.sstr = sstr;
		
		gtk_xtext_foreach (sess->res->buffer, fe_lastlog_foreach, &info);
	}
}

void
fe_set_lag (server *serv, int lag)
{
	GSList *list = sess_list;
	session *sess;
	gdouble per;
	char tip[64];
	unsigned long nowtim;

	if (lag == -1)
	{
		if (!serv->lag_sent)
			return;
		nowtim = make_ping_time ();
		lag = (nowtim - serv->lag_sent) / 100000;
	}

	per = (double)((double)lag / (double)40);
	if (per > 1.0)
		per = 1.0;

	snprintf (tip, sizeof (tip) - 1, "%s%d.%ds",
				 serv->lag_sent ? "+" : "", lag / 10, lag % 10);

	while (list)
	{
		sess = list->data;
		if (sess->server == serv)
		{
			if (!sess->gui->is_tab || current_tab == sess)
			{
				if (sess->gui->lagometer)
					gtk_progress_bar_set_fraction ((GtkProgressBar *) sess->gui->lagometer, per);
				if (sess->gui->laginfo)
					gtk_label_set_text ((GtkLabel *) sess->gui->laginfo, tip);
			} else
			{
				sess->res->lag_value = per;
				if (sess->res->lag_text)
					free (sess->res->lag_text);
				sess->res->lag_text = strdup (tip);
			}
		}
		list = list->next;
	}
}

void
fe_set_throttle (server *serv)
{
	GSList *list = sess_list;
	struct session *sess;
	float per;
	char tbuf[64];

	per = (float) serv->sendq_len / 1024.0;
	if (per > 1.0)
		per = 1.0;

	while (list)
	{
		sess = list->data;
		if (sess->server == serv)
		{
			snprintf (tbuf, sizeof (tbuf) - 1, "%d bytes", serv->sendq_len);

			if (!sess->gui->is_tab || current_tab == sess)
			{
				if (sess->gui->throttlemeter)
					gtk_progress_bar_set_fraction ((GtkProgressBar *) sess->gui->throttlemeter, per);
				if (sess->gui->throttleinfo)
					gtk_label_set_text ((GtkLabel *) sess->gui->throttleinfo, tbuf);
			} else
			{
				sess->res->queue_value = per;
				if (sess->res->queue_text)
					free (sess->res->queue_text);
				sess->res->queue_text = strdup (tbuf);
			}
		}
		list = list->next;
	}
}

void
fe_play_wave (const char *file)
{
    play_wave (file);
}
