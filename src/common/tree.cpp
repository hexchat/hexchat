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

/*
This is used for quick userlist insertion and lookup. It's not really
a tree, but it could be :)
*/

#include <new>
#include <vector>
#include <algorithm>


#include "tree.h"

static const int ARRAY_GROW = 32;

struct _tree
{
	int elements;
	std::vector<void*> data_array;
	tree_cmp_func *cmp;
	void *data;

	_tree(tree_cmp_func cmp, void* data)
		:cmp(cmp), data(data), elements(0)
	{
	}
	void grow(){
		if (this->data_array.size() < this->elements + 1)
		{
			int new_size = this->data_array.size() + ARRAY_GROW;
			this->data_array.resize(new_size);
		}
	}
	void insert_at_pos(void *key, int pos)
	{
		this->data_array.insert(this->data_array.cbegin() + pos, key);
		this->elements++;
	}

	int find_insertion_pos(void *key, int *done)
	{
		int c, u, l, idx;

		if (this->elements < 1)
		{
			*done = 1;
			this->data_array[0] = key;
			this->elements++;
			return 0;
		}

		if (this->elements < 2)
		{
			*done = 1;
			c = this->cmp(key, this->data_array[0], this->data);
			if (c == 0)
				return -1;
			this->elements++;
			if (c > 0)
			{
				this->data_array[1] = key;
				return 1;
			}
			this->data_array[1] = this->data_array[0];
			this->data_array[0] = key;
			return 0;
		}

		*done = 0;

		c = this->cmp(key, this->data_array[0], this->data);
		if (c < 0)
			return 0;	/* prepend */

		c = this->cmp(key, this->data_array[this->elements - 1], this->data);
		if (c > 0)
			return this->elements;	/* append */

		l = 0;
		u = this->elements - 1;
		while (1)
		{
			idx = (l + u) / 2;
			c = this->cmp(key, this->data_array[idx], this->data);

			if (0 > c)
				u = idx;
			else if (0 < c && 0 > this->cmp(key, this->data_array[idx + 1], this->data))
				return idx + 1;
			else if (c == 0)
				return -1;
			else
				l = idx + 1;
		}
	}
};

tree *
tree_new (tree_cmp_func *cmp, void *data)
{
	tree *t = new(::std::nothrow)tree(cmp, data);// calloc(1, sizeof(tree));
	if (!t)
		return nullptr;
	return t;
}

void
tree_destroy (tree *t)
{
	delete t;
}
namespace {
	static void *
		mybsearch(const void *key, const ::std::vector<void*> & data_array, size_t nmemb,
		int(*compar) (const void *, const void *, void *data), void *data, int *pos)
	{
		int l, u, idx;
		int comparison;

		l = 0;
		u = nmemb;
		while (l < u)
		{
			idx = (l + u) / 2;
			comparison = (*compar) (key, data_array[idx], data);
			if (comparison < 0)
				u = idx;
			else if (comparison > 0)
				l = idx + 1;
			else
			{
				*pos = idx;
				return data_array[idx];
			}
		}

		return nullptr;
	}
}

void *
tree_find (tree *t, const void *key, tree_cmp_func *cmp, void *data, int *pos)
{
	if (!t)
		return nullptr;

	return mybsearch (key, t->data_array, t->elements, cmp, data, pos);
}

void *
tree_remove_at_pos (tree *t, int pos)
{
	if (!t)
		return nullptr;
	void *ret = t->data_array[pos];

	t->elements--;
	if (pos != t->elements)
	{
		t->data_array.erase(t->data_array.cbegin() + pos);
	}
	return ret;
}

int
tree_remove (tree *t, void *key, int *pos)
{
	void *data;

	data = tree_find (t, key, t->cmp, t->data, pos);
	if (!data)
		return 0;

	tree_remove_at_pos (t, *pos);
	return 1;
}

void
tree_foreach (tree *t, tree_traverse_func *func, void *data)
{
	int j;

	if (!t)
		return;

	for (j = 0; j < t->elements; j++)
	{
		if (!func (t->data_array[j], data))
			break;
	}
}

int
tree_insert (tree *t, void *key)
{
	int pos, done;

	if (!t)
		return -1;

	t->grow();
	pos = t->find_insertion_pos (key, &done);
	if (!done && pos != -1)
		t->insert_at_pos (key, pos);

	return pos;
}

void
tree_append (tree *t, void *key)
{
	t->grow();
	t->insert_at_pos(key, t->elements);
}

int tree_size (tree *t)
{
	return t->elements;
}

