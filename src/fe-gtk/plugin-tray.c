/* Copyright (C) 2006 Peter Zelezny. */

#include <string.h>
#include "../common/xchat-plugin.h"
#include "../common/xchat.h"
#include "../common/xchatc.h"
#include "../common/server.h"
#include "fe-gtk.h"
#include "pixmaps.h"
#include "maingui.h"
#include "menu.h"
#include <gtk/gtk.h>

typedef enum	/* current icon status */
{
	TS_NONE,
	TS_MESSAGE,
	TS_HIGHLIGHT,
	TS_FILEOFFER,
	TS_CUSTOM /* plugin */
} TrayStatus;

enum	/* prefs.gui_tray_blink bits */
{
	BIT_MESSAGE = 2,
	BIT_HIGHLIGHT = 5,
	BIT_PRIVMSG = 8,
	BIT_FILEOFFER = 11
};

typedef GdkPixbuf* TrayIcon;
#define tray_icon_from_file(f) gdk_pixbuf_new_from_file(f,NULL)
#define tray_icon_free(i) g_object_unref(i)

#define ICON_NORMAL pix_xchat
#define ICON_MSG pix_tray_msg
#define ICON_HILIGHT pix_tray_hilight
#define ICON_FILE pix_tray_file
#define TIMEOUT 500

static GtkStatusIcon *sticon;
static gint flash_tag;
static TrayStatus tray_status;
static xchat_plugin *ph;

static TrayIcon custom_icon1;
static TrayIcon custom_icon2;

static int tray_priv_count = 0;
static int tray_pub_count = 0;
static int tray_hilight_count = 0;
static int tray_file_count = 0;


void tray_apply_setup (void);



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
		gtk_status_icon_set_tooltip (sticon, text);
}

void
fe_tray_set_balloon (const char *title, const char *text)
{
/*	if (sticon)
		gtk_status_icon_set_balloon (sticon, title, text);*/
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
			tray_set_tipf (_("XChat: Connected to %u networks and %u channels"),
								nets, chans);
		else
			tray_set_tipf ("XChat: %s", _("Not connected."));
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
	const char *st;

	if (!sticon)
		return;

	/* already flashing the same icon */
	if (flash_tag && gtk_status_icon_get_pixbuf (sticon) == icon)
		return;

	/* no flashing if window is focused */
	st = xchat_get_info (ph, "win_status");
	if (!strcmp (st, "active"))
		return;

	tray_stop_flash ();

	gtk_status_icon_set_from_pixbuf (sticon, icon);
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
fe_tray_set_icon (int icon)
{
	tray_apply_setup ();
	if (!sticon)
		return;

	tray_stop_flash ();

	switch (icon)
	{
	case BIT_MESSAGE:
		tray_set_flash (ICON_MSG);
		break;
	case BIT_HIGHLIGHT:
	case BIT_PRIVMSG:
		tray_set_flash (ICON_HILIGHT);
		break;
	case BIT_FILEOFFER:
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
tray_toggle_visibility (void)
{
	static int x, y;
	static GdkScreen *screen;
	GtkWindow *win;

	if (!sticon)
		return FALSE;

	win = (GtkWindow *)xchat_get_info (ph, "win_ptr");

	tray_stop_flash ();
	tray_reset_counts ();

	if (GTK_WIDGET_VISIBLE (win))
	{
		gtk_window_get_position (win, &x, &y);
		screen = gtk_window_get_screen (win);
		gtk_widget_hide (GTK_WIDGET (win));
	}
	else
	{
		gtk_window_set_screen (win, screen);
		gtk_window_move (win, x, y);
		gtk_widget_show (GTK_WIDGET (win));
		gtk_window_present (win);
	}

	return TRUE;
}

static void
tray_menu_restore_cb (GtkWidget *item, gpointer userdata)
{
	tray_toggle_visibility ();
}

static void
tray_dialog_cb (GtkWidget *dialog, gint arg1, gpointer userdata)
{
	gtk_widget_destroy (dialog);
	if (arg1 == GTK_RESPONSE_OK)
		gtk_widget_destroy ((GtkWidget *)xchat_get_info (ph, "win_ptr"));
}


static void
tray_menu_quit_cb (GtkWidget *item, gpointer userdata)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (parent_window), 0,
					GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL,
					_("You are connected to <b>%u</b> IRC networks.\nAre you sure you want to quit?"), tray_count_networks ());
	g_signal_connect (G_OBJECT (dialog), "response",
							G_CALLBACK (tray_dialog_cb), NULL);
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
	gtk_widget_show (dialog);
}

static void
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
}

static void
tray_toggle_cb (GtkCheckMenuItem *item, gpointer userdata)
{
	int bit = GPOINTER_TO_INT (userdata);

	if (item->active)
		prefs.gui_tray_blink |= (1 << bit);
	else
		prefs.gui_tray_blink &= ~(1 << bit);
}

static void
blink_item (int bit, GtkWidget *menu, char *label)
{
	int set = prefs.gui_tray_blink;
	menu_toggle_item (label, menu, tray_toggle_cb, GINT_TO_POINTER (bit), set & (1 << bit));
}

static void
tray_menu_destroy (GtkWidget *menu, gpointer userdata)
{
	gtk_widget_destroy (menu);
	g_object_unref (menu);
}

static void
tray_menu_cb (GtkWidget *widget, guint button, guint time, gpointer userdata)
{
	GtkWidget *menu;
	GtkWidget *submenu;
	const char *st;

	/* ph may have an invalid context now */
	xchat_set_context (ph, xchat_find_context (ph, NULL, NULL));

	menu = gtk_menu_new ();
	/*gtk_menu_set_screen (GTK_MENU (menu), gtk_widget_get_screen (widget));*/

	st = xchat_get_info (ph, "win_status");
	if (!strcmp (st, "hidden"))
		tray_make_item (menu, _("_Restore"), tray_menu_restore_cb, NULL);
	else
		tray_make_item (menu, _("_Hide"), tray_menu_restore_cb, NULL);
	tray_make_item (menu, NULL, tray_menu_quit_cb, NULL);

	submenu = mg_submenu (menu, _("_Blink on"));
	blink_item (BIT_MESSAGE, submenu, _("Channel Message"));
	blink_item (BIT_HIGHLIGHT, submenu, _("Highlighted Message"));
	blink_item (BIT_PRIVMSG, submenu, _("Private Message"));
	blink_item (BIT_FILEOFFER, submenu, _("File Offer"));

	tray_make_item (menu, NULL, tray_menu_quit_cb, NULL);
	mg_create_icon_item (_("Quit..."), GTK_STOCK_QUIT, menu, tray_menu_quit_cb, NULL);

	menu_add_plugin_items (menu, "\x5$TRAY");

	g_object_ref (menu);
	g_object_ref_sink (menu);
	g_object_unref (menu);
	g_signal_connect (G_OBJECT (menu), "selection-done",
							G_CALLBACK (tray_menu_destroy), NULL);

	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, gtk_status_icon_position_menu,
						 userdata, button, time);
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
	g_signal_connect (G_OBJECT (sticon), "popup-menu",
							G_CALLBACK (tray_menu_cb), sticon);
	g_signal_connect (G_OBJECT (sticon), "activate",
							G_CALLBACK (tray_menu_restore_cb), NULL);
}

static int
tray_hilight_cb (char *word[], void *userdata)
{
	if (tray_status == TS_HIGHLIGHT)
		return XCHAT_EAT_NONE;

	if (prefs.gui_tray_blink & (1 << BIT_HIGHLIGHT))
	{
		tray_set_flash (ICON_HILIGHT);

		/* FIXME: hides any previous private messages */
		tray_hilight_count++;
		if (tray_hilight_count == 1)
			tray_set_tipf (_("XChat: Highlighed message from: %s (%s)"),
								word[1], xchat_get_info (ph, "channel"));
		else
			tray_set_tipf (_("XChat: %u highlighted messages, latest from: %s (%s)"),
								tray_hilight_count, word[1], xchat_get_info (ph, "channel"));
	}

	return XCHAT_EAT_NONE;
}

static int
tray_message_cb (char *word[], void *userdata)
{
	if (tray_status == TS_MESSAGE || tray_status == TS_HIGHLIGHT)
		return XCHAT_EAT_NONE;

	if (prefs.gui_tray_blink & (1 << BIT_MESSAGE))
	{
		tray_set_flash (ICON_MSG);

		tray_pub_count++;
		if (tray_pub_count == 1)
			tray_set_tipf (_("XChat: New public message from: %s (%s)"),
								word[1], xchat_get_info (ph, "channel"));
		else
			tray_set_tipf (_("XChat: %u new public messages."), tray_pub_count);
	}

	return XCHAT_EAT_NONE;
}

static void
tray_priv (char *from)
{
	const char *network;

	tray_set_flash (ICON_HILIGHT);

	network = xchat_get_info (ph, "network");
	if (!network)
		network = xchat_get_info (ph, "server");

	tray_priv_count++;
	if (tray_priv_count == 1)
		tray_set_tipf (_("XChat: Private message from: %s (%s)"),
							from, network);
	else
		tray_set_tipf (_("XChat: %u private messages, latest from: %s (%s)"),
							tray_priv_count, from, network);
}

static int
tray_priv_cb (char *word[], void *userdata)
{
	if (tray_status == TS_HIGHLIGHT)
		return XCHAT_EAT_NONE;

	if (prefs.gui_tray_blink & (1 << BIT_PRIVMSG))
		tray_priv (word[1]);

	return XCHAT_EAT_NONE;
}

static int
tray_invited_cb (char *word[], void *userdata)
{
	if (tray_status == TS_HIGHLIGHT)
		return XCHAT_EAT_NONE;

	if (prefs.gui_tray_blink & (1 << BIT_PRIVMSG))
		tray_priv (word[2]);

	return XCHAT_EAT_NONE;
}

static int
tray_dcc_cb (char *word[], void *userdata)
{
	const char *network;

	if (tray_status == TS_FILEOFFER)
		return XCHAT_EAT_NONE;

	if (prefs.gui_tray_blink & (1 << BIT_FILEOFFER))
	{
		tray_set_flash (ICON_FILE);

		network = xchat_get_info (ph, "network");
		if (!network)
			network = xchat_get_info (ph, "server");

		tray_file_count++;
		if (tray_file_count == 1)
			tray_set_tipf (_("XChat: File offer from: %s (%s)"),
								word[1], network);
		else
			tray_set_tipf (_("XChat: %u file offers, latest from: %s (%s)"),
								tray_file_count, word[1], network);
	}

	return XCHAT_EAT_NONE;
}

static int
tray_focus_cb (char *word[], void *userdata)
{
	tray_stop_flash ();
	tray_reset_counts ();
	return XCHAT_EAT_NONE;
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
		if (!prefs.gui_tray)
			tray_cleanup ();
	}
	else
	{
		if (prefs.gui_tray)
			tray_init ();
	}
}

int
tray_plugin_init (xchat_plugin *plugin_handle, char **plugin_name,
				char **plugin_desc, char **plugin_version, char *arg)
{
	/* we need to save this for use with any xchat_* functions */
	ph = plugin_handle;

	*plugin_name = "";
	*plugin_desc = "";
	*plugin_version = "";

	xchat_hook_print (ph, "Channel Msg Hilight", -1, tray_hilight_cb, NULL);
	xchat_hook_print (ph, "Channel Action Hilight", -1, tray_hilight_cb, NULL);

	xchat_hook_print (ph, "Channel Message", -1, tray_message_cb, NULL);
	xchat_hook_print (ph, "Channel Action", -1, tray_message_cb, NULL);
	xchat_hook_print (ph, "Channel Notice", -1, tray_message_cb, NULL);

	xchat_hook_print (ph, "Private Message", -1, tray_priv_cb, NULL);
	xchat_hook_print (ph, "Private Message to Dialog", -1, tray_priv_cb, NULL);
	xchat_hook_print (ph, "Notice", -1, tray_priv_cb, NULL);
	xchat_hook_print (ph, "Invited", -1, tray_invited_cb, NULL);

	xchat_hook_print (ph, "DCC Offer", -1, tray_dcc_cb, NULL);

	xchat_hook_print (ph, "Focus Window", -1, tray_focus_cb, NULL);

	if (prefs.gui_tray)
		tray_init ();

	return 1;       /* return 1 for success */
}

int
tray_plugin_deinit (xchat_plugin *plugin_handle)
{
#ifdef WIN32
	tray_cleanup ();
#endif
	return 1;
}
