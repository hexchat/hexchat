/* HexChat
 * Copyright (C) 1998-2010 Peter Zelezny.
 * Copyright (C) 2009-2013 Berke Viktor.
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

#ifndef HEXCHAT_CUSTOM_LIST_H
#define HEXCHAT_CUSTOM_LIST_H

#include <gtk/gtk.h>

GType custom_list_get_type (void);

/* Some boilerplate GObject defines. 'klass' is used
 *   instead of 'class', because 'class' is a C++ keyword */

#define CUSTOM_TYPE_LIST            (custom_list_get_type ())
#define CUSTOM_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CUSTOM_TYPE_LIST, CustomList))
#define CUSTOM_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CUSTOM_TYPE_LIST, CustomListClass))
#define CUSTOM_IS_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CUSTOM_TYPE_LIST))
#define CUSTOM_IS_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CUSTOM_TYPE_LIST))
#define CUSTOM_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CUSTOM_TYPE_LIST, CustomListClass))

/* The data columns that we export via the tree model interface */

enum
{
	CUSTOM_LIST_COL_NAME,
	CUSTOM_LIST_COL_USERS,
	CUSTOM_LIST_COL_TOPIC,
	CUSTOM_LIST_N_COLUMNS
};

enum
{
	SORT_ID_CHANNEL,
	SORT_ID_USERS,
	SORT_ID_TOPIC
};

typedef struct
{
	char *topic;
	char *collation_key;
	guint32 pos;						  /* pos within the array */
	guint32 users;
	/* channel string lives beyond "users" */
#define GET_CHAN(row) (((char *)row)+sizeof(chanlistrow))
}
chanlistrow;

typedef struct _CustomList CustomList;
typedef struct _CustomListClass CustomListClass;



/* CustomList: this structure contains everything we need for our
 *             model implementation. You can add extra fields to
 *             this structure, e.g. hashtables to quickly lookup
 *             rows or whatever else you might need, but it is
 *             crucial that 'parent' is the first member of the
 *             structure.                                          */
struct _CustomList
{
	GObject parent;

	guint num_rows;     /* number of rows that we have used */
	guint num_alloc;    /* number of rows allocated */
	chanlistrow **rows; /* a dynamically allocated array of pointers to the
	                     * CustomRecord structure for each row */

	gint n_columns;
	GType column_types[CUSTOM_LIST_N_COLUMNS];

	gint sort_id;
	GtkSortType sort_order;
};


/* CustomListClass: more boilerplate GObject stuff */

struct _CustomListClass
{
	GObjectClass parent_class;
};


CustomList *custom_list_new (void);
void custom_list_append (CustomList *, chanlistrow *);
void custom_list_resort (CustomList *);
void custom_list_clear (CustomList *);

#endif /* HEXCHAT_CUSTOM_LIST_H */
