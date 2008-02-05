GtkWidget *menu_create_main (void *accel_group, int bar, int away, int toplevel, GtkWidget **menu_widgets);
void menu_urlmenu (GdkEventButton * event, char *url);
void menu_chanmenu (session *sess, GdkEventButton * event, char *chan);
void menu_addfavoritemenu (server *serv, GtkWidget *menu, char *channel);
void menu_nickmenu (session *sess, GdkEventButton * event, char *nick, int num_sel);
void menu_middlemenu (session *sess, GdkEventButton *event);
void userlist_button_cb (GtkWidget * button, char *cmd);
void nick_command_parse (session *sess, char *cmd, char *nick, char *allnick);
void usermenu_update (void);
GtkWidget *menu_toggle_item (char *label, GtkWidget *menu, void *callback, void *userdata, int state);
GtkWidget *menu_quick_item (char *cmd, char *label, GtkWidget * menu, int flags, gpointer userdata, char *icon);
GtkWidget *menu_quick_sub (char *name, GtkWidget *menu, GtkWidget **sub_item_ret, int flags, int pos);
GtkWidget *create_icon_menu (char *labeltext, void *stock_name, int is_stock);
void menu_create (GtkWidget *menu, GSList *list, char *target, int check_path);
void menu_bar_toggle (void);
void menu_add_plugin_items (GtkWidget *menu, char *root, char *target);
void menu_change_layout (void);

/* for menu_quick functions */
#define XCMENU_DOLIST 1
#define XCMENU_SHADED 1
#define XCMENU_MARKUP 2
#define XCMENU_MNEMONIC 4

/* menu items we keep a GtkWidget* for (to change their state) */
#define MENU_ID_AWAY 1
#define MENU_ID_MENUBAR 2
#define MENU_ID_TOPICBAR 3
#define MENU_ID_USERLIST 4
#define MENU_ID_ULBUTTONS 5
#define MENU_ID_MODEBUTTONS 6
#define MENU_ID_LAYOUT_TABS 7
#define MENU_ID_LAYOUT_TREE 8
#define MENU_ID_DISCONNECT 9
#define MENU_ID_RECONNECT 10
#define MENU_ID_JOIN 11
#define MENU_ID_USERMENU 12

#if (MENU_ID_NUM < MENU_ID_USERMENU)
#error MENU_ID_NUM is set wrong
#endif
