#include <string.h>
#include <stdlib.h>
#include "custom-list.h"

/* indent -i3 -ci3 -ut -ts3 -bli0 -c0 custom-list.c */

/* boring declarations of local functions */

static void custom_list_init (CustomList * pkg_tree);

static void custom_list_class_init (CustomListClass * klass);

static void custom_list_tree_model_init (GtkTreeModelIface * iface);

static void custom_list_finalize (GObject * object);

static GtkTreeModelFlags custom_list_get_flags (GtkTreeModel * tree_model);

static gint custom_list_get_n_columns (GtkTreeModel * tree_model);

static GType custom_list_get_column_type (GtkTreeModel * tree_model,
														gint index);

static gboolean custom_list_get_iter (GtkTreeModel * tree_model,
												  GtkTreeIter * iter, GtkTreePath * path);

static GtkTreePath *custom_list_get_path (GtkTreeModel * tree_model,
														GtkTreeIter * iter);

static void custom_list_get_value (GtkTreeModel * tree_model,
											  GtkTreeIter * iter,
											  gint column, GValue * value);

static gboolean custom_list_iter_next (GtkTreeModel * tree_model,
													GtkTreeIter * iter);

static gboolean custom_list_iter_children (GtkTreeModel * tree_model,
														 GtkTreeIter * iter,
														 GtkTreeIter * parent);

static gboolean custom_list_iter_has_child (GtkTreeModel * tree_model,
														  GtkTreeIter * iter);

static gint custom_list_iter_n_children (GtkTreeModel * tree_model,
													  GtkTreeIter * iter);

static gboolean custom_list_iter_nth_child (GtkTreeModel * tree_model,
														  GtkTreeIter * iter,
														  GtkTreeIter * parent, gint n);

static gboolean custom_list_iter_parent (GtkTreeModel * tree_model,
													  GtkTreeIter * iter,
													  GtkTreeIter * child);

  /* -- GtkTreeSortable interface functions -- */

static gboolean custom_list_sortable_get_sort_column_id (GtkTreeSortable *
																			sortable,
																			gint * sort_col_id,
																			GtkSortType * order);

static void custom_list_sortable_set_sort_column_id (GtkTreeSortable *
																	  sortable,
																	  gint sort_col_id,
																	  GtkSortType order);

static void custom_list_sortable_set_sort_func (GtkTreeSortable * sortable,
																gint sort_col_id,
																GtkTreeIterCompareFunc
																sort_func, gpointer user_data,
																GtkDestroyNotify
																destroy_func);

static void custom_list_sortable_set_default_sort_func (GtkTreeSortable *
																		  sortable,
																		  GtkTreeIterCompareFunc
																		  sort_func,
																		  gpointer user_data,
																		  GtkDestroyNotify
																		  destroy_func);

static gboolean custom_list_sortable_has_default_sort_func (GtkTreeSortable *
																				sortable);



static GObjectClass *parent_class = NULL;	/* GObject stuff - nothing to worry about */


static void
custom_list_sortable_init (GtkTreeSortableIface * iface)
{
	iface->get_sort_column_id = custom_list_sortable_get_sort_column_id;
	iface->set_sort_column_id = custom_list_sortable_set_sort_column_id;
	iface->set_sort_func = custom_list_sortable_set_sort_func;	/* NOT SUPPORTED */
	iface->set_default_sort_func = custom_list_sortable_set_default_sort_func;	/* NOT SUPPORTED */
	iface->has_default_sort_func = custom_list_sortable_has_default_sort_func;	/* NOT SUPPORTED */
}

/*****************************************************************************
 *
 *  custom_list_get_type: here we register our new type and its interfaces
 *                        with the type system. If you want to implement
 *                        additional interfaces like GtkTreeSortable, you
 *                        will need to do it here.
 *
 *****************************************************************************/

static GType
custom_list_get_type (void)
{
	static GType custom_list_type = 0;

	if (custom_list_type)
		return custom_list_type;

	/* Some boilerplate type registration stuff */
	if (1)
	{
		static const GTypeInfo custom_list_info = {
			sizeof (CustomListClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) custom_list_class_init,
			NULL,	/* class finalize */
			NULL,	/* class_data */
			sizeof (CustomList),
			0,	/* n_preallocs */
			(GInstanceInitFunc) custom_list_init
		};

		custom_list_type =
			g_type_register_static (G_TYPE_OBJECT, "CustomList",
											&custom_list_info, (GTypeFlags) 0);
	}

	/* Here we register our GtkTreeModel interface with the type system */
	if (1)
	{
		static const GInterfaceInfo tree_model_info = {
			(GInterfaceInitFunc) custom_list_tree_model_init,
			NULL,
			NULL
		};

		g_type_add_interface_static (custom_list_type, GTK_TYPE_TREE_MODEL,
											  &tree_model_info);
	}

	/* Add GtkTreeSortable interface */
	if (1)
	{
		static const GInterfaceInfo tree_sortable_info = {
			(GInterfaceInitFunc) custom_list_sortable_init,
			NULL,
			NULL
		};

		g_type_add_interface_static (custom_list_type,
											  GTK_TYPE_TREE_SORTABLE,
											  &tree_sortable_info);
	}

	return custom_list_type;
}

/*****************************************************************************
 *
 *  custom_list_class_init: more boilerplate GObject/GType stuff.
 *                          Init callback for the type system,
 *                          called once when our new class is created.
 *
 *****************************************************************************/

static void
custom_list_class_init (CustomListClass * klass)
{
	GObjectClass *object_class;

	parent_class = (GObjectClass *) g_type_class_peek_parent (klass);
	object_class = (GObjectClass *) klass;

	object_class->finalize = custom_list_finalize;
}

/*****************************************************************************
 *
 *  custom_list_tree_model_init: init callback for the interface registration
 *                               in custom_list_get_type. Here we override
 *                               the GtkTreeModel interface functions that
 *                               we implement.
 *
 *****************************************************************************/

static void
custom_list_tree_model_init (GtkTreeModelIface * iface)
{
	iface->get_flags = custom_list_get_flags;
	iface->get_n_columns = custom_list_get_n_columns;
	iface->get_column_type = custom_list_get_column_type;
	iface->get_iter = custom_list_get_iter;
	iface->get_path = custom_list_get_path;
	iface->get_value = custom_list_get_value;
	iface->iter_next = custom_list_iter_next;
	iface->iter_children = custom_list_iter_children;
	iface->iter_has_child = custom_list_iter_has_child;
	iface->iter_n_children = custom_list_iter_n_children;
	iface->iter_nth_child = custom_list_iter_nth_child;
	iface->iter_parent = custom_list_iter_parent;
}


/*****************************************************************************
 *
 *  custom_list_init: this is called everytime a new custom list object
 *                    instance is created (we do that in custom_list_new).
 *                    Initialise the list structure's fields here.
 *
 *****************************************************************************/

static void
custom_list_init (CustomList * custom_list)
{
	custom_list->n_columns = CUSTOM_LIST_N_COLUMNS;

	custom_list->column_types[0] = G_TYPE_STRING;	/* CUSTOM_LIST_COL_NAME      */
	custom_list->column_types[1] = G_TYPE_UINT;	/* CUSTOM_LIST_COL_USERS     */
	custom_list->column_types[2] = G_TYPE_STRING;	/* CUSTOM_LIST_COL_TOPIC     */

	custom_list->num_rows = 0;
	custom_list->num_alloc = 0;
	custom_list->rows = NULL;

	custom_list->sort_id = SORT_ID_CHANNEL;
	custom_list->sort_order = GTK_SORT_ASCENDING;
}


/*****************************************************************************
 *
 *  custom_list_finalize: this is called just before a custom list is
 *                        destroyed. Free dynamically allocated memory here.
 *
 *****************************************************************************/

static void
custom_list_finalize (GObject * object)
{
	custom_list_clear (CUSTOM_LIST (object));

	/* must chain up - finalize parent */
	(*parent_class->finalize) (object);
}


/*****************************************************************************
 *
 *  custom_list_get_flags: tells the rest of the world whether our tree model
 *                         has any special characteristics. In our case,
 *                         we have a list model (instead of a tree), and each
 *                         tree iter is valid as long as the row in question
 *                         exists, as it only contains a pointer to our struct.
 *
 *****************************************************************************/

static GtkTreeModelFlags
custom_list_get_flags (GtkTreeModel * tree_model)
{
	return (GTK_TREE_MODEL_LIST_ONLY /*| GTK_TREE_MODEL_ITERS_PERSIST */ );
}


/*****************************************************************************
 *
 *  custom_list_get_n_columns: tells the rest of the world how many data
 *                             columns we export via the tree model interface
 *
 *****************************************************************************/

static gint
custom_list_get_n_columns (GtkTreeModel * tree_model)
{
	return 3;/*CUSTOM_LIST (tree_model)->n_columns;*/
}


/*****************************************************************************
 *
 *  custom_list_get_column_type: tells the rest of the world which type of
 *                               data an exported model column contains
 *
 *****************************************************************************/

static GType
custom_list_get_column_type (GtkTreeModel * tree_model, gint index)
{
	return CUSTOM_LIST (tree_model)->column_types[index];
}


/*****************************************************************************
 *
 *  custom_list_get_iter: converts a tree path (physical position) into a
 *                        tree iter structure (the content of the iter
 *                        fields will only be used internally by our model).
 *                        We simply store a pointer to our chanlistrow
 *                        structure that represents that row in the tree iter.
 *
 *****************************************************************************/

static gboolean
custom_list_get_iter (GtkTreeModel * tree_model,
							 GtkTreeIter * iter, GtkTreePath * path)
{
	CustomList *custom_list = CUSTOM_LIST (tree_model);
	chanlistrow *record;
	gint n;

	n = gtk_tree_path_get_indices (path)[0];
	if (n >= custom_list->num_rows || n < 0)
		return FALSE;

	record = custom_list->rows[n];

	/* We simply store a pointer to our custom record in the iter */
	iter->user_data = record;

	return TRUE;
}


/*****************************************************************************
 *
 *  custom_list_get_path: converts a tree iter into a tree path (ie. the
 *                        physical position of that row in the list).
 *
 *****************************************************************************/

static GtkTreePath *
custom_list_get_path (GtkTreeModel * tree_model, GtkTreeIter * iter)
{
	GtkTreePath *path;
	chanlistrow *record;

	record = (chanlistrow *) iter->user_data;

	path = gtk_tree_path_new ();
	gtk_tree_path_append_index (path, record->pos);

	return path;
}


/*****************************************************************************
 *
 *  custom_list_get_value: Returns a row's exported data columns
 *                         (_get_value is what gtk_tree_model_get uses)
 *
 *****************************************************************************/

static void
custom_list_get_value (GtkTreeModel * tree_model,
							  GtkTreeIter * iter, gint column, GValue * value)
{
	chanlistrow *record;
	CustomList *custom_list = CUSTOM_LIST (tree_model);

	if (custom_list->num_rows == 0)
		return;

	g_value_init (value, custom_list->column_types[column]);

	record = (chanlistrow *) iter->user_data;

	switch (column)
	{
	case CUSTOM_LIST_COL_NAME:
		g_value_set_static_string (value, GET_CHAN (record));
		break;

	case CUSTOM_LIST_COL_USERS:
		g_value_set_uint (value, record->users);
		break;

	case CUSTOM_LIST_COL_TOPIC:
		g_value_set_static_string (value, record->topic);
		break;
	}
}


/*****************************************************************************
 *
 *  custom_list_iter_next: Takes an iter structure and sets it to point
 *                         to the next row.
 *
 *****************************************************************************/

static gboolean
custom_list_iter_next (GtkTreeModel * tree_model, GtkTreeIter * iter)
{
	chanlistrow *record, *nextrecord;
	CustomList *custom_list = CUSTOM_LIST (tree_model);

	record = (chanlistrow *) iter->user_data;

	/* Is this the last record in the list? */
	if ((record->pos + 1) >= custom_list->num_rows)
		return FALSE;

	nextrecord = custom_list->rows[(record->pos + 1)];

	g_assert (nextrecord != NULL);
	g_assert (nextrecord->pos == (record->pos + 1));

	iter->user_data = nextrecord;

	return TRUE;
}


/*****************************************************************************
 *
 *  custom_list_iter_children: Returns TRUE or FALSE depending on whether
 *                             the row specified by 'parent' has any children.
 *                             If it has children, then 'iter' is set to
 *                             point to the first child. Special case: if
 *                             'parent' is NULL, then the first top-level
 *                             row should be returned if it exists.
 *
 *****************************************************************************/

static gboolean
custom_list_iter_children (GtkTreeModel * tree_model,
									GtkTreeIter * iter, GtkTreeIter * parent)
{
	CustomList *custom_list = CUSTOM_LIST (tree_model);

	/* this is a list, nodes have no children */
	if (parent)
		return FALSE;

	/* parent == NULL is a special case; we need to return the first top-level row */
	/* No rows => no first row */
	if (custom_list->num_rows == 0)
		return FALSE;

	/* Set iter to first item in list */
	iter->user_data = custom_list->rows[0];

	return TRUE;
}


/*****************************************************************************
 *
 *  custom_list_iter_has_child: Returns TRUE or FALSE depending on whether
 *                              the row specified by 'iter' has any children.
 *                              We only have a list and thus no children.
 *
 *****************************************************************************/

static gboolean
custom_list_iter_has_child (GtkTreeModel * tree_model, GtkTreeIter * iter)
{
	return FALSE;
}


/*****************************************************************************
 *
 *  custom_list_iter_n_children: Returns the number of children the row
 *                               specified by 'iter' has. This is usually 0,
 *                               as we only have a list and thus do not have
 *                               any children to any rows. A special case is
 *                               when 'iter' is NULL, in which case we need
 *                               to return the number of top-level nodes,
 *                               ie. the number of rows in our list.
 *
 *****************************************************************************/

static gint
custom_list_iter_n_children (GtkTreeModel * tree_model, GtkTreeIter * iter)
{
	CustomList *custom_list = CUSTOM_LIST (tree_model);

	/* special case: if iter == NULL, return number of top-level rows */
	if (!iter)
		return custom_list->num_rows;

	return 0;	/* otherwise, this is easy again for a list */
}


/*****************************************************************************
 *
 *  custom_list_iter_nth_child: If the row specified by 'parent' has any
 *                              children, set 'iter' to the n-th child and
 *                              return TRUE if it exists, otherwise FALSE.
 *                              A special case is when 'parent' is NULL, in
 *                              which case we need to set 'iter' to the n-th
 *                              row if it exists.
 *
 *****************************************************************************/

static gboolean
custom_list_iter_nth_child (GtkTreeModel * tree_model,
									 GtkTreeIter * iter, GtkTreeIter * parent, gint n)
{
	CustomList *custom_list = CUSTOM_LIST (tree_model);

	/* a list has only top-level rows */
	if (parent)
		return FALSE;

	/* special case: if parent == NULL, set iter to n-th top-level row */
	if (n >= custom_list->num_rows)
		return FALSE;

	iter->user_data = custom_list->rows[n];
	return TRUE;
}


/*****************************************************************************
 *
 *  custom_list_iter_parent: Point 'iter' to the parent node of 'child'. As
 *                           we have a list and thus no children and no
 *                           parents of children, we can just return FALSE.
 *
 *****************************************************************************/

static gboolean
custom_list_iter_parent (GtkTreeModel * tree_model,
								 GtkTreeIter * iter, GtkTreeIter * child)
{
	return FALSE;
}

static gboolean
custom_list_sortable_get_sort_column_id (GtkTreeSortable * sortable,
													  gint * sort_col_id,
													  GtkSortType * order)
{
	CustomList *custom_list = CUSTOM_LIST (sortable);

	if (sort_col_id)
		*sort_col_id = custom_list->sort_id;

	if (order)
		*order = custom_list->sort_order;

	return TRUE;
}


static void
custom_list_sortable_set_sort_column_id (GtkTreeSortable * sortable,
													  gint sort_col_id, GtkSortType order)
{
	CustomList *custom_list = CUSTOM_LIST (sortable);

	if (custom_list->sort_id == sort_col_id
		 && custom_list->sort_order == order)
		return;

	custom_list->sort_id = sort_col_id;
	custom_list->sort_order = order;

	custom_list_resort (custom_list);

	/* emit "sort-column-changed" signal to tell any tree views
	 *  that the sort column has changed (so the little arrow
	 *  in the column header of the sort column is drawn
	 *  in the right column)                                     */

	gtk_tree_sortable_sort_column_changed (sortable);
}

static void
custom_list_sortable_set_sort_func (GtkTreeSortable * sortable,
												gint sort_col_id,
												GtkTreeIterCompareFunc sort_func,
												gpointer user_data,
												GtkDestroyNotify destroy_func)
{
}

static void
custom_list_sortable_set_default_sort_func (GtkTreeSortable * sortable,
														  GtkTreeIterCompareFunc sort_func,
														  gpointer user_data,
														  GtkDestroyNotify destroy_func)
{
}

static gboolean
custom_list_sortable_has_default_sort_func (GtkTreeSortable * sortable)
{
	return FALSE;
}

/* fast as possible compare func for sorting.
   TODO: If fast enough, use a unicode collation key and strcmp. */

#define TOSML(c) (((c) >= 'A' && (c) <= 'Z') ? (c) - 'A' + 'a' : (c))

static inline int
fast_ascii_stricmp (const char *s1, const char *s2)
{
	int c1, c2;

	while (*s1 && *s2)
	{
		c1 = (int) (unsigned char) TOSML (*s1);
		c2 = (int) (unsigned char) TOSML (*s2);
		if (c1 != c2)
			return (c1 - c2);
		s1++;
		s2++;
	}

	return (((int) (unsigned char) *s1) - ((int) (unsigned char) *s2));
}

static gint
custom_list_qsort_compare_func (chanlistrow ** a, chanlistrow ** b,
										  CustomList * custom_list)
{
	if (custom_list->sort_order == GTK_SORT_DESCENDING)
	{
		chanlistrow **tmp = a;
		a = b;
		b = tmp;
	}

	if (custom_list->sort_id == SORT_ID_USERS)
	{
		return (*a)->users - (*b)->users;
	}

	if (custom_list->sort_id == SORT_ID_TOPIC)
	{
		return fast_ascii_stricmp ((*a)->topic, (*b)->topic);
	}

	return strcmp ((*a)->collation_key, (*b)->collation_key);
}

/*****************************************************************************
 *
 *  custom_list_new:  This is what you use in your own code to create a
 *                    new custom list tree model for you to use.
 *
 *****************************************************************************/

CustomList *
custom_list_new (void)
{
	return (CustomList *) g_object_new (CUSTOM_TYPE_LIST, NULL);
}

void
custom_list_append (CustomList * custom_list, chanlistrow * newrecord)
{
	GtkTreeIter iter;
	GtkTreePath *path;
	gulong newsize;
	guint pos;

	if (custom_list->num_rows >= custom_list->num_alloc)
	{
		custom_list->num_alloc += 64;
		newsize = custom_list->num_alloc * sizeof (chanlistrow *);
		custom_list->rows = g_realloc (custom_list->rows, newsize);
	}

	/* TODO: Binary search insert? */

	pos = custom_list->num_rows;
	custom_list->rows[pos] = newrecord;
	custom_list->num_rows++;
	newrecord->pos = pos;

	/* inform the tree view and other interested objects
	 *  (e.g. tree row references) that we have inserted
	 *  a new row, and where it was inserted */

	path = gtk_tree_path_new ();
	gtk_tree_path_append_index (path, newrecord->pos);
/*  custom_list_get_iter(GTK_TREE_MODEL(custom_list), &iter, path);*/
	iter.user_data = newrecord;
	gtk_tree_model_row_inserted (GTK_TREE_MODEL (custom_list), path, &iter);
	gtk_tree_path_free (path);
}

void
custom_list_resort (CustomList * custom_list)
{
	GtkTreePath *path;
	gint *neworder, i;

	if (custom_list->num_rows < 2)
		return;

	/* resort */
	g_qsort_with_data (custom_list->rows,
							 custom_list->num_rows,
							 sizeof (chanlistrow *),
							 (GCompareDataFunc) custom_list_qsort_compare_func,
							 custom_list);

	/* let other objects know about the new order */
	neworder = malloc (sizeof (gint) * custom_list->num_rows);

	for (i = custom_list->num_rows - 1; i >= 0; i--)
	{
		/* Note that the API reference might be wrong about
		 * this, see bug number 124790 on bugs.gnome.org.
		 * Both will work, but one will give you 'jumpy'
		 * selections after row reordering. */
		/* neworder[(custom_list->rows[i])->pos] = i; */
		neworder[i] = (custom_list->rows[i])->pos;
		(custom_list->rows[i])->pos = i;
	}

	path = gtk_tree_path_new ();
	gtk_tree_model_rows_reordered (GTK_TREE_MODEL (custom_list), path, NULL,
											 neworder);
	gtk_tree_path_free (path);
	free (neworder);
}

void
custom_list_clear (CustomList * custom_list)
{
	int i, max = custom_list->num_rows - 1;
	GtkTreePath *path;

	for (i = max; i >= 0; i--)
	{
		path = gtk_tree_path_new ();
		gtk_tree_path_append_index (path, custom_list->rows[i]->pos);
		gtk_tree_model_row_deleted (GTK_TREE_MODEL (custom_list), path);
		gtk_tree_path_free (path);
	}

	custom_list->num_rows = 0;
	custom_list->num_alloc = 0;

	g_free (custom_list->rows);
	custom_list->rows = NULL;
}
