typedef struct xchat_plugin
{
	void *handle;		/* from dlopen */
	char *filename;	/* loaded from */
	char *name;
	char *desc;
	char *version;
	session *context;
	void *deinit_callback;	/* pointer to xchat_plugin_deinit */
	unsigned int fake:1;		/* fake plugin. Added by xchat_plugingui_add() */
} xchat_plugin;

extern GSList *plugin_list;

char *plugin_load (session *sess, char *filename, char *arg);
void plugin_add (session *sess, char *filename, void *handle, void *init_func, void *deinit_func, char *arg, int fake);
int plugin_kill (char *name, int by_filename);
void plugin_kill_all (void);
void plugin_auto_load (session *sess);
int plugin_emit_command (session *sess, char *name, char *word[], char *word_eol[]);
int plugin_emit_server (session *sess, char *name, char *word[], char *word_eol[]);
int plugin_emit_print (session *sess, char *word[]);
int plugin_emit_dummy_print (session *sess, char *name);
int plugin_show_help (session *sess, char *cmd);
