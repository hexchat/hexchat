/* GTK+ Preference Tool
 * Copyright (C) 2003-2005 Alex Shaduri.
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

#include <gtk/gtk.h>


void
on_main_showpreviewtoggle_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_main_ok_button_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_main_cancel_button_clicked          (GtkButton       *button,
                                        gpointer         user_data);

void
on_main_reset_button_clicked           (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_main_window_delete_event            (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_main_use_default_font_radio_toggled (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_main_font_selector_button_font_set  (GtkFontButton   *fontbutton,
                                        gpointer         user_data);

void
on_new2_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_open2_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_save2_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_save_as2_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_quit2_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_cut2_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_copy2_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_paste2_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_delete2_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about2_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);
