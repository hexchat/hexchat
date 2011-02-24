#ifndef XCHAT_TREE_H
#define XCHAT_TREE_H

typedef struct _tree tree;

typedef int (tree_cmp_func) (const void *keya, const void *keyb, void *data);
typedef int (tree_traverse_func) (const void *key, void *data);

tree *tree_new (tree_cmp_func *cmp, void *data);
void tree_destroy (tree *t);
void *tree_find (tree *t, void *key, tree_cmp_func *cmp, void *data, int *pos);
int tree_remove (tree *t, void *key, int *pos);
void tree_foreach (tree *t, tree_traverse_func *func, void *data);
int tree_insert (tree *t, void *key);

#endif
