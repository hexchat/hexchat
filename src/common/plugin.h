#ifdef PLUGIN_C
struct _xchat_plugin
{
#ifdef WIN32
	/* Keep these insync with xchat-plugin.h */
	xchat_hook *(*hook_command) (xchat_plugin *ph,
		    char *name,
		    int pri,
		    int (*callback) (char *word[], char *word_eol[], void *user_data),
		    char *help_text,
		    void *userdata);
	xchat_hook *(*hook_server) (xchat_plugin *ph,
		   char *name,
		   int pri,
		   int (*callback) (char *word[], char *word_eol[], void *user_data),
		   void *userdata);
	xchat_hook *(*hook_print) (xchat_plugin *ph,
		  char *name,
		  int pri,
		  int (*callback) (char *word[], void *user_data),
		  void *userdata);
	xchat_hook *(*hook_timer) (xchat_plugin *ph,
		  int timeout,
		  int (*callback) (void *user_data),
		  void *userdata);
	xchat_hook *(*hook_socket) (xchat_plugin *ph,
		   int fd,
		   int flags,
		   int (*callback) (int fd, int flags, void *user_data),
		   void *userdata);
	void *(*unhook) (xchat_plugin *ph,
	      xchat_hook *hook);
	void (*print) (xchat_plugin *ph,
	     char *text);
	void (*printf) (xchat_plugin *ph,
	      char *format, ...);
	void (*command) (xchat_plugin *ph,
	       char *command);
	void (*commandf) (xchat_plugin *ph,
		char *format, ...);
	int (*nickcmp) (xchat_plugin *ph,
	       char *s1,
	       char *s2);
	int (*set_context) (xchat_plugin *ph,
		   xchat_context *ctx);
	xchat_context *(*find_context) (xchat_plugin *ph,
		    char *servname,
		    char *channel);
	xchat_context *(*get_context) (xchat_plugin *ph);
	const char *(*get_info) (xchat_plugin *ph,
		const char *id);
	int (*get_prefs) (xchat_plugin *ph,
		 const char *name,
		 const char **string,
		 int *integer);
	xchat_list * (*list_get) (xchat_plugin *ph,
		const char *name);
	void (*list_free) (xchat_plugin *ph,
		 xchat_list *xlist);
	const char ** (*list_fields) (xchat_plugin *ph,
		   const char *name);
	int (*list_next) (xchat_plugin *ph,
		 xchat_list *xlist);
	const char * (*list_str) (xchat_plugin *ph,
		xchat_list *xlist,
		const char *name);
	int (*list_int) (xchat_plugin *ph,
		xchat_list *xlist,
		const char *name);
	void * (*plugingui_add) (xchat_plugin *ph,
		     char *filename,
		     char *name,
		     char *desc,
		     char *version,
		     char *reserved);
	void (*plugingui_remove) (xchat_plugin *ph,
			void *handle);
#endif
	/* PRIVATE FIELDS! */
	void *handle;		/* from dlopen */
	char *filename;	/* loaded from */
	char *name;
	char *desc;
	char *version;
	session *context;
	void *deinit_callback;	/* pointer to xchat_plugin_deinit */
	unsigned int fake:1;		/* fake plugin. Added by xchat_plugingui_add() */
};
#endif

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
