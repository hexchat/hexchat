/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Wed Jan  1 19:06:46 GMT+4 2003
    copyright            : (C) 2003 - 2005 by Alex Shaduri
    email                : ashaduri '@' gmail.com
 ***************************************************************************/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <sstream>
#include <gtk/gtk.h>

#ifdef _WIN32
#include "win32util.h"
#include <io.h>
#else
#include <sys/stat.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "interface.h"
#include "callbacks.h"
#include "support.h"

#include "main.h"


// ------------------------------------------------------



GtkWidget* g_main_window = 0;
// GtkWidget* g_fontsel_dialog = 0;


static std::string s_tmp_file;



// ------------------------------------------------------



static std::string gchar_to_string(gchar* gstr)
{
	std::string str = (gstr ? gstr : "");
	g_free(gstr);
	return str;
}



std::string get_home_dir()
{
	std::string dir;

	if (g_get_home_dir())
		dir = g_get_home_dir();

#ifdef _WIN32
	if (dir == "") {
		dir = win32_get_registry_value_string(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "Personal");
	}
	if (dir == "") {
		dir = win32_get_registry_value_string(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", "User");
	}
#endif

	return dir;

}




// ------------------------------------------------------


static std::string s_orig_theme;
static std::string s_orig_font;

std::string& get_orig_theme()
{
	return s_orig_theme;
}


std::string& get_orig_font()
{
	return s_orig_font;
}


// ------------------------------------------------------


std::string get_current_theme()
{

	GtkSettings* settings = gtk_settings_get_default();
	gchar* theme;
	g_object_get(settings, "gtk-theme-name", &theme, NULL);

	return gchar_to_string(theme);
}




std::string get_current_font()
{
	return gchar_to_string(pango_font_description_to_string(gtk_rc_get_style(g_main_window)->font_desc));
}



// ------------------------------------------------------



std::string get_selected_theme()
{
	GtkTreeView* treeview = GTK_TREE_VIEW(lookup_widget(g_main_window, "main_themelist"));
	GtkTreeModel* model = gtk_tree_view_get_model(treeview);
	GtkTreeSelection* selection = gtk_tree_view_get_selection(treeview);

	GtkTreeIter iter;
	gtk_tree_selection_get_selected(selection, 0, &iter);

	gchar* theme_name;
	gtk_tree_model_get(model, &iter, 0, &theme_name, -1);
// 	std::cout << theme_name << "\n";
	return gchar_to_string(theme_name);
}



std::string get_selected_font()
{
// 	GtkWidget* fontentry = lookup_widget(g_main_window, "main_fontentry");
// 	return gtk_entry_get_text(GTK_ENTRY(fontentry));

	bool default_font = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(g_main_window, "main_use_default_font_radio")));

	if (default_font)
		return "";

	GtkWidget* fontbutton = lookup_widget(g_main_window, "main_font_selector_button");
	return gtk_font_button_get_font_name(GTK_FONT_BUTTON(fontbutton));
}


// ------------------------------------------------------



static void themelist_selection_changed_cb(GtkTreeSelection* selection, gpointer data)
{
	if (gtk_tree_selection_get_selected (selection, 0, 0))
		apply_theme(get_selected_theme(), get_current_font());
}



// ------------------------------------------------------



static void populate_with_themes(GtkWidget* w)
{

	std::string search_path = gchar_to_string(gtk_rc_get_theme_dir());
	//std::string search_path = gchar_to_string(g_build_filename("lib", "gtk-2.0", "2.10.0", "engines", NULL));

	if (search_path.size() && search_path[search_path.size() -1] != G_DIR_SEPARATOR)
		search_path += G_DIR_SEPARATOR_S;

	GDir* gdir = g_dir_open(search_path.c_str(), 0, NULL);
	if (gdir == NULL)
		return;


	char* name;
	GList* glist = 0;

	while ( (name = const_cast<char*>(g_dir_read_name(gdir))) != NULL ) {
		std::string filename = name;

//		if (g_ascii_strup(fname.c_str(), -1) == "Default")
//			continue;

		std::string fullname = search_path + filename;
		std::string rc = fullname; rc += G_DIR_SEPARATOR_S; rc += "gtk-2.0"; rc += G_DIR_SEPARATOR_S; rc += "gtkrc";

		bool is_dir = 0;
		if (g_file_test(fullname.c_str(), G_FILE_TEST_IS_DIR))
			is_dir = 1;

		if (is_dir && g_file_test(rc.c_str(), G_FILE_TEST_IS_REGULAR)) {
			glist = g_list_insert_sorted(glist, g_strdup(filename.c_str()), (GCompareFunc)strcmp);
		}
	}

	g_dir_close(gdir);




	// ---------------- tree


	GtkTreeView* treeview = GTK_TREE_VIEW(w);
	GtkListStore *store = gtk_list_store_new (1, G_TYPE_STRING);
	gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(store));

	GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes (
												"Theme", gtk_cell_renderer_text_new(),
												"text", 0,
												NULL);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);


	GtkTreeIter   iter;

	int i =0, curr=0;
	while (char* theme = (char*)g_list_nth_data(glist, i)) {
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, theme, -1);

		if (strcmp(theme, get_current_theme().c_str()) == 0) {
			curr = i;
		}

		++i;
	}


	GtkTreeSelection* selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

	// set the default theme

	// THIS IS IMPORTANT!!!
	gtk_widget_grab_focus(w);

	std::stringstream str;
	str << curr;
	GtkTreePath* selpath = gtk_tree_path_new_from_string (str.str().c_str());
	if (selpath) {
		gtk_tree_selection_select_path(selection, selpath);
		gtk_tree_view_scroll_to_cell(treeview, selpath, NULL, true, 0.5, 0.0);
		gtk_tree_path_free(selpath);
	}

	g_signal_connect (G_OBJECT (selection), "changed",
                  G_CALLBACK (themelist_selection_changed_cb), NULL);

	g_object_unref (G_OBJECT (store));


	// ---------------- font

// 	GtkWidget* fontentry = lookup_widget(g_main_window, "main_fontentry");
// 	gtk_entry_set_text(GTK_ENTRY(fontentry), get_current_font().c_str());

	GtkWidget* fontbutton = lookup_widget(g_main_window, "main_font_selector_button");
	gtk_font_button_set_font_name(GTK_FONT_BUTTON(fontbutton), get_current_font().c_str());


}




// ------------------------------------------------------

#ifdef _WIN32
static void redirect_to_file (const gchar* log_domain, GLogLevelFlags log_level,
									const gchar* message, gpointer user_data)
{
	/* only write logs if running in portable mode, otherwise
	   we would get a permission error in program files */
	if ((_access( "portable-mode", 0 )) != -1)
	{
		std::fstream f;
		f.open("gtk2-prefs.log", std::ios::app);
		f << message << "\n";
		f.close();
	}
}
#endif

// ------------------------------------------------------




int main(int argc, char *argv[])
{

	// work around pango weirdness
#ifdef _WIN32
	// no longer needed as of pango 1.2.5, but won't do any harm
// 	putenv("PANGO_WIN32_NO_UNISCRIBE=1");
#endif




	std::string user = g_get_user_name();
	std::string tmp = g_get_tmp_dir();
	std::string tmp_file = tmp + G_DIR_SEPARATOR_S + "gtk_prefs_tmprc_" + user;
	s_tmp_file = tmp_file;
	gtk_rc_add_default_file(tmp_file.c_str());



	gtk_init (&argc, &argv);

	// redirect gtk warnings to file when in win32
#if defined _WIN32 && !defined DEBUG
	g_log_set_handler ("GLib", GLogLevelFlags(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
                     | G_LOG_FLAG_RECURSION), redirect_to_file, NULL);
	g_log_set_handler ("GModule", GLogLevelFlags(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
                     | G_LOG_FLAG_RECURSION), redirect_to_file, NULL);
	g_log_set_handler ("GLib-GObject", GLogLevelFlags(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
                     | G_LOG_FLAG_RECURSION), redirect_to_file, NULL);
	g_log_set_handler ("GThread", GLogLevelFlags(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
                     | G_LOG_FLAG_RECURSION), redirect_to_file, NULL);
	g_log_set_handler ("Gtk", GLogLevelFlags(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
                     | G_LOG_FLAG_RECURSION), redirect_to_file, NULL);
	g_log_set_handler ("Gdk", GLogLevelFlags(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
                     | G_LOG_FLAG_RECURSION), redirect_to_file, NULL);
	g_log_set_handler ("GdkPixbuf", GLogLevelFlags(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
                     | G_LOG_FLAG_RECURSION), redirect_to_file, NULL);
#endif


	add_pixmap_directory(".");

	g_main_window = create_main_window();


	populate_with_themes(lookup_widget(g_main_window, "main_themelist"));


	std::string about_text = std::string("GTK+ Preference Tool") + "\n\
\n\
by Alex Shaduri <ashaduri@gmail.com>\n\
\n\
  The authors make NO WARRANTY or representation, either express or implied, with respect to this software, its quality, accuracy, merchantability, or fitness for a particular purpose.  This software is provided \"AS IS\", and you, its user, assume the entire risk as to its quality and accuracy.\n\
\n\
  This is free software, and you are welcome to redistribute it under terms of GNU General Public License v2.";


	gtk_label_set_text(GTK_LABEL(lookup_widget(g_main_window, "about_label")), about_text.c_str());


	GtkTreeView* treeview = GTK_TREE_VIEW(lookup_widget(g_main_window, "preview_treeview"));
	GtkTreeStore *store = gtk_tree_store_new (2, G_TYPE_STRING, G_TYPE_INT);
	gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(store));


	GtkTreeViewColumn* column1 = gtk_tree_view_column_new_with_attributes ("Text", gtk_cell_renderer_text_new(), "text", 0, NULL);
	gtk_tree_view_column_set_sizing(column1, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
	gtk_tree_view_column_set_resizable(column1, true);
	gtk_tree_view_column_set_reorderable (column1, true);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column1);

	GtkTreeViewColumn* column2 = gtk_tree_view_column_new_with_attributes ("Number", gtk_cell_renderer_text_new(), "text", 1, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column2);


	GtkTreeIter   iter1, iter2;

	gtk_tree_store_append (store, &iter1, NULL);
	gtk_tree_store_set (store, &iter1, 0, "Parent 2", 1, 66, -1);

	gtk_tree_store_append (store, &iter1, NULL);
	gtk_tree_store_set (store, &iter1, 0, "Parent 1", 1, 65, -1);

	gtk_tree_store_append (store, &iter2, &iter1);
	gtk_tree_store_set (store, &iter2, 0, "Child 1", 1, 67, -1);

	gtk_tree_view_column_set_sort_column_id(column1, 0);
	gtk_tree_view_column_set_sort_order (column1, GTK_SORT_ASCENDING);
	gtk_tree_view_column_set_sort_indicator(column1, true);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), 0, GTK_SORT_ASCENDING);

	g_object_unref (G_OBJECT (store));


	get_orig_theme() = get_current_theme();
	get_orig_font() = get_current_font();


	gtk_widget_show (g_main_window);


	gtk_main();

	return EXIT_SUCCESS;

}



// -------------------------------



void set_theme(const std::string& theme_name, const std::string& font)
{
	// set widgets accordingly

	// tree
	GtkTreeView* treeview = GTK_TREE_VIEW(lookup_widget(g_main_window, "main_themelist"));
	GtkTreeModel* model = gtk_tree_view_get_model(treeview);
	GtkTreeSelection* selection = gtk_tree_view_get_selection(treeview);

	GtkTreeIter iter;
	gtk_tree_model_get_iter_first(model, &iter);

	while(gtk_tree_model_iter_next(model, &iter)) {

		gchar* text;
		gtk_tree_model_get (model, &iter, 0, &text, -1);
		std::string theme = gchar_to_string(text);

		if (theme_name == theme) {
			gtk_tree_selection_select_iter(selection, &iter);
			break;
		}

	}


	// font
// 	GtkWidget* fontentry = lookup_widget(g_main_window, "main_fontentry");
// 	gtk_entry_set_text(GTK_ENTRY(fontentry), font.c_str());

	if (font != "") {
		GtkWidget* fontbutton = lookup_widget(g_main_window, "main_font_selector_button");
		gtk_font_button_set_font_name(GTK_FONT_BUTTON(fontbutton), get_current_font().c_str());
	}


	apply_theme(get_selected_theme(), get_selected_font());

}




void apply_theme(const std::string& theme_name, const std::string& font)
{

	std::stringstream strstr;
	strstr << "gtk-theme-name = \"" << theme_name << "\"\n";

	if (font != "")
		strstr << "style \"user-font\"\n{\nfont_name=\"" << font << "\"\n}\nwidget_class \"*\" style \"user-font\"";

// 	std::cout << strstr.str() << "\n\n\n";
	std::fstream f;
	f.open(s_tmp_file.c_str(), std::ios::out);
		f << strstr.str();
	f.close();


	GtkSettings* settings = gtk_settings_get_default();

  	gtk_rc_reparse_all_for_settings (settings, true);
// 	gtk_rc_parse_string(strstr.str().c_str());
//  	gtk_rc_parse("/root/.gtk-tmp");
//  	gtk_rc_reset_styles(settings);

	unlink(s_tmp_file.c_str());

	while (gtk_events_pending())
		gtk_main_iteration();


}




#ifdef _WIN32
#ifdef HAVE_DIRECT_H
#include <direct.h>
#define mkdir(a) _mkdir(a)
#else
#define mkdir(a, b) mkdir(a)
#endif
#endif


bool save_current_theme()
{

	std::string conf_file = "";

	if ((_access( "portable-mode", 0 )) != -1) {

		char* themes_dir_c = gtk_rc_get_theme_dir();
		char* conf_file_c = g_build_filename("etc", "gtk-2.0", "gtkrc", NULL);

		conf_file = (conf_file_c ? conf_file_c : "");

		// file doesn't exist, try to get it from gtk.
		if (!g_file_test(conf_file.c_str(), G_FILE_TEST_EXISTS)) {

			gchar** rc_files = gtk_rc_get_default_files();
			if (rc_files[0] != 0) {
				conf_file = rc_files[0];
			}

		}


		g_free(themes_dir_c);
		g_free(conf_file_c);


		// mkdir a directory, only one level deep
		char* etc = g_path_get_dirname(conf_file.c_str());
		if (!g_file_test(etc, G_FILE_TEST_IS_DIR)) {
			mkdir(etc, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
		}
		g_free(etc);


	} else {

		conf_file = get_home_dir();

		if (conf_file[conf_file.length()-1] != G_DIR_SEPARATOR)
			conf_file += G_DIR_SEPARATOR_S;

		conf_file += ".gtkrc-2.0";

	}



	// ask

	GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GtkWidget* dialog = gtk_dialog_new_with_buttons ("Query", GTK_WINDOW(window),
											GtkDialogFlags(GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL),
											GTK_STOCK_NO, GTK_RESPONSE_REJECT,
											GTK_STOCK_YES, GTK_RESPONSE_ACCEPT,
											NULL);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
	GtkWidget* hbox = gtk_hbox_new(1, 1);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);

	gtk_container_add (GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), hbox);
	gtk_container_add (GTK_CONTAINER(hbox),
			gtk_label_new((std::string("\n\nThe file \"") + conf_file +  "\" will be overwritten.\nAre you sure?\n\n").c_str()));

	gtk_widget_show_all (dialog);

	bool ret = 0;
	gint result = gtk_dialog_run (GTK_DIALOG (dialog));
	switch (result) {
		case GTK_RESPONSE_ACCEPT:
			ret = 1;
			break;
	}
	gtk_widget_destroy(dialog);
	gtk_widget_destroy(window);

	if (!ret)  // the user pressed "No".
		return false;


	std::string font = get_selected_font();

	std::fstream f;
	f.open(conf_file.c_str(), std::ios::out);
		f << "# Auto-written by gtk2_prefs. Do not edit.\n\n";
		f << "gtk-theme-name = \"" << get_selected_theme() << "\"\n";

		if (font != "")
			f << "style \"user-font\"\n{\n\tfont_name=\"" << font << "\"\n}\nwidget_class \"*\" style \"user-font\"";
	f.close();


	return true;

}




void program_shutdown()
{
	gtk_main_quit();
}





