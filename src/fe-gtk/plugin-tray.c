#include <string.h>
#include "../common/xchat-plugin.h"
#include "../common/xchat.h"
#include "../common/xchatc.h"
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

typedef GdkPixbuf TrayIcon;
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

static TrayIcon *custom_icon1;
static TrayIcon *custom_icon2;

void tray_apply_setup (void);


static void
tray_stop_flash (void)
{
	if (flash_tag)
	{
		g_source_remove (flash_tag);
		flash_tag = 0;
	}

	if (sticon)
		gtk_status_icon_set_from_pixbuf (sticon, ICON_NORMAL);
	tray_status = TS_NONE;

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
}

static int
tray_timeout_cb (TrayIcon *icon)
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
tray_set_flash (TrayIcon *icon)
{
	const char *st;

	if (!sticon)
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
fe_tray_set_icon (const char *filename)
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

static void
tray_menu_restore_cb (GtkWidget *item, gpointer userdata)
{
	const char *st = xchat_get_info (ph, "win_status");

	tray_stop_flash ();
	if (!strcmp (st, "hidden"))
		xchat_command (ph, "GUI SHOW");
	else
		xchat_command (ph, "GUI HIDE");
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

	dialog = gtk_message_dialog_new (GTK_WINDOW (parent_window), 0,
					GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL,
					_("Quit XChat. Are you sure?"));
	g_signal_connect (G_OBJECT (dialog), "response",
							G_CALLBACK (tray_dialog_cb), NULL);
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
	gtk_widget_show (dialog);
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
tray_menu_cb (GtkWidget *widget, guint button, guint time, gpointer userdata)
{
	static GtkWidget *menu = NULL;
	static GtkWidget *restore_item;
	GtkWidget *submenu;
	const char *st;

	if (!menu)
	{
		menu = gtk_menu_new ();

		restore_item = tray_make_item (menu, _("_Restore"), tray_menu_restore_cb, NULL);
		tray_make_item (menu, NULL, tray_menu_quit_cb, NULL);

		submenu = mg_submenu (menu, _("_Blink on"));
		blink_item (2, submenu, _("Channel Message"));
		blink_item (5, submenu, _("Highlighted Message"));
		blink_item (8, submenu, _("Private Message"));
		blink_item (11, submenu, _("File Offer"));

		tray_make_item (menu, NULL, tray_menu_quit_cb, NULL);
		mg_create_icon_item (_("_Exit..."), GTK_STOCK_QUIT, menu, tray_menu_quit_cb, NULL);
	}

	st = xchat_get_info (ph, "win_status");
	if (!strcmp (st, "hidden"))
		gtk_label_set_text_with_mnemonic (GTK_LABEL (GTK_BIN (restore_item)->child), _("_Restore"));
	else
		gtk_label_set_text_with_mnemonic (GTK_LABEL (GTK_BIN (restore_item)->child), _("_Hide"));

	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, gtk_status_icon_position_menu,
						 userdata, button, time);
}

static void
tray_activate_cb (GtkWidget *widget, gpointer userdata)
{
	const char *st;

	st = xchat_get_info (ph, "win_status");
	if (!strcmp (st, "hidden") || !strcmp (st, "normal"))
	{
		tray_stop_flash ();
		xchat_command (ph, "GUI SHOW");
	}
	else
	{
		xchat_command (ph, "GUI HIDE");
	}
}

static void
tray_init (void)
{
	flash_tag = 0;
	tray_status = TS_NONE;
	custom_icon1 = NULL;
	custom_icon2 = NULL;

	sticon = gtk_status_icon_new_from_pixbuf (ICON_NORMAL);
	g_signal_connect (G_OBJECT (sticon), "popup-menu",
							G_CALLBACK (tray_menu_cb), sticon);
	g_signal_connect (G_OBJECT (sticon), "activate",
							G_CALLBACK (tray_activate_cb), NULL);
}

static int
tray_hilight_cb (char *word[], void *userdata)
{
	if (tray_status == TS_HIGHLIGHT)
		return XCHAT_EAT_NONE;

	tray_set_flash (ICON_HILIGHT);
	return XCHAT_EAT_NONE;
}

static int
tray_message_cb (char *word[], void *userdata)
{
	if (tray_status == TS_MESSAGE)
		return XCHAT_EAT_NONE;

	tray_set_flash (ICON_MSG);
	return XCHAT_EAT_NONE;
}

static int
tray_priv_cb (char *word[], void *userdata)
{
	if (tray_status == TS_HIGHLIGHT)
		return XCHAT_EAT_NONE;

	tray_set_flash (ICON_HILIGHT);
	return XCHAT_EAT_NONE;
}

static int
tray_dcc_cb (char *word[], void *userdata)
{
	if (tray_status == TS_FILEOFFER)
		return XCHAT_EAT_NONE;

	tray_set_flash (ICON_FILE);
	return XCHAT_EAT_NONE;
}

static int
tray_focus_cb (char *word[], void *userdata)
{
	tray_stop_flash ();
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

	xchat_hook_print (ph, "DCC Offer", -1, tray_dcc_cb, NULL);

	xchat_hook_print (ph, "Focus Window", -1, tray_focus_cb, NULL);

	if (prefs.gui_tray)
		tray_init ();

	return 1;       /* return 1 for success */
}

/*int
tray_plugin_deinit (xchat_plugin *plugin_handle)
{
	tray_cleanup ();
	return 1;
}*/
