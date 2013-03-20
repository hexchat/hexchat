/* X-Chat
 * Copyright (C) 2004-2007 Peter Zelezny.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../common/hexchat.h"
#include "../common/cfgfiles.h"
#include "../common/fe.h"
#include "../common/text.h"
#include "../common/userlist.h"
#include "../common/util.h"
#include "../common/hexchatc.h"
#include "fe-gtk.h"
#include "gtkutil.h"
#include "maingui.h"
#include "palette.h"
#include "pixmaps.h"
#include "menu.h"
#include "plugin-tray.h"

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
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkcombobox.h>
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
#ifdef USE_LIBSEXY
#include "sexy-spell-entry.h"
#endif

GtkStyle *create_input_style (GtkStyle *);

#define LABEL_INDENT 12

static int last_selected_page = 0;
static int last_selected_row = 0; /* sound row */
static gboolean color_change;
static struct hexchatprefs setup_prefs;
static GtkWidget *cancel_button;
static GtkWidget *font_dialog = NULL;

enum
{
	ST_END,
	ST_TOGGLE,
	ST_TOGGLR,
	ST_3OGGLE,
	ST_ENTRY,
	ST_EFONT,
	ST_EFILE,
	ST_EFOLDER,
	ST_MENU,
	ST_RADIO,
	ST_NUMBER,
	ST_HSCALE,
	ST_HEADER,
	ST_LABEL,
	ST_ALERTHEAD
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

#ifdef WIN32
static const char *const langsmenu[] =
{
	N_("Afrikaans"),
	N_("Albanian"),
	N_("Amharic"),
	N_("Asturian"),
	N_("Azerbaijani"),
	N_("Basque"),
	N_("Belarusian"),
	N_("Bulgarian"),
	N_("Catalan"),
	N_("Chinese (Simplified)"),
	N_("Chinese (Traditional)"),
	N_("Czech"),
	N_("Danish"),
	N_("Dutch"),
	N_("English (British)"),
	N_("English"),
	N_("Estonian"),
	N_("Finnish"),
	N_("French"),
	N_("Galician"),
	N_("German"),
	N_("Greek"),
	N_("Gujarati"),
	N_("Hindi"),
	N_("Hungarian"),
	N_("Indonesian"),
	N_("Italian"),
	N_("Japanese"),
	N_("Kannada"),
	N_("Kinyarwanda"),
	N_("Korean"),
	N_("Latvian"),
	N_("Lithuanian"),
	N_("Macedonian"),
	N_("Malay"),
	N_("Malayalam"),
	N_("Norwegian (Bokmal)"),
	N_("Norwegian (Nynorsk)"),
	N_("Polish"),
	N_("Portuguese"),
	N_("Portuguese (Brazilian)"),
	N_("Punjabi"),
	N_("Russian"),
	N_("Serbian"),
	N_("Slovak"),
	N_("Slovenian"),
	N_("Spanish"),
	N_("Swedish"),
	N_("Thai"),
	N_("Ukrainian"),
	N_("Vietnamese"),
	N_("Walloon"),
	NULL
};
#endif

static const setting appearance_settings[] =
{
	{ST_HEADER,	N_("General"),0,0,0},
#ifdef WIN32
	{ST_MENU,   N_("Language:"), P_OFFINTNL(hex_gui_lang), 0, langsmenu, 0},
	{ST_EFONT,  N_("Main font:"), P_OFFSETNL(hex_text_font_main), 0, 0, sizeof prefs.hex_text_font_main},
#else
	{ST_EFONT,  N_("Font:"), P_OFFSETNL(hex_text_font), 0, 0, sizeof prefs.hex_text_font},
#endif

	{ST_HEADER,	N_("Text Box"),0,0,0},
	{ST_TOGGLE, N_("Colored nick names"), P_OFFINTNL(hex_text_color_nicks), N_("Give each person on IRC a different color"),0,0},
	{ST_TOGGLR, N_("Indent nick names"), P_OFFINTNL(hex_text_indent), N_("Make nick names right-justified"),0,0},
#if defined(USE_XLIB) || defined(WIN32)
	{ST_TOGGLE, N_("Transparent background"), P_OFFINTNL(hex_text_transparent),0,0,0},
	{ST_TOGGLR, N_("Show marker line"), P_OFFINTNL(hex_text_show_marker), N_("Insert a red line after the last read text."),0,0},
	{ST_EFILE,  N_("Background image:"), P_OFFSETNL(hex_text_background), 0, 0, sizeof prefs.hex_text_background},

	{ST_HEADER, N_("Transparency Settings"), 0,0,0},
	{ST_HSCALE, N_("Red:"), P_OFFINTNL(hex_text_tint_red),0,0,0},
	{ST_HSCALE, N_("Green:"), P_OFFINTNL(hex_text_tint_green),0,0,0},
	{ST_HSCALE, N_("Blue:"), P_OFFINTNL(hex_text_tint_blue),0,0,0},
#else
	{ST_TOGGLE, N_("Show marker line"), P_OFFINTNL(hex_text_show_marker), N_("Insert a red line after the last read text."),0,0},
	{ST_EFILE,  N_("Background image:"), P_OFFSETNL(hex_text_background), 0, 0, sizeof prefs.hex_text_background},
#endif

	{ST_HEADER,	N_("Time Stamps"),0,0,0},
	{ST_TOGGLE, N_("Enable time stamps"), P_OFFINTNL(hex_stamp_text),0,0,2},
	{ST_ENTRY,  N_("Time stamp format:"), P_OFFSETNL(hex_stamp_text_format),
#ifdef WIN32
					N_("See the strftime MSDN article for details."),0,sizeof prefs.hex_stamp_text_format},
#else
					N_("See the strftime manpage for details."),0,sizeof prefs.hex_stamp_text_format},
#endif

	{ST_HEADER,	N_("Title Bar"),0,0,0},
	{ST_TOGGLE, N_("Show channel modes"), P_OFFINTNL(hex_gui_win_modes),0,0,0},
	{ST_TOGGLR, N_("Show number of users"), P_OFFINTNL(hex_gui_win_ucount),0,0,0},

	{ST_END, 0, 0, 0, 0, 0}
};

static const char *const tabcompmenu[] = 
{
	N_("A-Z"),
	N_("Last-spoke order"),
	NULL
};

static const setting inputbox_settings[] =
{
	{ST_HEADER, N_("Input Box"),0,0,0},
	{ST_TOGGLE, N_("Use the Text box font and colors"), P_OFFINTNL(hex_gui_input_style),0,0,0},
	{ST_TOGGLE, N_("Show nick box"), P_OFFINTNL(hex_gui_input_nick),0,0,0},
	{ST_TOGGLE, N_("Show user mode icon in nick box"), P_OFFINTNL(hex_gui_input_icon),0,0,0},
#if defined(USE_GTKSPELL) || defined(USE_LIBSEXY)
	{ST_TOGGLE, N_("Spell checking"), P_OFFINTNL(hex_gui_input_spell),0,0,2},
#ifdef HAVE_ISO_CODES /* Defined with static spelling */
	{ST_ENTRY,	N_("Dictionaries to use:"), P_OFFSETNL(hex_text_spell_langs),0,0,sizeof prefs.hex_text_spell_langs},
#ifdef WIN32
	{ST_LABEL,	N_("Use language codes (as in \"share\\myspell\\dicts\").\nSeparate multiple entries with commas.")},
#else
	{ST_LABEL,	N_("Use language codes. Separate multiple entries with commas.")},
#endif
#endif
#endif

	{ST_HEADER, N_("Nick Completion"),0,0,0},
	{ST_TOGGLE, N_("Automatic nick completion (without TAB key)"), P_OFFINTNL(hex_completion_auto),
					0,0,0},
	{ST_ENTRY,	N_("Nick completion suffix:"), P_OFFSETNL(hex_completion_suffix),0,0,sizeof prefs.hex_completion_suffix},
	{ST_MENU,	N_("Nick completion sorted:"), P_OFFINTNL(hex_completion_sort), 0, tabcompmenu, 0},

#if 0	/* obsolete */
	{ST_HEADER, N_("Input Box Codes"),0,0,0},
	{ST_TOGGLE, N_("Interpret %nnn as an ASCII value"), P_OFFINTNL(hex_input_perc_ascii),0,0,0},
	{ST_TOGGLE, N_("Interpret %C, %B as Color, Bold etc"), P_OFFINTNL(hex_input_perc_color),0,0,0},
#endif

	{ST_END, 0, 0, 0, 0, 0}
};

static const char *const lagmenutext[] = 
{
	N_("Off"),
	N_("Graphical"),
	N_("Text"),
	N_("Both"),
	NULL
};

static const char *const ulmenutext[] = 
{
	N_("A-Z, Ops first"),
	N_("A-Z"),
	N_("Z-A, Ops last"),
	N_("Z-A"),
	N_("Unsorted"),
	NULL
};

static const char *const cspos[] =
{
	N_("Left (Upper)"),
	N_("Left (Lower)"),
	N_("Right (Upper)"),
	N_("Right (Lower)"),
	N_("Top"),
	N_("Bottom"),
	N_("Hidden"),
	NULL
};

static const char *const ulpos[] =
{
	N_("Left (Upper)"),
	N_("Left (Lower)"),
	N_("Right (Upper)"),
	N_("Right (Lower)"),
	NULL
};

static const setting userlist_settings[] =
{
	{ST_HEADER,	N_("User List"),0,0,0},
	{ST_TOGGLE, N_("Show hostnames in user list"), P_OFFINTNL(hex_gui_ulist_show_hosts), 0, 0, 0},
	{ST_TOGGLE, N_("Use the Text box font and colors"), P_OFFINTNL(hex_gui_ulist_style),0,0,0},
	{ST_TOGGLE, N_("Show icons for user modes"), P_OFFINTNL(hex_gui_ulist_icons), N_("Use graphical icons instead of text symbols in the user list."), 0, 0},
	{ST_TOGGLE, N_("Show user count in channels"), P_OFFINTNL(hex_gui_ulist_count), 0, 0, 0},
/*	{ST_TOGGLE, N_("Resizable user list"), P_OFFINTNL(hex_gui_ulist_resizable),0,0,0},*/
	{ST_MENU,	N_("User list sorted by:"), P_OFFINTNL(hex_gui_ulist_sort), 0, ulmenutext, 0},
	{ST_MENU,	N_("Show user list at:"), P_OFFINTNL(hex_gui_ulist_pos), 0, ulpos, 1},

	{ST_HEADER,	N_("Away Tracking"),0,0,0},
	{ST_TOGGLE,	N_("Track the Away status of users and mark them in a different color"), P_OFFINTNL(hex_away_track),0,0,2},
	{ST_NUMBER, N_("On channels smaller than:"), P_OFFINTNL(hex_away_size_max),0,0,10000},

	{ST_HEADER,	N_("Action Upon Double Click"),0,0,0},
	{ST_ENTRY,	N_("Execute command:"), P_OFFSETNL(hex_gui_ulist_doubleclick), 0, 0, sizeof prefs.hex_gui_ulist_doubleclick},

	{ST_HEADER,	N_("Extra Gadgets"),0,0,0},
	{ST_MENU,	N_("Lag meter:"), P_OFFINTNL(hex_gui_lagometer), 0, lagmenutext, 0},
	{ST_MENU,	N_("Throttle meter:"), P_OFFINTNL(hex_gui_throttlemeter), 0, lagmenutext, 0},

	{ST_END, 0, 0, 0, 0, 0}
};

static const char *const tabwin[] =
{
	N_("Windows"),
	N_("Tabs"),
	NULL
};

static const char *const focusnewtabsmenu[] =
{
	N_("Never"),
	N_("Always"),
	N_("Only requested tabs"),
	NULL
};

static const char *const noticeposmenu[] =
{
	N_("Automatic"),
	N_("In an extra tab"),
	N_("In the front tab"),
	NULL
};

static const char *const swtype[] =
{
	N_("Tabs"),	/* 0 tabs */
	"",			/* 1 reserved */
	N_("Tree"),	/* 2 tree */
	NULL
};

static const setting tabs_settings[] =
{
	/*{ST_HEADER,	N_("Channel Switcher"),0,0,0},*/
	{ST_RADIO,  N_("Switcher type:"),P_OFFINTNL(hex_gui_tab_layout), 0, swtype, 0},
	{ST_TOGGLE, N_("Open an extra tab for server messages"), P_OFFINTNL(hex_gui_tab_server), 0, 0, 0},
	{ST_TOGGLE, N_("Open a new tab when you receive a private message"), P_OFFINTNL(hex_gui_autoopen_dialog), 0, 0, 0},
	{ST_TOGGLE, N_("Sort tabs in alphabetical order"), P_OFFINTNL(hex_gui_tab_sort), 0, 0, 0},
	{ST_TOGGLE, N_("Show icons in the channel tree"), P_OFFINTNL(hex_gui_tab_icons), 0, 0, 0},
	{ST_TOGGLE, N_("Show dotted lines in the channel tree"), P_OFFINTNL(hex_gui_tab_dots), 0, 0, 0},
	{ST_TOGGLE, N_("Smaller text"), P_OFFINTNL(hex_gui_tab_small), 0, 0, 0},
	{ST_MENU,	N_("Focus new tabs:"), P_OFFINTNL(hex_gui_tab_newtofront), 0, focusnewtabsmenu, 0},
	{ST_MENU,	N_("Placement of notices:"), P_OFFINTNL(hex_irc_notice_pos), 0, noticeposmenu, 0},
	{ST_MENU,	N_("Show channel switcher at:"), P_OFFINTNL(hex_gui_tab_pos), 0, cspos, 1},
	{ST_NUMBER,	N_("Shorten tab labels to:"), P_OFFINTNL(hex_gui_tab_trunc), 0, (const char **)N_("letters."), 99},

	{ST_HEADER,	N_("Tabs or Windows"),0,0,0},
	{ST_MENU,	N_("Open channels in:"), P_OFFINTNL(hex_gui_tab_chans), 0, tabwin, 0},
	{ST_MENU,	N_("Open dialogs in:"), P_OFFINTNL(hex_gui_tab_dialogs), 0, tabwin, 0},
	{ST_MENU,	N_("Open utilities in:"), P_OFFINTNL(hex_gui_tab_utils), N_("Open DCC, Ignore, Notify etc, in tabs or windows?"), tabwin, 0},

	{ST_END, 0, 0, 0, 0, 0}
};

static const setting color_settings[] =
{
	{ST_TOGGLE, N_("Messages"), P_OFFINTNL(hex_text_stripcolor_msg), 0, 0, 0},
	{ST_TOGGLE, N_("Scrollback"), P_OFFINTNL(hex_text_stripcolor_replay), 0, 0, 0},
	{ST_TOGGLE, N_("Topic"), P_OFFINTNL(hex_text_stripcolor_topic), 0, 0, 0},

	{ST_END, 0, 0, 0, 0, 0}
};

static const char *const dccaccept[] =
{
	N_("Ask for confirmation"),
	N_("Ask for download folder"),
	N_("Save without interaction"),
	NULL
};

static const setting filexfer_settings[] =
{
	{ST_HEADER, N_("Files and Directories"), 0, 0, 0},
	{ST_MENU,	N_("Auto accept file offers:"), P_OFFINTNL(hex_dcc_auto_recv), 0, dccaccept, 0},
	{ST_EFOLDER,N_("Download files to:"), P_OFFSETNL(hex_dcc_dir), 0, 0, sizeof prefs.hex_dcc_dir},
	{ST_EFOLDER,N_("Move completed files to:"), P_OFFSETNL(hex_dcc_completed_dir), 0, 0, sizeof prefs.hex_dcc_completed_dir},
	{ST_TOGGLE, N_("Save nick name in filenames"), P_OFFINTNL(hex_dcc_save_nick), 0, 0, 0},

	{ST_HEADER, N_("Network Settings"), 0, 0, 0},
	{ST_TOGGLE, N_("Get my address from the IRC server"), P_OFFINTNL(hex_dcc_ip_from_server),
					N_("Asks the IRC server for your real address. Use this if you have a 192.168.*.* address!"), 0, 0},
	{ST_ENTRY,	N_("DCC IP address:"), P_OFFSETNL(hex_dcc_ip),
					N_("Claim you are at this address when offering files."), 0, sizeof prefs.hex_dcc_ip},
	{ST_NUMBER,	N_("First DCC send port:"), P_OFFINTNL(hex_dcc_port_first), 0, 0, 65535},
	{ST_NUMBER,	N_("Last DCC send port:"), P_OFFINTNL(hex_dcc_port_last), 0, 
		(const char **)N_("!Leave ports at zero for full range."), 65535},

	{ST_HEADER, N_("Maximum File Transfer Speeds (bytes per second)"), 0, 0, 0},
	{ST_NUMBER,	N_("One upload:"), P_OFFINTNL(hex_dcc_max_send_cps), 
					N_("Maximum speed for one transfer"), 0, 1000000},
	{ST_NUMBER,	N_("One download:"), P_OFFINTNL(hex_dcc_max_get_cps),
					N_("Maximum speed for one transfer"), 0, 1000000},
	{ST_NUMBER,	N_("All uploads combined:"), P_OFFINTNL(hex_dcc_global_max_send_cps),
					N_("Maximum speed for all files"), 0, 1000000},
	{ST_NUMBER,	N_("All downloads combined:"), P_OFFINTNL(hex_dcc_global_max_get_cps),
					N_("Maximum speed for all files"), 0, 1000000},

	{ST_END, 0, 0, 0, 0, 0}
};

static const int balloonlist[3] =
{
	P_OFFINTNL(hex_input_balloon_chans), P_OFFINTNL(hex_input_balloon_priv), P_OFFINTNL(hex_input_balloon_hilight)
};

static const int trayblinklist[3] =
{
	P_OFFINTNL(hex_input_tray_chans), P_OFFINTNL(hex_input_tray_priv), P_OFFINTNL(hex_input_tray_hilight)
};

static const int taskbarlist[3] =
{
	P_OFFINTNL(hex_input_flash_chans), P_OFFINTNL(hex_input_flash_priv), P_OFFINTNL(hex_input_flash_hilight)
};

static const int beeplist[3] =
{
	P_OFFINTNL(hex_input_beep_chans), P_OFFINTNL(hex_input_beep_priv), P_OFFINTNL(hex_input_beep_hilight)
};

static const setting alert_settings[] =
{
	{ST_HEADER,	N_("Alerts"),0,0,0},

	{ST_ALERTHEAD},
#ifndef WIN32
	{ST_3OGGLE, N_("Show tray balloons on:"), 0, 0, (void *)balloonlist, 0},
#endif
	{ST_3OGGLE, N_("Blink tray icon on:"), 0, 0, (void *)trayblinklist, 0},
	{ST_3OGGLE, N_("Blink task bar on:"), 0, 0, (void *)taskbarlist, 0},
	{ST_3OGGLE, N_("Make a beep sound on:"), 0, 0, (void *)beeplist, 0},

	{ST_TOGGLE,	N_("Omit alerts when marked as being away"), P_OFFINTNL(hex_away_omit_alerts), 0, 0, 0},

	{ST_HEADER,	N_("Tray Behavior"), 0, 0, 0},
	{ST_TOGGLE,	N_("Enable system tray icon"), P_OFFINTNL(hex_gui_tray), 0, 0, 0},
	{ST_TOGGLE,	N_("Minimize to tray"), P_OFFINTNL(hex_gui_tray_minimize), 0, 0, 0},
	{ST_TOGGLE,	N_("Close to tray"), P_OFFINTNL(hex_gui_tray_close), 0, 0, 0},
	{ST_TOGGLE,	N_("Automatically mark away/back"), P_OFFINTNL(hex_gui_tray_away), N_("When hiding to tray automatically change status."), 0, 0},
#ifndef WIN32
	{ST_TOGGLE,	N_("Only show tray balloons when hidden or iconified"), P_OFFINTNL(hex_gui_tray_quiet), 0, 0, 0},
#endif

	{ST_HEADER,	N_("Highlighted Messages"),0,0,0},
	{ST_LABEL,	N_("Highlighted messages are ones where your nickname is mentioned, but also:"), 0, 0, 0, 1},

	{ST_ENTRY,	N_("Extra words to highlight:"), P_OFFSETNL(hex_irc_extra_hilight), 0, 0, sizeof prefs.hex_irc_extra_hilight},
	{ST_ENTRY,	N_("Nick names not to highlight:"), P_OFFSETNL(hex_irc_no_hilight), 0, 0, sizeof prefs.hex_irc_no_hilight},
	{ST_ENTRY,	N_("Nick names to always highlight:"), P_OFFSETNL(hex_irc_nick_hilight), 0, 0, sizeof prefs.hex_irc_nick_hilight},
	{ST_LABEL,	N_("Separate multiple words with commas.\nWildcards are accepted.")},

	{ST_END, 0, 0, 0, 0, 0}
};

static const setting alert_settings_hextray[] =
{
	{ST_HEADER,	N_("Alerts"),0,0,0},

	{ST_ALERTHEAD},
	{ST_3OGGLE, N_("Blink task bar on:"), 0, 0, (void *)taskbarlist, 0},
	{ST_3OGGLE, N_("Make a beep sound on:"), 0, 0, (void *)beeplist, 0},

	{ST_TOGGLE,	N_("Omit alerts when marked as being away"), P_OFFINTNL(hex_away_omit_alerts), 0, 0, 0},

	{ST_HEADER,	N_("Highlighted Messages"),0,0,0},
	{ST_LABEL,	N_("Highlighted messages are ones where your nickname is mentioned, but also:"), 0, 0, 0, 1},

	{ST_ENTRY,	N_("Extra words to highlight:"), P_OFFSETNL(hex_irc_extra_hilight), 0, 0, sizeof prefs.hex_irc_extra_hilight},
	{ST_ENTRY,	N_("Nick names not to highlight:"), P_OFFSETNL(hex_irc_no_hilight), 0, 0, sizeof prefs.hex_irc_no_hilight},
	{ST_ENTRY,	N_("Nick names to always highlight:"), P_OFFSETNL(hex_irc_nick_hilight), 0, 0, sizeof prefs.hex_irc_nick_hilight},
	{ST_LABEL,	N_("Separate multiple words with commas.\nWildcards are accepted.")},

	{ST_END, 0, 0, 0, 0, 0}
};

static const setting general_settings[] =
{
	{ST_HEADER,	N_("Default Messages"),0,0,0},
	{ST_ENTRY,	N_("Quit:"), P_OFFSETNL(hex_irc_quit_reason), 0, 0, sizeof prefs.hex_irc_quit_reason},
	{ST_ENTRY,	N_("Leave channel:"), P_OFFSETNL(hex_irc_part_reason), 0, 0, sizeof prefs.hex_irc_part_reason},
	{ST_ENTRY,	N_("Away:"), P_OFFSETNL(hex_away_reason), 0, 0, sizeof prefs.hex_away_reason},

	{ST_HEADER,	N_("Away"),0,0,0},
	{ST_TOGGLE,	N_("Announce away messages"), P_OFFINTNL(hex_away_show_message),
					N_("Announce your away messages to all channels."), 0, 0},
	{ST_TOGGLE,	N_("Show away once"), P_OFFINTNL(hex_away_show_once), N_("Show identical away messages only once."), 0, 0},
	{ST_TOGGLE,	N_("Automatically unmark away"), P_OFFINTNL(hex_away_auto_unmark), N_("Unmark yourself as away before sending messages."), 0, 0},

	{ST_END, 0, 0, 0, 0, 0}
};

static const setting advanced_settings[] =
{
	{ST_HEADER,	N_("Advanced Settings"),0,0,0},
	{ST_ENTRY,  N_("Real name:"), P_OFFSETNL(hex_irc_real_name), 0, 0, sizeof prefs.hex_irc_real_name},
#ifdef WIN32
	{ST_ENTRY,  N_("Alternative fonts:"), P_OFFSETNL(hex_text_font_alternative), "Separate multiple entries with commas without spaces before or after.", 0, sizeof prefs.hex_text_font_alternative},
#endif
	{ST_NUMBER,	N_("Auto reconnect delay:"), P_OFFINTNL(hex_net_reconnect_delay), 0, 0, 9999},
	{ST_NUMBER,	N_("Auto join delay:"), P_OFFINTNL(hex_irc_join_delay), 0, 0, 9999},
	{ST_TOGGLE,	N_("Display MODEs in raw form"), P_OFFINTNL(hex_irc_raw_modes), 0, 0, 0},
	{ST_TOGGLE,	N_("Whois on notify"), P_OFFINTNL(hex_notify_whois_online), N_("Sends a /WHOIS when a user comes online in your notify list."), 0, 0},
	{ST_TOGGLE,	N_("Hide join and part messages"), P_OFFINTNL(hex_irc_conf_mode), N_("Hide channel join/part messages by default."), 0, 0},
	{ST_TOGGLE,	N_("Display lists in compact mode"), P_OFFINTNL(hex_gui_compact), N_("Use less spacing between user list/channel tree rows."), 0, 0},
	{ST_HEADER,	N_("Auto Open DCC Windows"),0,0,0},
	{ST_TOGGLE, N_("Send window"), P_OFFINTNL(hex_gui_autoopen_send), 0, 0, 0},
	{ST_TOGGLE, N_("Receive window"), P_OFFINTNL(hex_gui_autoopen_recv), 0, 0, 0},
	{ST_TOGGLE, N_("Chat window"), P_OFFINTNL(hex_gui_autoopen_chat), 0, 0, 0},
	{ST_HEADER,	N_("Auto Copy Behavior"),0,0,0},
	{ST_TOGGLE, N_("Automatically copy selected text"), P_OFFINTNL(hex_text_autocopy_text),
					N_("Copy selected text to clipboard when left mouse button is released. "
						"Otherwise, CONTROL-SHIFT-C will copy the "
						"selected text to the clipboard."), 0, 0},
	{ST_TOGGLE, N_("Automatically include time stamps"), P_OFFINTNL(hex_text_autocopy_stamp),
					N_("Automatically include time stamps in copied lines of text. Otherwise, "
						"include time stamps if the SHIFT key is held down while selecting."), 0, 0},
	{ST_TOGGLE, N_("Automatically include color information"), P_OFFINTNL(hex_text_autocopy_color),
					N_("Automatically include color information in copied lines of text.  "
						"Otherwise, include color information if the CONTROL key is held down "
						"while selecting."), 0, 0},

	{ST_END, 0, 0, 0, 0, 0}
};

#ifdef WIN32
static const setting advanced_settings_oneinstance[] =
{
	{ST_HEADER,	N_("Advanced Settings"),0,0,0},
	{ST_ENTRY,  N_("Alternative fonts:"), P_OFFSETNL(hex_text_font_alternative), "Separate multiple entries with commas without spaces before or after.", 0, sizeof prefs.hex_text_font_alternative},
	{ST_NUMBER,	N_("Auto reconnect delay:"), P_OFFINTNL(hex_net_reconnect_delay), 0, 0, 9999},
	{ST_NUMBER,	N_("Auto join delay:"), P_OFFINTNL(hex_irc_join_delay), 0, 0, 9999},
	{ST_TOGGLE,	N_("Display MODEs in raw form"), P_OFFINTNL(hex_irc_raw_modes), 0, 0, 0},
	{ST_TOGGLE,	N_("Whois on notify"), P_OFFINTNL(hex_notify_whois_online), N_("Sends a /WHOIS when a user comes online in your notify list."), 0, 0},
	{ST_TOGGLE,	N_("Hide join and part messages"), P_OFFINTNL(hex_irc_conf_mode), N_("Hide channel join/part messages by default."), 0, 0},
	/* {ST_TOGGLE,	N_("Allow only one instance of HexChat to run"), P_OFFINTNL(hex_gui_single), 0, 0, 0}, */
	{ST_TOGGLE,	N_("Display lists in compact mode"), P_OFFINTNL(hex_gui_compact), N_("Use less spacing between user list/channel tree rows."), 0, 0},
	{ST_HEADER,	N_("Auto Open DCC Windows"),0,0,0},
	{ST_TOGGLE, N_("Send window"), P_OFFINTNL(hex_gui_autoopen_send), 0, 0, 0},
	{ST_TOGGLE, N_("Receive window"), P_OFFINTNL(hex_gui_autoopen_recv), 0, 0, 0},
	{ST_TOGGLE, N_("Chat window"), P_OFFINTNL(hex_gui_autoopen_chat), 0, 0, 0},
	{ST_HEADER,	N_("Auto Copy Behavior"),0,0,0},
	{ST_TOGGLE, N_("Automatically copy selected text"), P_OFFINTNL(hex_text_autocopy_text),
					N_("Copy selected text to clipboard when left mouse button is released. "
						"Otherwise, CONTROL-SHIFT-C will copy the "
						"selected text to the clipboard."), 0, 0},
	{ST_TOGGLE, N_("Automatically include time stamps"), P_OFFINTNL(hex_text_autocopy_stamp),
					N_("Automatically include time stamps in copied lines of text. Otherwise, "
						"include time stamps if the SHIFT key is held down while selecting."), 0, 0},
	{ST_TOGGLE, N_("Automatically include color information"), P_OFFINTNL(hex_text_autocopy_color),
					N_("Automatically include color information in copied lines of text.  "
						"Otherwise, include color information if the CONTROL key is held down "
						"while selecting."), 0, 0},

	{ST_END, 0, 0, 0, 0, 0}
};
#endif

static const setting logging_settings[] =
{
	{ST_HEADER,	N_("Logging"),0,0,0},
	{ST_TOGGLE,	N_("Display scrollback from previous session"), P_OFFINTNL(hex_text_replay), 0, 0, 0},
	{ST_NUMBER,	N_("Scrollback lines:"), P_OFFINTNL(hex_text_max_lines),0,0,100000},
	{ST_TOGGLE,	N_("Enable logging of conversations to disk"), P_OFFINTNL(hex_irc_logging), 0, 0, 2},
	{ST_ENTRY,	N_("Log filename:"), P_OFFSETNL(hex_irc_logmask), 0, 0, sizeof prefs.hex_irc_logmask},
	{ST_LABEL,	N_("%s=Server %c=Channel %n=Network.")},

	{ST_HEADER,	N_("Time Stamps"),0,0,0},
	{ST_TOGGLE,	N_("Insert timestamps in logs"), P_OFFINTNL(hex_stamp_log), 0, 0, 2},
	{ST_ENTRY,	N_("Log timestamp format:"), P_OFFSETNL(hex_stamp_log_format), 0, 0, sizeof prefs.hex_stamp_log_format},
#ifdef WIN32
	{ST_LABEL,	N_("See the strftime MSDN article for details.")},
#else
	{ST_LABEL,	N_("See the strftime manpage for details.")},
#endif

	{ST_HEADER,	N_("URLs"),0,0,0},
	{ST_TOGGLE,	N_("Enable logging of URLs to disk"), P_OFFINTNL(hex_url_logging), 0, 0, 0},
	{ST_TOGGLE,	N_("Enable URL grabber"), P_OFFINTNL(hex_url_grabber), 0, 0, 2},
	{ST_NUMBER,	N_("Maximum number of URLs to grab:"), P_OFFINTNL(hex_url_grabber_limit), 0, 0, 9999},

	{ST_END, 0, 0, 0, 0, 0}
};

static const char *const proxytypes[] =
{
	N_("(Disabled)"),
	N_("Wingate"),
	N_("Socks4"),
	N_("Socks5"),
	N_("HTTP"),
#ifdef USE_MSPROXY
	N_("MS Proxy (ISA)"),
#endif
#ifdef USE_LIBPROXY
	N_("Auto"),
#endif
	NULL
};

static const char *const proxyuse[] =
{
	N_("All Connections"),
	N_("IRC Server Only"),
	N_("DCC Get Only"),
	NULL
};

static const setting network_settings[] =
{
	{ST_HEADER,	N_("Your Address"), 0, 0, 0, 0},
	{ST_ENTRY,	N_("Bind to:"), P_OFFSETNL(hex_net_bind_host), 0, 0, sizeof prefs.hex_net_bind_host},
	{ST_LABEL,	N_("Only useful for computers with multiple addresses.")},

	{ST_HEADER,	N_("Proxy Server"), 0, 0, 0, 0},
	{ST_ENTRY,	N_("Hostname:"), P_OFFSETNL(hex_net_proxy_host), 0, 0, sizeof prefs.hex_net_proxy_host},
	{ST_NUMBER,	N_("Port:"), P_OFFINTNL(hex_net_proxy_port), 0, 0, 65535},
	{ST_MENU,	N_("Type:"), P_OFFINTNL(hex_net_proxy_type), 0, proxytypes, 0},
	{ST_MENU,	N_("Use proxy for:"), P_OFFINTNL(hex_net_proxy_use), 0, proxyuse, 0},

	{ST_HEADER,	N_("Proxy Authentication"), 0, 0, 0, 0},
#ifdef USE_MSPROXY
	{ST_TOGGLE,	N_("Use Authentication (MS Proxy, HTTP or Socks5 only)"), P_OFFINTNL(hex_net_proxy_auth), 0, 0, 0},
#else
	{ST_TOGGLE,	N_("Use Authentication (HTTP or Socks5 only)"), P_OFFINTNL(hex_net_proxy_auth), 0, 0, 0},
#endif
	{ST_ENTRY,	N_("Username:"), P_OFFSETNL(hex_net_proxy_user), 0, 0, sizeof prefs.hex_net_proxy_user},
	{ST_ENTRY,	N_("Password:"), P_OFFSETNL(hex_net_proxy_pass), 0, GINT_TO_POINTER(1), sizeof prefs.hex_net_proxy_pass},

	{ST_END, 0, 0, 0, 0, 0}
};

#define setup_get_str(pr,set) (((char *)pr)+set->offset)
#define setup_get_int(pr,set) *(((int *)pr)+set->offset)
#define setup_get_int3(pr,off) *(((int *)pr)+off) 

#define setup_set_int(pr,set,num) *((int *)pr+set->offset)=num
#define setup_set_str(pr,set,str) strcpy(((char *)pr)+set->offset,str)


static void
setup_3oggle_cb (GtkToggleButton *but, unsigned int *setting)
{
	*setting = but->active;
}

static void
setup_headlabel (GtkWidget *tab, int row, int col, char *text)
{
	GtkWidget *label;
	char buf[128];
	char *sp;

	snprintf (buf, sizeof (buf), "<b><span size=\"smaller\">%s</span></b>", text);
	sp = strchr (buf + 17, ' ');
	if (sp)
		*sp = '\n';

	label = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (label), buf);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (tab), label, col, col + 1, row, row + 1, 0, 0, 4, 0);
}

static void
setup_create_alert_header (GtkWidget *tab, int row, const setting *set)
{
	setup_headlabel (tab, row, 3, _("Channel Message"));
	setup_headlabel (tab, row, 4, _("Private Message"));
	setup_headlabel (tab, row, 5, _("Highlighted Message"));
}

/* makes 3 toggles side-by-side */

static void
setup_create_3oggle (GtkWidget *tab, int row, const setting *set)
{
	GtkWidget *label, *wid;
	int *offsets = (int *)set->list;

	label = gtk_label_new (_(set->label));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (tab), label, 2, 3, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, LABEL_INDENT, 0);

	wid = gtk_check_button_new ();
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wid),
											setup_get_int3 (&setup_prefs, offsets[0]));
	g_signal_connect (G_OBJECT (wid), "toggled",
							G_CALLBACK (setup_3oggle_cb), ((int *)&setup_prefs) + offsets[0]);
	gtk_table_attach (GTK_TABLE (tab), wid, 3, 4, row, row + 1, 0, 0, 0, 0);

	wid = gtk_check_button_new ();
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wid),
											setup_get_int3 (&setup_prefs, offsets[1]));
	g_signal_connect (G_OBJECT (wid), "toggled",
							G_CALLBACK (setup_3oggle_cb), ((int *)&setup_prefs) + offsets[1]);
	gtk_table_attach (GTK_TABLE (tab), wid, 4, 5, row, row + 1, 0, 0, 0, 0);

	wid = gtk_check_button_new ();
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wid),
											setup_get_int3 (&setup_prefs, offsets[2]));
	g_signal_connect (G_OBJECT (wid), "toggled",
							G_CALLBACK (setup_3oggle_cb), ((int *)&setup_prefs) + offsets[2]);
	gtk_table_attach (GTK_TABLE (tab), wid, 5, 6, row, row + 1, 0, 0, 0, 0);
}

static void
setup_toggle_cb (GtkToggleButton *but, const setting *set)
{
	GtkWidget *label, *disable_wid;

	setup_set_int (&setup_prefs, set, but->active ? 1 : 0);

	/* does this toggle also enable/disable another widget? */
	disable_wid = g_object_get_data (G_OBJECT (but), "nxt");
	if (disable_wid)
	{
		gtk_widget_set_sensitive (disable_wid, but->active);
		label = g_object_get_data (G_OBJECT (disable_wid), "lbl");
		gtk_widget_set_sensitive (label, but->active);
	}
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
							GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
}

static GtkWidget *
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
	gtk_table_attach (GTK_TABLE (tab), wid, 2, row==6 ? 6 : 4, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, LABEL_INDENT, 0);

	return wid;
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

static GtkWidget *
setup_create_italic_label (char *text)
{
	GtkWidget *label;
	char buf[256];

	label = gtk_label_new (NULL);
	snprintf (buf, sizeof (buf), "<i><span size=\"smaller\">%s</span></i>", text);
	gtk_label_set_markup (GTK_LABEL (label), buf);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);

	return label;
}

static void
setup_spin_cb (GtkSpinButton *spin, const setting *set)
{
	setup_set_int (&setup_prefs, set, gtk_spin_button_get_value_as_int (spin));
}

static GtkWidget *
setup_create_spin (GtkWidget *table, int row, const setting *set)
{
	GtkWidget *label, *wid, *rbox, *align;
	char *text;

	label = gtk_label_new (_(set->label));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, LABEL_INDENT, 0);

	align = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
	gtk_table_attach (GTK_TABLE (table), align, 3, 4, row, row + 1,
							GTK_EXPAND | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

	rbox = gtk_hbox_new (0, 0);
	gtk_container_add (GTK_CONTAINER (align), rbox);

	wid = gtk_spin_button_new_with_range (0, set->extra, 1);
	g_object_set_data (G_OBJECT (wid), "lbl", label);
	if (set->tooltip)
		add_tip (wid, _(set->tooltip));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (wid),
										setup_get_int (&setup_prefs, set));
	g_signal_connect (G_OBJECT (wid), "value_changed",
							G_CALLBACK (setup_spin_cb), (gpointer)set);
	gtk_box_pack_start (GTK_BOX (rbox), wid, 0, 0, 0);

	if (set->list)
	{
		text = _((char *)set->list);
		if (text[0] == '!')
			label = setup_create_italic_label (text + 1);
		else
			label = gtk_label_new (text);
		gtk_box_pack_start (GTK_BOX (rbox), label, 0, 0, 6);
	}

	return wid;
}

static gint
setup_apply_tint (int *tag)
{
	prefs.hex_text_tint_red = setup_prefs.hex_text_tint_red;
	prefs.hex_text_tint_green = setup_prefs.hex_text_tint_green;
	prefs.hex_text_tint_blue = setup_prefs.hex_text_tint_blue;
	mg_update_xtext (current_sess->gui->xtext);
	*tag = 0;
	return 0;
}

static void
setup_hscale_cb (GtkHScale *wid, const setting *set)
{
	static int tag = 0;

	setup_set_int (&setup_prefs, set, (int) gtk_range_get_value (GTK_RANGE (wid)));

	if (tag == 0)
	{
		tag = g_idle_add ((GSourceFunc) setup_apply_tint, &tag);
	}
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


static GtkWidget *proxy_user; 	/* username GtkEntry */
static GtkWidget *proxy_pass; 	/* password GtkEntry */

static void
setup_menu_cb (GtkWidget *cbox, const setting *set)
{
	int n = gtk_combo_box_get_active (GTK_COMBO_BOX (cbox));

	/* set the prefs.<field> */
	setup_set_int (&setup_prefs, set, n + set->extra);

	if (set->list == proxytypes)
	{
		/* only HTTP and Socks5 can use a username/pass */
		gtk_widget_set_sensitive (proxy_user, (n == 3 || n == 4 || n == 5));
		gtk_widget_set_sensitive (proxy_pass, (n == 3 || n == 4 || n == 5));
	}
}

static void
setup_radio_cb (GtkWidget *item, const setting *set)
{
	if (GTK_TOGGLE_BUTTON (item)->active)
	{
		int n = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item), "n"));
		/* set the prefs.<field> */
		setup_set_int (&setup_prefs, set, n);
	}
}

static int
setup_create_radio (GtkWidget *table, int row, const setting *set)
{
	GtkWidget *wid, *hbox;
	int i;
	const char **text = (const char **)set->list;
	GSList *group;

	wid = gtk_label_new (_(set->label));
	gtk_misc_set_alignment (GTK_MISC (wid), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), wid, 2, 3, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, LABEL_INDENT, 0);

	hbox = gtk_hbox_new (0, 0);
	gtk_table_attach (GTK_TABLE (table), hbox, 3, 4, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);

	i = 0;
	group = NULL;
	while (text[i])
	{
		if (text[i][0] != 0)
		{
			wid = gtk_radio_button_new_with_mnemonic (group, _(text[i]));
			/*if (set->tooltip)
				add_tip (wid, _(set->tooltip));*/
			group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (wid));
			gtk_container_add (GTK_CONTAINER (hbox), wid);
			if (i == setup_get_int (&setup_prefs, set))
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wid), TRUE);
			g_object_set_data (G_OBJECT (wid), "n", GINT_TO_POINTER (i));
			g_signal_connect (G_OBJECT (wid), "toggled",
									G_CALLBACK (setup_radio_cb), (gpointer)set);
		}
		i++;
		row++;
	}

	return i;
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
	GtkWidget *wid, *cbox, *box;
	const char **text = (const char **)set->list;
	int i;

	wid = gtk_label_new (_(set->label));
	gtk_misc_set_alignment (GTK_MISC (wid), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), wid, 2, 3, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, LABEL_INDENT, 0);

	cbox = gtk_combo_box_new_text ();

	for (i = 0; text[i]; i++)
		gtk_combo_box_append_text (GTK_COMBO_BOX (cbox), _(text[i]));

	gtk_combo_box_set_active (GTK_COMBO_BOX (cbox),
									  setup_get_int (&setup_prefs, set) - set->extra);
	g_signal_connect (G_OBJECT (cbox), "changed",
							G_CALLBACK (setup_menu_cb), (gpointer)set);

	box = gtk_hbox_new (0, 0);
	gtk_box_pack_start (GTK_BOX (box), cbox, 0, 0, 0);
	gtk_table_attach (GTK_TABLE (table), box, 3, 4, row, row + 1,
							GTK_EXPAND | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
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
	gtkutil_file_req (_("Select an Image File"), setup_filereq_cb, entry, NULL, NULL, 0);
}

static void
setup_fontsel_destroy (GtkWidget *button, GtkFontSelectionDialog *dialog)
{
	font_dialog = NULL;
}

static void
setup_fontsel_cb (GtkWidget *button, GtkFontSelectionDialog *dialog)
{
	GtkWidget *entry;
	char *font_name;

	entry = g_object_get_data (G_OBJECT (button), "e");
	font_name = gtk_font_selection_dialog_get_font_name (dialog);

	gtk_entry_set_text (GTK_ENTRY (entry), font_name);

	g_free (font_name);
	gtk_widget_destroy (GTK_WIDGET (dialog));
	font_dialog = NULL;
}

static void
setup_fontsel_cancel (GtkWidget *button, GtkFontSelectionDialog *dialog)
{
	gtk_widget_destroy (GTK_WIDGET (dialog));
	font_dialog = NULL;
}

static void
setup_browsefolder_cb (GtkWidget *button, GtkEntry *entry)
{
	gtkutil_file_req (_("Select Download Folder"), setup_filereq_cb, entry, entry->text, NULL, FRF_CHOOSEFOLDER);
}

static void
setup_browsefont_cb (GtkWidget *button, GtkWidget *entry)
{
	GtkFontSelection *sel;
	GtkFontSelectionDialog *dialog;

	dialog = (GtkFontSelectionDialog *) gtk_font_selection_dialog_new (_("Select font"));
	font_dialog = (GtkWidget *)dialog;	/* global var */

	sel = (GtkFontSelection *) dialog->fontsel;

	if (GTK_ENTRY (entry)->text[0])
		gtk_font_selection_set_font_name (sel, GTK_ENTRY (entry)->text);

	g_object_set_data (G_OBJECT (dialog->ok_button), "e", entry);

	g_signal_connect (G_OBJECT (dialog), "destroy",
							G_CALLBACK (setup_fontsel_destroy), dialog);
	g_signal_connect (G_OBJECT (dialog->ok_button), "clicked",
							G_CALLBACK (setup_fontsel_cb), dialog);
	g_signal_connect (G_OBJECT (dialog->cancel_button), "clicked",
							G_CALLBACK (setup_fontsel_cancel), dialog);

	gtk_widget_show (GTK_WIDGET (dialog));
}

static void
setup_entry_cb (GtkEntry *entry, setting *set)
{
	int size;
	int pos;
	int len = strlen (entry->text);
	unsigned char *p = entry->text;

	/* need to truncate? */
	if (len >= set->extra)
	{
		len = pos = 0;
		while (1)
		{
			size = g_utf8_skip [*p];
			len += size;
			p += size;
			/* truncate to "set->extra" BYTES */
			if (len >= set->extra)
			{
				gtk_editable_delete_text (GTK_EDITABLE (entry), pos, -1);
				break;
			}
			pos++;
		}
	}
	else
	{
		setup_set_str (&setup_prefs, set, entry->text);
	}
}

static void
setup_create_label (GtkWidget *table, int row, const setting *set)
{
	gtk_table_attach (GTK_TABLE (table), setup_create_italic_label (_(set->label)),
							set->extra ? 1 : 3, 5, row, row + 1, GTK_FILL,
							GTK_SHRINK | GTK_FILL, 0, 0);
}

static GtkWidget *
setup_create_entry (GtkWidget *table, int row, const setting *set)
{
	GtkWidget *label;
	GtkWidget *wid, *bwid;

	label = gtk_label_new (_(set->label));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, row, row + 1,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, LABEL_INDENT, 0);

	wid = gtk_entry_new ();
	g_object_set_data (G_OBJECT (wid), "lbl", label);
	if (set->list)
		gtk_entry_set_visibility (GTK_ENTRY (wid), FALSE);
	if (set->tooltip)
		add_tip (wid, _(set->tooltip));
	gtk_entry_set_max_length (GTK_ENTRY (wid), set->extra - 1);
	gtk_entry_set_text (GTK_ENTRY (wid), setup_get_str (&setup_prefs, set));
	g_signal_connect (G_OBJECT (wid), "changed",
							G_CALLBACK (setup_entry_cb), (gpointer)set);

	if (set->offset == P_OFFSETNL(hex_net_proxy_user))
		proxy_user = wid;
	if (set->offset == P_OFFSETNL(hex_net_proxy_pass))
		proxy_pass = wid; 

	/* only http and Socks5 can auth */
	if ( (set->offset == P_OFFSETNL(hex_net_proxy_pass) ||
			set->offset == P_OFFSETNL(hex_net_proxy_user)) &&
	     (setup_prefs.hex_net_proxy_type != 4 && setup_prefs.hex_net_proxy_type != 3 && setup_prefs.hex_net_proxy_type != 5) )
		gtk_widget_set_sensitive (wid, FALSE);

	if (set->type == ST_ENTRY)
		gtk_table_attach (GTK_TABLE (table), wid, 3, 6, row, row + 1,
								GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
	else
	{
		gtk_table_attach (GTK_TABLE (table), wid, 3, 5, row, row + 1,
								GTK_EXPAND | GTK_FILL, GTK_SHRINK, 0, 0);
		bwid = gtk_button_new_with_label (_("Browse..."));
		gtk_table_attach (GTK_TABLE (table), bwid, 5, 6, row, row + 1,
								GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);
		if (set->type == ST_EFILE)
			g_signal_connect (G_OBJECT (bwid), "clicked",
									G_CALLBACK (setup_browsefile_cb), wid);
		if (set->type == ST_EFONT)
			g_signal_connect (G_OBJECT (bwid), "clicked",
									G_CALLBACK (setup_browsefont_cb), wid);
		if (set->type == ST_EFOLDER)
			g_signal_connect (G_OBJECT (bwid), "clicked",
									G_CALLBACK (setup_browsefolder_cb), wid);
	}

	return wid;
}

static void
setup_create_header (GtkWidget *table, int row, char *labeltext)
{
	GtkWidget *label;
	char buf[128];

	if (row == 0)
		snprintf (buf, sizeof (buf), "<b>%s</b>", _(labeltext));
	else
		snprintf (buf, sizeof (buf), "\n<b>%s</b>", _(labeltext));

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
	gtk_container_set_border_width (GTK_CONTAINER (tab), 6);
	gtk_table_set_row_spacings (GTK_TABLE (tab), 2);
	gtk_table_set_col_spacings (GTK_TABLE (tab), 3);
	gtk_container_add (GTK_CONTAINER (inbox), tab);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (inbox), hbox);

	*left = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), *left, 0, 0, 0);

	return tab;
}

static void
open_data_cb (GtkWidget *button, gpointer data)
{
	fe_open_url (get_xdir ());
}

static GtkWidget *
setup_create_page (const setting *set)
{
	int i, row, do_disable;
	GtkWidget *tab, *box, *left;
	GtkWidget *wid = NULL, *prev;

	box = gtk_vbox_new (FALSE, 1);
	gtk_container_set_border_width (GTK_CONTAINER (box), 6);

	tab = setup_create_frame (&left, box);

	i = row = do_disable = 0;
	while (set[i].type != ST_END)
	{
		prev = wid;

		switch (set[i].type)
		{
		case ST_HEADER:
			setup_create_header (tab, row, set[i].label);
			break;
		case ST_EFONT:
		case ST_ENTRY:
		case ST_EFILE:
		case ST_EFOLDER:
			wid = setup_create_entry (tab, row, &set[i]);
			break;
		case ST_TOGGLR:
			row--;
			setup_create_toggleR (tab, row, &set[i]);
			break;
		case ST_TOGGLE:
			wid = setup_create_toggleL (tab, row, &set[i]);
			do_disable = set[i].extra;
			break;
		case ST_3OGGLE:
			setup_create_3oggle (tab, row, &set[i]);
			break;
		case ST_MENU:
			setup_create_menu (tab, row, &set[i]);
			break;
		case ST_RADIO:
			row += setup_create_radio (tab, row, &set[i]);
			break;
		case ST_NUMBER:
			wid = setup_create_spin (tab, row, &set[i]);
			break;
		case ST_HSCALE:
			setup_create_hscale (tab, row, &set[i]);
			break;
		case ST_LABEL:
			setup_create_label (tab, row, &set[i]);
			break;
		case ST_ALERTHEAD:
			setup_create_alert_header (tab, row, &set[i]);
		}

		/* will this toggle disable the "next" widget? */
		do_disable--;
		if (do_disable == 0)
		{
			/* setup_toggle_cb uses this data */
			g_object_set_data (G_OBJECT (prev), "nxt", wid);
			/* force initial sensitive state */
			gtk_widget_set_sensitive (wid, GTK_TOGGLE_BUTTON (prev)->active);
			gtk_widget_set_sensitive (g_object_get_data (G_OBJECT (wid), "lbl"),
											  GTK_TOGGLE_BUTTON (prev)->active);
		}

		i++;
		row++;
	}

#if 0
	if (set == general_settings)
	{
		setup_create_id_menu (tab, _("Mark identified users with:"),	
									 row, setup_prefs.hex_irc_id_ytext);
		setup_create_id_menu (tab, _("Mark not-identified users with:"),	
									 row + 1, setup_prefs.hex_irc_id_ntext);
	}
#endif

	if (set == logging_settings)
	{
		GtkWidget *but = gtk_button_new_with_label (_("Open Data Folder"));
		gtk_box_pack_start (GTK_BOX (left), but, 0, 0, 0);
		g_signal_connect (G_OBJECT (but), "clicked",
								G_CALLBACK (open_data_cb), 0);
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
	g_object_set_data (G_OBJECT (but), "hexchat-color", (gpointer)1);
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
	gtk_container_set_border_width (GTK_CONTAINER (box), 6);

	tab = gtk_table_new (9, 2, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (tab), 6);
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

	setup_create_header (tab, 5, N_("Selected Text"));

	setup_create_other_color (_("Foreground:"), COL_MARK_FG, 6, tab);
	setup_create_other_colorR (_("Background:"), COL_MARK_BG, 6, tab);

	setup_create_header (tab, 8, N_("Interface Colors"));

	setup_create_other_color (_("New data:"), COL_NEW_DATA, 9, tab);
	setup_create_other_colorR (_("Marker line:"), COL_MARKER, 9, tab);
	setup_create_other_color (_("New message:"), COL_NEW_MSG, 10, tab);
	setup_create_other_colorR (_("Away user:"), COL_AWAY, 10, tab);
	setup_create_other_color (_("Highlight:"), COL_HILIGHT, 11, tab);
#ifdef HAVE_ISO_CODES /* Defined with static spelling */
	setup_create_other_colorR (_("Spell checker:"), COL_SPELL, 11, tab);
#endif

	setup_create_header (tab, 15, N_("Color Stripping"));

	/* label = gtk_label_new (_("Strip colors from:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (tab), label, 2, 3, 16, 17,
							GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, LABEL_INDENT, 0); */

	for (i = 0; i < 3; i++)
	{
		setup_create_toggleL (tab, i + 16, &color_settings[i]);
	}

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
	strcpy (setup_prefs.hex_sound_dir, GTK_ENTRY (snddir_entry)->text);
	strcpy (setup_prefs.hex_sound_command, GTK_ENTRY (sndprog_entry)->text);
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
		setup_prefs.hex_sound_command[0] = 0;
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
	gtkutil_file_req (_("Select a sound file"), setup_snd_filereq_cb, entry, NULL, NULL, 0);
}

static void
setup_snd_play_cb (GtkWidget *button, GtkEntry *entry)
{
	sound_play (entry->text, FALSE);
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
	if (setup_prefs.hex_sound_command[0] == 0)
		gtk_widget_set_sensitive (sndprog_entry, FALSE);
	else
		gtk_entry_set_text (GTK_ENTRY (sndprog_entry), setup_prefs.hex_sound_command);
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
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_auto), setup_prefs.hex_sound_command[0] == 0);

	label4 = gtk_label_new_with_mnemonic (_("Sound files _directory:"));
	gtk_widget_show (label4);
	gtk_table_attach (GTK_TABLE (table2), label4, 0, 1, 3, 4,
							(GtkAttachOptions) (GTK_FILL),
							(GtkAttachOptions) (0), 0, 0);
	gtk_misc_set_alignment (GTK_MISC (label4), 0, 0.5);

	snddir_entry = entry3 = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (entry3), setup_prefs.hex_sound_dir);
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
		N_("Appearance"),
		N_("Input box"),
		N_("User list"),
		N_("Channel switcher"),
		N_("Colors"),
		NULL,
	N_("Chatting"),
		N_("Alerts"),
		N_("General"),
		N_("Logging"),
		N_("Sound"),
		N_("Advanced"),
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

	setup_add_page (cata[1], book, setup_create_page (appearance_settings));
	setup_add_page (cata[2], book, setup_create_page (inputbox_settings));
	setup_add_page (cata[3], book, setup_create_page (userlist_settings));
	setup_add_page (cata[4], book, setup_create_page (tabs_settings));
	setup_add_page (cata[5], book, setup_create_color_page ());

	if (hextray_mode ())
	{
		setup_add_page (cata[8], book, setup_create_page (alert_settings_hextray));
	}
	else
	{
		setup_add_page (cata[8], book, setup_create_page (alert_settings));
	}

	setup_add_page (cata[9], book, setup_create_page (general_settings));
	setup_add_page (cata[10], book, setup_create_page (logging_settings));
	setup_add_page (cata[11], book, setup_create_sound_page ());

#ifdef WIN32
	if (portable_mode ())
	{
		setup_add_page (cata[12], book, setup_create_page (advanced_settings));
	}
	else
	{
		setup_add_page (cata[12], book, setup_create_page (advanced_settings_oneinstance));
	}
#else
	setup_add_page (cata[12], book, setup_create_page (advanced_settings));
#endif

	setup_add_page (cata[14], book, setup_create_page (network_settings));
	setup_add_page (cata[15], book, setup_create_page (filexfer_settings));

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

	if (prefs.hex_gui_ulist_style)
		gtk_widget_set_style (gui->user_tree, input_style);

	if (prefs.hex_gui_input_style)
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

	if (prefs.hex_gui_ulist_buttons)
		gtk_widget_show (gui->button_box);
	else
		gtk_widget_hide (gui->button_box);

#ifdef USE_GTKSPELL
	spell = gtkspell_get_from_text_view (GTK_TEXT_VIEW (gui->input_box));
	if (prefs.hex_gui_input_spell)
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

#ifdef USE_LIBSEXY
	sexy_spell_entry_set_checked ((SexySpellEntry *)gui->input_box, prefs.hex_gui_input_spell);
#endif
}

static void
unslash (char *dir)
{
	if (dir[0])
	{
		int len = strlen (dir) - 1;
#ifdef WIN32
		if (dir[len] == '/' || dir[len] == '\\')
#else
		if (dir[len] == '/')
#endif
			dir[len] = 0;
	}
}

void
setup_apply_real (int new_pix, int do_ulist, int do_layout)
{
	GSList *list;
	session *sess;
	int done_main = FALSE;

	/* remove trailing slashes */
	unslash (prefs.hex_dcc_dir);
	unslash (prefs.hex_dcc_completed_dir);

	g_mkdir (prefs.hex_dcc_dir, 0700);
	g_mkdir (prefs.hex_dcc_completed_dir, 0700);

	if (new_pix)
	{
		if (channelwin_pix)
			g_object_unref (channelwin_pix);
		channelwin_pix = pixmap_load_from_file (prefs.hex_text_background);
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

		log_open_or_close (sess);

		if (do_ulist)
			userlist_rehash (sess);

		list = list->next;
	}

	mg_apply_setup ();
	tray_apply_setup ();

	if (do_layout)
		menu_change_layout ();
}

static void
setup_apply (struct hexchatprefs *pr)
{
#ifdef WIN32
	PangoFontDescription *old_desc;
	PangoFontDescription *new_desc;
	char buffer[4 * FONTNAMELEN + 1];
	time_t rawtime;
#endif
	int new_pix = FALSE;
	int noapply = FALSE;
	int do_ulist = FALSE;
	int do_layout = FALSE;

	if (strcmp (pr->hex_text_background, prefs.hex_text_background) != 0)
		new_pix = TRUE;

#define DIFF(a) (pr->a != prefs.a)

#ifdef WIN32
	if (DIFF (hex_gui_lang))
		noapply = TRUE;
#endif
	if (DIFF (hex_gui_compact))
		noapply = TRUE;
	if (DIFF (hex_gui_input_icon))
		noapply = TRUE;
	if (DIFF (hex_gui_input_nick))
		noapply = TRUE;
	if (DIFF (hex_gui_lagometer))
		noapply = TRUE;
	if (DIFF (hex_gui_tab_icons))
		noapply = TRUE;
	if (DIFF (hex_gui_tab_server))
		noapply = TRUE;
	if (DIFF (hex_gui_tab_small))
		noapply = TRUE;
	if (DIFF (hex_gui_tab_sort))
		noapply = TRUE;
	if (DIFF (hex_gui_tab_trunc))
		noapply = TRUE;
	if (DIFF (hex_gui_throttlemeter))
		noapply = TRUE;
	if (DIFF (hex_gui_ulist_count))
		noapply = TRUE;
	if (DIFF (hex_gui_ulist_icons))
		noapply = TRUE;
	if (DIFF (hex_gui_ulist_resizable))
		noapply = TRUE;
	if (DIFF (hex_gui_ulist_show_hosts))
		noapply = TRUE;
	if (DIFF (hex_gui_ulist_style))
		noapply = TRUE;

	if (DIFF (hex_gui_tab_dots))
		do_layout = TRUE;
	if (DIFF (hex_gui_tab_layout))
		do_layout = TRUE;

	if (color_change || (DIFF (hex_away_size_max)) || (DIFF (hex_away_track)))
		do_ulist = TRUE;

	if ((pr->hex_gui_tab_pos == 5 || pr->hex_gui_tab_pos == 6) &&
		 pr->hex_gui_tab_layout == 2 && pr->hex_gui_tab_pos != prefs.hex_gui_tab_pos)
		fe_message (_("You cannot place the tree on the top or bottom!\n"
						"Please change to the <b>Tabs</b> layout in the <b>View</b>"
						" menu first."),
						FE_MSG_WARN | FE_MSG_MARKUP);

	memcpy (&prefs, pr, sizeof (prefs));

#ifdef WIN32
	/* merge hex_font_main and hex_font_alternative into hex_font_normal */
	old_desc = pango_font_description_from_string (prefs.hex_text_font_main);
	sprintf (buffer, "%s,%s", pango_font_description_get_family (old_desc), prefs.hex_text_font_alternative);
	new_desc = pango_font_description_from_string (buffer);
	pango_font_description_set_weight (new_desc, pango_font_description_get_weight (old_desc));
	pango_font_description_set_style (new_desc, pango_font_description_get_style (old_desc));
	pango_font_description_set_size (new_desc, pango_font_description_get_size (old_desc));
	sprintf (prefs.hex_text_font, "%s", pango_font_description_to_string (new_desc));

	/* FIXME this is not required after pango_font_description_from_string()
	g_free (old_desc);
	g_free (new_desc);
	*/

	/* workaround for strftime differences between POSIX and MSVC */
	time (&rawtime);

	if (!strftime (buffer, sizeof (buffer), prefs.hex_stamp_text_format, localtime (&rawtime)) || !strftime (buffer, sizeof (buffer), prefs.hex_stamp_log_format, localtime (&rawtime)))
	{
		fe_message (_("Invalid time stamp format! See the strftime MSDN article for details."), FE_MSG_ERROR);
	}
#endif

	setup_apply_real (new_pix, do_ulist, do_layout);

	if (noapply)
		fe_message (_("Some settings were changed that require a"
						" restart to take full effect."), FE_MSG_WARN);

#ifndef WIN32
	if (prefs.hex_dcc_auto_recv)
	{
		if (!strcmp ((char *)g_get_home_dir (), prefs.hex_dcc_dir))
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
	/* setup_prefs -> hexchat */
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

	win = gtkutil_window_new (_(DISPLAY_NAME": Preferences"), "prefs", 0, 0, 3);

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

	if (font_dialog)
	{
		gtk_widget_destroy (font_dialog);
		font_dialog = NULL;
	}
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
