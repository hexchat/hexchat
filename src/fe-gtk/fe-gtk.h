#include "../../config.h"

#ifndef WIN32
#include <sys/types.h>
#include <regex.h>
#endif

#if defined(ENABLE_NLS) && !defined(_)
#  include <libintl.h>
#  define _(x) gettext(x)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#endif
#if !defined(ENABLE_NLS) && defined(_)
#  undef _
#  define N_(String) (String)
#  define _(x) (x)
#endif

#include <gtk/gtkwidget.h>
#include <gtk/gtkcontainer.h>
#include <gtk/gtksignal.h>

#undef gtk_signal_connect
#define gtk_signal_connect g_signal_connect

#define flag_t flag_wid[0]
#define flag_n flag_wid[1]
#define flag_s flag_wid[2]
#define flag_i flag_wid[3]
#define flag_p flag_wid[4]
#define flag_m flag_wid[5]
#define flag_l flag_wid[6]
#define flag_k flag_wid[7]
#define flag_b flag_wid[8]
#define NUM_FLAG_WIDS 9

struct server_gui
{
	GtkWidget *rawlog_window;
	GtkWidget *rawlog_textlist;

	/* chanlist variables */
	GtkWidget *chanlist_wild;
	GtkWidget *chanlist_window;
	GtkWidget *chanlist_list;
	GtkWidget *chanlist_refresh;
	GtkWidget *chanlist_label;

	GSList *chanlist_data_stored_rows;	/* stored list so it can be resorted  */

	gchar chanlist_wild_text[256];	/* text for the match expression */

	gboolean chanlist_match_wants_channel;	/* match in channel name */
	gboolean chanlist_match_wants_topic;	/* match in topic */

#ifndef WIN32
	regex_t chanlist_match_regex;	/* compiled regular expression here */
	unsigned int have_regex:1;
#else
	char *chanlist_match_regex;
#endif

	guint chanlist_users_found_count;	/* users total for all channels */
	guint chanlist_users_shown_count;	/* users total for displayed channels */
	guint chanlist_channels_found_count;	/* channel total for /LIST operation */
	guint chanlist_channels_shown_count;	/* total number of displayed 
														   channels */
	gint chanlist_last_column;	  /* track the last list column user clicked */

	GtkSortType chanlist_sort_type;

	int chanlist_maxusers;
	int chanlist_minusers;
};

/* this struct is persistant even when delinking/relinking */

typedef struct restore_gui
{
	/* banlist stuff */
	GtkWidget *banlist_window;
	GtkWidget *banlist_clistBan;
	GtkWidget *banlist_butRefresh;

	GtkWidget *tab;		/* toggleButton */

	/* information stored when this tab isn't front-most */
	void *user_model;	/* for filling the GtkTreeView */
	void *buffer;		/* xtext_Buffer */
	char *input_text;	/* input text buffer (while not-front tab) */
	char *topic_text;	/* topic GtkEntry buffer */
	char *key_text;
	char *limit_text;
	gfloat old_ul_value;	/* old userlist value (for adj) */
	gfloat lag_value;	/* lag-o-meter */
	char *lag_text;	/* lag-o-meter text */
	gfloat queue_value; /* outbound queue meter */
	char *queue_text;		/* outbound queue text */
	struct User *myself;	/* it's me in the Userlist */
	short flag_wid_state[NUM_FLAG_WIDS];
	unsigned int c_graph:1;	/* connecting graph, is there one? */
} restore_gui;

typedef struct session_gui
{
	GtkWidget
		*xtext,
		*vscrollbar,
		*window,	/* toplevel */
		*topic_entry,
		*note_book,
		*main_vbox,	/* container in TOPLEVEL window */
		*user_tree,	/* GtkTreeView */
		*user_box,
		*button_box_parent,
		*button_box,	/* userlist buttons' box */
		*dialogbutton_box,
		*topicbutton_box,
		*lagometer,
		*laginfo,
		*throttlemeter,
		*throttleinfo,
		*topic_bar,
		*menu,
		*away_item,
		*user_menu,
		*bar,				/* connecting progress bar */
		*nick_box,		/* contains label to the left of input_box */
		*nick_label,
		*op_xpm,			/* icon to the left of nickname */
		*tabs_box,	/* where the tabs are */
		*namelistinfo,	/* label above userlist */
		*input_box,
		*flag_wid[NUM_FLAG_WIDS],		/* channelmode buttons */
		*limit_entry,		  /* +l */
		*key_entry;		  /* +k */

	int bartag;		/*connecting progressbar timeout */

	unsigned int is_tab:1;	/* is tab or toplevel? */

} session_gui;

GdkFont *my_font_load (char *fontname);

extern GdkPixmap *channelwin_pix;
extern GdkPixmap *dialogwin_pix;
