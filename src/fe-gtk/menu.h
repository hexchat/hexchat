GtkWidget *menu_create_main (void *accel_group, int bar, int away, int toplevel, GtkWidget **menu_widgets);
void menu_urlmenu (GdkEventButton * event, char *url);
void menu_chanmenu (session *sess, GdkEventButton * event, char *chan);
void menu_nickmenu (session *sess, GdkEventButton * event, char *nick, int num_sel);
void menu_middlemenu (session *sess, GdkEventButton *event);
void userlist_button_cb (GtkWidget * button, char *cmd);
void nick_command_parse (session *sess, char *cmd, char *nick, char *allnick);
void usermenu_update (void);
GtkWidget *menu_toggle_item (char *label, GtkWidget *menu, void *callback, void *userdata, int state);
GtkWidget *create_icon_menu (char *labeltext, void *stock_name, int is_stock);
void menu_create (GtkWidget *menu, GSList *list, char *target, int check_path);
void menu_bar_toggle (void);
void menu_add_plugin_items (GtkWidget *menu, char *root);

/* menu items we keep a GtkWidget* for (to change their state) */
#define MENU_ID_AWAY 1
#define MENU_ID_MENUBAR 2
#define MENU_ID_TOPICBAR 3
#define MENU_ID_ULBUTTONS 4
#define MENU_ID_MODEBUTTONS 5
#define MENU_ID_LAYOUT_TABS 6
#define MENU_ID_LAYOUT_TREE 7
#define MENU_ID_DISCONNECT 8
#define MENU_ID_RECONNECT 9
#define MENU_ID_JOIN 10
#define MENU_ID_USERMENU 11

#if (MENU_ID_NUM < MENU_ID_USERMENU)
#error MENU_ID_NUM is set wrong
#endif
