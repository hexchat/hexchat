/*
This is used for quick userlist insertion and lookup. It's not really
a tree, but it could be :)
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "tree.h"

#define ARRAY_GROW 32

struct _tree
{
	int elements;
	int array_size;
	void **array;
	tree_cmp_func *cmp;
	void *data;
};

tree *
tree_new (tree_cmp_func *cmp, void *data)
{
	tree *t = calloc (1, sizeof (tree));
	t->cmp = cmp;
	t->data = data;
	return t;
}

void
tree_destroy (tree *t)
{
	if (t)
	{
		if (t->array)
			free (t->array);
		free (t);
	}
}

static int
tree_find_insertion_pos (tree *t, void *key, int *done)
{
	int c, u, l, idx;

	if (t->elements < 1)
	{
		*done = 1;
		t->array[0] = key;
		t->elements++;
		return 0;
	}

	if (t->elements < 2)
	{
		*done = 1;
		c = t->cmp (key, t->array[0], t->data);
		if (c == 0)
			return -1;
		t->elements++;
		if (c > 0)
		{
			t->array[1] = key;
			return 1;
		}
		t->array[1] = t->array[0];
		t->array[0] = key;
		return 0;
	}

	*done = 0;

	c = t->cmp (key, t->array[0], t->data);
	if (c < 0)
		return 0;	/* prepend */

	c = t->cmp (key, t->array[t->elements - 1], t->data);
	if (c > 0)
		return t->elements;	/* append */

	l = 0;
	u = t->elements - 1;
	while (1)
	{
		idx = (l + u) / 2;
		c = t->cmp (key, t->array[idx], t->data);

		if (0 > c)
			u = idx;
		else if (0 < c && 0 > t->cmp (key, t->array[idx+1], t->data))
			return idx + 1;
		else if (c == 0)
			return -1;
		else
			l = idx + 1;
	}
}

static void
tree_insert_at_pos (tree *t, void *key, int pos)
{
	int post_bytes;

	/* append is easy */
	if (pos != t->elements)
	{
		post_bytes = (t->elements - pos) * sizeof (void *);
		memmove (&t->array[pos + 1], &t->array[pos], post_bytes);
	}

	t->array[pos] = key;
	t->elements++;
}

static void *
mybsearch (const void *key, void **array, size_t nmemb,
			  int (*compar) (const void *, const void *, void *data), void *data, int *pos)
{
	int l, u, idx;
	int comparison;

	l = 0;
	u = nmemb;
	while (l < u)
	{
		idx = (l + u) / 2;
		comparison = (*compar) (key, array[idx], data);
		if (comparison < 0)
			u = idx;
		else if (comparison > 0)
			l = idx + 1;
		else
		{
			*pos = idx;
			return array[idx];
		}
	}

	return NULL;
}

void *
tree_find (tree *t, void *key, tree_cmp_func *cmp, void *data, int *pos)
{
	if (!t || !t->array)
		return NULL;

	return mybsearch (key, &t->array[0], t->elements, cmp, data, pos);
}

static void
tree_remove_at_pos (tree *t, int pos)
{
	int post_bytes;

	t->elements--;
	if (pos != t->elements)
	{
		post_bytes = (t->elements - pos) * sizeof (void *);
		memmove (&t->array[pos], &t->array[pos + 1], post_bytes);
	}
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

	if (!t || !t->array)
		return;

	for (j = 0; j < t->elements; j++)
	{
		if (!func (t->array[j], data))
			break;
	}
}

int
tree_insert (tree *t, void *key)
{
	int pos, done;

	if (!t)
		return -1;

	if (t->array_size < t->elements + 1)
	{
		int new_size = t->array_size + ARRAY_GROW;

		t->array = realloc (t->array, sizeof (void *) * new_size);
		t->array_size = new_size;
	}

	pos = tree_find_insertion_pos (t, key, &done);
	if (!done && pos != -1)
		tree_insert_at_pos (t, key, pos);

	return pos;
}
