#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <iostream>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include "main.h"



extern GtkWidget* g_main_window;

void
on_main_showpreviewtoggle_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	if (gtk_toggle_button_get_active (togglebutton)) {
		gtk_widget_show(lookup_widget(g_main_window, "main_previewhbox"));
		gtk_button_set_label(GTK_BUTTON(togglebutton), "Hide preview <<");
		gtk_window_resize(GTK_WINDOW(g_main_window), 700, 330);
	} else {
		gtk_widget_hide(lookup_widget(g_main_window, "main_previewhbox"));
		gtk_button_set_label(GTK_BUTTON(togglebutton), "Show preview >>");
		gtk_window_resize(GTK_WINDOW(g_main_window), 300, 330);
	}

}


/*
extern GtkWidget* g_fontsel_dialog;


void
on_fontsel_dialog_response             (GtkDialog       *dialog,
                                        gint             id,
                                        gpointer         user_data)
{

	switch(id) {
		case GTK_RESPONSE_OK:
		{
			const char* font = gtk_font_selection_dialog_get_font_name (GTK_FONT_SELECTION_DIALOG(dialog));
			std::string f = (font ? font : "");
			set_theme(get_selected_theme(), f);
			break;
		}
//		case GTK_RESPONSE_CANCEL:
//			gtk_widget_destroy(g_fontsel_dialog);
//			g_fontsel_dialog = 0;
//			return;

	}
	gtk_widget_destroy(g_fontsel_dialog);
	g_fontsel_dialog = 0;

}


gboolean
on_fontsel_dialog_delete_event         (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	gtk_widget_destroy(g_fontsel_dialog);
	g_fontsel_dialog = 0;

	return FALSE;
}


void
on_main_fontselectbutton_clicked       (GtkButton       *button,
                                        gpointer         user_data)
{
	g_fontsel_dialog = create_fontsel_dialog();
	gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(g_fontsel_dialog), get_current_font().c_str());
	gtk_widget_show(g_fontsel_dialog);
}
*/





void
on_main_ok_button_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
	if (save_current_theme())
		gtk_main_quit();
}


void
on_main_cancel_button_clicked          (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_main_quit();
}


void
on_main_reset_button_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{
	set_theme(get_orig_theme(), get_orig_font());
}


gboolean
on_main_window_delete_event            (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	program_shutdown();
	return true;
}


void
on_main_use_default_font_radio_toggled (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	bool default_font = gtk_toggle_button_get_active(togglebutton);

	gtk_widget_set_sensitive(lookup_widget(g_main_window, "main_font_selector_button"), !default_font);

	apply_theme(get_selected_theme(), get_selected_font());
}


void
on_main_font_selector_button_font_set  (GtkFontButton   *fontbutton,
                                        gpointer         user_data)
{
	apply_theme(get_selected_theme(), get_selected_font());
}


void
on_new2_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_open2_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_save2_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_save_as2_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_quit2_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_cut2_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_copy2_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_paste2_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_delete2_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_about2_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}

