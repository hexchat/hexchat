/* Copyright (c) 2005 Peter Zelezny
   All Rights Reserved.

   joind.c - The Join Dialog.

   Popups up when you connect without any autojoin channels and helps you
   to find or join a channel.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <gtk/gtkbbox.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkimage.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkwindow.h>

#include "../common/xchat.h"
#include "../common/xchatc.h"
#include "../common/server.h"
#include "../common/fe.h"
#include "fe-gtk.h"
#include "chanlist.h"


static void
joind_radio2_cb (GtkWidget *radio, server *serv)
{
	if (GTK_TOGGLE_BUTTON (radio)->active)
	{
		gtk_widget_grab_focus (serv->gui->joind_entry);
		gtk_editable_set_position (GTK_EDITABLE (serv->gui->joind_entry), 999);
	}
}

static void
joind_entryenter_cb (GtkWidget *entry, GtkWidget *ok)
{
	gtk_widget_grab_focus (ok);
}

static void
joind_entryfocus_cb (GtkWidget *entry, GdkEventFocus *event, server *serv)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (serv->gui->joind_radio2), TRUE);
}

static void
joind_destroy_cb (GtkWidget *win, server *serv)
{
	if (is_server (serv))
		serv->gui->joind_win = NULL;
}

static void
joind_ok_cb (GtkWidget *ok, server *serv)
{
	if (!is_server (serv))
	{
		gtk_widget_destroy (gtk_widget_get_toplevel (ok));
		return;
	}

	/* do nothing */
	if (GTK_TOGGLE_BUTTON (serv->gui->joind_radio1)->active)
		goto xit;

	/* join specific channel */
	if (GTK_TOGGLE_BUTTON (serv->gui->joind_radio2)->active)
	{
		char *text = GTK_ENTRY (serv->gui->joind_entry)->text;
		if (strlen (text) < 2)
		{
			fe_message (_("Channel name too short, try again."), FE_MSG_ERROR);
			return;
		}
		serv->p_join (serv, text, "");
		goto xit;
	}

	/* channel list */
	chanlist_opengui (serv, TRUE);

xit:
	prefs.gui_join_dialog = 0;
	if (GTK_TOGGLE_BUTTON (serv->gui->joind_check)->active)
		prefs.gui_join_dialog = 1;

	gtk_widget_destroy (serv->gui->joind_win);
	serv->gui->joind_win = NULL;
}

static void
joind_show_dialog (server *serv)
{
	GtkWidget *dialog1;
	GtkWidget *dialog_vbox1;
	GtkWidget *vbox1;
	GtkWidget *hbox1;
	GtkWidget *image1;
	GtkWidget *vbox2;
	GtkWidget *label;
	GtkWidget *radiobutton1;
	GtkWidget *radiobutton2;
	GtkWidget *radiobutton3;
	GSList *radiobutton1_group;
	GtkWidget *hbox2;
	GtkWidget *entry1;
	GtkWidget *checkbutton1;
	GtkWidget *dialog_action_area1;
	GtkWidget *okbutton1;
	char buf[256];
	char buf2[256];

	serv->gui->joind_win = dialog1 = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (dialog1), _("XChat: Connection Complete"));
	gtk_window_set_type_hint (GTK_WINDOW (dialog1), GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_window_set_position (GTK_WINDOW (dialog1), GTK_WIN_POS_MOUSE);

	dialog_vbox1 = GTK_DIALOG (dialog1)->vbox;
	gtk_widget_show (dialog_vbox1);

	vbox1 = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox1);
	gtk_box_pack_start (GTK_BOX (dialog_vbox1), vbox1, TRUE, TRUE, 0);

	hbox1 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox1);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 0);

	image1 = gtk_image_new_from_stock ("gtk-yes", GTK_ICON_SIZE_DIALOG);
	gtk_widget_show (image1);
	gtk_box_pack_start (GTK_BOX (hbox1), image1, FALSE, TRUE, 24);
	gtk_misc_set_alignment (GTK_MISC (image1), 0.5, 0.06);

	vbox2 = gtk_vbox_new (FALSE, 10);
	gtk_container_set_border_width (GTK_CONTAINER (vbox2), 6);
	gtk_widget_show (vbox2);
	gtk_box_pack_start (GTK_BOX (hbox1), vbox2, TRUE, TRUE, 0);

	snprintf (buf2, sizeof (buf2), _("Connection to %s complete."),
				 server_get_network (serv, TRUE));
	snprintf (buf, sizeof (buf), "\n<b>%s</b>", buf2);
	label = gtk_label_new (buf);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

	label = gtk_label_new (_("In the Server-List window, no channel (chat room) has been entered to be automatically joined for this network."));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 0);
	GTK_LABEL (label)->wrap = TRUE;
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

	label = gtk_label_new (_("What would you like to do next?"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 0);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

	serv->gui->joind_radio1 = radiobutton1 = gtk_radio_button_new_with_mnemonic (NULL, _("_Nothing, I'll join a channel later."));
	gtk_widget_show (radiobutton1);
	gtk_box_pack_start (GTK_BOX (vbox2), radiobutton1, FALSE, FALSE, 0);
	radiobutton1_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radiobutton1));

	hbox2 = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox2);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, FALSE, 0);

	serv->gui->joind_radio2 = radiobutton2 = gtk_radio_button_new_with_mnemonic (NULL, _("_Join this channel:"));
	gtk_widget_show (radiobutton2);
	gtk_box_pack_start (GTK_BOX (hbox2), radiobutton2, FALSE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (radiobutton2), radiobutton1_group);
	radiobutton1_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radiobutton2));

	serv->gui->joind_entry = entry1 = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (entry1), "#");
	gtk_widget_show (entry1);
	gtk_box_pack_start (GTK_BOX (hbox2), entry1, TRUE, TRUE, 8);

	snprintf (buf, sizeof (buf), "<small>     %s</small>",
				 _("If you know the name of the channel you want to join, enter it here."));
	label = gtk_label_new (buf);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

	radiobutton3 = gtk_radio_button_new_with_mnemonic (NULL, _("O_pen the Channel-List window."));
	gtk_widget_show (radiobutton3);
	gtk_box_pack_start (GTK_BOX (vbox2), radiobutton3, FALSE, FALSE, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (radiobutton3), radiobutton1_group);
	radiobutton1_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (radiobutton3));

	snprintf (buf, sizeof (buf), "<small>     %s</small>",
				 _("Retrieving the Channel-List may take a minute or two."));
	label = gtk_label_new (buf);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 0);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

	serv->gui->joind_check = checkbutton1 = gtk_check_button_new_with_mnemonic (_("_Always show this dialog after connecting."));
	if (prefs.gui_join_dialog)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton1), TRUE);
	gtk_widget_show (checkbutton1);
	gtk_box_pack_start (GTK_BOX (vbox1), checkbutton1, FALSE, FALSE, 0);

	dialog_action_area1 = GTK_DIALOG (dialog1)->action_area;
	gtk_widget_show (dialog_action_area1);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_END);

	okbutton1 = gtk_button_new_from_stock ("gtk-ok");
	gtk_widget_show (okbutton1);
	gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dialog1)->action_area), okbutton1, FALSE, TRUE, 0);
	GTK_WIDGET_SET_FLAGS (okbutton1, GTK_CAN_DEFAULT);

	g_signal_connect (G_OBJECT (dialog1), "destroy",
							G_CALLBACK (joind_destroy_cb), serv);
	g_signal_connect (G_OBJECT (entry1), "focus_in_event",
							G_CALLBACK (joind_entryfocus_cb), serv);
	g_signal_connect (G_OBJECT (entry1), "activate",
							G_CALLBACK (joind_entryenter_cb), okbutton1);
	g_signal_connect (G_OBJECT (radiobutton2), "toggled",
							G_CALLBACK (joind_radio2_cb), serv);
	g_signal_connect (G_OBJECT (okbutton1), "clicked",
							G_CALLBACK (joind_ok_cb), serv);

	gtk_widget_grab_focus (okbutton1);
	gtk_widget_show_all (dialog1);
}

void
joind (int action, server *serv)
{
	switch (action)
	{
	case 0:
		if (prefs.gui_join_dialog)
			joind_show_dialog (serv);
		break;

	case 1:
		if (serv->gui->joind_win)
		{
			gtk_widget_destroy (serv->gui->joind_win);
			serv->gui->joind_win = NULL;
		}
	}
}
