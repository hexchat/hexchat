GtkWidget *tab_group_new (void *callback, gboolean vertical);
void tab_group_switch (GtkWidget *group, int relative, int num);
void tab_group_cleanup (GtkWidget *group);
int tab_group_get_size (GtkWidget *group);
GtkWidget *tab_group_add (GtkWidget *group, char *name, void *family, void *userdata, void *click_cb);

void tab_focus (GtkWidget *tab);
void tab_rename (GtkWidget *tab, char *new_name);
void tab_remove (GtkWidget *tab);
void tab_style (GtkWidget *tab, GtkStyle *style);
