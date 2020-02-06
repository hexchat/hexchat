/* X-Chat
 * Copyright (C) 2006-2007 Peter Zelezny.
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

#include <string.h>
#include "../common/hexchat-plugin.h"
#include "../common/hexchat.h"
#include "../common/hexchatc.h"
#include "../common/inbound.h"
#include "../common/server.h"
#include "../common/fe.h"
#include "../common/util.h"
#include "../common/outbound.h"
#include "fe-gtk.h"
#include "pixmaps.h"
#include "maingui.h"
#include "menu.h"
#include "gtkutil.h"

#ifndef WIN32
#include <unistd.h>
#endif

typedef enum	/* current icon status */
{
	TS_NONE,
	TS_MESSAGE,
	TS_HIGHLIGHT,
	TS_FILEOFFER,
	TS_CUSTOM /* plugin */
} TrayStatus;

typedef enum
{
	WS_FOCUSED,
	WS_NORMAL,
	WS_HIDDEN
} WinStatus;

typedef GdkPixbuf* TrayIcon;
#define tray_icon_from_file(f) gdk_pixbuf_new_from_file(f,NULL)
#define tray_icon_free(i) g_object_unref(i)

#define ICON_NORMAL pix_tray_normal
#define ICON_MSG pix_tray_message
#define ICON_HILIGHT pix_tray_highlight
#define ICON_FILE pix_tray_fileoffer
#define TIMEOUT 500

static GtkStatusIcon *sticon;
static GtkWidget *openMenu = NULL;
static gint flash_tag;
static TrayStatus tray_status;
#ifdef WIN32
static guint tray_menu_timer;
static gint64 tray_menu_inactivetime;
#endif
static hexchat_plugin *ph;

static TrayIcon custom_icon1;
static TrayIcon custom_icon2;

static int tray_priv_count = 0;
static int tray_pub_count = 0;
static int tray_hilight_count = 0;
static int tray_file_count = 0;
static int tray_restore_timer = 0;


void tray_apply_setup (void);
static gboolean tray_menu_try_restore (void);
static void tray_cleanup (void);
static void tray_init (void);


void _DebugMessage( const char *Fmt, ... )
{
     va_list vl;
     void *TabArgVar[20];
     int i;

     va_start( vl, Fmt );
     for( i=0; i<20; i++ )
         TabArgVar[i] =  va_arg(vl, void *);
     va_end(vl);

     g_print( " >>> " );

     g_print( Fmt, TabArgVar[0],
                   TabArgVar[1],
                   TabArgVar[2],
                   TabArgVar[3],
                   TabArgVar[4],
                   TabArgVar[5],
                   TabArgVar[6],
                   TabArgVar[7],
                   TabArgVar[8],
                   TabArgVar[9],
                   TabArgVar[10],
                   TabArgVar[11],
                   TabArgVar[12],
                   TabArgVar[13],
                   TabArgVar[14],
                   TabArgVar[15],
                   TabArgVar[16],
                   TabArgVar[17],
                   TabArgVar[18],
                   TabArgVar[19] );

     g_print( "\n" );
}

#define _DEBUGLOG if( 0 ) _DebugMessage


static WinStatus
tray_get_window_status (void)
{
	const char *st;

	st = hexchat_get_info (ph, "win_status");

	if (!st)
		return WS_HIDDEN;

	if (!strcmp (st, "active"))
		return WS_FOCUSED;

	if (!strcmp (st, "hidden"))
		return WS_HIDDEN;

	return WS_NORMAL;
}

static int
tray_count_channels (void)
{
	int cons = 0;
	GSList *list;
	session *sess;

	for (list = sess_list; list; list = list->next)
	{
		sess = list->data;
		if (sess->server->connected && sess->channel[0] &&
			 sess->type == SESS_CHANNEL)
			cons++;
	}
	return cons;
}

static int
tray_count_networks (void)
{
	int cons = 0;
	GSList *list;

	for (list = serv_list; list; list = list->next)
	{
		if (((server *)list->data)->connected)
			cons++;
	}
	return cons;
}

void
fe_tray_set_tooltip (const char *text)
{
	if (sticon)
		gtk_status_icon_set_tooltip_text (sticon, text);
}

static void
tray_set_tipf (const char *format, ...)
{
	va_list args;
	char *buf;

	va_start (args, format);
	buf = g_strdup_vprintf (format, args);
	va_end (args);

	fe_tray_set_tooltip (buf);
	g_free (buf);
}

static void
tray_stop_flash (void)
{
	int nets, chans;

	if (flash_tag)
	{
		g_source_remove (flash_tag);
		flash_tag = 0;
	}

	if (sticon)
	{
		gtk_status_icon_set_from_pixbuf (sticon, ICON_NORMAL);
		nets = tray_count_networks ();
		chans = tray_count_channels ();
		if (nets)
			tray_set_tipf (_("Connected to %u networks and %u channels - %s"),
								nets, chans, _(DISPLAY_NAME));
		else
			tray_set_tipf ("%s - %s", _("Not connected."), _(DISPLAY_NAME));
	}

	if (custom_icon1)
	{
		tray_icon_free (custom_icon1);
		custom_icon1 = NULL;
	}

	if (custom_icon2)
	{
		tray_icon_free (custom_icon2);
		custom_icon2 = NULL;
	}

	tray_status = TS_NONE;
}

static void
tray_reset_counts (void)
{
	tray_priv_count = 0;
	tray_pub_count = 0;
	tray_hilight_count = 0;
	tray_file_count = 0;
}

static int
tray_timeout_cb (TrayIcon icon)
{
	if (custom_icon1)
	{
		if (gtk_status_icon_get_pixbuf (sticon) == custom_icon1)
		{
			if (custom_icon2)
				gtk_status_icon_set_from_pixbuf (sticon, custom_icon2);
			else
				gtk_status_icon_set_from_pixbuf (sticon, ICON_NORMAL);
		}
		else
		{
			gtk_status_icon_set_from_pixbuf (sticon, custom_icon1);
		}
	}
	else
	{
		if (gtk_status_icon_get_pixbuf (sticon) == ICON_NORMAL)
			gtk_status_icon_set_from_pixbuf (sticon, icon);
		else
			gtk_status_icon_set_from_pixbuf (sticon, ICON_NORMAL);
	}
	return 1;
}

static void
tray_set_flash (TrayIcon icon)
{
	if (!sticon)
		return;

	/* already flashing the same icon */
	if (flash_tag && gtk_status_icon_get_pixbuf (sticon) == icon)
		return;

	/* no flashing if window is focused */
	if (tray_get_window_status () == WS_FOCUSED)
		return;

	tray_stop_flash ();

	gtk_status_icon_set_from_pixbuf (sticon, icon);
	if (prefs.hex_gui_tray_blink)
		flash_tag = g_timeout_add (TIMEOUT, (GSourceFunc) tray_timeout_cb, icon);
}

void
fe_tray_set_flash (const char *filename1, const char *filename2, int tout)
{
	tray_apply_setup ();
	if (!sticon)
		return;

	tray_stop_flash ();

	if (tout == -1)
		tout = TIMEOUT;

	custom_icon1 = tray_icon_from_file (filename1);
	if (filename2)
		custom_icon2 = tray_icon_from_file (filename2);

	gtk_status_icon_set_from_pixbuf (sticon, custom_icon1);
	flash_tag = g_timeout_add (tout, (GSourceFunc) tray_timeout_cb, NULL);
	tray_status = TS_CUSTOM;
}

void
fe_tray_set_icon (feicon icon)
{
	tray_apply_setup ();
	if (!sticon)
		return;

	tray_stop_flash ();

	switch (icon)
	{
	case FE_ICON_NORMAL:
		break;
	case FE_ICON_MESSAGE:
	case FE_ICON_PRIVMSG:
		tray_set_flash (ICON_MSG);
		break;
	case FE_ICON_HIGHLIGHT:
		tray_set_flash (ICON_HILIGHT);
		break;
	case FE_ICON_FILEOFFER:
		tray_set_flash (ICON_FILE);
	}
}

void
fe_tray_set_file (const char *filename)
{
	tray_apply_setup ();
	if (!sticon)
		return;

	tray_stop_flash ();

	if (filename)
	{
		custom_icon1 = tray_icon_from_file (filename);
		gtk_status_icon_set_from_pixbuf (sticon, custom_icon1);
		tray_status = TS_CUSTOM;
	}
}

gboolean
tray_toggle_visibility (gboolean force_hide)
{
	static int x, y;
	static GdkScreen *screen;
	static int maximized;
	static int fullscreen;
	GtkWindow *win;

	if (!sticon)
		return FALSE;

	/* ph may have an invalid context now */
	hexchat_set_context (ph, hexchat_find_context (ph, NULL, NULL));

	win = GTK_WINDOW (hexchat_get_info (ph, "gtkwin_ptr"));

	tray_stop_flash ();
	tray_reset_counts ();

	if (!win)
		return FALSE;

	if (force_hide || gtk_widget_get_visible (GTK_WIDGET (win)))
	{
		if (prefs.hex_gui_tray_away)
			hexchat_command (ph, "ALLSERV AWAY");
		gtk_window_get_position (win, &x, &y);
		screen = gtk_window_get_screen (win);
		maximized = prefs.hex_gui_win_state;
		fullscreen = prefs.hex_gui_win_fullscreen;
		gtk_widget_hide (GTK_WIDGET (win));
	}
	else
	{
		if (prefs.hex_gui_tray_away)
			hexchat_command (ph, "ALLSERV BACK");
		gtk_window_set_screen (win, screen);
		gtk_window_move (win, x, y);
		if (maximized)
			gtk_window_maximize (win);
		if (fullscreen)
			gtk_window_fullscreen (win);
		gtk_widget_show (GTK_WIDGET (win));
		gtk_window_deiconify (win);
		gtk_window_present (win);
	}

	return TRUE;
}

static void
tray_menu_restore_cb (GtkWidget *item, gpointer userdata)
{
    _DEBUGLOG( "tray_menu_restore_cb: BEGIN" );
	tray_toggle_visibility (FALSE);
    _DEBUGLOG( "tray_menu_restore_cb: END" );
}

static void
tray_menu_notify_cb (GObject *tray, GParamSpec *pspec, gpointer user_data)
{
    _DEBUGLOG( "tray_menu_notify_cb: BEGIN" );

	if (sticon)
	{
		if (!gtk_status_icon_is_embedded (sticon))
		{
			tray_restore_timer = g_timeout_add(500, (GSourceFunc)tray_menu_try_restore, NULL);
		}
		else
		{
			if (tray_restore_timer)
			{
				g_source_remove (tray_restore_timer);
				tray_restore_timer = 0;
			}
		}
	}

    _DEBUGLOG( "tray_menu_notify_cb: END" );
}

static gboolean
tray_menu_try_restore (void)
{
	tray_cleanup();
	tray_init();
	return TRUE;
}

static void
tray_menu_quit_cb (GtkWidget *item, gpointer userdata)
{
	mg_open_quit_dialog (FALSE);
}

/* returns 0-mixed 1-away 2-back */

static int
tray_find_away_status (void)
{
	GSList *list;
	server *serv;
	int away = 0;
	int back = 0;

	for (list = serv_list; list; list = list->next)
	{
		serv = list->data;

		if (serv->is_away || serv->reconnect_away)
			away++;
		else
			back++;
	}

	if (away && back)
		return 0;

	if (away)
		return 1;

	return 2;
}

static void
tray_foreach_server (GtkWidget *item, char *cmd)
{
	GSList *list;
	server *serv;

	for (list = serv_list; list; list = list->next)
	{
		serv = list->data;
		if (serv->connected)
			handle_command (serv->server_session, cmd, FALSE);
	}
}

static GtkWidget *
tray_make_item (GtkWidget *menu, char *label, void *callback, void *userdata)
{
	GtkWidget *item;

	if (label)
		item = gtk_menu_item_new_with_mnemonic (label);
	else
		item = gtk_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	g_signal_connect (G_OBJECT (item), "activate",
							G_CALLBACK (callback), userdata);
	gtk_widget_show (item);

	return item;
}

#ifndef WIN32
static void
tray_toggle_cb (GtkCheckMenuItem *item, unsigned int *setting)
{
	*setting = gtk_check_menu_item_get_active (item);
}

static void
blink_item (unsigned int *setting, GtkWidget *menu, char *label)
{
	menu_toggle_item (label, menu, tray_toggle_cb, setting, *setting);
}
#endif

static void
tray_menu_destroy (GtkWidget *menu, gpointer userdata)
{
    _DEBUGLOG( "tray_menu_destroy: BEGIN" );

	gtk_widget_destroy (menu);

    if( menu == openMenu )
    {
	    g_object_unref (menu);
        openMenu = NULL;
    }

#ifdef WIN32
	g_source_remove (tray_menu_timer);
#endif

    _DEBUGLOG( "tray_menu_destroy: END" );
}

#ifdef WIN32
static gboolean
tray_menu_enter_cb (GtkWidget *menu)
{
	tray_menu_inactivetime = 0;
	return FALSE;
}

static gboolean
tray_menu_left_cb (GtkWidget *menu)
{
	tray_menu_inactivetime = g_get_real_time ();
	return FALSE;
}

static gboolean
tray_check_hide (GtkWidget *menu)
{
	if (tray_menu_inactivetime && g_get_real_time () - tray_menu_inactivetime  >= 2000000)
	{
		tray_menu_destroy (menu, NULL);
		return G_SOURCE_REMOVE;
	}

	return G_SOURCE_CONTINUE;
}
#endif

static void
tray_menu_settings (GtkWidget * wid, gpointer none)
{
	extern void setup_open (void);
	setup_open ();
}

static void 
osx_event_menu (GtkWidget *widget,
                GdkEvent  *event,
                gpointer   user_data)
{
    GdkEventType type = event->any.type;
    GdkWindow *window = event->any.window;
    gint8 send_event = event->any.send_event;

    GtkWidget *menu =  GTK_WIDGET(user_data);
    GdkWindow *menu_window = gtk_widget_get_window(menu);

    GtkWidget *menu_toplevel = gtk_widget_get_toplevel (menu);
    GdkWindow *menu_toplevel_window = gtk_widget_get_window(menu_toplevel);

    GdkWindow *widget_window = gtk_widget_get_window(widget);

    switch( type )
    {
        case GDK_ENTER_NOTIFY:
            _DEBUGLOG( "osx_event_menu: ENTER window:%08X menu:%08X(%08X/%08X) widget:%08X(%08X)", window, menu, menu_window, menu_toplevel_window, widget, widget_window );
            break;

        case GDK_LEAVE_NOTIFY:
            _DEBUGLOG( "osx_event_menu: LEAVE window:%08X menu:%08X(%08X/%08X) widget:%08X(%08X)", window, menu, menu_window, menu_toplevel_window, widget, widget_window );
            if( window == menu_toplevel_window )
            {
                tray_menu_destroy (menu, NULL);
            }
            break;

        case GDK_MOTION_NOTIFY:
            break;

        case GDK_EXPOSE:
            _DEBUGLOG( "osx_event_menu: GDK_EXPOSE count:%d window:%08X menu:%08X(%08X/%08X) widget:%08X(%08X)", 
                       event->expose.count,
                       window, menu, menu_window, menu_toplevel_window, widget, widget_window );
            if( window == menu_window )
            {
                _DEBUGLOG( "osx_event_menu: 1 for Empty branch of conditional, lgtm-com[bot]!" );
            }
            break;

        case GDK_GRAB_BROKEN:
            _DEBUGLOG( "osx_event_menu: GDK_GRAB_BROKEN keyboard:%s implicit:%s grab_window:%08X window:%08X menu:%08X(%08X/%08X) widget:%08X(%08X)", 
                       ( event->grab_broken.keyboard ? "TRUE" : "FALSE"),
                       ( event->grab_broken.implicit ? "TRUE" : "FALSE"),
                       event->grab_broken.grab_window,
                       window, menu, menu_window, menu_toplevel_window, widget, widget_window );
            break;

        case GDK_WINDOW_STATE:
            _DEBUGLOG( "osx_event_menu: GDK_WINDOW_STATE (%d -> %d) window:%08X menu:%08X(%08X/%08X) widget:%08X(%08X)", 
                       event->window_state.changed_mask, event->window_state.new_window_state,
                       window, menu, menu_window, menu_toplevel_window, widget, widget_window );
            break;

        default:
            _DEBUGLOG( "osx_event_menu: type:%08X(%ld) send_event:%08X window:%08X", type, type, send_event, window );
            break;
    }

}

static void
osx_pos_menu (GtkMenu *menu,
              gint *x,
              gint *y,
              gboolean *push_in,
              gpointer user_data)
{
    // Move the MENU under THE MOUSE
    GdkRectangle rect;
    GdkScreen *screen = gtk_widget_get_screen(GTK_WIDGET(menu));
    gint monitor_num = gdk_screen_get_monitor_at_point( screen, *x, *y );
    gdk_screen_get_monitor_geometry( screen, monitor_num, &rect );

    *x = *x - 20;
    if( *x < rect.x )
        *x = rect.x;

    *y = *y - 20;
    if( *y < rect.y )
        *y = rect.y;

    _DEBUGLOG( "osx_pos_menu: x:%d y:%d rx:%d ry:%d rw:%d rh:%d", *x, *y, rect.x, rect.y, rect.width, rect.height );

}


static void
tray_menu_cb (GtkWidget *widget, guint button, guint time, gpointer userdata)
{
	//static GtkWidget *menu;
	GtkWidget *menu = openMenu;
	GtkWidget *submenu;
	GtkWidget *item;
	int away_status;

    _DEBUGLOG( "tray_menu_cb: BEGIN" );

	/* ph may have an invalid context now */
	hexchat_set_context (ph, hexchat_find_context (ph, NULL, NULL));

     _DEBUGLOG( "tray_menu_cb: BEFORE G_IS_OBJECT" );
    
	/* close any old menu */
	if (G_IS_OBJECT (menu))
	{
        _DEBUGLOG( "tray_menu_cb: BEFORE tray_menu_destroy" );
		tray_menu_destroy (menu, NULL);
        _DEBUGLOG( "tray_menu_cb: AFTER tray_menu_destroy" );
	}


	menu = gtk_menu_new ();
	/*gtk_menu_set_screen (GTK_MENU (menu), gtk_widget_get_screen (widget));*/

	if (tray_get_window_status () == WS_HIDDEN)
		tray_make_item (menu, _("_Restore Window"), tray_menu_restore_cb, NULL);
	else
		tray_make_item (menu, _("_Hide Window"), tray_menu_restore_cb, NULL);
	tray_make_item (menu, NULL, tray_menu_quit_cb, NULL);

#ifndef WIN32 /* submenus are buggy on win32 */
	submenu = mg_submenu (menu, _("_Blink on"));
	blink_item (&prefs.hex_input_tray_chans, submenu, _("Channel Message"));
	blink_item (&prefs.hex_input_tray_priv, submenu, _("Private Message"));
	blink_item (&prefs.hex_input_tray_hilight, submenu, _("Highlighted Message"));
	/*blink_item (BIT_FILEOFFER, submenu, _("File Offer"));*/

	submenu = mg_submenu (menu, _("_Change status"));
#else /* so show away/back in main tray menu */
	submenu = menu;
#endif

	away_status = tray_find_away_status ();
	item = tray_make_item (submenu, _("_Away"), tray_foreach_server, "away");
	if (away_status == 1)
		gtk_widget_set_sensitive (item, FALSE);
	item = tray_make_item (submenu, _("_Back"), tray_foreach_server, "back");
	if (away_status == 2)
		gtk_widget_set_sensitive (item, FALSE);

	menu_add_plugin_items (menu, "\x5$TRAY", NULL);

	tray_make_item (menu, NULL, tray_menu_quit_cb, NULL);
	mg_create_icon_item (_("_Preferences"), GTK_STOCK_PREFERENCES, menu, tray_menu_settings, NULL);
	tray_make_item (menu, NULL, tray_menu_quit_cb, NULL);
	mg_create_icon_item (_("_Quit"), GTK_STOCK_QUIT, menu, tray_menu_quit_cb, NULL);

	g_object_ref (menu);
	g_object_ref_sink (menu);
	g_object_unref (menu);

#ifndef __APPLE__
    // SEE BELOW: LISTEN EVENTS TO CLOSE THE MENU (WHENEVER THE MOUSE WILL LEAVE THE MENU AREA)
    g_signal_connect (G_OBJECT (menu), "selection-done",
                      G_CALLBACK (tray_menu_destroy), NULL);
#endif

#ifdef WIN32
	g_signal_connect (G_OBJECT (menu), "leave-notify-event",
							G_CALLBACK (tray_menu_left_cb), NULL);
	g_signal_connect (G_OBJECT (menu), "enter-notify-event",
							G_CALLBACK (tray_menu_enter_cb), NULL);

	tray_menu_timer = g_timeout_add (500, (GSourceFunc)tray_check_hide, menu);
#endif

#ifdef __APPLE__
    // LISTEN EVENTS TO CLOSE THE MENU (WHENEVER THE MOUSE WILL LEAVE THE MENU AREA)
    g_signal_connect(G_OBJECT(menu), "event-after",
                     G_CALLBACK(osx_event_menu), menu);

    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, osx_pos_menu, userdata, button, time - 1000 );
#else
    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, userdata, button, time);
#endif
    openMenu = menu;


    _DEBUGLOG( "tray_menu_cb: END" );
}

static void
tray_init (void)
{
	flash_tag = 0;
	tray_status = TS_NONE;
	custom_icon1 = NULL;
	custom_icon2 = NULL;

	sticon = gtk_status_icon_new_from_pixbuf (ICON_NORMAL);
	if (!sticon)
		return;

#ifdef __APPLE__
    _DEBUGLOG( "tray_init: INIT OSX" );
#endif

    g_signal_connect (G_OBJECT (sticon), "popup-menu",
				      G_CALLBACK (tray_menu_cb), sticon);
#ifndef __APPLE__ 
    // ON OSX IF THE MAIN WINDOW DON'T HAVE THE FOCUS, THE FIRST CLICK ON THE STATUS MENU'S ICON STARTS TO SEND ACTIVATE SIGNAL 
    // TO THE MAIN WINDOW 
    // -> SO DON'T CLOSE MENU ON ACTIVATE SIGNAL !!!
    // -> THE MENU WILL BE CLOSE WHENEVER THE MOUSE WILL LEAVE MENU AREA
    g_signal_connect (G_OBJECT (sticon), "activate",
                      G_CALLBACK (tray_menu_restore_cb), NULL);
#endif

    g_signal_connect (G_OBJECT (sticon), "notify::embedded",
                      G_CALLBACK (tray_menu_notify_cb), NULL);
}

static int
tray_hilight_cb (char *word[], void *userdata)
{
	/*if (tray_status == TS_HIGHLIGHT)
		return HEXCHAT_EAT_NONE;*/

	if (prefs.hex_input_tray_hilight)
	{
		tray_set_flash (ICON_HILIGHT);

		/* FIXME: hides any previous private messages */
		tray_hilight_count++;
		if (tray_hilight_count == 1)
			tray_set_tipf (_("Highlighted message from: %s (%s) - %s"),
								word[1], hexchat_get_info (ph, "channel"), _(DISPLAY_NAME));
		else
			tray_set_tipf (_("%u highlighted messages, latest from: %s (%s) - %s"),
								tray_hilight_count, word[1], hexchat_get_info (ph, "channel"),
								_(DISPLAY_NAME));
	}

	return HEXCHAT_EAT_NONE;
}

static int
tray_message_cb (char *word[], void *userdata)
{
	if (/*tray_status == TS_MESSAGE ||*/ tray_status == TS_HIGHLIGHT)
		return HEXCHAT_EAT_NONE;
		
	if (prefs.hex_input_tray_chans)
	{
		tray_set_flash (ICON_MSG);

		tray_pub_count++;
		if (tray_pub_count == 1)
			tray_set_tipf (_("Channel message from: %s (%s) - %s"),
								word[1], hexchat_get_info (ph, "channel"), _(DISPLAY_NAME));
		else
			tray_set_tipf (_("%u channel messages. - %s"), tray_pub_count, _(DISPLAY_NAME));
	}

	return HEXCHAT_EAT_NONE;
}

static void
tray_priv (char *from, char *text)
{
	const char *network;

	if (alert_match_word (from, prefs.hex_irc_no_hilight))
		return;

	network = hexchat_get_info (ph, "network");
	if (!network)
		network = hexchat_get_info (ph, "server");

	if (prefs.hex_input_tray_priv)
	{
		tray_set_flash (ICON_MSG);

		tray_priv_count++;
		if (tray_priv_count == 1)
			tray_set_tipf (_("Private message from: %s (%s) - %s"), from,
								network, _(DISPLAY_NAME));
		else
			tray_set_tipf (_("%u private messages, latest from: %s (%s) - %s"),
								tray_priv_count, from, network, _(DISPLAY_NAME));
	}
}

static int
tray_priv_cb (char *word[], void *userdata)
{
	tray_priv (word[1], word[2]);

	return HEXCHAT_EAT_NONE;
}

static int
tray_invited_cb (char *word[], void *userdata)
{
	if (!prefs.hex_away_omit_alerts || tray_find_away_status () != 1)
		tray_priv (word[2], "Invited");

	return HEXCHAT_EAT_NONE;
}

static int
tray_dcc_cb (char *word[], void *userdata)
{
	const char *network;

/*	if (tray_status == TS_FILEOFFER)
		return HEXCHAT_EAT_NONE;*/

	network = hexchat_get_info (ph, "network");
	if (!network)
		network = hexchat_get_info (ph, "server");

	if (prefs.hex_input_tray_priv && (!prefs.hex_away_omit_alerts || tray_find_away_status () != 1))
	{
		tray_set_flash (ICON_FILE);

		tray_file_count++;
		if (tray_file_count == 1)
			tray_set_tipf (_("File offer from: %s (%s) - %s"), word[1], network,
								_(DISPLAY_NAME));
		else
			tray_set_tipf (_("%u file offers, latest from: %s (%s) - %s"),
								tray_file_count, word[1], network, _(DISPLAY_NAME));
	}

	return HEXCHAT_EAT_NONE;
}

static int
tray_focus_cb (char *word[], void *userdata)
{
	tray_stop_flash ();
	tray_reset_counts ();
	return HEXCHAT_EAT_NONE;
}

static void
tray_cleanup (void)
{
	tray_stop_flash ();

	if (sticon)
	{
		g_object_unref ((GObject *)sticon);
		sticon = NULL;
	}
}

void
tray_apply_setup (void)
{
	if (sticon)
	{
		if (!prefs.hex_gui_tray)
			tray_cleanup ();
	}
	else
	{
		GtkWindow *window = GTK_WINDOW(hexchat_get_info (ph, "gtkwin_ptr"));
		if (prefs.hex_gui_tray && gtkutil_tray_icon_supported (window))
			tray_init ();
	}
}

int
tray_plugin_init (hexchat_plugin *plugin_handle, char **plugin_name,
				char **plugin_desc, char **plugin_version, char *arg)
{
	/* we need to save this for use with any hexchat_* functions */
	ph = plugin_handle;

	*plugin_name = "";
	*plugin_desc = "";
	*plugin_version = "";

	hexchat_hook_print (ph, "Channel Msg Hilight", -1, tray_hilight_cb, NULL);
	hexchat_hook_print (ph, "Channel Action Hilight", -1, tray_hilight_cb, NULL);

	hexchat_hook_print (ph, "Channel Message", -1, tray_message_cb, NULL);
	hexchat_hook_print (ph, "Channel Action", -1, tray_message_cb, NULL);
	hexchat_hook_print (ph, "Channel Notice", -1, tray_message_cb, NULL);

	hexchat_hook_print (ph, "Private Message", -1, tray_priv_cb, NULL);
	hexchat_hook_print (ph, "Private Message to Dialog", -1, tray_priv_cb, NULL);
	hexchat_hook_print (ph, "Private Action", -1, tray_priv_cb, NULL);
	hexchat_hook_print (ph, "Private Action to Dialog", -1, tray_priv_cb, NULL);
	hexchat_hook_print (ph, "Notice", -1, tray_priv_cb, NULL);
	hexchat_hook_print (ph, "Invited", -1, tray_invited_cb, NULL);

	hexchat_hook_print (ph, "DCC Offer", -1, tray_dcc_cb, NULL);

	hexchat_hook_print (ph, "Focus Window", -1, tray_focus_cb, NULL);

	GtkWindow *window = GTK_WINDOW(hexchat_get_info (ph, "gtkwin_ptr"));
	if (prefs.hex_gui_tray && gtkutil_tray_icon_supported (window))
		tray_init ();

	return 1;       /* return 1 for success */
}

int
tray_plugin_deinit (hexchat_plugin *plugin_handle)
{
#ifdef WIN32
	tray_cleanup ();
#endif
	return 1;
}
