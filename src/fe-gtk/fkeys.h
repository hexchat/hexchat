void key_init (void);
void key_dialog_show (void);
int key_handle_key_pressAFTER (GtkWidget * wid, GdkEventKey * evt, session *sess);
int key_handle_key_press (GtkWidget * wid, GdkEventKey * evt, session *sess);
int key_action_insert (GtkWidget * wid, GdkEventKey * evt, char *d1, char *d2,
						 session *sess);
