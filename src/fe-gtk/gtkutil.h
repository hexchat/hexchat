typedef void (*filereqcallback) (void *, void *, char *file);

void gtkutil_get_str (char *msg, char *def, void *callback, void *userdata);
void gtkutil_file_req (char *title, void *callback, void *userdata,
							  void *userdata2, int write);
void gtkutil_destroy (GtkWidget * igad, GtkWidget * dgad);
GtkWidget *gtkutil_simpledialog (char *msg);
GtkWidget *gtkutil_button (GtkWidget *box, char *stock, char *tip, void *callback,
				 void *userdata, char *labeltext);
void gtkutil_label_new (char *text, GtkWidget * box);
GtkWidget *gtkutil_entry_new (int max, GtkWidget * box, void *callback,
										gpointer userdata);
GtkWidget *gtkutil_clist_new (int columns, char *titles[], GtkWidget * box,
										int policy, void *select_callback,
										gpointer select_userdata,
										void *unselect_callback,
										gpointer unselect_userdata, int selection_mode);
int gtkutil_clist_selection (GtkWidget * clist);
void add_tip (GtkWidget * wid, char *text);
void show_and_unfocus (GtkWidget * wid);
void gtkutil_set_icon (GtkWidget *win);
GtkWidget *gtkutil_window_new (char *title, int width, int height, int mouse_pos);
