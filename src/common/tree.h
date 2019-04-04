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

#ifndef HEXCHAT_TREE_H
#define HEXCHAT_TREE_H

#include <glib.h>

typedef struct _tree tree;

typedef int (tree_cmp_func) (const void *keya, const void *keyb, void *data);
typedef int (tree_traverse_func) (const void *key, void *data);

tree *tree_new (tree_cmp_func *cmp, void *data);
void tree_destroy (tree *t);
void *tree_find (tree *t, const void *key, tree_cmp_func *cmp, void *data, int *pos);
int tree_remove (tree *t, void *key, int *pos);
void *tree_remove_at_pos (tree *t, int pos);
void tree_foreach (tree *t, tree_traverse_func *func, void *data);
int tree_insert (tree *t, void *key);
void tree_append (tree* t, void *key);
int tree_size (tree *t);

#endif
