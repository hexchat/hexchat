/* abstract channel view: tabs or tree or anything you like */

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "chanview.h"
#include "gtkutil.h"


/* treeStore columns */

#define COL_NAME 0		/* (char *) */
#define COL_CHAN 1		/* (chan *) */
#define COL_ATTR 2		/* (PangoAttrList *) */
#define COL_PIXBUF 3		/* (GdkPixbuf *) */

struct _chanview
{
	/* impl scratch area */
	char implscratch[sizeof (void *) * 8];

	GtkTreeStore *store;
	int size;			/* number of channels in view */

	GtkWidget *box;	/* the box we destroy when changing implementations */
	GtkStyle *style;	/* style used for tree */
	chan *focused;		/* currently focused channel */
	int trunc_len;

	/* callbacks */
	void (*cb_focus) (chanview *, chan *, int tag, void *userdata);
	void (*cb_xbutton) (chanview *, chan *, int tag, void *userdata);
	gboolean (*cb_contextmenu) (chanview *, chan *, int tag, void *userdata, GdkEventButton *);
	int (*cb_compare) (void *a, void *b);

	/* impl */
	void (*func_init) (chanview *);
	void (*func_postinit) (chanview *);
	void *(*func_add) (chanview *, chan *, char *, GtkTreeIter *);
	void (*func_move_focus) (chanview *, gboolean, int);
	void (*func_change_orientation) (chanview *);
	void (*func_remove) (chan *);
	void (*func_move) (chan *, int delta);
	void (*func_move_family) (chan *, int delta);
	void (*func_focus) (chan *);
	void (*func_set_color) (chan *, PangoAttrList *);
	void (*func_rename) (chan *, char *);
	gboolean (*func_is_collapsed) (chan *);
	chan *(*func_get_parent) (chan *);
	void (*func_cleanup) (chanview *);

	unsigned int sorted:1;
	unsigned int vertical:1;
	unsigned int use_icons:1;
};

struct _chan
{
	chanview *cv;	/* our owner */
	GtkTreeIter iter;
	void *userdata;	/* session * */
	void *family;		/* server * or null */
	void *impl;	/* togglebutton or null */
	GdkPixbuf *icon;
	short allow_closure;	/* allow it to be closed when it still has children? */
	short tag;
};

static chan *cv_find_chan_by_number (chanview *cv, int num);
static int cv_find_number_of_chan (chanview *cv, chan *find_ch);


/* ======= TABS ======= */

#include "chanview-tabs.c"


/* ======= TREE ======= */

#include "chanview-tree.c"


/* ==== ABSTRACT CHANVIEW ==== */

static char *
truncate_tab_name (char *name, int max)
{
	char *buf;

	if (max > 2 && g_utf8_strlen (name, -1) > max)
	{
		/* truncate long channel names */
		buf = malloc (strlen (name) + 4);
		strcpy (buf, name);
		g_utf8_offset_to_pointer (buf, max)[0] = 0;
		strcat (buf, "..");
		return buf;
	}

	return name;
}

/* iterate through a model, into 1 depth of children */

static void
model_foreach_1 (GtkTreeModel *model, void (*func)(void *, GtkTreeIter *),
					  void *userdata)
{
	GtkTreeIter iter, inner;

	if (gtk_tree_model_get_iter_first (model, &iter))
	{
		do
		{
			func (userdata, &iter);
			if (gtk_tree_model_iter_children (model, &inner, &iter))
			{
				do
					func (userdata, &inner);
				while (gtk_tree_model_iter_next (model, &inner));
			}
		}
		while (gtk_tree_model_iter_next (model, &iter));
	}
}

static void
chanview_pop_cb (chanview *cv, GtkTreeIter *iter)
{
	chan *ch;
	char *name;
	PangoAttrList *attr;

	gtk_tree_model_get (GTK_TREE_MODEL (cv->store), iter,
							  COL_NAME, &name, COL_CHAN, &ch, COL_ATTR, &attr, -1);
	ch->impl = cv->func_add (cv, ch, name, NULL);
	if (attr)
	{
		cv->func_set_color (ch, attr);
		pango_attr_list_unref (attr);
	}
	g_free (name);
}

static void
chanview_populate (chanview *cv)
{
	model_foreach_1 (GTK_TREE_MODEL (cv->store), (void *)chanview_pop_cb, cv);
}

void
chanview_set_impl (chanview *cv, int type)
{
	/* cleanup the old one */
	if (cv->func_cleanup)
		cv->func_cleanup (cv);

	switch (type)
	{
	case 0:
		cv->func_init = cv_tabs_init;
		cv->func_postinit = cv_tabs_postinit;
		cv->func_add = cv_tabs_add;
		cv->func_move_focus = cv_tabs_move_focus;
		cv->func_change_orientation = cv_tabs_change_orientation;
		cv->func_remove = cv_tabs_remove;
		cv->func_move = cv_tabs_move;
		cv->func_move_family = cv_tabs_move_family;
		cv->func_focus = cv_tabs_focus;
		cv->func_set_color = cv_tabs_set_color;
		cv->func_rename = cv_tabs_rename;
		cv->func_is_collapsed = cv_tabs_is_collapsed;
		cv->func_get_parent = cv_tabs_get_parent;
		cv->func_cleanup = cv_tabs_cleanup;
		break;

	default:
		cv->func_init = cv_tree_init;
		cv->func_postinit = cv_tree_postinit;
		cv->func_add = cv_tree_add;
		cv->func_move_focus = cv_tree_move_focus;
		cv->func_change_orientation = cv_tree_change_orientation;
		cv->func_remove = cv_tree_remove;
		cv->func_move = cv_tree_move;
		cv->func_move_family = cv_tree_move_family;
		cv->func_focus = cv_tree_focus;
		cv->func_set_color = cv_tree_set_color;
		cv->func_rename = cv_tree_rename;
		cv->func_is_collapsed = cv_tree_is_collapsed;
		cv->func_get_parent = cv_tree_get_parent;
		cv->func_cleanup = cv_tree_cleanup;
		break;
	}

	/* now rebuild a new tabbar or tree */
	cv->func_init (cv);

	chanview_populate (cv);

	cv->func_postinit (cv);

	/* force re-focus */
	if (cv->focused)
		cv->func_focus (cv->focused);
}

static void
chanview_free_ch (chanview *cv, GtkTreeIter *iter)
{
	chan *ch;

	gtk_tree_model_get (GTK_TREE_MODEL (cv->store), iter, COL_CHAN, &ch, -1);
	free (ch);
}

static void
chanview_destroy_store (chanview *cv)	/* free every (chan *) in the store */
{
	model_foreach_1 (GTK_TREE_MODEL (cv->store), (void *)chanview_free_ch, cv);
	g_object_unref (cv->store);
}

static void
chanview_destroy (chanview *cv)
{
	if (cv->func_cleanup)
		cv->func_cleanup (cv);

	if (cv->box)
		gtk_widget_destroy (cv->box);

	chanview_destroy_store (cv);
	free (cv);
}

static void
chanview_box_destroy_cb (GtkWidget *box, chanview *cv)
{
	cv->box = NULL;
	chanview_destroy (cv);
}

chanview *
chanview_new (int type, int trunc_len, gboolean sort, gboolean use_icons,
				  GtkStyle *style)
{
	chanview *cv;

	cv = calloc (1, sizeof (chanview));
	cv->store = gtk_tree_store_new (4, G_TYPE_STRING, G_TYPE_POINTER,
											  PANGO_TYPE_ATTR_LIST, GDK_TYPE_PIXBUF);
	cv->style = style;
	cv->box = gtk_hbox_new (0, 0);
	cv->trunc_len = trunc_len;
	cv->sorted = sort;
	cv->use_icons = use_icons;
	gtk_widget_show (cv->box);
	chanview_set_impl (cv, type);

	g_signal_connect (G_OBJECT (cv->box), "destroy",
							G_CALLBACK (chanview_box_destroy_cb), cv);

	return cv;
}

/* too lazy for signals */

void
chanview_set_callbacks (chanview *cv,
	void (*cb_focus) (chanview *, chan *, int tag, void *userdata),
	void (*cb_xbutton) (chanview *, chan *, int tag, void *userdata),
	gboolean (*cb_contextmenu) (chanview *, chan *, int tag, void *userdata, GdkEventButton *),
	int (*cb_compare) (void *a, void *b))
{
	cv->cb_focus = cb_focus;
	cv->cb_xbutton = cb_xbutton;
	cv->cb_contextmenu = cb_contextmenu;
	cv->cb_compare = cb_compare;
}

/* find a place to insert this new entry, based on the compare function */

static void
chanview_insert_sorted (chanview *cv, GtkTreeIter *add_iter, GtkTreeIter *parent, void *ud)
{
	GtkTreeIter iter;
	chan *ch;

	if (cv->sorted && gtk_tree_model_iter_children (GTK_TREE_MODEL (cv->store), &iter, parent))
	{
		do
		{
			gtk_tree_model_get (GTK_TREE_MODEL (cv->store), &iter, COL_CHAN, &ch, -1);
			if (ch->tag == 0 && cv->cb_compare (ch->userdata, ud) > 0)
			{
				gtk_tree_store_insert_before (cv->store, add_iter, parent, &iter);
				return;
			}
		}
		while (gtk_tree_model_iter_next (GTK_TREE_MODEL (cv->store), &iter));
	}

	gtk_tree_store_append (cv->store, add_iter, parent);
}

/* find a parent node with the same "family" pointer (i.e. the Server tab) */

static int
chanview_find_parent (chanview *cv, void *family, GtkTreeIter *search_iter, chan *avoid)
{
	chan *search_ch;

	/* find this new row's parent, if any */
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (cv->store), search_iter))
	{
		do
		{
			gtk_tree_model_get (GTK_TREE_MODEL (cv->store), search_iter, 
									  COL_CHAN, &search_ch, -1);
			if (family == search_ch->family && search_ch != avoid /*&&
				 gtk_tree_store_iter_depth (cv->store, search_iter) == 0*/)
				return TRUE;
		}
		while (gtk_tree_model_iter_next (GTK_TREE_MODEL (cv->store), search_iter));
	}

	return FALSE;
}

static chan *
chanview_add_real (chanview *cv, char *name, void *family, void *userdata,
						 gboolean allow_closure, int tag, GdkPixbuf *icon,
						 chan *ch, chan *avoid)
{
	GtkTreeIter parent_iter;
	GtkTreeIter iter;
	gboolean has_parent = FALSE;

	if (chanview_find_parent (cv, family, &parent_iter, avoid))
	{
		chanview_insert_sorted (cv, &iter, &parent_iter, userdata);
		has_parent = TRUE;
	} else
	{
		gtk_tree_store_append (cv->store, &iter, NULL);
	}

	if (!ch)
	{
		ch = calloc (1, sizeof (chan));
		ch->userdata = userdata;
		ch->family = family;
		ch->cv = cv;
		ch->allow_closure = allow_closure;
		ch->tag = tag;
		ch->icon = icon;
	}
	memcpy (&(ch->iter), &iter, sizeof (iter));

	gtk_tree_store_set (cv->store, &iter, COL_NAME, name, COL_CHAN, ch,
							  COL_PIXBUF, icon, -1);

	cv->size++;
	if (!has_parent)
		ch->impl = cv->func_add (cv, ch, name, NULL);
	else
		ch->impl = cv->func_add (cv, ch, name, &parent_iter);

	return ch;
}

chan *
chanview_add (chanview *cv, char *name, void *family, void *userdata, gboolean allow_closure, int tag, GdkPixbuf *icon)
{
	char *new_name;
	chan *ret;

	new_name = truncate_tab_name (name, cv->trunc_len);

	ret = chanview_add_real (cv, new_name, family, userdata, allow_closure, tag, icon, NULL, NULL);

	if (new_name != name)
		free (new_name);

	return ret;
}

int
chanview_get_size (chanview *cv)
{
	return cv->size;
}

GtkWidget *
chanview_get_box (chanview *cv)
{
	return cv->box;
}

void
chanview_move_focus (chanview *cv, gboolean relative, int num)
{
	cv->func_move_focus (cv, relative, num);
}

GtkOrientation
chanview_get_orientation (chanview *cv)
{
	return (cv->vertical ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL);
}

void
chanview_set_orientation (chanview *cv, gboolean vertical)
{
	if (vertical != cv->vertical)
	{
		cv->vertical = vertical;
		cv->func_change_orientation (cv);
	}
}

int
chan_get_tag (chan *ch)
{
	return ch->tag;
}

void *
chan_get_userdata (chan *ch)
{
	return ch->userdata;
}

void
chan_focus (chan *ch)
{
	if (ch->cv->focused == ch)
		return;

	ch->cv->func_focus (ch);
}

void
chan_move (chan *ch, int delta)
{
	ch->cv->func_move (ch, delta);
}

void
chan_move_family (chan *ch, int delta)
{
	ch->cv->func_move_family (ch, delta);
}

void
chan_set_color (chan *ch, PangoAttrList *list)
{
	gtk_tree_store_set (ch->cv->store, &ch->iter, COL_ATTR, list, -1);	
	ch->cv->func_set_color (ch, list);
}

void
chan_rename (chan *ch, char *name, int trunc_len)
{
	char *new_name;

	new_name = truncate_tab_name (name, trunc_len);

	gtk_tree_store_set (ch->cv->store, &ch->iter, COL_NAME, new_name, -1);
	ch->cv->func_rename (ch, new_name);
	ch->cv->trunc_len = trunc_len;

	if (new_name != name)
		free (new_name);
}

/* this thing is overly complicated */

static int
cv_find_number_of_chan (chanview *cv, chan *find_ch)
{
	GtkTreeIter iter, inner;
	chan *ch;
	int i = 0;

	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (cv->store), &iter))
	{
		do
		{
			gtk_tree_model_get (GTK_TREE_MODEL (cv->store), &iter, COL_CHAN, &ch, -1);
			if (ch == find_ch)
				return i;
			i++;

			if (gtk_tree_model_iter_children (GTK_TREE_MODEL (cv->store), &inner, &iter))
			{
				do
				{
					gtk_tree_model_get (GTK_TREE_MODEL (cv->store), &inner, COL_CHAN, &ch, -1);
					if (ch == find_ch)
						return i;
					i++;
				}
				while (gtk_tree_model_iter_next (GTK_TREE_MODEL (cv->store), &inner));
			}
		}
		while (gtk_tree_model_iter_next (GTK_TREE_MODEL (cv->store), &iter));
	}

	return 0;	/* WARNING */
}

/* this thing is overly complicated too */

static chan *
cv_find_chan_by_number (chanview *cv, int num)
{
	GtkTreeIter iter, inner;
	chan *ch;
	int i = 0;

	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (cv->store), &iter))
	{
		do
		{
			if (i == num)
			{
				gtk_tree_model_get (GTK_TREE_MODEL (cv->store), &iter, COL_CHAN, &ch, -1);
				return ch;
			}
			i++;

			if (gtk_tree_model_iter_children (GTK_TREE_MODEL (cv->store), &inner, &iter))
			{
				do
				{
					if (i == num)
					{
						gtk_tree_model_get (GTK_TREE_MODEL (cv->store), &inner, COL_CHAN, &ch, -1);
						return ch;
					}
					i++;
				}
				while (gtk_tree_model_iter_next (GTK_TREE_MODEL (cv->store), &inner));
			}
		}
		while (gtk_tree_model_iter_next (GTK_TREE_MODEL (cv->store), &iter));
	}

	return NULL;
}

static void
chan_emancipate_children (chan *ch)
{
	char *name;
	chan *childch;
	GtkTreeIter childiter;
	PangoAttrList *attr;

	while (gtk_tree_model_iter_children (GTK_TREE_MODEL (ch->cv->store), &childiter, &ch->iter))
	{
		/* remove and re-add all the children, but avoid using "ch" as parent */
		gtk_tree_model_get (GTK_TREE_MODEL (ch->cv->store), &childiter,
								  COL_NAME, &name, COL_CHAN, &childch, COL_ATTR, &attr, -1);
		ch->cv->func_remove (childch);
		gtk_tree_store_remove (ch->cv->store, &childiter);
		ch->cv->size--;
		chanview_add_real (childch->cv, name, childch->family, childch->userdata, childch->allow_closure, childch->tag, childch->icon, childch, ch);
		if (attr)
		{
			childch->cv->func_set_color (childch, attr);
			pango_attr_list_unref (attr);
		}
		g_free (name);
	}
}

gboolean
chan_remove (chan *ch, gboolean force)
{
	chan *new_ch;
	int i, num;
	extern int xchat_is_quitting;

	if (xchat_is_quitting)	/* avoid lots of looping on exit */
		return TRUE;

	/* is this ch allowed to be closed while still having children? */
	if (!force &&
		 gtk_tree_model_iter_has_child (GTK_TREE_MODEL (ch->cv->store), &ch->iter) &&
		 !ch->allow_closure)
		return FALSE;

	chan_emancipate_children (ch);
	ch->cv->func_remove (ch);

	/* is it the focused one? */
	if (ch->cv->focused == ch)
	{
		ch->cv->focused = NULL;

		/* try to move the focus to some other valid channel */
		num = cv_find_number_of_chan (ch->cv, ch);
		/* move to the one left of the closing tab */
		new_ch = cv_find_chan_by_number (ch->cv, num - 1);
		if (new_ch && new_ch != ch)
		{
			chan_focus (new_ch);	/* this'll will set ch->cv->focused for us too */
		} else
		{
			/* if it fails, try focus from tab 0 and up */
			for (i = 0; i < ch->cv->size; i++)
			{
				new_ch = cv_find_chan_by_number (ch->cv, i);
				if (new_ch && new_ch != ch)
				{
					chan_focus (new_ch);	/* this'll will set ch->cv->focused for us too */
					break;
				}
			}
		}
	}

	ch->cv->size--;
	gtk_tree_store_remove (ch->cv->store, &ch->iter);
	free (ch);
	return TRUE;
}

gboolean
chan_is_collapsed (chan *ch)
{
	return ch->cv->func_is_collapsed (ch);
}

chan *
chan_get_parent (chan *ch)
{
	return ch->cv->func_get_parent (ch);
}
