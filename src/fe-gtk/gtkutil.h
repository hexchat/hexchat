#include <gtk/gtktreeview.h>
#include <gtk/gtktreemodel.h>

typedef void (*filereqcallback) (void *, char *file);

#define FRF_WRITE 1
#define FRF_MULTIPLE 2

void gtkutil_file_req (char *title, void *file_callback, void *userdata,
						void *clean_callback, char *filter, int flags);
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
GtkWidget *gtkutil_window_new (char *title, char *role, int width, int height, int flags);
void gtkutil_copy_to_clipboard (GtkWidget *widget, GdkAtom selection,
                                const gchar *str);
GtkWidget *gtkutil_treeview_new (GtkWidget *box, GtkTreeModel *model,
                                 GtkTreeCellDataFunc mapper, ...);
gboolean gtkutil_treemodel_string_to_iter (GtkTreeModel *model, gchar *pathstr, GtkTreeIter *iter_ret);
gboolean gtkutil_treeview_get_selected_iter (GtkTreeView *view, GtkTreeIter *iter_ret);
gboolean gtkutil_treeview_get_selected (GtkTreeView *view, GtkTreeIter *iter_ret, ...);

