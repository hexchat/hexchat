void userlist_set_value (GtkWidget *treeview, gfloat val);
gfloat userlist_get_value (GtkWidget *treeview);
GtkWidget *userlist_create (GtkWidget *box);
void *userlist_create_model (void);
void userlist_show (session *sess);
void userlist_select (session *sess, char *name);
char **userlist_selection_list (GtkWidget *widget, int *num_ret);
GdkPixbuf *get_user_icon (server *serv, struct User *user);
