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

struct _chanview
{
	/* impl scratch area */
	char implscratch[sizeof (void *) * 8];

	GtkTreeStore *store;
	int size;			/* number of channels in view */

	GtkWidget *box;	/* the box we destroy when changing implementations */
	chan *focused;		/* currently focused channel */
	int trunc_len;

	/* callbacks */
	void (*cb_focus) (chanview *, chan *, void *family, void *userdata);
	void (*cb_xbutton) (chanview *, chan *, void *family, void *userdata);
	gboolean (*cb_contextmenu) (chanview *, chan *, void *family, void *userdata, GdkEventButton *);
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
	void (*func_cleanup) (chanview *);

	unsigned int sorted:1;
	unsigned int vertical:1;
};

struct _chan
{
	chanview *cv;	/* our owner */
	GtkTreeIter iter;
	void *userdata;	/* session * */
	void *family;		/* server * or null */
	void *impl;	/* togglebutton or null */
};


/* ======= TABS ======= */

#include "chanview-tabs.c"


/* ======= TREE ======= */

#include "chanview-tree.c"


/* ==== ABSTRACT CHANVIEW ==== */


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

	gtk_tree_model_get (GTK_TREE_MODEL (cv->store), iter,
							  COL_NAME, &name, COL_CHAN, &ch, -1);
	ch->impl = cv->func_add (cv, ch, name, NULL);
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
	printf("chanview_set_impl(%d : %s)\n", type, type ? "TREE" : "TABS");

	/* cleanup the old one */
	if (cv->func_cleanup)
		cv->func_cleanup (cv);

	printf("chanview_set_impl cleanup done\n");

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
		cv->func_cleanup = cv_tree_cleanup;
		break;
	}

	/* now rebuild a new tabbar or tree */
	cv->func_init (cv);
	printf("chanview_set_impl init done\n");

	chanview_populate (cv);
	printf("chanview_set_impl populate done\n");

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

	printf("destroy entire chanview %p\n", cv);
}

static void
chanview_box_destroy_cb (GtkWidget *box, chanview *cv)
{
	cv->box = NULL;
	chanview_destroy (cv);
}

chanview *
chanview_new (int type, int trunc_len, gboolean sort)
{
	chanview *cv;

	cv = calloc (1, sizeof (chanview));
	cv->store = gtk_tree_store_new (3, G_TYPE_STRING, G_TYPE_POINTER, PANGO_TYPE_ATTR_LIST);
	cv->box = gtk_hbox_new (0, 0);
	cv->trunc_len = trunc_len;
	cv->sorted = sort;
	gtk_widget_show (cv->box);
	chanview_set_impl (cv, type);

	g_signal_connect (G_OBJECT (cv->box), "destroy",
							G_CALLBACK (chanview_box_destroy_cb), cv);

	return cv;
}

/* too lazy for signals */

void
chanview_set_callbacks (chanview *cv,
	void (*cb_focus) (chanview *, chan *, void *family, void *userdata),
	void (*cb_xbutton) (chanview *, chan *, void *family, void *userdata),
	gboolean (*cb_contextmenu) (chanview *, chan *, void *family, void *userdata, GdkEventButton *),
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
			if (ch->family && cv->cb_compare (ch->userdata, ud) > 0)
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
chanview_find_parent (chanview *cv, void *family, GtkTreeIter *search_iter)
{
	chan *search_ch;

	/* find this new row's parent, if any */
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (cv->store), search_iter))
	{
		do
		{
			gtk_tree_model_get (GTK_TREE_MODEL (cv->store), search_iter, 
									  COL_CHAN, &search_ch, -1);
			if (family == search_ch->family /*&&
				 gtk_tree_store_iter_depth (cv->store, search_iter) == 0*/)
				return TRUE;
		}
		while (gtk_tree_model_iter_next (GTK_TREE_MODEL (cv->store), search_iter));
	}

	return FALSE;
}

chan *
chanview_add (chanview *cv, char *name, void *family, void *userdata)
{
	GtkTreeIter parent_iter;
	GtkTreeIter iter;
	chan *ch;
	gboolean has_parent = FALSE;

	if (chanview_find_parent (cv, family, &parent_iter))
	{
		chanview_insert_sorted (cv, &iter, &parent_iter, userdata);
		has_parent = TRUE;
	} else
	{
		gtk_tree_store_append (cv->store, &iter, NULL);
	}

	ch = calloc (1, sizeof (chan));
	ch->userdata = userdata;
	ch->family = family;
	ch->cv = cv;
	memcpy (&(ch->iter), &iter, sizeof (iter));

	gtk_tree_store_set (cv->store, &iter, COL_NAME, name, COL_CHAN, ch, -1);

	cv->size++;
	if (!has_parent)
		ch->impl = cv->func_add (cv, ch, name, NULL);
	else
		ch->impl = cv->func_add (cv, ch, name, &parent_iter);

printf("chanview_add(): new impl %p (ch=%p)\n", ch->impl, ch);

	return ch;
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

chan *
chanview_get_focused (chanview *cv)
{
	return cv->focused;
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

void *
chan_get_family (chan *ch)
{
	return ch->family;
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
chan_rename (chan *ch, char *new_name, int trunc_len)
{
	gtk_tree_store_set (ch->cv->store, &ch->iter, COL_NAME, new_name, -1);
	ch->cv->trunc_len = trunc_len;
	ch->cv->func_rename (ch, new_name);
}

gboolean
chan_remove (chan *ch)
{
	GtkTreeIter iter;
	chan *new_ch;

	printf("remove ch=%p (focused ch=%p)\n", ch, ch->cv->focused); 

	if (gtk_tree_model_iter_has_child (GTK_TREE_MODEL (ch->cv->store), &ch->iter))
		return FALSE;

	ch->cv->func_remove (ch);

	/* is it the focused one? */
	if (ch->cv->focused == ch)
	{
		ch->cv->focused = NULL;

		/* try to move the focus to some other valid channel */
		memcpy (&iter, &ch->iter, sizeof (iter));

		/* FIXME: support depth 1 */
		if (!gtk_tree_model_iter_next (GTK_TREE_MODEL (ch->cv->store), &iter))
			if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (ch->cv->store), &iter))
				goto errout;

		gtk_tree_model_get (GTK_TREE_MODEL (ch->cv->store), &iter, COL_CHAN, &new_ch, -1);
		printf("moving focus to ch=%p\n", new_ch);
		chan_focus (new_ch);	/* this'll will set ch->cv->focused for us too */
	}

errout:
	ch->cv->size--;
	gtk_tree_store_remove (ch->cv->store, &ch->iter);
	free (ch);
	return TRUE;
}
