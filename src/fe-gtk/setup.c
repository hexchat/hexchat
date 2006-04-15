/* X-Chat
 * Copyright (C) 2004-2005 Peter Zelezny.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../common/xchat.h"
#include "../common/cfgfiles.h"
#include "../common/fe.h"
#include "../common/text.h"
#include "../common/userlist.h"
#include "../common/util.h"
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
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtkstock.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkhbbox.h>
#include <gtk/gtkhseparator.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtktreestore.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkhscale.h>
#ifdef WIN32
#include "../common/fe.h"
#endif
#ifdef USE_GTKSPELL
#include <gtk/gtktextview.h>
#include <gtkspell/gtkspell.h>
#endif

GtkStyle *create_input_style (GtkStyle *);

#define LABEL_INDENT 6

static int last_selected_page = 0;
static int last_selected_row = 0; /* sound row */
static gboolean color_change;
static struct xchatprefs setup_prefs;
static GtkWidget *cancel_button;

enum
{
	ST_END,
	ST_TOGGLE,
	ST_TOGGLR,
	ST_ENTRY,
	ST_EFONT,
	ST_EFILE,
	ST_EOPEN,
	ST_MENU,
	ST_RADIO,
	ST_NUMBER,
	ST_HSCALE,
	ST_HEADER,
	ST_LABEL
};

typedef struct
{
	int type;
	char *label;
	int offset;
	char *tooltip;
	char const *const *list;
	int extra;
} setting;


static const setting textbox_settings[] =
{
	{ST_HEADER,	N_("Text Box Appearance"),0,0,0},
	{ST_EFONT,  N_("Font:"), P_OFFSETNL(font_normal), 0, 0, sizeof prefs.font_normal},
	{ST_EFILE,  N_("Background image:"), P_OFFSETNL(background), 0, 0, sizeof prefs.background},
	{ST_NUMBER,	N_("Scrollback lines:"), P_OFFINTNL(max_lines),0,0,100000},
	{ST_TOGGLE, N_("Colored nick names"), P_OFFINTNL(colorednicks),
					N_("Give each person on IRC a different color"),0,0},
	{ST_TOGGLR, N_("Indent nick names"), P_OFFINTNL(indent_nicks),
					N_("Make nick names right-justified"),0,0},
	{ST_TOGGLE, N_("Transparent background"), P_OFFINTNL(transparent),0,0,0},
	{ST_TOGGLR, N_("Show marker line"), P_OFFINTNL(show_marker),
					N_("Insert a red line after the last read text."),0,0},
	{ST_HEADER, N_("Transparency Settings"), 0,0,0},
	{ST_HSCALE, N_("Red:"), P_OFFINTNL(tint_red),0,0,0},
	{ST_HSCALE, N_("Green:"), P_OFFINTNL(tint_green),0,0,0},
	{ST_HSCALE, N_("Blue:"), P_OFFINTNL(tint_blue),0,0,0},

	{ST_HEADER,	N_("Time Stamps"),0,0,0},
	{ST_ENTRY,  N_("Time stamp format:"), P_OFFSETNL(stamp_format),
					N_("See strftime manpage for details."),0,sizeof prefs.stamp_format},
	{ST_TOGGLE, N_("Enable time stamps"), P_OFFINTNL(timestamp),0,0,0},

	{ST_END, 0, 0, 0, 0, 0}
};

static const setting inputbox_settings[] =
{
	{ST_HEADER, N_("Input box"),0,0,0},
	{ST_TOGGLE, N_("Use the Text box font and colors"), P_OFFINTNL(style_inputbox),0,0,0},
#ifdef USE_GTKSPELL
	{ST_TOGGLE, N_("Spell checking"), P_OFFINTNL(gui_input_spell),0,0,0},
#endif

	{ST_HEADER, N_("Nick Completion"),0,0,0},
	{ST_TOGGLE, N_("Automatic nick completion"), P_OFFINTNL(nickcompletion),
					N_("Completes nick names without using the TAB key"),0,0},
	{ST_ENTRY,	N_("Nick completion suffix:"), P_OFFSETNL(nick_suffix),0,0,sizeof prefs.nick_suffix},

#if 0	/* obsolete */
	{ST_HEADER, N_("Input Box Codes"),0,0,0},
	{ST_TOGGLE, N_("Interpret %nnn as an ASCII value"), P_OFFINTNL(perc_ascii),0,0,0},
	{ST_TOGGLE, N_("Interpret %C, %B as Color, Bold etc"), P_OFFINTNL(perc_color),0,0,0},
#endif

	{ST_END, 0, 0, 0, 0, 0}
};

/*static const char *const lagmenutext[] = 
{
	N_("Off"),
	N_("Graph"),
	N_("Info text"),
	N_("Both"),
	NULL
};*/

static const char *const ulmenutext[] = 
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
	{ST_HEADER,	N_("User List"),0,0,0},
	{ST_MENU,	N_("User list sorted by:"), P_OFFINTNL(userlist_sort), 0, ulmenutext, 0},
	{ST_TOGGLE, N_("Show hostnames in user list"), P_OFFINTNL(showhostname_in_userlist), 0, 0, 0},
	{ST_TOGGLE, N_("Use the Text box font and colors"), P_OFFINTNL(style_namelistgad),0,0,0},
	{ST_TOGGLE, N_("Resizable user list"), P_OFFINTNL(paned_userlist),0,0,0},

	{ST_HEADER,	N_("Away tracking"),0,0,0},
	{ST_TOGGLE,	N_("Track the Away status of users and mark them in a different color"), P_OFFINTNL(away_track),0,0,0},
	{ST_NUMBER, N_("On channels smaller than:"), P_OFFINTNL(away_size_max),0,0,10000},

	{ST_HEADER,	N_("Action Upon Double Click"),0,0,0},
	{ST_ENTRY,	N_("Execute command:"), P_OFFSETNL(doubleclickuser), 0, 0, sizeof prefs.doubleclickuser},

/*	{ST_HEADER,	N_("Extra Gadgets"),0,0,0},
	{ST_MENU,	N_("Lag meter:"), P_OFFINTNL(lagometer), 0, lagmenutext, 0},
	{ST_MENU,	N_("Throttle meter:"), P_OFFINTNL(throttlemeter), 0, lagmenutext, 0},*/

	{ST_END, 0, 0, 0, 0, 0}
};

static const char *const tabwin[] =
{
	N_("Windows"),
	N_("Tabs"),
	NULL
};

static const char *const tabpos[] =
{
	N_("Bottom"),
	N_("Top"),
	N_("Left"),
	N_("Right"),
	N_("Hidden"),
	NULL
};

static const char *const focusnewtabsmenu[] =
{
	N_("Never"),
	N_("Always"),
	N_("Only requested tabs"),
	NULL
};

static const setting tabs_settings[] =
{
	{ST_HEADER,	N_("Tabs"),0,0,0},
	{ST_TOGGLE, N_("Open an extra tab for server messages"), P_OFFINTNL(use_server_tab), 0, 0, 0},
	{ST_TOGGLE, N_("Open an extra tab for server notices"), P_OFFINTNL(notices_tabs), 0, 0, 0},
	{ST_TOGGLE, N_("Small tabs"), P_OFFINTNL(tab_small), 0, 0, 0},
	{ST_TOGGLE, N_("Sort tabs in alphabetical order"), P_OFFINTNL(tab_sort), 0, 0, 0},
	{ST_MENU,	N_("Focus new tabs:"), P_OFFINTNL(newtabstofront), 0, focusnewtabsmenu, 0},
	{ST_NUMBER,	N_("Shorten tab labels to:"), P_OFFINTNL(truncchans), 0, (const char **)N_("letters."), 99},

	{ST_HEADER,	N_("Tabs Location"),0,0,0},
	{ST_MENU,	N_("Show tabs at:"), P_OFFINTNL(tabs_position), 0, tabpos, 0},

	{ST_HEADER,	N_("Tabs or Windows"),0,0,0},
	{ST_MENU,	N_("Open channels in:"), P_OFFINTNL(tabchannels), 0, tabwin, 0},
	{ST_MENU,	N_("Open dialogs in:"), P_OFFINTNL(privmsgtab), 0, tabwin, 0},
	{ST_MENU,	N_("Open utilities in:"), P_OFFINTNL(windows_as_tabs), N_("Open DCC, Ignore, Notify etc, in tabs or windows?"), tabwin, 0},

	{ST_END, 0, 0, 0, 0, 0}
};

static const setting filexfer_settings[] =
{
	{ST_HEADER, N_("Files and Directories"), 0, 0, 0},
	{ST_EOPEN,	N_("Download files to:"), P_OFFSETNL(dccdir), 0, 0, sizeof prefs.dccdir},
	{ST_EOPEN,	N_("Move completed files to:"), P_OFFSETNL(dcc_completed_dir), 0, 0, sizeof prefs.dcc_completed_dir},
	{ST_TOGGLE, N_("Auto accept file offers"), P_OFFINTNL(autodccsend), 0, 0, 0},
	{ST_TOGGLE, N_("Save nick name in filenames"), P_OFFINTNL(dccwithnick), 0, 0, 0},
	{ST_TOGGLE, N_("Convert spaces to underscore before sending"), P_OFFINTNL(dcc_send_fillspaces),0,0,0},

	{ST_HEADER, N_("Network Settings"), 0, 0, 0},
	{ST_ENTRY,	N_("DCC IP address:"), P_OFFSETNL(dcc_ip_str),
					N_("Claim you are at this address when offering files."), 0, sizeof prefs.dcc_ip_str},
	{ST_NUMBER,	N_("First DCC send port:"), P_OFFINTNL(first_dcc_send_port), 0, 0, 65535},
	{ST_NUMBER,	N_("Last DCC send port:"), P_OFFINTNL(last_dcc_send_port), 0, 
		(const char **)N_("(Leave ports at zero for full range)."), 65535},
	{ST_TOGGLE, N_("Get my address from the IRC server"), P_OFFINTNL(ip_from_server),
					N_("Asks the IRC server for your real address. Use this if you have a 192.168.*.* address!"), 0, 0},

	{ST_HEADER, N_("Maximum File Transfer Speeds (bytes per second)"), 0, 0, 0},
	{ST_NUMBER,	N_("One upload:"), P_OFFINTNL(dcc_max_send_cps), 
					N_("Maximum speed for one transfer"), 0, 1000000},
	{ST_NUMBER,	N_("One download:"), P_OFFINTNL(dcc_max_get_cps),
					N_("Maximum speed for one transfer"), 0, 1000000},
	{ST_NUMBER,	N_("All uploads combined:"), P_OFFINTNL(dcc_global_max_send_cps),
					N_("Maximum speed for all files"), 0, 1000000},
	{ST_NUMBER,	N_("All downloads combined:"), P_OFFINTNL(dcc_global_max_get_cps),
					N_("Maximum speed for all files"), 0, 1000000},

	{ST_END, 0, 0, 0, 0, 0}
};

static const setting general_settings[] =
{
	{ST_HEADER,	N_("Default Messages"),0,0,0},
	{ST_ENTRY,	N_("Quit:"), P_OFFSETNL(quitreason), 0, 0, sizeof prefs.quitreason},
	{ST_ENTRY,	N_("Leave channel:"), P_OFFSETNL(partreason), 0, 0, sizeof prefs.partreason},
	{ST_ENTRY,	N_("Away:"), P_OFFSETNL(awayreason), 0, 0, sizeof prefs.awayreason},

	{ST_HEADER,	N_("Away"),0,0,0},
	{ST_TOGGLE,	N_("Announce away messages"), P_OFFINTNL(show_away_message),
					N_("Announce your away messages to all channels"), 0, 0},
	{ST_TOGGLE,	N_("Show away once"), P_OFFINTNL(show_away_once), N_("Show identical away messages only once"), 0, 0},
	{ST_TOGGLE,	N_("Automatically unmark away"), P_OFFINTNL(auto_unmark_away), N_("Unmark yourself as away before sending messages"), 0, 0},
#if 0
#ifndef WIN32
	{ST_LABEL,	N_("(Can be a text file relative to ~/.xchat2/).")},
#else
	{ST_LABEL,	N_("(Can be a text file relative to config dir).")},
#endif
#endif

	{ST_HEADER,	N_("Alerts"),0,0,0},
#if defined(WIN32) || defined(USE_XLIB)
	{ST_TOGGLE,	N_("Flash taskbar on highlighted messages"), P_OFFINTNL(flash_hilight), 0, 0, 0},
#endif
	{ST_TOGGLE,	N_("Beep on highlighted messages"), P_OFFINTNL(beephilight), 0, 0, 0},
	{ST_ENTRY,	N_("Extra words to highlight on:"), P_OFFSETNL(irc_extra_hilight), 0, 0, sizeof prefs.irc_extra_hilight},
	{ST_ENTRY,	N_("Nicks not to highlight on:"), P_OFFSETNL(irc_no_hilight), 0, 0, sizeof prefs.irc_no_hilight},
	{ST_LABEL,	N_("(Separate multiple words with commas).")},
	{ST_TOGGLE,	N_("Beep on private messages"), P_OFFINTNL(beepmsg), 0, 0, 0},
	{ST_TOGGLE,	N_("Beep on channel messages"), P_OFFINTNL(beepchans), 0, 0, 0},
	{ST_END, 0, 0, 0, 0, 0}
};

#if 0
static const setting advanced_settings[] =
{
	{ST_HEADER,	N_("Advanced Settings"),0,0,0},
	{ST_NUMBER,	N_("Auto reconnect delay:"), P_OFFINTNL(recon_delay), 0, 0, 9999},
	{ST_TOGGLE,	N_("Display MODEs in raw form"), P_OFFINTNL(raw_modes), 0, 0, 0},
	{ST_TOGGLE,	N_("Whois on notify"), P_OFFINTNL(whois_on_notifyonline), N_("Sends a /WHOIS when a user comes online in your notify list"), 0, 0},
	{ST_TOGGLE,	N_("Hide join and part messages"), P_OFFINTNL(confmode), N_("Hide channel join/part messages by default"), 0, 0},
	{ST_HEADER,	N_("Auto Open DCC Windows"),0,0,0},
	{ST_TOGGLE, N_("Send window"), P_OFFINTNL(autoopendccsendwindow), 0, 0, 0},
	{ST_TOGGLE, N_("Receive window"), P_OFFINTNL(autoopendccrecvwindow), 0, 0, 0},
	{ST_TOGGLE, N_("Chat window"), P_OFFINTNL(autoopendccchatwindow), 0, 0, 0},

	{ST_END, 0, 0, 0, 0, 0}
};
#endif

static const setting logging_settings[] =
{
	{ST_HEADER,	N_("Logging"),0,0,0},
	{ST_TOGGLE,	N_("Enable logging of conversations"), P_OFFINTNL(logging), 0, 0, 0},
	{ST_ENTRY,	N_("Log filename:"), P_OFFSETNL(logmask), 0, 0, sizeof prefs.logmask},
	{ST_LABEL,	N_("(%s=Server %c=Channel %n=Network).")},

	{ST_HEADER,	N_("Time Stamps"),0,0,0},
	{ST_TOGGLE,	N_("Insert timestamps in logs"), P_OFFINTNL(timestamp_logs), 0, 0, 0},
	{ST_ENTRY,	N_("Log timestamp format:"), P_OFFSETNL(timestamp_log_format), 0, 0, sizeof prefs.timestamp_log_format},
	{ST_LABEL,	N_("(See strftime manpage for details).")},

	{ST_END, 0, 0, 0, 0, 0}
};

static const char *const proxytypes[] =
{
	N_("(Disabled)"),
	N_("Wingate"),
	N_("Socks4"),
	N_("Socks5"),
	N_("HTTP"),
	NULL
};

static const char *const proxyuse[] =
{
	N_("All Connections"),
	N_("IRC Server Only"),
	N_("DCC Only"),
	NULL
};

static const setting network_settings[] =
{
	{ST_HEADER,	N_("Your Address"), 0, 0, 0, 0},
	{ST_ENTRY,	N_("Bind to:"), P_OFFSETNL(hostname), 0, 0, sizeof prefs.hostname},
	{ST_LABEL,	N_("(Only useful for computers with multiple addresses).")},

	{ST_HEADER,	N_("Proxy Server"), 0, 0, 0, 0},
	{ST_ENTRY,	N_("Hostname:"), P_OFFSETNL(proxy_host), 0, 0, sizeof prefs.proxy_host},
	{ST_NUMBER,	N_("Port:"), P_OFFINTNL(proxy_port), 0, 0, 65535},
	{ST_MENU,	N_("Type:"), P_OFFINTNL(proxy_type), 0, proxytypes, 0},

	{ST_TOGGLE,	N_("Authenticate to the proxy server (HTTP and Socks5)"), P_OFFINTNL(proxy_auth), 0, 0, 0},
	{ST_ENTRY,	N_("Username:"), P_OFFSETNL(proxy_user), 0, 0, sizeof prefs.proxy_user},
	{ST_ENTRY,	N_("Password:"), P_OFFSETNL(proxy_pass), 0, GINT_TO_POINTER(1), sizeof prefs.proxy_pass},

	{ST_MENU,	N_("Use proxy for:"), P_OFFINTNL(proxy_use), 0, proxyuse, 0},

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
setup_create_toggleR (GtkWidget *tab, int row, const setting *set)
{
	GtkWidget *wid;

	wid = gtk_check_button_new_with_label (_(set->label));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wid),
											setup_get_int (&setup_prefs, set));
	g_signal_connect (G_OBJECT (wid), "toggled",
							G_CALLBACK (setup_toggle_cb), (gpointer)set);
	if (set->tooltip)
		add_tip (wid, _(set->tooltip));
	gtk_table_attach (GTK_TABLE (tab), wid, 4, 5, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
}

static void
setup_create_toggleL (GtkWidget *tab, int row, const setting *set)
{
	GtkWidget *wid;

	wid = gtk_check_button_new_with_label (_(set->label));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wid),
											setup_get_int (&setup_prefs, set));
	g_signal_connect (G_OBJECT (wid), "toggled",
							G_CALLBACK (setup_toggle_cb), (gpointer)set);
	if (set->tooltip)
		add_tip (wid, _(set->tooltip));
	gtk_table_attach (GTK_TABLE (tab), wid, 2, 4, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, LABEL_INDENT, 0);
}

#if 0
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
#endif

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
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, LABEL_INDENT, 0);

	align = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
	gtk_table_attach (GTK_TABLE (table), align, 3, 4, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

	rbox = gtk_hbox_new (0, 0);
	gtk_container_add (GTK_CONTAINER (align), rbox);

	wid = gtk_spin_button_new_with_range (0, set->extra, 1);
	if (set->tooltip)
		add_tip (wid, _(set->tooltip));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (wid),
										setup_get_int (&setup_prefs, set));
	g_signal_connect (G_OBJECT (wid), "value_changed",
							G_CALLBACK (setup_spin_cb), (gpointer)set);
	gtk_box_pack_start (GTK_BOX (rbox), wid, 0, 0, 0);

	if (set->list)
	{
		label = gtk_label_new (_((char *)set->list));
		gtk_box_pack_start (GTK_BOX (rbox), label, 0, 0, 5);
	}
}

static gint
setup_apply_tint (int *tag)
{
	prefs.tint_red = setup_prefs.tint_red;
	prefs.tint_green = setup_prefs.tint_green;
	prefs.tint_blue = setup_prefs.tint_blue;
	mg_update_xtext (current_sess->gui->xtext);
	*tag = 0;
	return 0;
}

static void
setup_hscale_cb (GtkHScale *wid, const setting *set)
{
	static int tag = 0;

	setup_set_int (&setup_prefs, set, gtk_range_get_value(GTK_RANGE(wid)));
	if(tag == 0)
		tag = g_idle_add ((GSourceFunc)setup_apply_tint, &tag);
}

static void
setup_create_hscale (GtkWidget *table, int row, const setting *set)
{
	GtkWidget *wid;

	wid = gtk_label_new (_(set->label));
	gtk_misc_set_alignment (GTK_MISC (wid), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), wid, 2, 3, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, LABEL_INDENT, 0);

	wid = gtk_hscale_new_with_range (0., 255., 1.);
	gtk_scale_set_value_pos (GTK_SCALE (wid), GTK_POS_RIGHT);
	gtk_range_set_value (GTK_RANGE (wid), setup_get_int (&setup_prefs, set));
	g_signal_connect (G_OBJECT(wid), "value_changed",
							G_CALLBACK (setup_hscale_cb), (gpointer)set);
	gtk_table_attach (GTK_TABLE (table), wid, 3, 6, row, row + 1,
							GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
}

#if 0
static int
setup_create_radio (GtkWidget *table, int row, const setting *set)
{
	GtkWidget *wid;
	int i;
	const char **text = (const char **)set->list;
	GSList *group;

	wid = gtk_label_new (_(set->label));
	gtk_misc_set_alignment (GTK_MISC (wid), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), wid, 2, 3, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, LABEL_INDENT, 0);

	i = 0;
	group = NULL;
	while (text[i])
	{
		wid = gtk_radio_button_new_with_label (group, text[i]);
		if (set->tooltip)
			add_tip (wid, _(set->tooltip));
		group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (wid));
		gtk_table_attach_defaults (GTK_TABLE (table), wid, 3, 4, row, row + 1);
		if (i == setup_get_int (&setup_prefs, set))
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wid), TRUE);
		i++;
		row++;
	}

	return i;
}
#endif

static GtkWidget *proxy_user; 	/* username GtkEntry */
static GtkWidget *proxy_pass; 	/* password GtkEntry */


static void
setup_menu_cb (GtkWidget *item, const setting *set)
{
	int n = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item), "n"));

	/* set the prefs.<field> */
	setup_set_int (&setup_prefs, set, n);

	if (g_object_get_data (G_OBJECT (item), "p"))
	{
		/* only HTTP and Socks5 can use a username/pass */
		gtk_widget_set_sensitive (proxy_user, (n == 3 || n == 4));
		gtk_widget_set_sensitive (proxy_pass, (n == 3 || n == 4));
	}
}

/*
static const char *id_strings[] =
{
	"",
	"*",
	"%C4*%C18%B%B",
	"%U"
};

static void
setup_id_menu_cb (GtkWidget *item, char *dest)
{
	int n = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item), "n"));

	strcpy (dest, id_strings[n]);
}

static void
setup_create_id_menu (GtkWidget *table, char *label, int row, char *dest)
{
	GtkWidget *wid, *menu, *item;
	int i, def = 0;
	static const char *text[] =
	{
		("(disabled)"),
		("A star (*)"),
		("A red star (*)"),
		("Underlined")
	};

	wid = gtk_label_new (label);
	gtk_misc_set_alignment (GTK_MISC (wid), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), wid, 2, 3, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, LABEL_INDENT, 0);

	wid = gtk_option_menu_new ();
	menu = gtk_menu_new ();

	for (i = 0; i < 4; i++)
	{
		if (strcmp (id_strings[i], dest) == 0)
		{
			def = i;
			break;
		}
	}

	i = 0;
	while (text[i])
	{
		item = gtk_menu_item_new_with_label (_(text[i]));
		g_object_set_data (G_OBJECT (item), "n", GINT_TO_POINTER (i));

		gtk_widget_show (item);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		g_signal_connect (G_OBJECT (item), "activate",
								G_CALLBACK (setup_id_menu_cb), dest);
		i++;
	}

	gtk_option_menu_set_menu (GTK_OPTION_MENU (wid), menu);
	gtk_option_menu_set_history (GTK_OPTION_MENU (wid), def);

	gtk_table_attach (GTK_TABLE (table), wid, 3, 4, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
}

*/

static void
setup_create_menu (GtkWidget *table, int row, const setting *set)
{
	GtkWidget *wid, *box, *menu, *item;
	int i;
	const char **text = (const char **)set->list;

	wid = gtk_label_new (_(set->label));
	gtk_misc_set_alignment (GTK_MISC (wid), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), wid, 2, 3, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, LABEL_INDENT, 0);

	wid = gtk_option_menu_new ();
	if (set->tooltip)
		add_tip (wid, _(set->tooltip));
	menu = gtk_menu_new ();

	i = 0;
	while (text[i])
	{
		item = gtk_menu_item_new_with_label (_(text[i]));
		g_object_set_data (G_OBJECT (item), "n", GINT_TO_POINTER (i));

		/* set a flag for the callback to use */
		if (text == proxytypes)
			g_object_set_data (G_OBJECT (item), "p", GINT_TO_POINTER (1));	

		gtk_widget_show (item);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		g_signal_connect (G_OBJECT (item), "activate",
								G_CALLBACK (setup_menu_cb), (gpointer)set);
		i++;
	}

	gtk_option_menu_set_menu (GTK_OPTION_MENU (wid), menu);
	gtk_option_menu_set_history (GTK_OPTION_MENU (wid),
										  setup_get_int (&setup_prefs, set));

	box = gtk_hbox_new (0, 0);
	gtk_box_pack_start (GTK_BOX (box), wid, 0, 0, 0);
	gtk_widget_show (box);

	gtk_table_attach (GTK_TABLE (table), box, 3, 4, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
}

static void
setup_filereq_cb (GtkWidget *entry, char *file)
{
	if (file)
	{
		if (file[0])
			gtk_entry_set_text (GTK_ENTRY (entry), file);
	}
}

static void
setup_browsefile_cb (GtkWidget *button, GtkWidget *entry)
{
	gtkutil_file_req (_("Select an Image File"), setup_filereq_cb, entry, NULL, 0);
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

#ifdef WIN32
static void
setup_browsefolder_cb (GtkWidget *button, GtkWidget *entry)
{
	if (GTK_ENTRY (entry)->text[0])
		fe_open_url (GTK_ENTRY (entry)->text);
}
#endif

static void
setup_browsefont_cb (GtkWidget *button, GtkWidget *entry)
{
	GtkFontSelection *sel;
	GtkFontSelectionDialog *dialog;

	dialog = (GtkFontSelectionDialog *) gtk_font_selection_dialog_new (_("Select font"));

	sel = (GtkFontSelection *) dialog->fontsel;

	if (GTK_ENTRY (entry)->text[0])
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
							3, 4, row, row + 1, GTK_FILL,
							GTK_SHRINK | GTK_FILL, 0, 0);
}

static void
setup_create_entry (GtkWidget *table, int row, const setting *set)
{
	GtkWidget *label;
	GtkWidget *wid, *bwid;

	label = gtk_label_new (_(set->label));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, LABEL_INDENT, 0);

	wid = gtk_entry_new ();
	if (set->list)
		gtk_entry_set_visibility (GTK_ENTRY (wid), FALSE);
	if (set->tooltip)
		add_tip (wid, _(set->tooltip));
	gtk_entry_set_max_length (GTK_ENTRY (wid), set->extra - 1);
	gtk_entry_set_text (GTK_ENTRY (wid), setup_get_str (&setup_prefs, set));
	g_signal_connect (G_OBJECT (wid), "changed",
							G_CALLBACK (setup_entry_cb), (gpointer)set);

	if (set->offset == P_OFFSETNL(proxy_user))
		proxy_user = wid;
	if (set->offset == P_OFFSETNL(proxy_pass))
		proxy_pass = wid; 

	/* only http and Socks5 can auth */
	if ( (set->offset == P_OFFSETNL(proxy_pass) ||
			set->offset == P_OFFSETNL(proxy_user)) &&
	     (setup_prefs.proxy_type != 4 && setup_prefs.proxy_type != 3) )
		gtk_widget_set_sensitive (wid, FALSE);

#ifdef WIN32
	if (set->type == ST_ENTRY)
#else
	if (set->type == ST_ENTRY || set->type == ST_EOPEN)
#endif
		gtk_table_attach (GTK_TABLE (table), wid, 3, 6, row, row + 1,
								GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
	else
	{
		gtk_table_attach (GTK_TABLE (table), wid, 3, 5, row, row + 1,
								GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
		bwid = gtk_button_new_with_label (_("Browse..."));
		gtk_table_attach (GTK_TABLE (table), bwid, 5, 6, row, row + 1,
								GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
		if (set->type == ST_EFILE)
			g_signal_connect (G_OBJECT (bwid), "clicked",
									G_CALLBACK (setup_browsefile_cb), wid);
		if (set->type == ST_EFONT)
			g_signal_connect (G_OBJECT (bwid), "clicked",
									G_CALLBACK (setup_browsefont_cb), wid);
#ifdef WIN32
		if (set->type == ST_EOPEN)
			g_signal_connect (G_OBJECT (bwid), "clicked",
									G_CALLBACK (setup_browsefolder_cb), wid);
#endif
	}
}

static void
setup_create_header (GtkWidget *table, int row, char *labeltext)
{
	GtkWidget *label;
	char buf[128];

	snprintf (buf, sizeof (buf), "<b>%s</b>", _(labeltext));

	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label), buf);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 4, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 5);
}

static GtkWidget *
setup_create_frame (GtkWidget **left, GtkWidget *box)
{
	GtkWidget *tab, *hbox, *inbox = box;

	tab = gtk_table_new (3, 2, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (tab), 2);
	gtk_table_set_row_spacings (GTK_TABLE (tab), 2);
	gtk_table_set_col_spacings (GTK_TABLE (tab), 3);
	gtk_container_add (GTK_CONTAINER (inbox), tab);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (inbox), hbox);

	*left = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), *left, 0, 0, 0);

	return tab;
}

#ifdef WIN32
static void
open_data_cb (GtkWidget *button, gpointer data)
{
	fe_open_url (get_xdir_utf8 ());
}
#endif

static GtkWidget *
setup_create_page (const setting *set)
{
	int i, j, row;
	GtkWidget *tab, *box, *left;

	box = gtk_vbox_new (FALSE, 20);
	gtk_container_set_border_width (GTK_CONTAINER (box), 4);

	tab = setup_create_frame (&left, box);

	i = j = row = 0;
	while (set[i].type != ST_END)
	{
		switch (set[i].type)
		{
		case ST_HEADER:
			setup_create_header (tab, row, set[i].label);
			break;
		case ST_EFONT:
		case ST_ENTRY:
		case ST_EFILE:
		case ST_EOPEN:
			setup_create_entry (tab, row, &set[i]);
			break;
		case ST_TOGGLR:
			row--;
			setup_create_toggleR (tab, row, &set[i]);
			break;
		case ST_TOGGLE:
			setup_create_toggleL (tab, row, &set[i]);
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

#if 0
	if (set == general_settings)
	{
		setup_create_id_menu (tab, _("Mark identified users with:"),	
									 row, setup_prefs.irc_id_ytext);
		setup_create_id_menu (tab, _("Mark not-identified users with:"),	
									 row + 1, setup_prefs.irc_id_ntext);
	}
#endif

#ifdef WIN32
	if (set == logging_settings)
	{
		GtkWidget *but = gtk_button_new_with_label ("Open Data Folder");
		gtk_box_pack_start (GTK_BOX (left), but, 0, 0, 0);
		g_signal_connect (G_OBJECT (but), "clicked",
								G_CALLBACK (open_data_cb), 0);
	}
#endif

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

	color_change = TRUE;

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
	char buf[64];

	if (num > 31)
		strcpy (buf, "<span size=\"x-small\"> </span>");
	else
						/* 12345678901 23456789 01  23456789 */
		sprintf (buf, "<span size=\"x-small\">%d</span>", num);
	but = gtk_button_new_with_label (" ");
	gtk_label_set_markup (GTK_LABEL (GTK_BIN (but)->child), buf);
	/* win32 build uses this to turn off themeing */
	g_object_set_data (G_OBJECT (but), "xchat-color", (gpointer)1);
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
setup_create_other_colorR (char *text, int num, int row, GtkWidget *tab)
{
	GtkWidget *label;

	label = gtk_label_new (text);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (tab), label, 5, 9, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, LABEL_INDENT, 0);
	setup_create_color_button (tab, num, row, 9);
}

static void
setup_create_other_color (char *text, int num, int row, GtkWidget *tab)
{
	GtkWidget *label;

	label = gtk_label_new (text);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (tab), label, 2, 3, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, LABEL_INDENT, 0);
	setup_create_color_button (tab, num, row, 3);
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

	setup_create_header (tab, 0, N_("Text Colors"));

	label = gtk_label_new (_("mIRC colors:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (tab), label, 2, 3, 1, 2,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, LABEL_INDENT, 0);

	for (i = 0; i < 16; i++)
		setup_create_color_button (tab, i, 1, i+3);

	label = gtk_label_new (_("Local colors:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (tab), label, 2, 3, 2, 3,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, LABEL_INDENT, 0);

	for (i = 16; i < 32; i++)
		setup_create_color_button (tab, i, 2, (i+3) - 16);

	setup_create_other_color (_("Foreground:"), COL_FG, 3, tab);
	setup_create_other_colorR (_("Background:"), COL_BG, 3, tab);

	setup_create_header (tab, 5, N_("Marking Text"));

	setup_create_other_color (_("Foreground:"), COL_MARK_FG, 6, tab);
	setup_create_other_colorR (_("Background:"), COL_MARK_BG, 6, tab);

	setup_create_header (tab, 8, N_("Interface Colors"));

	setup_create_other_color (_("New data:"), COL_NEW_DATA, 9, tab);
	setup_create_other_colorR (_("Marker line:"), COL_MARKER, 9, tab);
	setup_create_other_color (_("New message:"), COL_NEW_MSG, 10, tab);
	setup_create_other_colorR (_("Away user:"), COL_AWAY, 10, tab);
	setup_create_other_color (_("Highlight:"), COL_HILIGHT, 11, tab);

	return box;
}

/* === GLOBALS for sound GUI === */

static GtkWidget *sndprog_entry;
static GtkWidget *sndfile_entry;
static GtkWidget *snddir_entry;
static int ignore_changed = FALSE;

extern struct text_event te[]; /* text.c */
extern char *sound_files[];

static void
setup_snd_apply (void)
{
	strcpy (setup_prefs.sounddir, GTK_ENTRY (snddir_entry)->text);
	strcpy (setup_prefs.soundcmd, GTK_ENTRY (sndprog_entry)->text);
}

static void
setup_snd_populate (GtkTreeView * treeview)
{
	GtkListStore *store;
	GtkTreeIter iter;
	GtkTreeSelection *sel;
	GtkTreePath *path;
	int i;

	sel = gtk_tree_view_get_selection (treeview);
	store = (GtkListStore *)gtk_tree_view_get_model (treeview);

	for (i = NUM_XP-1; i >= 0; i--)
	{
		gtk_list_store_prepend (store, &iter);
		if (sound_files[i])
			gtk_list_store_set (store, &iter, 0, te[i].name, 1, sound_files[i], 2, i, -1);
		else
			gtk_list_store_set (store, &iter, 0, te[i].name, 1, "", 2, i, -1);
		if (i == last_selected_row)
		{
			gtk_tree_selection_select_iter (sel, &iter);
			path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);
			if (path)
			{
				gtk_tree_view_scroll_to_cell (treeview, path, NULL, TRUE, 0.5, 0.5);
				gtk_tree_view_set_cursor (treeview, path, NULL, FALSE);
				gtk_tree_path_free (path);
			}
		}
	}
}

static int
setup_snd_get_selected (GtkTreeSelection *sel, GtkTreeIter *iter)
{
	int n;
	GtkTreeModel *model;

	if (!gtk_tree_selection_get_selected (sel, &model, iter))
		return -1;

	gtk_tree_model_get (model, iter, 2, &n, -1);
	return n;
}

static void
setup_snd_row_cb (GtkTreeSelection *sel, gpointer user_data)
{
	int n;
	GtkTreeIter iter;

	n = setup_snd_get_selected (sel, &iter);
	if (n == -1)
		return;
	last_selected_row = n;

	ignore_changed = TRUE;
	if (sound_files[n])
		gtk_entry_set_text (GTK_ENTRY (sndfile_entry), sound_files[n]);
	else
		gtk_entry_set_text (GTK_ENTRY (sndfile_entry), "");
	ignore_changed = FALSE;
}

static void
setup_snd_add_columns (GtkTreeView * treeview)
{
	GtkCellRenderer *renderer;
	GtkTreeModel *model;

	/* event column */
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
																-1, _("Event"), renderer,
																"text", 0, NULL);

	/* file column */
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
																-1, _("Sound file"), renderer,
																"text", 1, NULL);

	model = GTK_TREE_MODEL (gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT));
	gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), model);
	g_object_unref (model);
}

static void
setup_autotoggle_cb (GtkToggleButton *but, GtkToggleButton *ext)
{
	if (but->active)
	{
		setup_prefs.soundcmd[0] = 0;
		gtk_entry_set_text (GTK_ENTRY (sndprog_entry), "");
		gtk_widget_set_sensitive (sndprog_entry, FALSE);
	} else
	{
		gtk_widget_set_sensitive (sndprog_entry, TRUE);
	}
}

static void
setup_snd_filereq_cb (GtkWidget *entry, char *file)
{
	if (file)
	{
		if (file[0])
			gtk_entry_set_text (GTK_ENTRY (entry), file);
	}
}

static void
setup_snd_browse_cb (GtkWidget *button, GtkEntry *entry)
{
	gtkutil_file_req (_("Select a sound file"), setup_snd_filereq_cb, entry, NULL, 0);
}

static void
setup_snd_play_cb (GtkWidget *button, GtkEntry *entry)
{
	sound_play (entry->text);
}

static void
setup_snd_changed_cb (GtkEntry *ent, GtkTreeView *tree)
{
	int n;
	GtkTreeIter iter;
	GtkListStore *store;
	GtkTreeSelection *sel;

	if (ignore_changed)
		return;

	sel = gtk_tree_view_get_selection (tree);
	n = setup_snd_get_selected (sel, &iter);
	if (n == -1)
		return;

	/* get the new sound file */
	if (sound_files[n])
		free (sound_files[n]);
	sound_files[n] = strdup (GTK_ENTRY (ent)->text);

	/* update the TreeView list */
	store = (GtkListStore *)gtk_tree_view_get_model (tree);
	gtk_list_store_set (store, &iter, 1, sound_files[n], -1);

	gtk_widget_set_sensitive (cancel_button, FALSE);
}

static GtkWidget *
setup_create_sound_page (void)
{
	GtkWidget *vbox1;
	GtkWidget *vbox2;
	GtkWidget *table2;
	GtkWidget *label2;
	GtkWidget *label3;
	GtkWidget *radio_external;
	GSList *radio_group = NULL;
	GtkWidget *radio_auto;
	GtkWidget *label4;
	GtkWidget *entry3;
	GtkWidget *scrolledwindow1;
	GtkWidget *sound_tree;
	GtkWidget *table1;
	GtkWidget *sound_label;
	GtkWidget *sound_browse;
	GtkWidget *sound_play;
	GtkTreeSelection *sel;

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), 6);
	gtk_widget_show (vbox1);

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox2);
	gtk_container_add (GTK_CONTAINER (vbox1), vbox2);

	table2 = gtk_table_new (4, 3, FALSE);
	gtk_widget_show (table2);
	gtk_box_pack_start (GTK_BOX (vbox2), table2, FALSE, TRUE, 8);
	gtk_table_set_row_spacings (GTK_TABLE (table2), 2);
	gtk_table_set_col_spacings (GTK_TABLE (table2), 4);

	label2 = gtk_label_new (_("Sound playing method:"));
	gtk_widget_show (label2);
	gtk_table_attach (GTK_TABLE (table2), label2, 0, 1, 0, 1,
							(GtkAttachOptions) (GTK_FILL),
							(GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label2), 0, 0.5);

	label3 =
		gtk_label_new_with_mnemonic (_("External sound playing _program:"));
	gtk_widget_show (label3);
	gtk_table_attach (GTK_TABLE (table2), label3, 0, 1, 2, 3,
							(GtkAttachOptions) (GTK_FILL),
							(GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label3), 0, 0.5);

	sndprog_entry = gtk_entry_new ();
	if (setup_prefs.soundcmd[0] == 0)
		gtk_widget_set_sensitive (sndprog_entry, FALSE);
	else
		gtk_entry_set_text (GTK_ENTRY (sndprog_entry), setup_prefs.soundcmd);
	gtk_widget_show (sndprog_entry);
	gtk_table_attach (GTK_TABLE (table2), sndprog_entry, 1, 3, 2, 3,
							(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
							(GtkAttachOptions) (0), 0, 0);

	radio_external =
		gtk_radio_button_new_with_mnemonic (NULL, _("_External program"));
	gtk_widget_show (radio_external);
	gtk_table_attach (GTK_TABLE (table2), radio_external, 1, 3, 1, 2,
							(GtkAttachOptions) (GTK_FILL),
							(GtkAttachOptions) (0), 0, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (radio_external),
										 radio_group);
	radio_group =
		gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio_external));

	radio_auto = gtk_radio_button_new_with_mnemonic (NULL, _("_Automatic"));
	g_signal_connect (G_OBJECT (radio_auto), "toggled",
							G_CALLBACK (setup_autotoggle_cb), radio_external);
	gtk_widget_show (radio_auto);
	gtk_table_attach (GTK_TABLE (table2), radio_auto, 1, 3, 0, 1,
							(GtkAttachOptions) (GTK_FILL),
							(GtkAttachOptions) (0), 0, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (radio_auto),
										 radio_group);
	radio_group =
		gtk_radio_button_get_group (GTK_RADIO_BUTTON (radio_auto));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_auto), setup_prefs.soundcmd[0] == 0);

	label4 = gtk_label_new_with_mnemonic (_("Sound files _directory:"));
	gtk_widget_show (label4);
	gtk_table_attach (GTK_TABLE (table2), label4, 0, 1, 3, 4,
							(GtkAttachOptions) (GTK_FILL),
							(GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label4), 0, 0.5);

	snddir_entry = entry3 = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (entry3), setup_prefs.sounddir);
	gtk_widget_show (entry3);
	gtk_table_attach (GTK_TABLE (table2), entry3, 1, 3, 3, 4,
							(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
							(GtkAttachOptions) (0), 0, 0);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow1);
	gtk_container_add (GTK_CONTAINER (vbox2), scrolledwindow1);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
											  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow1),
													 GTK_SHADOW_IN);

	sound_tree = gtk_tree_view_new ();
	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (sound_tree));
	gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);
	setup_snd_add_columns (GTK_TREE_VIEW (sound_tree));
	setup_snd_populate (GTK_TREE_VIEW (sound_tree));
	g_signal_connect (G_OBJECT (sel), "changed",
							G_CALLBACK (setup_snd_row_cb), NULL);
	gtk_widget_show (sound_tree);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), sound_tree);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (sound_tree), TRUE);

	table1 = gtk_table_new (2, 3, FALSE);
	gtk_widget_show (table1);
	gtk_box_pack_start (GTK_BOX (vbox2), table1, FALSE, TRUE, 8);
	gtk_table_set_row_spacings (GTK_TABLE (table1), 2);
	gtk_table_set_col_spacings (GTK_TABLE (table1), 4);

	sound_label = gtk_label_new_with_mnemonic (_("Sound file:"));
	gtk_widget_show (sound_label);
	gtk_table_attach (GTK_TABLE (table1), sound_label, 0, 1, 0, 1,
							(GtkAttachOptions) (GTK_FILL),
							(GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (sound_label), 0, 0.5);

	sndfile_entry = gtk_entry_new ();
	g_signal_connect (G_OBJECT (sndfile_entry), "changed",
							G_CALLBACK (setup_snd_changed_cb), sound_tree);
	gtk_widget_show (sndfile_entry);
	gtk_table_attach (GTK_TABLE (table1), sndfile_entry, 0, 1, 1, 2,
							(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
							(GtkAttachOptions) (0), 0, 0);

	sound_browse = gtk_button_new_with_mnemonic (_("_Browse..."));
	g_signal_connect (G_OBJECT (sound_browse), "clicked",
							G_CALLBACK (setup_snd_browse_cb), sndfile_entry);
	gtk_widget_show (sound_browse);
	gtk_table_attach (GTK_TABLE (table1), sound_browse, 1, 2, 1, 2,
							(GtkAttachOptions) (GTK_FILL),
							(GtkAttachOptions) (0), 0, 0);

#ifdef GTK_STOCK_MEDIA_PLAY
	sound_play = gtk_button_new_from_stock (GTK_STOCK_MEDIA_PLAY);
#else
	sound_play = gtk_button_new_with_mnemonic (_("_Play"));
#endif
	g_signal_connect (G_OBJECT (sound_play), "clicked",
							G_CALLBACK (setup_snd_play_cb), sndfile_entry);
	gtk_widget_show (sound_play);
	gtk_table_attach (GTK_TABLE (table1), sound_play, 2, 3, 1, 2,
							(GtkAttachOptions) (GTK_FILL),
							(GtkAttachOptions) (0), 0, 0);

	gtk_label_set_mnemonic_widget (GTK_LABEL (label3), sndprog_entry);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label4), entry3);
	setup_snd_row_cb (sel, NULL);

	return vbox1;
}

static void
setup_add_page (const char *title, GtkWidget *book, GtkWidget *tab)
{
	GtkWidget *oframe, *frame, *label, *vvbox;
	char buf[128];

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
	label = gtk_label_new (NULL);
	snprintf (buf, sizeof (buf), "<b><big>%s</big></b>", _(title));
	gtk_label_set_markup (GTK_LABEL (label), buf);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_misc_set_padding (GTK_MISC (label), 2, 1);
	gtk_container_add (GTK_CONTAINER (frame), label);

	gtk_container_add (GTK_CONTAINER (vvbox), tab);

	gtk_notebook_append_page (GTK_NOTEBOOK (book), oframe, NULL);
}

static const char *const cata[] =
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
		N_("Sound"),
/*		N_("Advanced"),*/
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

	setup_add_page (cata[1], book, setup_create_page (textbox_settings));
	setup_add_page (cata[2], book, setup_create_page (inputbox_settings));
	setup_add_page (cata[3], book, setup_create_page (userlist_settings));
	setup_add_page (cata[4], book, setup_create_page (tabs_settings));
	setup_add_page (cata[5], book, setup_create_color_page ());
	setup_add_page (cata[8], book, setup_create_page (general_settings));
	setup_add_page (cata[9], book, setup_create_page (logging_settings));
	setup_add_page (cata[10], book, setup_create_sound_page ());
	/*setup_add_page (cata[11], book, setup_create_page (advanced_settings));*/
	setup_add_page (cata[13], book, setup_create_page (network_settings));
	setup_add_page (cata[14], book, setup_create_page (filexfer_settings));

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
	GtkWidget *frame;
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
		gtk_tree_store_set (model, &iter, 0, _(cata[i]), 1, -1, -1);
		i++;

		do
		{
			gtk_tree_store_append (model, &child_iter, &iter);
			gtk_tree_store_set (model, &child_iter, 0, _(cata[i]), 1, page, -1);
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

	frame = gtk_frame_new (NULL);
	gtk_container_add (GTK_CONTAINER (frame), tree);
	gtk_box_pack_start (GTK_BOX (box), frame, 0, 0, 0);
	gtk_box_reorder_child (GTK_BOX (box), frame, 0);

	if (sel_iter)
	{
		gtk_tree_selection_select_iter (sel, sel_iter);
		gtk_tree_iter_free (sel_iter);
	}
}

static void
setup_apply_entry_style (GtkWidget *entry)
{
	gtk_widget_modify_base (entry, GTK_STATE_NORMAL, &colors[COL_BG]);
	gtk_widget_modify_text (entry, GTK_STATE_NORMAL, &colors[COL_FG]);
	gtk_widget_modify_font (entry, input_style->font_desc);
}

static void
setup_apply_to_sess (session_gui *gui)
{
#ifdef USE_GTKSPELL
	GtkSpell *spell;
#endif

	mg_update_xtext (gui->xtext);

	if (prefs.style_namelistgad)
		gtk_widget_set_style (gui->user_tree, input_style);

	if (prefs.style_inputbox)
	{
		extern char cursor_color_rc[];
		char buf[256];
		sprintf (buf, cursor_color_rc,
				(colors[COL_FG].red >> 8),
				(colors[COL_FG].green >> 8),
				(colors[COL_FG].blue >> 8));
		gtk_rc_parse_string (buf);

		setup_apply_entry_style (gui->input_box);
		setup_apply_entry_style (gui->limit_entry);
		setup_apply_entry_style (gui->key_entry);
		setup_apply_entry_style (gui->topic_entry);
	}

	if (prefs.userlistbuttons)
		gtk_widget_show (gui->button_box);
	else
		gtk_widget_hide (gui->button_box);

#ifdef USE_GTKSPELL
	spell = gtkspell_get_from_text_view (GTK_TEXT_VIEW (gui->input_box));
	if (prefs.gui_input_spell)
	{
		if (!spell)
			gtkspell_new_attach (GTK_TEXT_VIEW (gui->input_box), NULL, NULL);
	}
	else
	{
		if (spell)
			gtkspell_detach (spell);
	}
#endif
}

static void
unslash (char *dir)
{
	int len = strlen (dir) - 1;
#ifdef WIN32
	if (dir[len] == '/' || dir[len] == '\\')
#else
	if (dir[len] == '/')
#endif
		dir[len] = 0;
}

static void
setup_apply (struct xchatprefs *pr)
{
	GSList *list;
	int done_main = FALSE;
	session *sess;
	int new_pix = FALSE;
	int noapply = FALSE;
	int do_ulist = FALSE;

	if (strcmp (pr->background, prefs.background) != 0)
		new_pix = TRUE;

#define DIFF(a) (pr->a != prefs.a)

	if (DIFF (paned_userlist))
		noapply = TRUE;
	if (DIFF (lagometer))
		noapply = TRUE;
	if (DIFF (throttlemeter))
		noapply = TRUE;
	if (DIFF (showhostname_in_userlist))
		noapply = TRUE;
	if (DIFF (tab_small))
		noapply = TRUE;
	if (DIFF (tab_sort))
		noapply = TRUE;
	if (DIFF (use_server_tab))
		noapply = TRUE;
	if (DIFF (style_namelistgad))
		noapply = TRUE;
	if (DIFF (truncchans))
		noapply = TRUE;

	if (color_change || (DIFF (away_size_max)) || (DIFF (away_track)))
		do_ulist = TRUE;

	memcpy (&prefs, pr, sizeof (prefs));

	/* remove trailing slashes */
	unslash (prefs.dccdir);
	unslash (prefs.dcc_completed_dir);

	mkdir_utf8 (prefs.dccdir);
	mkdir_utf8 (prefs.dcc_completed_dir);

	if (new_pix)
	{
		if (channelwin_pix)
			g_object_unref (channelwin_pix);
		channelwin_pix = pixmap_load_from_file (prefs.background);
	}

	input_style = create_input_style (input_style);

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
				setup_apply_to_sess (sess->gui);
			}
		} else
		{
			setup_apply_to_sess (sess->gui);
		}

		if (prefs.logging)
			log_open (sess);
		else
			log_close (sess);

		if (do_ulist)
			userlist_rehash (sess);

		list = list->next;
	}

	mg_apply_setup ();

	if (noapply)
		fe_message (_("Some settings were changed that require a"
						" restart to take full effect."), FE_MSG_WARN);

#ifndef WIN32
	if (prefs.autodccsend)
	{
		if (!strcmp ((char *)g_get_home_dir (), prefs.dccdir))
		{
			fe_message (_("*WARNING*\n"
							 "Auto accepting DCC to your home directory\n"
							 "can be dangerous and is exploitable. Eg:\n"
							 "Someone could send you a .bash_profile"), FE_MSG_WARN);
		}
	}
#endif
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
	setup_snd_apply ();
	gtk_widget_destroy (win);
	setup_apply (&setup_prefs);
	save_config ();
	palette_save ();
}

static GtkWidget *
setup_window_open (void)
{
	GtkWidget *wid, *win, *vbox, *hbox, *hbbox;

	win = gtkutil_window_new (_("XChat: Preferences"), "prefs", 0, 0, 3);

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

	cancel_button = wid = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
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

	color_change = FALSE;
	setup_window = setup_window_open ();

	g_signal_connect (G_OBJECT (setup_window), "destroy",
							G_CALLBACK (setup_close_cb), &setup_window);
}
