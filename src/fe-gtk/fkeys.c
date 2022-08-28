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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#define GLIB_DISABLE_DEPRECATION_WARNINGS
#include "fe-gtk.h"

#include "../common/hexchat.h"
#include "../common/hexchatc.h"
#include "../common/cfgfiles.h"
#include "../common/fe.h"
#include "../common/userlist.h"
#include "../common/outbound.h"
#include "../common/util.h"
#include "../common/text.h"
#include "../common/plugin.h"
#include "../common/typedef.h"
#include <gdk/gdkkeysyms.h>
#include "gtkutil.h"
#include "menu.h"
#include "xtext.h"
#include "palette.h"
#include "maingui.h"
#include "textgui.h"
#include "fkeys.h"

static void replace_handle (GtkWidget * wid);
void key_action_tab_clean (void);

/***************** Key Binding Code ******************/

/* NOTES:

   To add a new action:
   1) inc KEY_MAX_ACTIONS
   2) write the function at the bottom of this file (with all the others)
   FIXME: Write about calling and returning
   3) Add it to key_actions

   --AGL

 */

/* Remember that the *number* of actions is this *plus* 1 --AGL */
#define KEY_MAX_ACTIONS 14

struct key_binding
{
	guint keyval;					  /* keyval from gdk */
	GdkModifierType mod;			  /* Modifier, always ran through key_modifier_get_valid() */
	int action;						  /* Index into key_actions */
	char *data1, *data2;			  /* Pointers to strings, these must be freed */
};

struct key_action
{
	int (*handler) (GtkWidget * wid, GdkEventKey * evt, char *d1, char *d2,
						 struct session * sess);
	char *name;
	char *help;
};

struct gcomp_data
{
	char data[CHANLEN];
	int elen;
};

static int key_load_kbs (void);
static int key_save_kbs (void);
static int key_action_handle_command (GtkWidget * wid, GdkEventKey * evt,
												  char *d1, char *d2,
												  struct session *sess);
static int key_action_page_switch (GtkWidget * wid, GdkEventKey * evt,
											  char *d1, char *d2, struct session *sess);
int key_action_insert (GtkWidget * wid, GdkEventKey * evt, char *d1, char *d2,
							  struct session *sess);
static int key_action_scroll_page (GtkWidget * wid, GdkEventKey * evt,
											  char *d1, char *d2, struct session *sess);
static int key_action_set_buffer (GtkWidget * wid, GdkEventKey * evt,
											 char *d1, char *d2, struct session *sess);
static int key_action_history_up (GtkWidget * wid, GdkEventKey * evt,
											 char *d1, char *d2, struct session *sess);
static int key_action_history_down (GtkWidget * wid, GdkEventKey * evt,
												char *d1, char *d2, struct session *sess);
static int key_action_tab_comp (GtkWidget * wid, GdkEventKey * evt, char *d1,
										  char *d2, struct session *sess);
static int key_action_comp_chng (GtkWidget * wid, GdkEventKey * evt, char *d1,
                                                                                        char *d2, struct session *sess);
static int key_action_replace (GtkWidget * wid, GdkEventKey * evt, char *d1,
										 char *d2, struct session *sess);
static int key_action_move_tab_left (GtkWidget * wid, GdkEventKey * evt,
												 char *d1, char *d2,
												 struct session *sess);
static int key_action_move_tab_right (GtkWidget * wid, GdkEventKey * evt,
												  char *d1, char *d2,
												  struct session *sess);
static int key_action_move_tab_family_left (GtkWidget * wid, GdkEventKey * evt,
												 char *d1, char *d2,
												 struct session *sess);
static int key_action_move_tab_family_right (GtkWidget * wid, GdkEventKey * evt,
												  char *d1, char *d2,
												  struct session *sess);
static int key_action_put_history (GtkWidget * wid, GdkEventKey * evt,
												  char *d1, char *d2,
												  struct session *sess);

static GSList *keybind_list = NULL;

static const struct key_action key_actions[KEY_MAX_ACTIONS + 1] = {

	{key_action_handle_command, "Run Command",
	 N_("The \002Run Command\002 action runs the data in Data 1 as if it had been typed into the entry box where you pressed the key sequence. Thus it can contain text (which will be sent to the channel/person), commands or user commands. When run all \002\\n\002 characters in Data 1 are used to deliminate separate commands so it is possible to run more than one command. If you want a \002\\\002 in the actual text run then enter \002\\\\\002")},
	{key_action_page_switch, "Change Page",
	 N_("The \002Change Page\002 command switches between pages in the notebook. Set Data 1 to the page you want to switch to. If Data 2 is set to anything then the switch will be relative to the current position. Set Data 1 to auto to switch to the page with the most recent and important activity (queries first, then channels with hilight, channels with dialogue, channels with other data)")},
	{key_action_insert, "Insert in Buffer",
	 N_("The \002Insert in Buffer\002 command will insert the contents of Data 1 into the entry where the key sequence was pressed at the current cursor position")},
	{key_action_scroll_page, "Scroll Page",
	 N_("The \002Scroll Page\002 command scrolls the text widget up or down one page or one line. Set Data 1 to either Top, Bottom, Up, Down, +1 or -1.")},
	{key_action_set_buffer, "Set Buffer",
	 N_("The \002Set Buffer\002 command sets the entry where the key sequence was entered to the contents of Data 1")},
	{key_action_history_up, "Last Command",
	 N_("The \002Last Command\002 command sets the entry to contain the last command entered - the same as pressing up in a shell")},
	{key_action_history_down, "Next Command",
	 N_("The \002Next Command\002 command sets the entry to contain the next command entered - the same as pressing down in a shell")},
	{key_action_tab_comp, "Complete nick/command",
	 N_("This command changes the text in the entry to finish an incomplete nickname or command. If Data 1 is set then double-tabbing in a string will select the last nick, not the next")},
	{key_action_comp_chng, "Change Selected Nick",
	 N_("This command scrolls up and down through the list of nicks. If Data 1 is set to anything it will scroll up, else it scrolls down")},
	{key_action_replace, "Check For Replace",
	 N_("This command checks the last word entered in the entry against the replace list and replaces it if it finds a match")},
	{key_action_move_tab_left, "Move front tab left",
	 N_("This command moves the front tab left by one")},
	{key_action_move_tab_right, "Move front tab right",
	 N_("This command moves the front tab right by one")},
	{key_action_move_tab_family_left, "Move tab family left",
	 N_("This command moves the current tab family to the left")},
	{key_action_move_tab_family_right, "Move tab family right",
	 N_("This command moves the current tab family to the right")},
	{key_action_put_history, "Push input line into history",
	 N_("Push input line into history but doesn't send to server")},
};

#define default_kb_cfg \
	"ACCEL=<Primary>Page_Up\nChange Page\nD1:-1\nD2:Relative\n\n"\
	"ACCEL=<Primary>Page_Down\nChange Page\nD1:1\nD2:Relative\n\n"\
	"ACCEL=<Alt>9\nChange Page\nD1:9\nD2!\n\n"\
	"ACCEL=<Alt>8\nChange Page\nD1:8\nD2!\n\n"\
	"ACCEL=<Alt>7\nChange Page\nD1:7\nD2!\n\n"\
	"ACCEL=<Alt>6\nChange Page\nD1:6\nD2!\n\n"\
	"ACCEL=<Alt>5\nChange Page\nD1:5\nD2!\n\n"\
	"ACCEL=<Alt>4\nChange Page\nD1:4\nD2!\n\n"\
	"ACCEL=<Alt>3\nChange Page\nD1:3\nD2!\n\n"\
	"ACCEL=<Alt>2\nChange Page\nD1:2\nD2!\n\n"\
	"ACCEL=<Alt>1\nChange Page\nD1:1\nD2!\n\n"\
	"ACCEL=<Alt>grave\nChange Page\nD1:auto\nD2!\n\n"\
	"ACCEL=<Primary>o\nInsert in Buffer\nD1:\017\nD2!\n\n"\
	"ACCEL=<Primary>b\nInsert in Buffer\nD1:\002\nD2!\n\n"\
	"ACCEL=<Primary>k\nInsert in Buffer\nD1:\003\nD2!\n\n"\
	"ACCEL=<Primary>i\nInsert in Buffer\nD1:\035\nD2!\n\n"\
	"ACCEL=<Primary>u\nInsert in Buffer\nD1:\037\nD2!\n\n"\
	"ACCEL=<Primary>r\nInsert in Buffer\nD1:\026\nD2!\n\n"\
	"ACCEL=<Shift>Page_Down\nChange Selected Nick\nD1!\nD2!\n\n"\
	"ACCEL=<Shift>Page_Up\nChange Selected Nick\nD1:Up\nD2!\n\n"\
	"ACCEL=Page_Down\nScroll Page\nD1:Down\nD2!\n\n"\
	"ACCEL=<Primary>Home\nScroll Page\nD1:Top\nD2!\n\n"\
	"ACCEL=<Primary>End\nScroll Page\nD1:Bottom\nD2!\n\n"\
	"ACCEL=Page_Up\nScroll Page\nD1:Up\nD2!\n\n"\
	"ACCEL=<Shift>Down\nScroll Page\nD1:+1\nD2!\n\n"\
	"ACCEL=<Shift>Up\nScroll Page\nD1:-1\nD2!\n\n"\
	"ACCEL=Down\nNext Command\nD1!\nD2!\n\n"\
	"ACCEL=Up\nLast Command\nD1!\nD2!\n\n"\
	"ACCEL=Tab\nComplete nick/command\nD1!\nD2!\n\n"\
	"ACCEL=<Shift>ISO_Left_Tab\nComplete nick/command\nD1:Previous\nD2!\n\n"\
	"ACCEL=space\nCheck For Replace\nD1!\nD2!\n\n"\
	"ACCEL=Return\nCheck For Replace\nD1!\nD2!\n\n"\
	"ACCEL=KP_Enter\nCheck For Replace\nD1!\nD2!\n\n"\
	"ACCEL=<Primary>Tab\nComplete nick/command\nD1:Up\nD2!\n\n"\
	"ACCEL=<Alt>Left\nMove front tab left\nD1!\nD2!\n\n"\
	"ACCEL=<Alt>Right\nMove front tab right\nD1!\nD2!\n\n"\
	"ACCEL=<Primary><Shift>Page_Up\nMove tab family left\nD1!\nD2!\n\n"\
	"ACCEL=<Primary><Shift>Page_Down\nMove tab family right\nD1!\nD2!\n\n"\
	"ACCEL=F9\nRun Command\nD1:/GUI MENU TOGGLE\nD2!\n\n"

void
key_init ()
{
	if (key_load_kbs () == 1)
	{
		fe_message (_("There was an error loading key"
							" bindings configuration"), FE_MSG_ERROR);
	}
}

static inline int
key_get_action_from_string (char *text)
{
	int i;

	for (i = 0; i < KEY_MAX_ACTIONS + 1; i++)
	{
		if (strcmp (key_actions[i].name, text) == 0)
		{
			return i;
		}
	}

	return 0;
}

static void
key_free (gpointer data)
{
	struct key_binding *kb = (struct key_binding*)data;

	g_return_if_fail (kb != NULL);

	g_free (kb->data1);
	g_free (kb->data2);
	g_free (kb);
}

/* Ok, here are the NOTES

   key_handle_key_press now handles all the key presses and history_keypress is
   now defunct. It goes thru the linked list keys_root and finds a matching
   key. It runs the action func and switches on these values:
   0) Return
   1) Find next
   2) stop signal and return

   * history_keypress is now dead (and gone)
   * key_handle_key_press now takes its role
   * All the possible actions are in a struct called key_actions (in fkeys.c)
   * it is made of {function, name, desc}
   * key bindings can pass 2 *text* strings to the handler. If more options are nee
   ded a format can be put on one of these options
   * key actions are passed {
   the entry widget
   the Gdk event
   data 1
   data 2
   session struct
   }
   * key bindings are stored in a linked list of key_binding structs
   * which looks like {
   guint keyval;  GDK keynumber
   int action;  Index into key_actions 
   GdkModiferType mod; modifier, only ones from key_modifer_get_valid()
   char *data1, *data2;  Pointers to strings, these must be freed 
   struct key_binding *next;
   }
   * remember that is (data1 || data2) != NULL then they need to be free()'ed

   --AGL

 */

static inline GdkModifierType
key_modifier_get_valid (GdkModifierType mod)
{
	GdkModifierType ret;

#ifdef __APPLE__
	ret = mod & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK | GDK_META_MASK);
#else
	/* These masks work on both Windows and Unix */
	ret = mod & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK);
#endif

	return ret;
}

gboolean
key_handle_key_press (GtkWidget *wid, GdkEventKey *evt, session *sess)
{
	struct key_binding *kb;
	int n;
	GSList *list;

	/* where did this event come from? */
	list = sess_list;
	while (list)
	{
		sess = list->data;
		if (sess->gui->input_box == wid)
		{
			if (sess->gui->is_tab)
				sess = current_tab;
			break;
		}
		list = list->next;
	}
	if (!list)
		return FALSE;
	current_sess = sess;

	if (plugin_emit_keypress (sess, evt->state, evt->keyval, gdk_keyval_to_unicode (evt->keyval)))
		return 1;

	/* maybe the plugin closed this tab? */
	if (!is_session (sess))
		return 1;

	list = keybind_list;
	while (list)
	{
		kb = (struct key_binding*)list->data;

		if (kb->keyval == evt->keyval && kb->mod == key_modifier_get_valid (evt->state))
		{
			if (kb->action < 0 || kb->action > KEY_MAX_ACTIONS)
				return 0;

			/* Run the function */
			n = key_actions[kb->action].handler (wid, evt, kb->data1,
															 kb->data2, sess);
			switch (n)
			{
			case 0:
				return 1;
			case 2:
				g_signal_stop_emission_by_name (G_OBJECT (wid),
														"key_press_event");
				return 1;
			}
		}
		list = g_slist_next (list);
	}

	switch (evt->keyval)
	{
	case GDK_KEY_space:
		key_action_tab_clean ();
		break;
	}

	return 0;
}


/* ***** GUI code here ******************* */

enum
{
	KEY_COLUMN,
	ACCEL_COLUMN,
	ACTION_COLUMN,
	D1_COLUMN,
	D2_COLUMN,
	N_COLUMNS
};

static GtkWidget *key_dialog = NULL;

static inline GtkTreeModel *
get_store (void)
{
	return gtk_tree_view_get_model (g_object_get_data (G_OBJECT (key_dialog), "view"));
}

static void
key_dialog_print_text (GtkXText *xtext, char *text)
{
	unsigned int old = prefs.hex_stamp_text;
	prefs.hex_stamp_text = 0;	/* temporarily disable stamps */
	gtk_xtext_clear (GTK_XTEXT (xtext)->buffer, 0);
	PrintTextRaw (GTK_XTEXT (xtext)->buffer, text, 0, 0);
	prefs.hex_stamp_text = old;
}

static void
key_dialog_set_key (GtkCellRendererAccel *accel, gchar *pathstr, guint accel_key, 
					GdkModifierType accel_mods, guint hardware_keycode, gpointer userdata)
{
	GtkTreeModel *model = get_store ();
	GtkTreePath *path = gtk_tree_path_new_from_string (pathstr);
	GtkTreeIter iter;
	gchar *label_name, *accel_name;

	/* Shift tab requires an exception, hopefully that list ends here.. */
	if (accel_key == GDK_KEY_Tab && accel_mods & GDK_SHIFT_MASK)
		accel_key = GDK_KEY_ISO_Left_Tab;

	label_name = gtk_accelerator_get_label (accel_key, key_modifier_get_valid (accel_mods));
	accel_name = gtk_accelerator_name (accel_key, key_modifier_get_valid (accel_mods));

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, KEY_COLUMN, label_name,
						ACCEL_COLUMN, accel_name, -1);

	gtk_tree_path_free (path);
	g_free (label_name);
	g_free (accel_name);
}

static void
key_dialog_combo_changed (GtkCellRendererCombo *combo, gchar *pathstr,
						GtkTreeIter *new_iter, gpointer data)
{
	GtkTreeModel *model;
	GtkXText *xtext;
	gchar *actiontext = NULL;
	gint action;

	xtext = GTK_XTEXT (g_object_get_data (G_OBJECT (key_dialog), "xtext"));
	model = GTK_TREE_MODEL (data);

	gtk_tree_model_get (model, new_iter, 0, &actiontext, -1);

	if (actiontext)
	{
#ifdef WIN32
		/* We need to manually update the store */
		GtkTreePath *path;
		GtkTreeIter iter;

		path = gtk_tree_path_new_from_string (pathstr);
		model = get_store ();

		gtk_tree_model_get_iter (model, &iter, path);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, ACTION_COLUMN, actiontext, -1);

		gtk_tree_path_free (path);
#endif

		action = key_get_action_from_string (actiontext);
		key_dialog_print_text (xtext, key_actions[action].help);

		g_free (actiontext);
	}
}

static void
key_dialog_entry_edited (GtkCellRendererText *render, gchar *pathstr, gchar *new_text, gpointer data)
{
	GtkTreeModel *model = get_store ();
	GtkTreePath *path = gtk_tree_path_new_from_string (pathstr);
	GtkTreeIter iter;
	gint column = GPOINTER_TO_INT (data);

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, column, new_text, -1);

	gtk_tree_path_free (path);
}

static gboolean
key_dialog_keypress (GtkWidget *wid, GdkEventKey *evt, gpointer userdata)
{
	GtkTreeView *view = g_object_get_data (G_OBJECT (key_dialog), "view");
	GtkTreeModel *store;
	GtkTreeIter iter1, iter2;
	GtkTreeSelection *sel;
	GtkTreePath *path;
	gboolean handled = FALSE;
	int delta;

	if (evt->state & GDK_SHIFT_MASK)
	{
		if (evt->keyval == GDK_KEY_Up)
		{
			handled = TRUE;
			delta = -1;
		}
		else if (evt->keyval == GDK_KEY_Down)
		{
			handled = TRUE;
			delta = 1;
		}
	}

	if (handled)
	{
		sel = gtk_tree_view_get_selection (view);
		gtk_tree_selection_get_selected (sel, &store, &iter1);
		path = gtk_tree_model_get_path (store, &iter1);
		if (delta == 1)
			gtk_tree_path_next (path);
		else
			gtk_tree_path_prev (path);
		gtk_tree_model_get_iter (store, &iter2, path);
		gtk_tree_path_free (path);
		gtk_list_store_swap (GTK_LIST_STORE (store), &iter1, &iter2);
	}

	return handled;
}

static void
key_dialog_selection_changed (GtkTreeSelection *sel, gpointer userdata)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkXText *xtext;
	char *actiontext;
	int action;

	if (!gtk_tree_selection_get_selected (sel, &model, &iter) || model == NULL)
		return;

	xtext = GTK_XTEXT (g_object_get_data (G_OBJECT (key_dialog), "xtext"));
	gtk_tree_model_get (model, &iter, ACTION_COLUMN, &actiontext, -1);

	if (actiontext)
	{
		action = key_get_action_from_string (actiontext);
		key_dialog_print_text (xtext, key_actions[action].help);
		g_free (actiontext);
	}
	else
		key_dialog_print_text (xtext, _("Select a row to get help information on its Action."));
}

static void
key_dialog_close (GtkWidget *wid, gpointer userdata)
{
	gtk_widget_destroy (key_dialog);
	key_dialog = NULL;
}

static void
key_dialog_save (GtkWidget *wid, gpointer userdata)
{
	GtkTreeModel *store = get_store ();
	GtkTreeIter iter;
	struct key_binding *kb;
	char *data1, *data2, *accel, *actiontext;
	guint keyval;
	GdkModifierType mod;

	if (keybind_list)
	{
		g_slist_free_full (keybind_list, key_free);
		keybind_list = NULL;
	}

	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter))
	{
		do
		{
			kb = g_new0 (struct key_binding, 1);

			gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, ACCEL_COLUMN, &accel,
															ACTION_COLUMN, &actiontext,
															D1_COLUMN, &data1,
															D2_COLUMN, &data2,
															-1);
			kb->data1 = data1;
			kb->data2 = data2;

			if (accel)
			{
				gtk_accelerator_parse (accel, &keyval, &mod);

				kb->keyval = keyval;
				kb->mod = key_modifier_get_valid (mod);

				g_free (accel);
			}

			if (actiontext)
			{
				kb->action = key_get_action_from_string (actiontext);
				g_free (actiontext);
			}

			if (!accel || !actiontext)
				key_free (kb);
			else
				keybind_list = g_slist_append (keybind_list, kb);

		}
		while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter));
	}

	if (key_save_kbs () == 0)
		key_dialog_close (wid, NULL);
}

static void
key_dialog_add (GtkWidget *wid, gpointer userdata)
{
	GtkTreeView *view = g_object_get_data (G_OBJECT (key_dialog), "view");
	GtkTreeViewColumn *col;
	GtkListStore *store = GTK_LIST_STORE (get_store ());
	GtkTreeIter iter;
	GtkTreePath *path;

	gtk_list_store_append (store, &iter);

	/* make sure the new row is visible and selected */
	path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);
	col = gtk_tree_view_get_column (view, ACTION_COLUMN);
	gtk_tree_view_scroll_to_cell (view, path, NULL, FALSE, 0.0, 0.0);
	gtk_tree_view_set_cursor (view, path, col, TRUE);
	gtk_tree_path_free (path);
}

static void
key_dialog_delete (GtkWidget *wid, gpointer userdata)
{
	GtkTreeView *view = g_object_get_data (G_OBJECT (key_dialog), "view");
	GtkListStore *store = GTK_LIST_STORE (gtk_tree_view_get_model (view));
	GtkTreeIter iter;
	GtkTreePath *path;

	if (gtkutil_treeview_get_selected (view, &iter, -1))
	{
		/* delete this row, select next one */
		if (gtk_list_store_remove (store, &iter))
		{
			path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);
			gtk_tree_view_scroll_to_cell (view, path, NULL, TRUE, 1.0, 0.0);
			gtk_tree_view_set_cursor (view, path, NULL, FALSE);
			gtk_tree_path_free (path);
		}
	}
}

static GtkWidget *
key_dialog_treeview_new (GtkWidget *box)
{
	GtkWidget *scroll;
	GtkListStore *store, *combostore;
	GtkTreeViewColumn *col;
	GtkWidget *view;
	GtkCellRenderer *render;
	int i;

	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_IN);

	store = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
								G_TYPE_STRING, G_TYPE_STRING);
	g_return_val_if_fail (store != NULL, NULL);

	view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
	gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (view), TRUE);
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW (view), FALSE);
	gtk_tree_view_set_reorderable (GTK_TREE_VIEW (view), TRUE);

	g_signal_connect (G_OBJECT (view), "key-press-event",
					G_CALLBACK (key_dialog_keypress), NULL);
	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW(view))),
					"changed", G_CALLBACK (key_dialog_selection_changed), NULL);

	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (view), TRUE);

	render = gtk_cell_renderer_accel_new ();
	g_object_set (render, "editable", TRUE,
#ifndef WIN32
					"accel-mode", GTK_CELL_RENDERER_ACCEL_MODE_OTHER,
#endif
					NULL);
	g_signal_connect (G_OBJECT (render), "accel-edited",
					G_CALLBACK (key_dialog_set_key), NULL);
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view), KEY_COLUMN,
												"Key", render,
												"text", KEY_COLUMN,
												NULL);

	render = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (
							GTK_TREE_VIEW (view), ACCEL_COLUMN,
							"Accel", render,
							"text", ACCEL_COLUMN,
							NULL);

	combostore = gtk_list_store_new (1, G_TYPE_STRING);
	for (i = 0; i <= KEY_MAX_ACTIONS; i++)
	{
		GtkTreeIter iter;

		if (key_actions[i].name[0])
		{
			gtk_list_store_append (combostore, &iter);
			gtk_list_store_set (combostore, &iter, 0, key_actions[i].name, -1);
		}
	}

	render = gtk_cell_renderer_combo_new ();
	g_object_set (G_OBJECT (render), "model", combostore,
									"has-entry", FALSE,
									"editable", TRUE, 
									"text-column", 0,
									NULL);
	g_signal_connect (G_OBJECT (render), "edited",
					G_CALLBACK (key_dialog_entry_edited), GINT_TO_POINTER (ACTION_COLUMN));
	g_signal_connect (G_OBJECT (render), "changed",
					G_CALLBACK (key_dialog_combo_changed), combostore);
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view), ACTION_COLUMN,
													"Action", render,
													"text", ACTION_COLUMN, 
													NULL);

	render = gtk_cell_renderer_text_new ();
	g_object_set (render, "editable", TRUE, NULL);
	g_signal_connect (G_OBJECT (render), "edited",
				G_CALLBACK (key_dialog_entry_edited), GINT_TO_POINTER (D1_COLUMN));
	gtk_tree_view_insert_column_with_attributes (
							GTK_TREE_VIEW (view), D1_COLUMN,
							"Data1", render,
							"text", D1_COLUMN,
							NULL);

	render = gtk_cell_renderer_text_new ();
	g_object_set (render, "editable", TRUE, NULL);
	g_signal_connect (G_OBJECT (render), "edited",
				G_CALLBACK (key_dialog_entry_edited), GINT_TO_POINTER (D2_COLUMN));
	gtk_tree_view_insert_column_with_attributes (
							GTK_TREE_VIEW (view), D2_COLUMN,
							"Data2", render,
							"text", D2_COLUMN,
							NULL);

	col = gtk_tree_view_get_column (GTK_TREE_VIEW (view), KEY_COLUMN);
	gtk_tree_view_column_set_fixed_width (col, 200);
	gtk_tree_view_column_set_resizable (col, TRUE);
	col = gtk_tree_view_get_column (GTK_TREE_VIEW (view), ACCEL_COLUMN);
	gtk_tree_view_column_set_visible (col, FALSE);
	col = gtk_tree_view_get_column (GTK_TREE_VIEW (view), ACTION_COLUMN);
	gtk_tree_view_column_set_fixed_width (col, 160);
	col = gtk_tree_view_get_column (GTK_TREE_VIEW (view), D1_COLUMN);
	gtk_tree_view_column_set_sizing (col, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_min_width (col, 80);
	gtk_tree_view_column_set_resizable (col, TRUE);
	col = gtk_tree_view_get_column (GTK_TREE_VIEW (view), D2_COLUMN);
	gtk_tree_view_column_set_sizing (col, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_min_width (col, 80);
	gtk_tree_view_column_set_resizable (col, TRUE);

	gtk_container_add (GTK_CONTAINER (scroll), view);
	gtk_container_add (GTK_CONTAINER (box), scroll);

	return view;
}

static void
key_dialog_load (GtkListStore *store)
{
	struct key_binding *kb = NULL;
	char *label_text, *accel_text;
	GtkTreeIter iter;
	GSList *list = keybind_list;

	while (list)
	{
		kb = (struct key_binding*)list->data;

		label_text = gtk_accelerator_get_label (kb->keyval, kb->mod);
		accel_text = gtk_accelerator_name (kb->keyval, kb->mod);

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
							KEY_COLUMN, label_text,
							ACCEL_COLUMN, accel_text,
							ACTION_COLUMN, key_actions[kb->action].name,
							D1_COLUMN, kb->data1,
							D2_COLUMN, kb->data2, -1);

		g_free (accel_text);
		g_free (label_text);

		list = g_slist_next (list);
	}
}

void
key_dialog_show ()
{
	GtkWidget *vbox, *box;
	GtkWidget *view, *xtext;
	GtkListStore *store;
	char buf[128];

	if (key_dialog)
	{
		mg_bring_tofront (key_dialog);
		return;
	}

	g_snprintf(buf, sizeof(buf), _("Keyboard Shortcuts - %s"), _(DISPLAY_NAME));
	key_dialog = mg_create_generic_tab ("editkeys", buf, TRUE, FALSE, key_dialog_close,
									NULL, 600, 360, &vbox, 0);

	view = key_dialog_treeview_new (vbox);
	xtext = gtk_xtext_new (colors, 0);
	gtk_box_pack_start (GTK_BOX (vbox), xtext, FALSE, TRUE, 2);
	gtk_xtext_set_font (GTK_XTEXT (xtext), prefs.hex_text_font);

	g_object_set_data (G_OBJECT (key_dialog), "view", view);
	g_object_set_data (G_OBJECT (key_dialog), "xtext", xtext);

	box = gtk_hbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (box), GTK_BUTTONBOX_SPREAD);
	gtk_box_pack_start (GTK_BOX (vbox), box, FALSE, FALSE, 2);
	gtk_container_set_border_width (GTK_CONTAINER (box), 5);

	gtkutil_button (box, GTK_STOCK_NEW, NULL, key_dialog_add,
					NULL, _("Add"));
	gtkutil_button (box, GTK_STOCK_DELETE, NULL, key_dialog_delete,
					NULL, _("Delete"));
	gtkutil_button (box, GTK_STOCK_CANCEL, NULL, key_dialog_close,
					NULL, _("Cancel"));
	gtkutil_button (box, GTK_STOCK_SAVE, NULL, key_dialog_save,
					NULL, _("Save"));

	store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (view)));
	key_dialog_load (store);

	gtk_widget_show_all (key_dialog);
}

static int
key_save_kbs (void)
{
	int fd;
	char buf[512];
	char *accel_text;
	GSList *list = keybind_list;
	struct key_binding *kb;

	fd = hexchat_open_file ("keybindings.conf", O_CREAT | O_TRUNC | O_WRONLY,
									 0x180, XOF_DOMODE);
	if (fd < 0)
		return 1;
	write (fd, buf, g_snprintf (buf, 510, "# HexChat key bindings config file\n\n"));

	while (list)
	{
		kb = list->data;

		accel_text = gtk_accelerator_name (kb->keyval, kb->mod);

		g_snprintf (buf, 510, "ACCEL=%s\n%s\n", accel_text, key_actions[kb->action].name);
		write (fd, buf, strlen (buf));
		g_free (accel_text);

		if (kb->data1 && kb->data1[0])
			write (fd, buf, g_snprintf (buf, 510, "D1:%s\n", kb->data1));
		else
			write (fd, "D1!\n", 4);

		if (kb->data2 && kb->data2[0])
			write (fd, buf, g_snprintf (buf, 510, "D2:%s\n", kb->data2));
		else
			write (fd, "D2!\n", 4);

		write (fd, "\n", 1);

		list = g_slist_next (list);
	}

	close (fd);
	return 0;
}

#define KBSTATE_MOD 0
#define KBSTATE_KEY 1
#define KBSTATE_ACT 2
#define KBSTATE_DT1 3
#define KBSTATE_DT2 4

#define STRIP_WHITESPACE \
	while (buf[0] == ' ' || buf[0] == '\t') \
		buf++; \
	len = strlen (buf); \
	while (buf[len] == ' ' || buf[len] == '\t') \
	{ \
		buf[len] = 0; \
		len--; \
	} \

static inline int
key_load_kbs_helper_mod (char *buf, GdkModifierType *out)
{
	int n, len, mod = 0;

	/* First strip off the fluff */
	STRIP_WHITESPACE

	if (strcmp (buf, "None") == 0)
	{
		*out = 0;
		return 0;
	}
	for (n = 0; n < len; n++)
	{
		switch (buf[n])
		{
		case 'C':
			mod |= GDK_CONTROL_MASK;
			break;
		case 'A':
			mod |= GDK_MOD1_MASK;
			break;
		case 'S':
			mod |= GDK_SHIFT_MASK;
			break;
		default:
			return 1;
		}
	}

	*out = mod;
	return 0;
}

static int
key_load_kbs (void)
{
	char *buf, *ibuf;
	struct stat st;
	struct key_binding *kb = NULL;
	int fd, len, state = 0, pnt = 0;
	guint keyval;
	GdkModifierType mod = 0;
	off_t size;

	fd = hexchat_open_file ("keybindings.conf", O_RDONLY, 0, 0);
	if (fd < 0)
	{
		ibuf = g_strdup (default_kb_cfg);
		size = strlen (default_kb_cfg);
	}
	else
	{
		if (fstat (fd, &st) != 0)
		{
			close (fd);
			return 1;
		}

		ibuf = g_malloc(st.st_size);
		read (fd, ibuf, st.st_size);
		size = st.st_size;
		close (fd);
	}

	if (keybind_list)
	{
		g_slist_free_full (keybind_list, key_free);
		keybind_list = NULL;
	}

	while (buf_get_line (ibuf, &buf, &pnt, size))
	{		
		if (buf[0] == '#')
			continue;
		if (strlen (buf) == 0)
			continue;

		switch (state)
		{
		case KBSTATE_MOD:
			kb = g_new0 (struct key_binding, 1);

			/* New format */
			if (strncmp (buf, "ACCEL=", 6) == 0)
			{
				buf += 6;

				gtk_accelerator_parse (buf, &keyval, &mod);


				kb->keyval = keyval;
				kb->mod = key_modifier_get_valid (mod);

				state = KBSTATE_ACT; 
				continue;
			}

			if (key_load_kbs_helper_mod (buf, &mod))
				goto corrupt_file;

			kb->mod = mod;

			state = KBSTATE_KEY;
			continue;

		case KBSTATE_KEY:
			STRIP_WHITESPACE

			keyval = gdk_keyval_from_name (buf);
			if (keyval == 0)
			{
				g_free (ibuf);
				return 2;
			}

			kb->keyval = keyval;

			state = KBSTATE_ACT;
			continue;

		case KBSTATE_ACT:
			STRIP_WHITESPACE

			kb->action = key_get_action_from_string (buf);

			if (kb->action == KEY_MAX_ACTIONS + 1)
			{
				g_free (ibuf);
				return 3;
			}

			state = KBSTATE_DT1;
			continue;

		case KBSTATE_DT1:
		case KBSTATE_DT2:
			if (state == KBSTATE_DT1)
				kb->data1 = kb->data2 = NULL;

			while (buf[0] == ' ' || buf[0] == '\t')
				buf++;

			if (buf[0] != 'D')
			{
				g_free (ibuf);
				return 4;
			}

			switch (buf[1])
			{
			case '1':
				if (state != KBSTATE_DT1)
					goto corrupt_file;
				break;
			case '2':
				if (state != KBSTATE_DT2)
					goto corrupt_file;
				break;
			default:
				goto corrupt_file;
			}

			if (buf[2] == ':')
			{
				len = strlen (buf);
				/* Add one for the NULL, subtract 3 for the "Dx:" */
				len++;
				len -= 3;
				if (state == KBSTATE_DT1)
				{
					kb->data1 = g_strndup (&buf[3], len);
				} else
				{
					kb->data2 = g_strndup (&buf[3], len);
				}
			} else if (buf[2] == '!')
			{
				if (state == KBSTATE_DT1)
					kb->data1 = NULL;
				else
					kb->data2 = NULL;
			}
			if (state == KBSTATE_DT1)
			{
				state = KBSTATE_DT2;
				continue;
			} else
			{
				keybind_list = g_slist_append (keybind_list, kb);

				state = KBSTATE_MOD;
			}

			continue;
		}
	}
	g_free (ibuf);
	return 0;

corrupt_file:
	g_free (ibuf);
	g_free (kb);
	return 5;
}

/* ***** Key actions start here *********** */

/* See the NOTES above --AGL */

/* "Run command" */
static int
key_action_handle_command (GtkWidget * wid, GdkEventKey * evt, char *d1,
									char *d2, struct session *sess)
{
	int ii, oi, len;
	char out[2048], d = 0;

	if (!d1)
		return 0;

	len = strlen (d1);

	/* Replace each "\n" substring with '\n' */
	for (ii = oi = 0; ii < len; ii++)
	{
		d = d1[ii];
		if (d == '\\')
		{
			ii++;
			d = d1[ii];
			if (d == 'n')
				out[oi++] = '\n';
			else if (d == '\\')
				out[oi++] = '\\';
			else
			{
				out[oi++] = '\\';
				out[oi++] = d;
			}
			continue;
		}
		out[oi++] = d;
	}
	out[oi] = 0;

	handle_multiline (sess, out, 0, 0);
	return 0;
}

/*
 * Check if the given session is inside the main window. This predicate
 * is passed to lastact_getfirst() as a way to filter out detached sessions.
 * XXX: Consider moving this in a different file?
 */
static int
session_check_is_tab(session *sess)
{
	if (!sess || !sess->gui)
		return FALSE;

	return (sess->gui->is_tab);
}

static int
key_action_page_switch (GtkWidget * wid, GdkEventKey * evt, char *d1,
								char *d2, struct session *sess)
{
	session *newsess;
	int len, i, num;

	if (!d1)
		return 1;

	len = strlen (d1);
	if (!len)
		return 1;

	if (strcasecmp(d1, "auto") == 0)
	{
		/* Auto switch makes no sense in detached sessions */
		if (!sess->gui->is_tab)
			return 1;

		/* Obtain a session with recent activity */
		newsess = lastact_getfirst(session_check_is_tab);

		if (newsess)
		{
			/*
			 * Only sessions in the current window should be considered (i.e.
			 * we don't want to move the focus on a different window). This
			 * call could, in theory, do this, but we checked before that
			 * newsess->gui->is_tab and sess->gui->is_tab.
			 */
			mg_bring_tofront_sess(newsess);
			return 0;
		}
		else
			return 1;
	}

	for (i = 0; i < len; i++)
	{
		if (d1[i] < '0' || d1[i] > '9')
		{
			if (i == 0 && (d1[i] == '+' || d1[i] == '-'))
				continue;
			else
				return 1;
		}
	}

	num = atoi (d1);
	if (!d2)
		num--;
	if (!d2 || d2[0] == 0)
		mg_switch_page (FALSE, num);
	else
		mg_switch_page (TRUE, num);
	return 0;
}

int
key_action_insert (GtkWidget * wid, GdkEventKey * evt, char *d1, char *d2,
						 struct session *sess)
{
	int tmp_pos;

	if (!d1)
		return 1;

	tmp_pos = SPELL_ENTRY_GET_POS (wid);
	SPELL_ENTRY_INSERT (wid, d1, strlen (d1), &tmp_pos);
	SPELL_ENTRY_SET_POS (wid, tmp_pos);
	return 2;
}

/* handles PageUp/Down keys */
static int
key_action_scroll_page (GtkWidget * wid, GdkEventKey * evt, char *d1,
								char *d2, struct session *sess)
{
	int value, end;
	GtkAdjustment *adj;
	enum scroll_type { PAGE_TOP, PAGE_BOTTOM, PAGE_UP, PAGE_DOWN, LINE_UP, LINE_DOWN };
	int type = PAGE_DOWN;

	if (d1)
	{
		if (!g_ascii_strcasecmp (d1, "top"))
			type = PAGE_TOP;
		else if (!g_ascii_strcasecmp (d1, "bottom"))
			type = PAGE_BOTTOM;
		else if (!g_ascii_strcasecmp (d1, "up"))
			type = PAGE_UP;
		else if (!g_ascii_strcasecmp (d1, "down"))
			type = PAGE_DOWN;
		else if (!g_ascii_strcasecmp (d1, "+1"))
			type = LINE_DOWN;
		else if (!g_ascii_strcasecmp (d1, "-1"))
			type = LINE_UP;
	}

	if (!sess)
		return 0;

	adj = gtk_range_get_adjustment (GTK_RANGE (sess->gui->vscrollbar));
	end = gtk_adjustment_get_upper (adj) - gtk_adjustment_get_lower (adj) - gtk_adjustment_get_page_size (adj);

	switch (type)
	{
	case PAGE_TOP:
		value = 0;
		break;

	case PAGE_BOTTOM:
		value = end;
		break;

	case PAGE_UP:
		value = gtk_adjustment_get_value (adj) - (gtk_adjustment_get_page_size (adj) - 1);
		break;

	case PAGE_DOWN:
		value = gtk_adjustment_get_value (adj) + (gtk_adjustment_get_page_size (adj) - 1);
		break;

	case LINE_UP:
		value = gtk_adjustment_get_value (adj) - 1.0;
		break;

	case LINE_DOWN:
		value = gtk_adjustment_get_value (adj) + 1.0;
		break;
	}

	if (value < 0)
		value = 0;
	if (value > end)
		value = end;

	gtk_adjustment_set_value (adj, value);

	return 0;
}

static int
key_action_set_buffer (GtkWidget * wid, GdkEventKey * evt, char *d1, char *d2,
							  struct session *sess)
{
	if (!d1)
		return 1;
	if (d1[0] == 0)
		return 1;

	SPELL_ENTRY_SET_TEXT (wid, d1);
	SPELL_ENTRY_SET_POS (wid, -1);

	return 2;
}

static int
key_action_history_up (GtkWidget * wid, GdkEventKey * ent, char *d1, char *d2,
							  struct session *sess)
{
	char *new_line;

	new_line = history_up (&sess->history, SPELL_ENTRY_GET_TEXT (wid));
	if (new_line)
	{
		SPELL_ENTRY_SET_TEXT (wid, new_line);
		SPELL_ENTRY_SET_POS (wid, -1);
	}

	return 2;
}

static int
key_action_history_down (GtkWidget * wid, GdkEventKey * ent, char *d1,
								 char *d2, struct session *sess)
{
	char *new_line;

	new_line = history_down (&sess->history);
	if (new_line)
	{
		SPELL_ENTRY_SET_TEXT (wid, new_line);
		SPELL_ENTRY_SET_POS (wid, -1);
	}

	return 2;
}

/* old data that we reuse */
static struct gcomp_data old_gcomp;

/* work on the data, ie return only channels */
static int
double_chan_cb (session *lsess, GList **list)
{
	if (lsess->type == SESS_CHANNEL)
		*list = g_list_prepend(*list, lsess->channel);
	return TRUE;
}

/* convert a slist -> list. */
static GList *
chanlist_double_list (GSList *inlist)
{
	GList *list = NULL;
	g_slist_foreach(inlist, (GFunc)double_chan_cb, &list);
	return list;
}

/* handle commands */
static int
double_cmd_cb (struct popup *pop, GList **list)
{
	*list = g_list_prepend(*list, pop->name);
	return TRUE;
}

/* convert a slist -> list. */
static GList *
cmdlist_double_list (GSList *inlist)
{
	GList *list = NULL;
	g_slist_foreach (inlist, (GFunc)double_cmd_cb, &list);
	return list;
}

static char *
gcomp_nick_func (char *data)
{
	if (data)
		return ((struct User *)data)->nick;
	return "";
}

void
key_action_tab_clean(void)
{
	if (old_gcomp.elen)
	{
		old_gcomp.data[0] = 0;
		old_gcomp.elen = 0;
	}
}

/* For use in sorting the user list for completion

This sorts everyone by the last talked time except your own nick
which is forced to the bottom of the list to avoid completing your
own name, which is very unlikely.
*/
static int
talked_recent_cmp (struct User *a, struct User *b)
{
	if (a->me)
		return -1;

	if (b->me)
		return 1;

	if (a->lasttalk < b->lasttalk)
		return -1;

	if (a->lasttalk > b->lasttalk)
		return 1;

	return 0;
}

#define COMP_BUF 2048

static inline glong
len_to_offset (const char *str, glong len)
{
	return g_utf8_pointer_to_offset (str, str + len);
}

static inline glong
offset_to_len (const char *str, glong offset)
{
	return g_utf8_offset_to_pointer (str, offset) - str;
}

static int
key_action_tab_comp (GtkWidget *t, GdkEventKey *entry, char *d1, char *d2,
							struct session *sess)
{
	int len = 0, elen = 0, i = 0, cursor_pos, ent_start = 0, comp = 0, prefix_len, skip_len = 0;
	gboolean is_nick = FALSE, is_cmd = FALSE, found = FALSE, has_nick_prefix = FALSE;
	char ent[CHANLEN], *postfix = NULL, *result, *ch;
	GList *list = NULL, *tmp_list = NULL;
	const char *text;
	GCompletion *gcomp = NULL;
	GString *buf;

	/* force the IM Context to reset */
	SPELL_ENTRY_SET_EDITABLE (t, FALSE);
	SPELL_ENTRY_SET_EDITABLE (t, TRUE);

	text = SPELL_ENTRY_GET_TEXT (t);
	if (text[0] == 0)
		return 1;

	len = g_utf8_strlen (text, -1); /* must be null terminated */

	cursor_pos = SPELL_ENTRY_GET_POS (t);

	/* handle "nick: " or "nick " or "#channel "*/
	ch = g_utf8_find_prev_char(text, g_utf8_offset_to_pointer(text,cursor_pos));
	if (ch && ch[0] == ' ')
	{
		skip_len++;
		ch = g_utf8_find_prev_char(text, ch);
		if (!ch)
			return 2;

		cursor_pos = g_utf8_pointer_to_offset(text, ch);
		if (cursor_pos && (g_utf8_get_char_validated(ch, -1) == ':' || 
					g_utf8_get_char_validated(ch, -1) == ',' ||
					g_utf8_get_char_validated (ch, -1) == g_utf8_get_char_validated (prefs.hex_completion_suffix, -1)))
		{
			skip_len++;
		}
		else
			cursor_pos = g_utf8_pointer_to_offset(text, g_utf8_offset_to_pointer(ch, 1));
	}

	comp = skip_len;
	
	/* store the text following the cursor for reinsertion later */
	if ((cursor_pos + skip_len) < len)
		postfix = g_utf8_offset_to_pointer(text, cursor_pos + skip_len);

	for (ent_start = cursor_pos; ; --ent_start)
	{
		if (ent_start == 0)
			break;
		ch = g_utf8_offset_to_pointer(text, ent_start - 1);
		if (ch && ch[0] == ' ')
			break;
	}

	if (ent_start == 0 && text[0] == prefs.hex_input_command_char[0])
	{
		ent_start++;
		is_cmd = TRUE;
	}
	else if (strchr (sess->server->chantypes, text[ent_start]) == NULL)
	{
		is_nick = TRUE;
		if (strchr (sess->server->nick_prefixes, text[ent_start]) != NULL)
		{
			if (ent_start == 0)
				has_nick_prefix = TRUE;
			ent_start++;
		}
	}

	prefix_len = ent_start;
	elen = cursor_pos - ent_start;

	g_utf8_strncpy (ent, g_utf8_offset_to_pointer (text, prefix_len), elen);
	
	if (sess->type == SESS_DIALOG && is_nick)
	{
		/* tab in a dialog completes the other person's name */
		if (rfc_ncasecmp (sess->channel, ent, elen) == 0)
		{
			result =  sess->channel;
			is_nick = FALSE;
		}
		else
			return 2;
	}
	else
	{
		if (is_nick)
		{
			gcomp = g_completion_new((GCompletionFunc)gcomp_nick_func);
			tmp_list = userlist_double_list(sess); /* create a temp list so we can free the memory */
			if (prefs.hex_completion_sort == 1)	/* sort in last-talk order? */
				tmp_list = g_list_sort (tmp_list, (void *)talked_recent_cmp);
		}
		else
		{
			gcomp = g_completion_new (NULL);
			if (is_cmd)
			{
				tmp_list = cmdlist_double_list (command_list);
				for(i = 0; xc_cmds[i].name != NULL ; i++)
				{
					tmp_list = g_list_prepend (tmp_list, xc_cmds[i].name);
				}
				tmp_list = plugin_command_list(tmp_list);
			}
			else
				tmp_list = chanlist_double_list (sess_list);
		}
		tmp_list = g_list_reverse(tmp_list); /* make the comp entries turn up in the right order */
		g_completion_set_compare (gcomp, (GCompletionStrncmpFunc)rfc_ncasecmp);
		if (tmp_list)
		{
			g_completion_add_items (gcomp, tmp_list);
			g_list_free (tmp_list);
		}

		if (comp && !(rfc_ncasecmp(old_gcomp.data, ent, old_gcomp.elen) == 0))
		{
			key_action_tab_clean ();
			comp = 0;
		}
	
		list = g_completion_complete_utf8 (gcomp, comp ? old_gcomp.data : ent, &result);
		
		if (result == NULL) /* No matches found */
		{
			g_completion_free(gcomp);
			return 2;
		}

		if (comp) /* existing completion */
		{
			while(list) /* find the current entry */
			{
				if(rfc_ncasecmp(list->data, ent, elen) == 0)
				{
					found = TRUE;
					break;
				}
				list = list->next;
			}

			if (found)
			{
				if (!(d1 && d1[0])) /* not holding down shift */
				{
					if (g_list_next(list) == NULL)
						list = g_list_first(list);
					else
						list = g_list_next(list);
				}
				else
				{
					if (g_list_previous(list) == NULL)
						list = g_list_last(list);
					else
						list = g_list_previous(list);
				}
				g_free(result);
				result = (char*)list->data;
			}
			else
			{
				g_free(result);
				g_completion_free(gcomp);
				return 2;
			}
		}
		else
		{
			strcpy(old_gcomp.data, ent);
			old_gcomp.elen = elen;

			/* Get the first nick and put out the data for future nickcompletes */
			if (prefs.hex_completion_amount > 0 && g_list_length (list) <= (guint) prefs.hex_completion_amount)
			{
				g_free(result);
				result = (char*)list->data;
			}
			else
			{
				/* bash style completion */
				if (g_list_next(list) != NULL)
				{
					buf = g_string_sized_new (MAX(COMP_BUF, len + NICKLEN));
					if (strlen (result) > elen) /* the largest common prefix is larger than nick, change the data */
					{
						if (prefix_len)
							g_string_append_len (buf, text, offset_to_len (text, prefix_len));
						g_string_append (buf, result);
						cursor_pos = buf->len;
						g_free(result);
						if (postfix)
						{
							g_string_append_c (buf, ' ');
							g_string_append (buf, postfix);
						}
						SPELL_ENTRY_SET_TEXT (t, buf->str);
						SPELL_ENTRY_SET_POS (t, len_to_offset (buf->str, cursor_pos));
						g_string_erase (buf, 0, -1);
					}
					else
						g_free(result);

					while (list)
					{
						len = buf->len;
						elen = strlen (list->data);	/* next item to add */
						if (len + elen + 2 >= COMP_BUF) /* +2 is space + null */
						{
							PrintText (sess, buf->str);
							g_string_erase (buf, 0, -1);
						}
						g_string_append (buf, (char*)list->data);
						g_string_append_c (buf, ' ');
						list = list->next;
					}
					PrintText (sess, buf->str);
					g_completion_free(gcomp);
					g_string_free (buf, TRUE);
					return 2;
				}
				/* Only one matching entry */
				g_free(result);
				result = list->data;
			}
		}
	}
	
	if(result)
	{
		buf = g_string_sized_new (len + NICKLEN);
		if (prefix_len)
			g_string_append_len (buf, text, offset_to_len (text, prefix_len));
		g_string_append (buf, result);
		if((!prefix_len || has_nick_prefix) && is_nick && prefs.hex_completion_suffix[0] != '\0')
			g_string_append_unichar (buf, g_utf8_get_char_validated (prefs.hex_completion_suffix, -1));
		g_string_append_c (buf, ' ');
		cursor_pos = buf->len;
		if (postfix)
			g_string_append (buf, postfix);
		SPELL_ENTRY_SET_TEXT (t, buf->str);
		SPELL_ENTRY_SET_POS (t, len_to_offset (buf->str, cursor_pos));
		g_string_free (buf, TRUE);
	}
	if (gcomp)
		g_completion_free(gcomp);
	return 2;
}
#undef COMP_BUF

static int
key_action_comp_chng (GtkWidget * wid, GdkEventKey * ent, char *d1, char *d2,
		struct session *sess)
{
	key_action_tab_comp(wid, ent, d1, d2, sess);
	return 2;
}


static int
key_action_replace (GtkWidget * wid, GdkEventKey * ent, char *d1, char *d2,
						  struct session *sess)
{
	replace_handle (wid);
	return 1;
}


static int
key_action_move_tab_left (GtkWidget * wid, GdkEventKey * ent, char *d1,
								  char *d2, struct session *sess)
{
	mg_move_tab (sess, +1);
	return 2;						  /* don't allow default action */
}

static int
key_action_move_tab_right (GtkWidget * wid, GdkEventKey * ent, char *d1,
									char *d2, struct session *sess)
{
	mg_move_tab (sess, -1);
	return 2;						  /* -''- */
}

static int
key_action_move_tab_family_left (GtkWidget * wid, GdkEventKey * ent, char *d1,
								  char *d2, struct session *sess)
{
	mg_move_tab_family (sess, +1);
	return 2;						  /* don't allow default action */
}

static int
key_action_move_tab_family_right (GtkWidget * wid, GdkEventKey * ent, char *d1,
									char *d2, struct session *sess)
{
	mg_move_tab_family (sess, -1);
	return 2;						  /* -''- */
}

static int
key_action_put_history (GtkWidget * wid, GdkEventKey * ent, char *d1,
									char *d2, struct session *sess)
{
	history_add (&sess->history, SPELL_ENTRY_GET_TEXT (wid));
	SPELL_ENTRY_SET_TEXT (wid, "");
	return 2;						  /* -''- */
}


/* -------- */

static void
replace_handle (GtkWidget *t)
{
	const char *text, *postfix_pnt;
	struct popup *pop;
	GSList *list = replace_list;
	char word[256];
	char postfix[256];
	char outbuf[4096];
	int c, len, xlen;

	text = SPELL_ENTRY_GET_TEXT (t);

	len = strlen (text);
	if (len < 1)
		return;

	for (c = len - 1; c > 0; c--)
	{
		if (text[c] == ' ')
			break;
	}
	if (text[c] == ' ')
		c++;
	xlen = c;
	if (len - c >= (sizeof (word) - 12))
		return;
	if (len - c < 1)
		return;
	memcpy (word, &text[c], len - c);
	word[len - c] = 0;
	len = strlen (word);
	if (word[0] == '\'' && word[len] == '\'')
		return;
	postfix_pnt = NULL;
	for (c = 0; c < len; c++)
	{
		if (word[c] == '\'')
		{
			postfix_pnt = &word[c + 1];
			word[c] = 0;
			break;
		}
	}

	if (postfix_pnt != NULL)
	{
		if (strlen (postfix_pnt) > sizeof (postfix) - 12)
			return;
		strcpy (postfix, postfix_pnt);
	}
	while (list)
	{
		pop = (struct popup *) list->data;
		if (strcmp (pop->name, word) == 0)
		{
			memcpy (outbuf, text, xlen);
			outbuf[xlen] = 0;
			if (postfix_pnt == NULL)
				g_snprintf (word, sizeof (word), "%s", pop->cmd);
			else
				g_snprintf (word, sizeof (word), "%s%s", pop->cmd, postfix);
			g_strlcat (outbuf, word, sizeof(outbuf));
			SPELL_ENTRY_SET_TEXT (t, outbuf);
			SPELL_ENTRY_SET_POS (t, -1);
			return;
		}
		list = list->next;
	}
}

