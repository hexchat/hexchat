typedef struct _chanview chanview;
typedef struct _chan chan;

chanview *chanview_new (int type, int trunc_len, gboolean sort, gboolean use_icons, GtkStyle *style);
void chanview_set_callbacks (chanview *cv,
	void (*cb_focus) (chanview *, chan *, int tag, void *userdata),
	void (*cb_xbutton) (chanview *, chan *, int tag, void *userdata),
	gboolean (*cb_contextmenu) (chanview *, chan *, int tag, void *userdata, GdkEventButton *),
	int (*cb_compare) (void *a, void *b));
void chanview_set_impl (chanview *cv, int type);
chan *chanview_add (chanview *cv, char *name, void *family, void *userdata, gboolean allow_closure, int tag, GdkPixbuf *icon);
int chanview_get_size (chanview *cv);
GtkWidget *chanview_get_box (chanview *cv);
void chanview_move_focus (chanview *cv, gboolean relative, int num);
chan *chanview_get_focused (chanview *cv);
GtkOrientation chanview_get_orientation (chanview *cv);
void chanview_set_orientation (chanview *cv, gboolean vertical);

int chan_get_tag (chan *ch);
void chan_focus (chan *ch);
void chan_move (chan *ch, int delta);
void chan_move_family (chan *ch, int delta);
void chan_set_color (chan *ch, PangoAttrList *list);
void chan_rename (chan *ch, char *new_name, int trunc_len);
gboolean chan_remove (chan *ch, gboolean force);

#define FOCUS_NEW_ALL 1
#define FOCUS_NEW_ONLY_ASKED 2
#define FOCUS_NEW_NONE 0
