#define GTK_DISABLE_DEPRECATED

#include <gtk/gtkhbox.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkcontainer.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkviewport.h>
#include <gtk/gtkvseparator.h>

/* keep this code generic, don't include xchat.h! */

#include "tabs.h"
#undef TABS_SPREAD		/* left justify tabs instead */


/* ignore "toggled" signal? */
static int ignore_toggle = FALSE;


/* userdata for gobjects used here:
 *
 * group:
 *   "c" tab-focus callback function
 *   "foc" currently focused tab
 *   "i" inner hbox (inside the viewport)
 *
 * family boxes inside group
 *   "f" family
 *
 * tab (togglebuttons inside boxes):
 *   "u" userdata passed to tab-focus callback function (sess)
 *   "f" family
 *   "g" group this tab is apart of
 *
 */

/*
 * GtkViewports request at least as much space as their children do.
 * If we don't intervene here, the GtkViewport will be granted its
 * request, even at the expense of resizing the top-level window.
 */
static void
tab_viewport_size_request (GtkWidget *widget, GtkRequisition *requisition, gpointer user_data)
{
	requisition->width = 1;
	requisition->height = 1;
}

static gint
tab_search_offset (GtkWidget *inner, gint start_offset, gboolean forward)
{
	GList *boxes;
	GList *tabs;
	GtkWidget *box;
	GtkWidget *button;

	boxes = GTK_BOX (inner)->children;
	if (!forward && boxes)
		boxes = g_list_last (boxes);

	while (boxes)
	{
		box = ((GtkBoxChild *)boxes->data)->widget;
		boxes = (forward ? boxes->next : boxes->prev);

		tabs = GTK_BOX (box)->children;
		if (!forward && tabs)
			tabs = g_list_last (tabs);

		while (tabs)
		{
			button = ((GtkBoxChild *)tabs->data)->widget;
			tabs = (forward ? tabs->next : tabs->prev);

			if (!GTK_IS_TOGGLE_BUTTON (button))
				continue;

			if ((forward && button->allocation.x > start_offset) ||
			    (!forward && button->allocation.x < start_offset))
				return button->allocation.x;
		}
	}

	return 0;
}

static void
tab_scroll_left_clicked (GtkWidget *widget, GtkWidget *group)
{
	GtkAdjustment *adj;
	gint viewport_width;
	gfloat new_value;
	GtkWidget *inner;
	gfloat i;

	inner = g_object_get_data (G_OBJECT (group), "i");
	adj = gtk_viewport_get_hadjustment (GTK_VIEWPORT (inner->parent));
	gdk_window_get_geometry (inner->parent->window, 0, 0, &viewport_width, 0, 0);
	new_value = tab_search_offset (inner, adj->value, 0);

	if (new_value + viewport_width > adj->upper)
		new_value = adj->upper - viewport_width;

	for (i = adj->value; i > new_value; i -= 0.1)
	{
		gtk_adjustment_set_value (adj, i);
		while (g_main_pending ())
			g_main_iteration (TRUE);
	}

	gtk_adjustment_set_value (adj, new_value);
}

static void
tab_scroll_right_clicked (GtkWidget *widget, GtkWidget *group)
{
	GtkAdjustment *adj;
	gint viewport_width;
	gfloat new_value;
	GtkWidget *inner;
	gfloat i;

	inner = g_object_get_data (G_OBJECT (group), "i");
	adj = gtk_viewport_get_hadjustment (GTK_VIEWPORT (inner->parent));
	gdk_window_get_geometry (inner->parent->window, 0, 0, &viewport_width, 0, 0);
	new_value = tab_search_offset (inner, adj->value, 1);

	if (new_value == 0 || new_value + viewport_width > adj->upper)
		new_value = adj->upper - viewport_width;

	for (i = adj->value; i < new_value; i += 0.1)
	{
		gtk_adjustment_set_value (adj, i);
		while (g_main_pending ())
			g_main_iteration (TRUE);
	}

	gtk_adjustment_set_value (adj, new_value);
}

/* called when a tab is clicked (button down) */

static void
tab_pressed_cb (GtkToggleButton *tab, GtkWidget *group)
{
	void (*callback) (GtkWidget *tab, gpointer userdata, gpointer family);
	GtkWidget *old_tab;

	ignore_toggle = TRUE;
	gtk_toggle_button_set_active (tab, TRUE);

	/* de-activate the old tab */
	old_tab = g_object_get_data (G_OBJECT (group), "foc");
	if (old_tab)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (old_tab), FALSE);
	ignore_toggle = FALSE;

	if (tab->active)
	{
		callback = g_object_get_data (G_OBJECT (group), "c");
		callback (GTK_WIDGET (tab), g_object_get_data (G_OBJECT (tab), "u"),
					 g_object_get_data (G_OBJECT (tab), "f"));

		g_object_set_data (G_OBJECT (group), "foc", tab);
	}
}

GtkWidget *
tab_group_new (void *callback, gboolean vertical)
{
	GtkWidget *box;
	GtkWidget *viewport;
	GtkWidget *group;
	GtkWidget *button;

	group = gtk_hbox_new (0, 0);
	g_object_set_data (G_OBJECT (group), "c", callback);
	gtk_widget_show (group);

	viewport = gtk_viewport_new (0, 0);
	gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport), GTK_SHADOW_NONE);
	g_signal_connect (G_OBJECT (viewport), "size_request",
							G_CALLBACK (tab_viewport_size_request), 0);
	gtk_box_pack_start (GTK_BOX (group), viewport, 1, 1, 0);
	gtk_widget_show (viewport);

	box = gtk_hbox_new (FALSE, 0);
	g_object_set_data (G_OBJECT (group), "i", box);
	gtk_container_add (GTK_CONTAINER (viewport), box);
	gtk_widget_show (box);

	button = gtk_button_new_with_label (">");
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	g_signal_connect (G_OBJECT (button), "clicked",
							G_CALLBACK (tab_scroll_right_clicked), group);
	gtk_widget_show (button);
	gtk_box_pack_end (GTK_BOX (group), button, 0, 0, 0);

	button = gtk_button_new_with_label ("<");
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	g_signal_connect (G_OBJECT (button), "clicked",
							G_CALLBACK (tab_scroll_left_clicked), group);
	gtk_widget_show (button);
	gtk_box_pack_end (GTK_BOX (group), button, 0, 0, 0);

	return group;
}

/* traverse all the family boxes of tabs 
 *
 * A "group" is basically:
 * GtkHBox
 * `-GtkViewPort
 *   `-GtkHBox (inner box)
 *     `- GtkBox (family box)
 *        `- GtkToggleButton
 *        `- GtkToggleButton
 *        `- ...
 *     `- GtkBox
 *        `- GtkToggleButton
 *        `- GtkToggleButton
 *        `- ...
 *     `- ...
 *
 * */

static int
tab_group_for_each_tab (GtkWidget *group,
								int (*callback) (GtkWidget *tab, int num, int usernum),
								int usernum)
{
	GList *tabs;
	GList *boxes;
	GtkBoxChild *child;
	GtkBox *innerbox;
	int i;

	innerbox = g_object_get_data (G_OBJECT (group), "i");
	boxes = innerbox->children;
	i = 0;
	while (boxes)
	{
		child = boxes->data;
		tabs = GTK_BOX (child->widget)->children;

		while (tabs)
		{
			child = tabs->data;

			if (!GTK_IS_VSEPARATOR (child->widget))
			{
				if (callback (child->widget, i, usernum) != -1)
					return i;
				i++;
			}
			tabs = tabs->next;
		}

		boxes = boxes->next;
	}

	return i;
}

static int
dummy (GtkWidget *tab, int num, int usernum)
{
	return -1;
}

/* returns the number of tabs a tab-group has */

int
tab_group_get_size (GtkWidget *group)
{
	return tab_group_for_each_tab (group, dummy, 0);
}

static int
tab_check_focus_cb (GtkWidget *tab, int num, int unused)
{
	if (GTK_TOGGLE_BUTTON (tab)->active)
		return num;

	return -1;
}

/* returns the currently focused tab number */

static int
tab_group_get_cur_page (GtkWidget *group)
{
	return tab_group_for_each_tab (group, tab_check_focus_cb, 0);
}

static int
tab_focus_num_cb (GtkWidget *tab, int num, int want)
{
	if (num == want)
	{
		tab_focus (tab);
		return 1;
	}

	return -1;
}

/* switch to the tab number specified */

void
tab_group_switch (GtkWidget *group, int relative, int num)
{
	int i, max;

	if (relative)
	{
		max = tab_group_get_size (group);
		i = tab_group_get_cur_page (group) + num;
		/* make it wrap around at both ends */
		if (i < 0)
			i = max - 1;
		if (i >= max)
			i = 0;
		tab_group_for_each_tab (group, tab_focus_num_cb, i);
		return;
	}

	tab_group_for_each_tab (group, tab_focus_num_cb, num);
}

void
tab_focus (GtkWidget *tab)
{
	GtkWidget *focus_tab;
	GtkWidget *group;

	group = g_object_get_data (G_OBJECT (tab), "g");
	focus_tab = g_object_get_data (G_OBJECT (group), "foc");

	if (focus_tab == tab)
		return;

	/* focus the new one (tab_pressed_cb defocuses the old one) */
	tab_pressed_cb (GTK_TOGGLE_BUTTON (tab), group);
}

/* remove empty boxes and Vseparators */

void
tab_group_cleanup (GtkWidget *group)
{
	GList *boxes, *children;
	GtkWidget *box, *inner;
	GtkBoxChild *child;
	int empty;

	inner = g_object_get_data (G_OBJECT (group), "i");
	boxes = GTK_BOX (inner)->children;
	while (boxes)
	{
		child = boxes->data;
		box = child->widget;
		boxes = boxes->next;

#ifndef TABS_SPREAD
		/* check if the box is empty (except a vseperator) */
		empty = TRUE;
		children = GTK_BOX (box)->children;
		while (children)
		{
			if (!GTK_IS_VSEPARATOR (((GtkBoxChild *)children->data)->widget))
			{
				empty = FALSE;
				break;
			}
			children = children->next;
		}

		if (empty)
#else
		/* use this chance to destroy empty boxes */
		if (GTK_BOX (box)->children == NULL)
#endif
			gtk_widget_destroy (box);
	}
}

static void
tab_add_real (GtkWidget *group, GtkWidget *tab, void *family)
{
	GList *boxes, *children;
	GtkWidget *sep, *box, *inner;
	GtkBoxChild *child;
	int empty;

	inner = g_object_get_data (G_OBJECT (group), "i");
	/* see if a family for this tab already exists */
	boxes = GTK_BOX (inner)->children;
	while (boxes)
	{
		child = boxes->data;
		box = child->widget;

		if (g_object_get_data (G_OBJECT (box), "f") == family)
		{
			gtk_box_pack_start (GTK_BOX (box), tab, 0, 0, 0);
			gtk_widget_show (tab);
			return;
		}

		boxes = boxes->next;

#ifndef TABS_SPREAD
		/* check if the box is empty (except a vseperator) */
		empty = TRUE;
		children = GTK_BOX (box)->children;
		while (children)
		{
			if (!GTK_IS_VSEPARATOR (((GtkBoxChild *)children->data)->widget))
			{
				empty = FALSE;
				break;
			}
			children = children->next;
		}

		if (empty)
			gtk_widget_destroy (box);
#else
		/* use this chance to destroy empty boxes */
		if (GTK_BOX (box)->children == NULL)
			gtk_widget_destroy (box);
#endif
	}

	/* create a new family box */
	box = gtk_hbox_new (FALSE, 0);
#ifndef TABS_SPREAD
	sep = gtk_vseparator_new ();
	gtk_box_pack_end (GTK_BOX (box), sep, 0, 0, 4);
	gtk_widget_show (sep);
	gtk_box_pack_start (GTK_BOX (inner), box, 0, 0, 0);
#else
	gtk_container_add (GTK_CONTAINER (inner), box);
#endif
	g_object_set_data (G_OBJECT (box), "f", family);
	gtk_box_pack_start (GTK_BOX (box), tab, 0, 0, 0);
	gtk_widget_show (tab);
	gtk_widget_show (box);
}

static void
tab_release_cb (GtkToggleButton *widget, gpointer user_data)
{
	/* don't let all tabs be OFF at the same time */
	ignore_toggle = TRUE;
	gtk_toggle_button_set_active (widget, TRUE);
	ignore_toggle = FALSE;
}

static gboolean
tab_ignore_cb (GtkWidget *widget, GdkEventCrossing *event, gpointer user_data)
{
	return TRUE;
}

/* called for keyboard tab toggles only */
static void
tab_toggled_cb (GtkToggleButton *tab, gpointer user_data)
{
	if (ignore_toggle)
		return;

	if (tab->active)
	{
		/* activated a tab via keyboard */
		tab_pressed_cb (tab, g_object_get_data (G_OBJECT (tab), "g"));
		return;
	}

	/* activate it */
	tab_release_cb (tab, NULL);
}

GtkWidget *
tab_group_add (GtkWidget *group, char *name, void *family, void *userdata,
			void *click_cb)
{
	GtkWidget *but;

	but = gtk_toggle_button_new_with_label (name);
	/* used to trap right-clicks */
	g_signal_connect (G_OBJECT (but), "button-press-event",
						 	G_CALLBACK (click_cb), NULL);
	/* avoid prelights */
	g_signal_connect (G_OBJECT (but), "enter-notify-event",
						 	G_CALLBACK (tab_ignore_cb), NULL);
	g_signal_connect (G_OBJECT (but), "leave-notify-event",
						 	G_CALLBACK (tab_ignore_cb), NULL);
	g_signal_connect (G_OBJECT (but), "pressed",
							G_CALLBACK (tab_pressed_cb), group);
	g_signal_connect (G_OBJECT (but), "released",
						 	G_CALLBACK (tab_release_cb), NULL);
	/* for keyboard */
	g_signal_connect (G_OBJECT (but), "toggled",
						 	G_CALLBACK (tab_toggled_cb), NULL);

	g_object_set_data (G_OBJECT (but), "f", family);
	g_object_set_data (G_OBJECT (but), "g", group);
	g_object_set_data (G_OBJECT (but), "u", userdata);

	tab_add_real (group, but, family);

	return but;
}

void
tab_style (GtkWidget *tab, GtkStyle *style)
{
	gtk_widget_set_style (GTK_BIN (tab)->child, style);
}

void
tab_rename (GtkWidget *tab, char *new_name)
{
	gtk_button_set_label (GTK_BUTTON (tab), new_name);
}

void
tab_remove (GtkWidget *tab)
{
	GtkWidget *focus_tab;
	GtkWidget *group;

	group = g_object_get_data (G_OBJECT (tab), "g");
	gtk_widget_destroy (tab);

	focus_tab = g_object_get_data (G_OBJECT (group), "foc");
	if (focus_tab == tab)
	{
		g_object_set_data (G_OBJECT (group), "foc", NULL);
		tab_group_switch (group, 0, FALSE);
	}
}
