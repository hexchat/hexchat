/* You can distribute this header with your plugins for easy compilation */

#define XCHAT_IFACE_MAJOR	1
#define XCHAT_IFACE_MINOR	9
#define XCHAT_IFACE_MICRO	7
#define XCHAT_IFACE_VERSION	((XCHAT_IFACE_MAJOR * 10000) + \
				 (XCHAT_IFACE_MINOR * 100) + \
				 (XCHAT_IFACE_MICRO))

#define PRI_HIGHEST	127
#define PRI_HIGH	64
#define PRI_NORM	0
#define PRI_LOW		-64
#define PRI_LOWEST	-128

#define EAT_NONE	(0)	/* pass it on through! */
#define EAT_XCHAT	(1)	/* don't let xchat see this event */
#define EAT_PLUGIN	(2)	/* don't let other plugins see this event */
#define EAT_ALL		(EAT_XCHAT|EAT_PLUGIN)	/* don't let anything see this event */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _xchat_plugin xchat_plugin;
typedef struct _xchat_list xchat_list;
typedef struct _xchat_hook xchat_hook;
#ifndef PLUGIN_C
typedef struct _xchat_context xchat_context;
#endif

#if !defined(PLUGIN_C) && defined(WIN32)
/* === FOR WINDOWS === */
struct _xchat_plugin
{
	xchat_hook *(*xchat_hook_command) (xchat_plugin *ph,
		    char *name,
		    int pri,
		    int (*callback) (char *word[], char *word_eol[], void *user_data),
		    char *help_text,
		    void *userdata);
	xchat_hook *(*xchat_hook_server) (xchat_plugin *ph,
		   char *name,
		   int pri,
		   int (*callback) (char *word[], char *word_eol[], void *user_data),
		   void *userdata);
	xchat_hook *(*xchat_hook_print) (xchat_plugin *ph,
		  char *name,
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
	     char *text);
	void (*xchat_printf) (xchat_plugin *ph,
	      char *format, ...);
	void (*xchat_command) (xchat_plugin *ph,
	       char *command);
	void (*xchat_commandf) (xchat_plugin *ph,
		char *format, ...);
	int (*xchat_nickcmp) (xchat_plugin *ph,
	       char *s1,
	       char *s2);
	int (*xchat_set_context) (xchat_plugin *ph,
		   xchat_context *ctx);
	xchat_context *(*xchat_find_context) (xchat_plugin *ph,
		    char *servname,
		    char *channel);
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
	const char ** (*xchat_list_fields) (xchat_plugin *ph,
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
		     char *filename,
		     char *name,
		     char *desc,
		     char *version,
		     char *reserved);
	void (*xchat_plugingui_remove) (xchat_plugin *ph,
			void *handle);
};
#endif


xchat_hook *
xchat_hook_command (xchat_plugin *ph,
		    char *name,
		    int pri,
		    int (*callback) (char *word[], char *word_eol[], void *user_data),
		    char *help_text,
		    void *userdata);

xchat_hook *
xchat_hook_server (xchat_plugin *ph,
		   char *name,
		   int pri,
		   int (*callback) (char *word[], char *word_eol[], void *user_data),
		   void *userdata);

xchat_hook *
xchat_hook_print (xchat_plugin *ph,
		  char *name,
		  int pri,
		  int (*callback) (char *word[], void *user_data),
		  void *userdata);

xchat_hook *
xchat_hook_timer (xchat_plugin *ph,
		  int timeout,
		  int (*callback) (void *user_data),
		  void *userdata);

xchat_hook *
xchat_hook_fd (xchat_plugin *ph,
		   int fd,
		   int flags,
		   int (*callback) (int fd, int flags, void *user_data),
		   void *userdata);

void *
xchat_unhook (xchat_plugin *ph,
	      xchat_hook *hook);

void
xchat_print (xchat_plugin *ph,
	     char *text);

void
xchat_printf (xchat_plugin *ph,
	      char *format, ...);

void
xchat_command (xchat_plugin *ph,
	       char *command);

void
xchat_commandf (xchat_plugin *ph,
		char *format, ...);

int
xchat_nickcmp (xchat_plugin *ph,
	       char *s1,
	       char *s2);

int
xchat_set_context (xchat_plugin *ph,
		   xchat_context *ctx);

xchat_context *
xchat_find_context (xchat_plugin *ph,
		    char *servname,
		    char *channel);

xchat_context *
xchat_get_context (xchat_plugin *ph);

const char *
xchat_get_info (xchat_plugin *ph,
		const char *id);

int
xchat_get_prefs (xchat_plugin *ph,
		 const char *name,
		 const char **string,
		 int *integer);

xchat_list *
xchat_list_get (xchat_plugin *ph,
		const char *name);

void
xchat_list_free (xchat_plugin *ph,
		 xchat_list *xlist);

const char **
xchat_list_fields (xchat_plugin *ph,
		   const char *name);

int
xchat_list_next (xchat_plugin *ph,
		 xchat_list *xlist);

const char *
xchat_list_str (xchat_plugin *ph,
		xchat_list *xlist,
		const char *name);

int
xchat_list_int (xchat_plugin *ph,
		xchat_list *xlist,
		const char *name);

void *
xchat_plugingui_add (xchat_plugin *ph,
		     char *filename,
		     char *name,
		     char *desc,
		     char *version,
		     char *reserved);

void
xchat_plugingui_remove (xchat_plugin *ph,
			void *handle);

#if !defined(PLUGIN_C) && defined(WIN32)
#define xchat_hook_command ((ph)->xchat_hook_command)
#define xchat_hook_server ((ph)->xchat_hook_server)
#define xchat_hook_print ((ph)->xchat_hook_print)
#define xchat_hook_timer ((ph)->xchat_hook_timer)
#define xchat_hook_fd ((ph)->xchat_hook_fd)
#define xchat_unhook ((ph)->xchat_unhook)
#define xchat_print ((ph)->xchat_print)
#define xchat_printf ((ph)->xchat_printf)
#define xchat_command ((ph)->xchat_command)
#define xchat_commandf ((ph)->xchat_commandf)
#define xchat_nickcmp ((ph)->xchat_nickcmp)
#define xchat_set_context ((ph)->xchat_set_context)
#define xchat_find_context ((ph)->xchat_find_context)
#define xchat_get_context ((ph)->xchat_get_context)
#define xchat_get_info ((ph)->xchat_get_info)
#define xchat_get_prefs ((ph)->xchat_get_prefs)
#define xchat_list_get ((ph)->xchat_list_get)
#define xchat_list_free ((ph)->xchat_list_free)
#define xchat_list_fields ((ph)->xchat_list_fields)
#define xchat_list_str ((ph)->xchat_list_str)
#define xchat_list_int ((ph)->xchat_list_int)
#define xchat_list_next ((ph)->xchat_list_next)
#define xchat_plugingui_add ((ph)->xchat_plugingui_add)
#define xchat_plugingui_remove ((ph)->xchat_plugingui_remove)
#endif

#ifdef __cplusplus
}
#endif
