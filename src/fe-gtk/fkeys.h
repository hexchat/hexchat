/* These are cp'ed from history.c --AGL */
#define STATE_SHIFT		GDK_SHIFT_MASK
#define	STATE_ALT		GDK_MOD1_MASK
#ifdef __APPLE__
#define STATE_CTRL		GDK_META_MASK
#else
#define STATE_CTRL		GDK_CONTROL_MASK
#endif

void key_init (void);
void key_dialog_show (void);
int key_handle_key_press (GtkWidget * wid, GdkEventKey * evt, session *sess);
int key_action_insert (GtkWidget * wid, GdkEventKey * evt, char *d1, char *d2,
						 session *sess);
