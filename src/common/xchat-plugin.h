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
xchat_hook_socket (xchat_plugin *ph,
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
#define xchat_hook_command ((ph)->hook_command)
#define xchat_hook_server ((ph)->hook_server)
#define xchat_hook_print ((ph)->hook_print)
#define xchat_hook_timer ((ph)->hook_timer)
#define xchat_unhook ((ph)->unhook)
#define xchat_print ((ph)->print)
#define xchat_printf ((ph)->printf)
#define xchat_command ((ph)->command)
#define xchat_commandf ((ph)->commandf)
#define xchat_nickcmp ((ph)->nickcmp)
#define xchat_set_context ((ph)->set_context)
#define xchat_find_context ((ph)->find_context)
#define xchat_get_context ((ph)->get_context)
#define xchat_get_info ((ph)->get_info)
#define xchat_get_prefs ((ph)->get_prefs)
#define xchat_list_get ((ph)->list_get)
#define xchat_list_free ((ph)->list_free)
#define xchat_list_fields ((ph)->list_fields)
#define xchat_list_str ((ph)->list_str)
#define xchat_list_int ((ph)->list_int)
#define xchat_plugingui_add ((ph)->plugingui_add)
#define xchat_plugingui_remove ((ph)->plugingui_remove)
#endif

#ifdef __cplusplus
}
#endif
