#define GTK_DISABLE_DEPRECATED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/xchat.h"
#include "../common/cfgfiles.h"
#include "../common/text.h"
#include "../common/xchatc.h"
#include "fe-gtk.h"
#include "gtkutil.h"
#include "maingui.h"
#include "palette.h"
#include "pixmaps.h"

#include <gtk/gtkcolorseldialog.h>
#include <gtk/gtktable.h>
#include <gtk/gtkentry.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmisc.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkalignment.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkfontsel.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtkstock.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkhbbox.h>
#include <gtk/gtkhseparator.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtktreestore.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkhscale.h>


GtkStyle *create_input_style (void);


static int last_selected_page = 0;
static struct xchatprefs setup_prefs;

enum
{
	ST_END,
	ST_TOGGLE,
	ST_ENTRY,
	ST_EFONT,
	ST_EFILE,
	ST_MENU,
	ST_RADIO,
	ST_NUMBER,
	ST_HSCALE,
	ST_LABEL,
};

typedef struct
{
	int type;
	char *label;
	int offset;
	char *tooltip;
	char **list;
	int extra;
} setting;


static const setting textbox_settings[] =
{
	{ST_EFONT,  N_("Font:"), P_OFFSET(font_normal), 0, 0, sizeof prefs.font_normal},
	{ST_EFILE,  N_("Background image:"), P_OFFSET(background), 0, 0, sizeof prefs.background},
	{ST_ENTRY,  N_("Time stamp format:"), P_OFFSET(stamp_format),
					N_("See strftime manpage for details."),0,sizeof prefs.stamp_format},
	{ST_TOGGLE, N_("Time stamp text"), P_OFFINT(timestamp),0,0,0},
	{ST_TOGGLE, N_("Transparent background"), P_OFFINT(transparent),0,0,0},
	{ST_TOGGLE, N_("Indent nicks"), P_OFFINT(indent_nicks),0,0,0},
	{ST_TOGGLE, N_("Tint transparency"), P_OFFINT(tint),0,0,0},
	{ST_TOGGLE, N_("Colored nicks"), P_OFFINT(colorednicks),0,0,0},
	{ST_TOGGLE, N_("Strip mIRC color"), P_OFFINT(stripcolor),0,0,0},
	{ST_NUMBER,	N_("Scrollback lines:"), P_OFFINT(max_lines),0,0,100000},
	{ST_HSCALE, N_("Tint red:"), P_OFFINT(tint_red),0,0,0},
	{ST_HSCALE, N_("Tint green:"), P_OFFINT(tint_green),0,0,0},
	{ST_HSCALE, N_("Tint blue:"), P_OFFINT(tint_blue),0,0,0},
	{ST_END, 0, 0, 0, 0, 0}
};

static const setting inputbox_settings[] =
{
	{ST_TOGGLE, N_("Automatic nick completion"), P_OFFINT(nickcompletion),0,0,0},
	{ST_TOGGLE, N_("Use the Text box font"), P_OFFINT(style_inputbox),0,0,0},
	{ST_TOGGLE, N_("Interpret %nnn as an ASCII value"), P_OFFINT(perc_color),0,0,0},
	{ST_ENTRY, N_("Nick completion suffix:"), P_OFFSET(nick_suffix),0,0,sizeof prefs.nick_suffix},
	{ST_END, 0, 0, 0, 0, 0}
};

static char *lagmenutext[] = 
{
	N_("Off"),
	N_("Graph"),
	N_("Info text"),
	N_("Both"),
	NULL
};

static char *ulmenutext[] = 
{
	N_("A-Z, Ops first"),
	N_("A-Z"),
	N_("Z-A, Ops last"),
	N_("Z-A"),
	N_("Unsorted"),
	NULL
};

static const setting userlist_settings[] =
{
	{ST_TOGGLE, N_("Show hostnames in userlist"), P_OFFINT(showhostname_in_userlist), 0, 0, 0},
	{ST_TOGGLE, N_("Userlist buttons enabled"), P_OFFINT(userlistbuttons), 0, 0, 0},
	{ST_ENTRY,	N_("Double-click command:"), P_OFFSET(doubleclickuser), 0, 0, sizeof prefs.doubleclickuser},
	{ST_MENU,	N_("Userlist sorted by:"), P_OFFINT(userlist_sort), 0, ulmenutext, 0},
	{ST_MENU,	N_("Lag meter:"), P_OFFINT(lagometer), 0, lagmenutext, 0},
	{ST_MENU,	N_("Throttle meter:"), P_OFFINT(throttlemeter), 0, lagmenutext, 0},
	{ST_END, 0, 0, 0, 0, 0}
};

static char *tabwin[] =
{
	N_("Windows"),
	N_("Tabs"),
	NULL
};

static char *tabpos[] =
{
	N_("Bottom"),
	N_("Top"),
/*	N_("Left"),
	N_("Right"),*/
	N_("Hidden"),
	NULL
};

static const setting tabs_settings[] =
{
	{ST_MENU,	N_("Open channels in:"), P_OFFINT(tabchannels), 0, tabwin, 0},
	{ST_MENU,	N_("Open dialogs in:"), P_OFFINT(privmsgtab), 0, tabwin, 0},
	{ST_MENU,	N_("Open utilities in:"), P_OFFINT(windows_as_tabs), N_("Open DCC, Ignore, Notify etc, in tabs or windows?"), tabwin, 0},
	{ST_MENU,	N_("Show tabs at:"), P_OFFINT(tabs_position), 0, tabpos, 0},
	{ST_TOGGLE, N_("Open tab for server messages"), P_OFFINT(use_server_tab), 0, 0, 0},
	{ST_TOGGLE, N_("Only highlight tabs on channel messages"), P_OFFINT(limitedtabhighlight), 0, 0, 0},
	{ST_TOGGLE, N_("Open tab for server notices"), P_OFFINT(notices_tabs), 0, 0, 0},
	{ST_TOGGLE, N_("Pop new tabs to front"), P_OFFINT(newtabstofront), 0, 0, 0},
	{ST_END, 0, 0, 0, 0, 0}
};

static const setting filexfer_settings[] =
{
	{ST_ENTRY,	N_("Download files to:"), P_OFFSET(dccdir), 0, 0, sizeof prefs.dccdir},
	{ST_ENTRY,	N_("Move completed files to:"), P_OFFSET(dcc_completed_dir), 0, 0, sizeof prefs.dcc_completed_dir},
	{ST_ENTRY,	N_("DCC IP address:"), P_OFFSET(dcc_ip_str),
					N_("Claim you are at this address when offering files."), 0, sizeof prefs.dcc_ip_str},
	{ST_NUMBER,	N_("First DCC send port:"), P_OFFINT(first_dcc_send_port), 0, 0, 65535},
	{ST_NUMBER,	N_("Last DCC send port:"), P_OFFINT(last_dcc_send_port), 0, 0, 65535},
	{ST_LABEL,	N_("(Leave ports at zero for full range).")},
	{ST_TOGGLE, N_("Auto open DCC send list"), P_OFFINT(autoopendccsendwindow), 0, 0, 0},
	{ST_TOGGLE, N_("Convert spaces to underscore"), P_OFFINT(dcc_send_fillspaces),
					N_("In filenames, before sending"), 0, 0},
	{ST_TOGGLE, N_("Auto open DCC chat list"), P_OFFINT(autoopendccchatwindow), 0, 0, 0},
	{ST_TOGGLE, N_("Save nickname in filenames"), P_OFFINT(dccwithnick), 0, 0, 0},
	{ST_TOGGLE, N_("Auto open DCC receive list"), P_OFFINT(autoopendccrecvwindow), 0, 0, 0},
	{ST_TOGGLE, N_("Get my IP from IRC server"), P_OFFINT(ip_from_server),
					N_("/WHOIS yourself to find your real address. Use this if you have a 192.168.*.* address!"), 0, 0},
	{ST_NUMBER,	N_("Max. send CPS:"), P_OFFINT(dcc_max_send_cps), 
					N_("Max. speed for one transfer"), 0, 1000000},
	{ST_NUMBER,	N_("Max. receive CPS:"), P_OFFINT(dcc_max_get_cps),
					N_("Max. speed for one transfer"), 0, 1000000},
	{ST_NUMBER,	N_("Max. global send CPS:"), P_OFFINT(dcc_global_max_send_cps),
					N_("Max. speed for all traffic"), 0, 1000000},
	{ST_NUMBER,	N_("Max. global receive CPS:"), P_OFFINT(dcc_global_max_get_cps),
					N_("Max. speed for all traffic"), 0, 1000000},
	{ST_LABEL,	N_("(Leave at zero for full speed file transfers).")},
	{ST_END, 0, 0, 0, 0, 0}
};

static const setting general_settings[] =
{
	{ST_ENTRY,	N_("Default quit message:"), P_OFFSET(quitreason), 0, 0, sizeof prefs.quitreason},
	{ST_ENTRY,	N_("Default part message:"), P_OFFSET(partreason), 0, 0, sizeof prefs.partreason},
	{ST_ENTRY,	N_("Default away message:"), P_OFFSET(awayreason), 0, 0, sizeof prefs.awayreason},
#ifndef WIN32
	{ST_LABEL,	N_("(Can be a text file relative to ~/.xchat2/).")},
#else
	{ST_LABEL,	N_("(Can be a text file relative to conf\\).")},
#endif
	{ST_ENTRY,	N_("Extra words to highlight on:"), P_OFFSET(bluestring), 0, 0, sizeof prefs.bluestring},
	{ST_LABEL,	N_("(Separate multiple words with commas).")},
	{ST_TOGGLE,	N_("Show away once"), P_OFFINT(show_away_once), N_("Show identical away messages only once"), 0, 0},
	{ST_TOGGLE,	N_("Beep on private messages"), P_OFFINT(beepmsg), 0, 0, 0},
	{ST_TOGGLE,	N_("Automatically unmark away"), P_OFFINT(auto_unmark_away), N_("Unmark yourself as away before sending messages"), 0, 0},
	{ST_TOGGLE,	N_("Beep on channel messages"), P_OFFINT(beepchans), 0, 0, 0},
	{ST_TOGGLE,	N_("Announce away messages"), P_OFFINT(show_away_message), N_("Announce your away messages to all channels"), 0, 0},
	{ST_TOGGLE,	N_("Display MODEs in raw form"), P_OFFINT(raw_modes), 0, 0, 0},
	{ST_NUMBER,	N_("Auto reconnect delay:"), P_OFFINT(recon_delay), 0, 0, 9999},
	{ST_END, 0, 0, 0, 0, 0}
};

static const setting logging_settings[] =
{
	{ST_ENTRY,	N_("Log filename mask:"), P_OFFSET(logmask), 0, 0, sizeof prefs.logmask},
	{ST_LABEL,	N_("(%s=Server %c=Channel %n=Network).")},
	{ST_ENTRY,	N_("Log timestamp format:"), P_OFFSET(timestamp_log_format), 0, 0, sizeof prefs.timestamp_log_format},
	{ST_LABEL,	N_("(See strftime manpage for details).")},
	{ST_TOGGLE,	N_("Enable logging of conversations"), P_OFFINT(logging), 0, 0, 0},
	{ST_TOGGLE,	N_("Insert timestamps in logs"), P_OFFINT(timestamp_logs), 0, 0, 0},
	{ST_END, 0, 0, 0, 0, 0}
};

static char *proxytypes[] =
{
	N_("(Disabled)"),
	N_("Wingate"),
	N_("Socks4"),
	N_("Socks5"),
	N_("HTTP"),
	NULL
};

static const setting network_settings[] =
{
	{ST_ENTRY,	N_("Address to bind to:"), P_OFFSET(hostname), 0, 0, sizeof prefs.hostname},
	{ST_LABEL,	N_("(Only useful for computers with multiple addresses).")},
	{ST_ENTRY,	N_("Proxy server:"), P_OFFSET(proxy_host), 0, 0, sizeof prefs.proxy_host},
	{ST_NUMBER,	N_("Proxy port:"), P_OFFINT(proxy_port), 0, 0, 65535},
	{ST_MENU,	N_("Proxy type:"), P_OFFINT(proxy_type), 0, proxytypes, 0},
	{ST_END, 0, 0, 0, 0, 0}
};

#define setup_get_str(pr,set) (((char *)pr)+set->offset)
#define setup_get_int(pr,set) *(((int *)pr)+set->offset)

#define setup_set_int(pr,set,num) *((int *)pr+set->offset)=num
#define setup_set_str(pr,set,str) strcpy(((char *)pr)+set->offset,str)


static void
setup_toggle_cb (GtkToggleButton *but, const setting *set)
{
	setup_set_int (&setup_prefs, set, but->active ? 1 : 0);
}

static void
setup_create_toggle (GtkWidget *box, int row, const setting *set)
{
	GtkWidget *wid;

	wid = gtk_check_button_new_with_label (_(set->label));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wid),
											setup_get_int (&setup_prefs, set));
	g_signal_connect (G_OBJECT (wid), "toggled",
							G_CALLBACK (setup_toggle_cb), (gpointer)set);
	if (set->tooltip)
		add_tip (wid, _(set->tooltip));
	gtk_box_pack_start (GTK_BOX (box), wid, 0, 0, 0);
}

static void
setup_spin_cb (GtkSpinButton *spin, const setting *set)
{
	setup_set_int (&setup_prefs, set, gtk_spin_button_get_value_as_int (spin));
}

static void
setup_create_spin (GtkWidget *table, int row, const setting *set)
{
	GtkWidget *label, *wid, *rbox, *align;

	label = gtk_label_new (_(set->label));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

	align = gtk_alignment_new (0.0, 1.0, 0.0, 0.0);
	gtk_table_attach_defaults (GTK_TABLE (table), align, 1, 2, row, row + 1);

	rbox = gtk_hbox_new (0, 0);
	gtk_container_add (GTK_CONTAINER (align), rbox);

	wid = gtk_spin_button_new_with_range (0, set->extra, 1);
	if (set->tooltip)
		add_tip (wid, _(set->tooltip));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (wid),
										setup_get_int (&setup_prefs, set));
	g_signal_connect (G_OBJECT (wid), "value-changed",
							G_CALLBACK (setup_spin_cb), (gpointer)set);
	gtk_box_pack_start (GTK_BOX (rbox), wid, 0, 0, 0);
}


static void
setup_hscale_cb (GtkHScale *wid, const setting *set)
{
	setup_set_int (&setup_prefs, set, gtk_range_get_value(GTK_RANGE(wid)));
}

static void
setup_create_hscale (GtkWidget *table, int row, const setting *set)
{
	GtkWidget *wid;

	wid = gtk_label_new (_(set->label));
	gtk_misc_set_alignment (GTK_MISC (wid), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), wid, 0, 1, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

	wid = gtk_hscale_new_with_range (0., 255., 1.);
	gtk_scale_set_value_pos (GTK_SCALE (wid), GTK_POS_RIGHT);
	gtk_range_set_value (GTK_RANGE (wid), setup_get_int (&setup_prefs, set));
	g_signal_connect (G_OBJECT(wid), "value_changed",
							G_CALLBACK (setup_hscale_cb), (gpointer)set);
	gtk_table_attach (GTK_TABLE (table), wid, 1, 5, row, row + 1,
							GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
}

#if 0
static int
setup_create_radio (GtkWidget *table, int row, setting *set)
{
	GtkWidget *wid;
	int i;
	char **text = set->list;
	GSList *group;

	wid = gtk_label_new (set->label);
	gtk_misc_set_alignment (GTK_MISC (wid), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), wid, 0, 1, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

	i = 0;
	group = NULL;
	while (text[i])
	{
		wid = gtk_radio_button_new_with_label (group, text[i]);
		group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (wid));
		gtk_table_attach_defaults (GTK_TABLE (table), wid, 2, 3, row, row + 1);
		if (i == setup_get_int (&setup_prefs, set))
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wid), TRUE);
		i++;
		row++;
	}

	return i;
}
#endif

static void
setup_menu_cb (GtkWidget *item, const setting *set)
{
	setup_set_int (&setup_prefs, set,
						GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item), "n")));
}

static void
setup_create_menu (GtkWidget *table, int row, const setting *set)
{
	GtkWidget *wid, *menu, *item, *align;
	int i;
	char **text = set->list;

	wid = gtk_label_new (_(set->label));
	gtk_misc_set_alignment (GTK_MISC (wid), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), wid, 0, 1, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

	wid = gtk_option_menu_new ();
	if (set->tooltip)
		add_tip (wid, _(set->tooltip));
	menu = gtk_menu_new ();

	i = 0;
	while (text[i])
	{
		item = gtk_menu_item_new_with_label (_(text[i]));
		g_object_set_data (G_OBJECT (item), "n", GINT_TO_POINTER (i));
		gtk_widget_show (item);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		g_signal_connect (G_OBJECT (item), "activate",
								G_CALLBACK (setup_menu_cb), (gpointer)set);
		i++;
	}

	gtk_option_menu_set_menu (GTK_OPTION_MENU (wid), menu);
	gtk_option_menu_set_history (GTK_OPTION_MENU (wid),
										  setup_get_int (&setup_prefs, set));

	align = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
	gtk_container_add (GTK_CONTAINER (align), wid);
	gtk_table_attach (GTK_TABLE (table), align, 1, 4, row, row + 1,
							GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
}

static void
setup_filereq_cb (GtkWidget *entry, void *data2, char *file)
{
	if (file)
	{
		if (file[0])
			gtk_entry_set_text (GTK_ENTRY (entry), file);
		free (file);
	}
}

static void
setup_browsefile_cb (GtkWidget *button, GtkWidget *entry)
{
	gtkutil_file_req (_("Choose File"), setup_filereq_cb, entry, 0, FALSE);
}

static void
setup_fontsel_cb (GtkWidget *button, GtkFontSelectionDialog *dialog)
{
	GtkWidget *entry;

	entry = g_object_get_data (G_OBJECT (button), "e");

	gtk_entry_set_text (GTK_ENTRY (entry),
							  gtk_font_selection_dialog_get_font_name (dialog));
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
setup_fontsel_cancel (GtkWidget *button, GtkFontSelectionDialog *dialog)
{
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
setup_browsefont_cb (GtkWidget *button, GtkWidget *entry)
{
	GtkFontSelection *sel;
	GtkFontSelectionDialog *dialog;

	dialog = (GtkFontSelectionDialog *) gtk_font_selection_dialog_new (_("Select font"));

	sel = (GtkFontSelection *) dialog->fontsel;

	gtk_font_selection_set_font_name (sel, GTK_ENTRY (entry)->text);

	g_object_set_data (G_OBJECT (dialog->ok_button), "e", entry);

	g_signal_connect (G_OBJECT (dialog->ok_button), "clicked",
							G_CALLBACK (setup_fontsel_cb), dialog);
	g_signal_connect (G_OBJECT (dialog->cancel_button), "clicked",
							G_CALLBACK (setup_fontsel_cancel), dialog);

	gtk_widget_show (GTK_WIDGET (dialog));
}

static void
setup_entry_cb (GtkEntry *entry, setting *set)
{
	setup_set_str (&setup_prefs, set, entry->text);
}

static void
setup_create_label (GtkWidget *table, int row, const setting *set)
{
	gtk_table_attach (GTK_TABLE (table), gtk_label_new (_(set->label)),
							1, 2, row, row + 1, GTK_SHRINK | GTK_FILL,
							GTK_SHRINK | GTK_FILL, 0, 0);
}

static void
setup_create_entry (GtkWidget *table, int row, const setting *set)
{
	GtkWidget *label;
	GtkWidget *wid, *bwid;

	label = gtk_label_new (_(set->label));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

	wid = gtk_entry_new ();
	if (set->tooltip)
		add_tip (wid, _(set->tooltip));
	gtk_entry_set_max_length (GTK_ENTRY (wid), set->extra);
	gtk_entry_set_text (GTK_ENTRY (wid), setup_get_str (&setup_prefs, set));
	g_signal_connect (G_OBJECT (wid), "changed",
							G_CALLBACK (setup_entry_cb), (gpointer)set);

	if (set->type == ST_ENTRY)
		gtk_table_attach (GTK_TABLE (table), wid, 1, 5, row, row + 1,
								GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
	else
	{
		gtk_table_attach (GTK_TABLE (table), wid, 1, 4, row, row + 1,
								GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
		bwid = gtk_button_new_with_label (_("Browse..."));
		gtk_table_attach (GTK_TABLE (table), bwid, 4, 5, row, row + 1,
								GTK_SHRINK, GTK_SHRINK, 0, 0);
		if (set->type == ST_EFILE)
			g_signal_connect (G_OBJECT (bwid), "clicked",
									G_CALLBACK (setup_browsefile_cb), wid);
		if (set->type == ST_EFONT)
			g_signal_connect (G_OBJECT (bwid), "clicked",
									G_CALLBACK (setup_browsefont_cb), wid);
	}
}

static GtkWidget *
setup_create_page (const setting *set)
{
	int i, row;
	GtkWidget *tab, *box, *hbox, *left, *right;

	box = gtk_vbox_new (FALSE, 20);
	gtk_container_set_border_width (GTK_CONTAINER (box), 4);

	tab = gtk_table_new (3, 2, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (tab), 2);
	gtk_table_set_row_spacings (GTK_TABLE (tab), 2);
	gtk_table_set_col_spacings (GTK_TABLE (tab), 3);
	gtk_container_add (GTK_CONTAINER (box), tab);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (box), hbox);

	left = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (hbox), left);

	right = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (hbox), right);

	i = row = 0;
	while (set[i].type != ST_END)
	{
		switch (set[i].type)
		{
		case ST_EFONT:
		case ST_ENTRY:
		case ST_EFILE:
			setup_create_entry (tab, row, &set[i]);
			break;
		case ST_TOGGLE:
			if (i % 2)
				setup_create_toggle (right, row, &set[i]);
			else
				setup_create_toggle (left, row, &set[i]);
			break;
		case ST_MENU:
			setup_create_menu (tab, row, &set[i]);
			break;
#if 0
		case ST_RADIO:
			row += setup_create_radio (tab, row, &set[i]);
			break;
#endif
		case ST_NUMBER:
			setup_create_spin (tab, row, &set[i]);
			break;
		case ST_HSCALE:
			setup_create_hscale (tab, row, &set[i]);
			break;
		case ST_LABEL:
			setup_create_label (tab, row, &set[i]);
			break;
		}
		i++;
		row++;
	}

	return box;
}

static void
setup_color_ok_cb (GtkWidget *button, GtkWidget *dialog)
{
	GtkColorSelectionDialog *cdialog = GTK_COLOR_SELECTION_DIALOG (dialog);
	GdkColor *col;
	GdkColor old_color;
	GtkStyle *style;

	col = g_object_get_data (G_OBJECT (button), "c");
	old_color = *col;

	button = g_object_get_data (G_OBJECT (button), "b");

	if (!GTK_IS_WIDGET (button))
	{
		gtk_widget_destroy (dialog);
		return;
	}

	gtk_color_selection_get_current_color (GTK_COLOR_SELECTION (cdialog->colorsel), col);

	gdk_colormap_alloc_color (gtk_widget_get_colormap (button), col, TRUE, TRUE);

	style = gtk_style_new ();
	style->bg[0] = *col;
	gtk_widget_set_style (button, style);
	g_object_unref (style);

	/* is this line correct?? */
	gdk_colormap_free_colors (gtk_widget_get_colormap (button), &old_color, 1);

	gtk_widget_destroy (dialog);
}

static void
setup_color_cb (GtkWidget *button, gpointer userdata)
{
	GtkWidget *dialog;
	GtkColorSelectionDialog *cdialog;
	GdkColor *color;

	color = &colors[GPOINTER_TO_INT (userdata)];

	dialog = gtk_color_selection_dialog_new (_("Select color"));
	cdialog = GTK_COLOR_SELECTION_DIALOG (dialog);

	gtk_widget_hide (cdialog->help_button);
	g_signal_connect (G_OBJECT (cdialog->ok_button), "clicked",
							G_CALLBACK (setup_color_ok_cb), dialog);
	g_signal_connect (G_OBJECT (cdialog->cancel_button), "clicked",
							G_CALLBACK (gtkutil_destroy), dialog);
	g_object_set_data (G_OBJECT (cdialog->ok_button), "c", color);
	g_object_set_data (G_OBJECT (cdialog->ok_button), "b", button);
	gtk_widget_set_sensitive (cdialog->help_button, FALSE);
	gtk_color_selection_set_current_color (GTK_COLOR_SELECTION (cdialog->colorsel), color);
	gtk_widget_show (dialog);
}

static void
setup_create_color_button (GtkWidget *table, int num, int row, int col)
{
	GtkWidget *but;
	GtkStyle *style;
	char buf[16];

	if (num > 15)
		strcpy (buf, " ");
	else
		sprintf (buf, "%d", num);
	but = gtk_button_new_with_label (buf);
	gtk_table_attach (GTK_TABLE (table), but, col, col+1, row, row+1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
	g_signal_connect (G_OBJECT (but), "clicked",
							G_CALLBACK (setup_color_cb), GINT_TO_POINTER (num));
	style = gtk_style_new ();
	style->bg[GTK_STATE_NORMAL] = colors[num];
	gtk_widget_set_style (but, style);
	g_object_unref (style);
}

static void
setup_create_other_color (char *text, int num, int row, GtkWidget *tab)
{
	GtkWidget *label;

	label = gtk_label_new (text);
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (tab), label, 0, 1, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
	setup_create_color_button (tab, num, row, 1);
}

static GtkWidget *
setup_create_color_page (void)
{
	GtkWidget *tab, *box, *label;
	int i;

	box = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (box), 4);

	tab = gtk_table_new (9, 2, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (tab), 2);
	gtk_table_set_row_spacings (GTK_TABLE (tab), 2);
	gtk_table_set_col_spacings (GTK_TABLE (tab), 3);
	gtk_container_add (GTK_CONTAINER (box), tab);

	label = gtk_label_new (_("mIRC colors:"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (tab), label, 0, 1, 0, 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

	for (i = 0; i < 16; i++)
		setup_create_color_button (tab, i, 0, i+1);

	setup_create_other_color (_("Foreground:"), 18, 1, tab);
	setup_create_other_color (_("Background:"), 19, 2, tab);

	setup_create_other_color (_("Mark fore:"), 17, 6, tab);
	setup_create_other_color (_("Mark back:"), 16, 7, tab);

	setup_create_other_color (_("New Data:"), 20, 10, tab);
	setup_create_other_color (_("New Message:"), 22, 11, tab);
	setup_create_other_color (_("Highlight:"), 21, 12, tab);
	
	return box;
}

static void
setup_add_page (char *title, GtkWidget *book, GtkWidget *tab)
{
	GtkWidget *oframe, *frame, *label, *vvbox;

	/* frame for whole page */
	oframe = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (oframe), GTK_SHADOW_IN);

	vvbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (oframe), vvbox);

	/* border for the label */
	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
	gtk_box_pack_start (GTK_BOX (vvbox), frame, FALSE, TRUE, 0);

	/* label */
	label = gtk_label_new (title);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_misc_set_padding (GTK_MISC (label), 2, 1);
	gtk_container_add (GTK_CONTAINER (frame), label);

	gtk_box_pack_start (GTK_BOX (vvbox), tab, FALSE, FALSE, 0);

	gtk_notebook_append_page (GTK_NOTEBOOK (book), oframe, NULL);
}

static char *cata[] =
{
	N_("Interface"),
		N_("Text box"),
		N_("Input box"),
		N_("User list"),
		N_("Tabs"),
		N_("Colors"),
		NULL,
	N_("Chatting"),
		N_("General"),
		N_("Logging"),
		NULL,
	N_("Network"),
		N_("Network setup"),
		N_("File transfers"),
		NULL,
	NULL
};

static GtkWidget *
setup_create_pages (GtkWidget *box)
{
	GtkWidget *book;

	book = gtk_notebook_new ();

	setup_add_page (_(cata[1]), book, setup_create_page (textbox_settings));
	setup_add_page (_(cata[2]), book, setup_create_page (inputbox_settings));
	setup_add_page (_(cata[3]), book, setup_create_page (userlist_settings));
	setup_add_page (_(cata[4]), book, setup_create_page (tabs_settings));
	setup_add_page (_(cata[5]), book, setup_create_color_page ());
	setup_add_page (_(cata[8]), book, setup_create_page (general_settings));
	setup_add_page (_(cata[9]), book, setup_create_page (logging_settings));
	setup_add_page (_(cata[12]), book, setup_create_page (network_settings));
	setup_add_page (_(cata[13]), book, setup_create_page (filexfer_settings));

	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (book), FALSE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (book), FALSE);
	gtk_container_add (GTK_CONTAINER (box), book);

	return book;
}

static void
setup_tree_cb (GtkTreeView *treeview, GtkWidget *book)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection (treeview);
	GtkTreeIter iter;
	GtkTreeModel *model;
	int page;

	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		gtk_tree_model_get (model, &iter, 1, &page, -1);
		if (page != -1)
		{
			gtk_notebook_set_current_page (GTK_NOTEBOOK (book), page);
			last_selected_page = page;
		}
	}
}

static gboolean
setup_tree_select_filter (GtkTreeSelection *selection, GtkTreeModel *model,
								  GtkTreePath *path, gboolean path_selected,
								  gpointer data)
{
	if (gtk_tree_path_get_depth (path) > 1)
		return TRUE;
	return FALSE;
}

static void
setup_create_tree (GtkWidget *box, GtkWidget *book)
{
	GtkWidget *tree;
	GtkTreeStore *model;
	GtkTreeIter iter;
	GtkTreeIter child_iter;
	GtkTreeIter *sel_iter = NULL;
	GtkCellRenderer *renderer;
	GtkTreeSelection *sel;
	int i, page;

	model = gtk_tree_store_new (2, G_TYPE_STRING, G_TYPE_INT);

	i = 0;
	page = 0;
	do
	{
		gtk_tree_store_append (model, &iter, NULL);
		gtk_tree_store_set (model, &iter, 0, cata[i], 1, -1, -1);
		i++;

		do
		{
			gtk_tree_store_append (model, &child_iter, &iter);
			gtk_tree_store_set (model, &child_iter, 0, cata[i], 1, page, -1);
			if (page == last_selected_page)
				sel_iter = gtk_tree_iter_copy (&child_iter);
			page++;
			i++;
		} while (cata[i]);

		i++;

	} while (cata[i]);

	tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
	g_object_unref (G_OBJECT (model));
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	gtk_tree_selection_set_mode (sel, GTK_SELECTION_BROWSE);
	gtk_tree_selection_set_select_function (sel, setup_tree_select_filter,
														 NULL, NULL);
	g_signal_connect (G_OBJECT (tree), "cursor_changed",
							G_CALLBACK (setup_tree_cb), book);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tree),
							    -1, _("Categories"), renderer, "text", 0, NULL);
	gtk_tree_view_expand_all (GTK_TREE_VIEW (tree));
	gtk_box_pack_start (GTK_BOX (box), tree, 0, 0, 0);
	gtk_box_reorder_child (GTK_BOX (box), tree, 0);

	if (sel_iter)
	{
		gtk_tree_selection_select_iter (sel, sel_iter);
		gtk_tree_iter_free (sel_iter);
	}
}

static void
setup_apply_to_sess (session *sess)
{
	mg_update_xtext (sess->gui->xtext);
	if (prefs.style_inputbox)
		gtk_widget_set_style (sess->gui->input_box, input_style);
}

static void
setup_apply (struct xchatprefs *pr)
{
	GSList *list;
	int done_main = FALSE;
	session *sess;
	GtkStyle *old_style;
	int new_pix = FALSE;

	if (strcmp (pr->background, prefs.background) != 0)
		new_pix = TRUE;

	memcpy (&prefs, pr, sizeof (prefs));

	if (new_pix)
	{
		if (channelwin_pix)
			g_object_unref (channelwin_pix);
		channelwin_pix = pixmap_load_from_file (prefs.background);
	}

	old_style = input_style;
	input_style = create_input_style ();

	list = sess_list;
	while (list)
	{
		sess = list->data;
		if (sess->gui->is_tab)
		{
			/* only apply to main tabwindow once */
			if (!done_main)
			{
				done_main = TRUE;
				setup_apply_to_sess (sess);
			}
		} else
		{
			setup_apply_to_sess (sess);
		}

		if (prefs.logging)
			log_open (sess);
		else
			log_close (sess);

		list = list->next;
	}

	mg_apply_setup ();

	g_object_unref (old_style);
}

#if 0
static void
setup_apply_cb (GtkWidget *but, GtkWidget *win)
{
	/* setup_prefs -> xchat */
	setup_apply (&setup_prefs);
}
#endif

static void
setup_ok_cb (GtkWidget *but, GtkWidget *win)
{
	gtk_widget_destroy (win);
	setup_apply (&setup_prefs);
	save_config ();
	palette_save ();
}

static GtkWidget *
setup_window_open (void)
{
	GtkWidget *wid, *win, *vbox, *hbox, *hbbox;

	win = gtkutil_window_new (_("X-Chat: Preferences"), 0, 0, TRUE);

	vbox = gtk_vbox_new (FALSE, 5);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
	gtk_container_add (GTK_CONTAINER (win), vbox);

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_container_add (GTK_CONTAINER (vbox), hbox);

	setup_create_tree (hbox, setup_create_pages (hbox));

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	/* prepare the button box */
	hbbox = gtk_hbutton_box_new ();
	gtk_box_set_spacing (GTK_BOX (hbbox), 4);
	gtk_box_pack_end (GTK_BOX (hbox), hbbox, FALSE, FALSE, 0);

	/* standard buttons */
	/* GNOME doesn't like apply */
#if 0
	wid = gtk_button_new_from_stock (GTK_STOCK_APPLY);
	g_signal_connect (G_OBJECT (wid), "clicked",
							G_CALLBACK (setup_apply_cb), win);
	gtk_box_pack_start (GTK_BOX (hbbox), wid, FALSE, FALSE, 0);
#endif

	wid = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
	g_signal_connect (G_OBJECT (wid), "clicked",
							G_CALLBACK (gtkutil_destroy), win);
	gtk_box_pack_start (GTK_BOX (hbbox), wid, FALSE, FALSE, 0);

	wid = gtk_button_new_from_stock (GTK_STOCK_OK);
	g_signal_connect (G_OBJECT (wid), "clicked",
							G_CALLBACK (setup_ok_cb), win);
	gtk_box_pack_start (GTK_BOX (hbbox), wid, FALSE, FALSE, 0);

	wid = gtk_hseparator_new ();
	gtk_box_pack_end (GTK_BOX (vbox), wid, FALSE, FALSE, 0);

	gtk_widget_show_all (win);

	return win;
}

static void
setup_close_cb (GtkWidget *win, GtkWidget **swin)
{
	*swin = NULL;
}

void
setup_open (void)
{
	static GtkWidget *setup_window = NULL;

	if (setup_window)
	{
		gtk_window_present (GTK_WINDOW (setup_window));
		return;
	}

	memcpy (&setup_prefs, &prefs, sizeof (prefs));

	setup_window = setup_window_open ();

	g_signal_connect (G_OBJECT (setup_window), "destroy",
							G_CALLBACK (setup_close_cb), &setup_window);
}
