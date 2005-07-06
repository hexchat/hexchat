#ifndef XCHAT_COMMONPLUGIN_H
#define XCHAT_COMMONPLUGIN_H

#ifdef PLUGIN_C
struct _xchat_plugin
{
	/* Keep these insync with xchat-plugin.h */
	/* !!don't change the order, to keep binary compat!! */
	xchat_hook *(*xchat_hook_command) (xchat_plugin *ph,
		    const char *name,
		    int pri,
		    int (*callback) (char *word[], char *word_eol[], void *user_data),
		    const char *help_text,
		    void *userdata);
	xchat_hook *(*xchat_hook_server) (xchat_plugin *ph,
		   const char *name,
		   int pri,
		   int (*callback) (char *word[], char *word_eol[], void *user_data),
		   void *userdata);
	xchat_hook *(*xchat_hook_print) (xchat_plugin *ph,
		  const char *name,
		  int pri,
		  int (*callback) (char *word[], void *user_data),
		  void *userdata);
	xchat_hook *(*xchat_hook_timer) (xchat_plugin *ph,
		  int timeout,
		  int (*callback) (void *user_data),
		  void *userdata);
	xchat_hook *(*xchat_hook_fd) (xchat_plugin *ph,
		   int fd,
		   int flags,
		   int (*callback) (int fd, int flags, void *user_data),
		   void *userdata);
	void *(*xchat_unhook) (xchat_plugin *ph,
	      xchat_hook *hook);
	void (*xchat_print) (xchat_plugin *ph,
	     const char *text);
	void (*xchat_printf) (xchat_plugin *ph,
	      const char *format, ...);
	void (*xchat_command) (xchat_plugin *ph,
	       const char *command);
	void (*xchat_commandf) (xchat_plugin *ph,
		const char *format, ...);
	int (*xchat_nickcmp) (xchat_plugin *ph,
	       const char *s1,
	       const char *s2);
	int (*xchat_set_context) (xchat_plugin *ph,
		   xchat_context *ctx);
	xchat_context *(*xchat_find_context) (xchat_plugin *ph,
		    const char *servname,
		    const char *channel);
	xchat_context *(*xchat_get_context) (xchat_plugin *ph);
	const char *(*xchat_get_info) (xchat_plugin *ph,
		const char *id);
	int (*xchat_get_prefs) (xchat_plugin *ph,
		 const char *name,
		 const char **string,
		 int *integer);
	xchat_list * (*xchat_list_get) (xchat_plugin *ph,
		const char *name);
	void (*xchat_list_free) (xchat_plugin *ph,
		 xchat_list *xlist);
	const char * const * (*xchat_list_fields) (xchat_plugin *ph,
		   const char *name);
	int (*xchat_list_next) (xchat_plugin *ph,
		 xchat_list *xlist);
	const char * (*xchat_list_str) (xchat_plugin *ph,
		xchat_list *xlist,
		const char *name);
	int (*xchat_list_int) (xchat_plugin *ph,
		xchat_list *xlist,
		const char *name);
	void * (*xchat_plugingui_add) (xchat_plugin *ph,
		     const char *filename,
		     const char *name,
		     const char *desc,
		     const char *version,
		     char *reserved);
	void (*xchat_plugingui_remove) (xchat_plugin *ph,
			void *handle);
	int (*xchat_emit_print) (xchat_plugin *ph,
			const char *event_name, ...);
	void *(*xchat_read_fd) (xchat_plugin *ph);
	time_t (*xchat_list_time) (xchat_plugin *ph,
		xchat_list *xlist,
		const char *name);
	char *(*xchat_gettext) (xchat_plugin *ph,
		const char *msgid);
	void (*xchat_send_modes) (xchat_plugin *ph,
		  const char **targets,
		  int ntargets,
		  int modes_per_line,
		  char sign,
		  char mode);
	char *(*xchat_strip) (xchat_plugin *ph,
	     const char *str,
	     int len,
	     int flags);
	void (*xchat_free) (xchat_plugin *ph,
	    void *ptr);
	void *(*xchat_dummy4) (xchat_plugin *ph);
	void *(*xchat_dummy3) (xchat_plugin *ph);
	void *(*xchat_dummy2) (xchat_plugin *ph);
	void *(*xchat_dummy1) (xchat_plugin *ph);
	/* PRIVATE FIELDS! */
	void *handle;		/* from dlopen */
	char *filename;	/* loaded from */
	char *name;
	char *desc;
	char *version;
	session *context;
	void *deinit_callback;	/* pointer to xchat_plugin_deinit */
	unsigned int fake:1;		/* fake plugin. Added by xchat_plugingui_add() */
	unsigned int free_strings:1;		/* free name,desc,version? */
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
int plugin_emit_keypress (session *sess, unsigned int state, unsigned int keyval, int len, char *string);
GList* plugin_command_list(GList *tmp_list);
int plugin_show_help (session *sess, char *cmd);
void plugin_command_foreach (session *sess, void *userdata, void (*cb) (session *sess, void *userdata, char *name, char *usage));

#endif
