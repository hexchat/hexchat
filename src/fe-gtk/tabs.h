GtkWidget *tab_group_new (void *callback, gboolean vertical);
void tab_group_switch (GtkWidget *group, int relative, int num);
void tab_group_cleanup (GtkWidget *group);
int tab_group_get_size (GtkWidget *group);
GtkWidget *tab_group_add (GtkWidget *group, char *name, void *family, void *userdata, void *click_cb, void *delink_cb, int trunc_len, int dnd);
GtkOrientation tab_group_get_orientation (GtkWidget *group);
GtkWidget *tab_group_set_orientation (GtkWidget *group, gboolean vertical);
int tab_group_resize (GtkWidget *group);
GtkWidget *tab_group_get_focused (GtkWidget *group);

void tab_focus (GtkWidget *tab);
void tab_rename (GtkWidget *tab, char *new_name, int trunc_len);
void tab_remove (GtkWidget *tab);
void tab_move (GtkWidget *tab, int delta);
void tab_family_move (GtkWidget *tab, int delta);
void tab_set_attrlist (GtkWidget *tab, PangoAttrList *list);
