/* You can distribute this header with your plugins for easy compilation */
#ifndef XCHAT_PLUGIN_H
#define XCHAT_PLUGIN_H

#include <time.h>

#define XCHAT_IFACE_MAJOR	1
#define XCHAT_IFACE_MINOR	9
#define XCHAT_IFACE_MICRO	11
#define XCHAT_IFACE_VERSION	((XCHAT_IFACE_MAJOR * 10000) + \
				 (XCHAT_IFACE_MINOR * 100) + \
				 (XCHAT_IFACE_MICRO))

#define XCHAT_PRI_HIGHEST	127
#define XCHAT_PRI_HIGH		64
#define XCHAT_PRI_NORM		0
#define XCHAT_PRI_LOW		(-64)
#define XCHAT_PRI_LOWEST	(-128)

#define XCHAT_FD_READ		1
#define XCHAT_FD_WRITE		2
#define XCHAT_FD_EXCEPTION	4
#define XCHAT_FD_NOTSOCKET	8

#define XCHAT_EAT_NONE		0	/* pass it on through! */
#define XCHAT_EAT_XCHAT		1	/* don't let xchat see this event */
#define XCHAT_EAT_PLUGIN	2	/* don't let other plugins see this event */
#define XCHAT_EAT_ALL		(XCHAT_EAT_XCHAT|XCHAT_EAT_PLUGIN)	/* don't let anything see this event */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _xchat_plugin xchat_plugin;
typedef struct _xchat_list xchat_list;
typedef struct _xchat_hook xchat_hook;
#ifndef PLUGIN_C
typedef struct _xchat_context xchat_context;
#endif

#ifndef PLUGIN_C
struct _xchat_plugin
{
	/* these are only used on win32 */
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
	int (*xchat_read_fd) (xchat_plugin *ph,
			void *src,
			char *buf,
			int *len);
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
};
#endif


xchat_hook *
xchat_hook_command (xchat_plugin *ph,
		    const char *name,
		    int pri,
		    int (*callback) (char *word[], char *word_eol[], void *user_data),
		    const char *help_text,
		    void *userdata);

xchat_hook *
xchat_hook_server (xchat_plugin *ph,
		   const char *name,
		   int pri,
		   int (*callback) (char *word[], char *word_eol[], void *user_data),
		   void *userdata);

xchat_hook *
xchat_hook_print (xchat_plugin *ph,
		  const char *name,
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
	     const char *text);

void
xchat_printf (xchat_plugin *ph,
	      const char *format, ...);

void
xchat_command (xchat_plugin *ph,
	       const char *command);

void
xchat_commandf (xchat_plugin *ph,
		const char *format, ...);

int
xchat_nickcmp (xchat_plugin *ph,
	       const char *s1,
	       const char *s2);

int
xchat_set_context (xchat_plugin *ph,
		   xchat_context *ctx);

xchat_context *
xchat_find_context (xchat_plugin *ph,
		    const char *servname,
		    const char *channel);

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

const char * const *
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

time_t
xchat_list_time (xchat_plugin *ph,
		 xchat_list *xlist,
		 const char *name);

void *
xchat_plugingui_add (xchat_plugin *ph,
		     const char *filename,
		     const char *name,
		     const char *desc,
		     const char *version,
		     char *reserved);

void
xchat_plugingui_remove (xchat_plugin *ph,
			void *handle);

int 
xchat_emit_print (xchat_plugin *ph,
		  const char *event_name, ...);

char *
xchat_gettext (xchat_plugin *ph,
	       const char *msgid);

void
xchat_send_modes (xchat_plugin *ph,
		  const char **targets,
		  int ntargets,
		  int modes_per_line,
		  char sign,
		  char mode);

char *
xchat_strip (xchat_plugin *ph,
	     const char *str,
	     int len,
	     int flags);

void
xchat_free (xchat_plugin *ph,
	    void *ptr);

#if !defined(PLUGIN_C) && defined(WIN32)
#ifndef XCHAT_PLUGIN_HANDLE
#define XCHAT_PLUGIN_HANDLE (ph)
#endif
#define xchat_hook_command ((XCHAT_PLUGIN_HANDLE)->xchat_hook_command)
#define xchat_hook_server ((XCHAT_PLUGIN_HANDLE)->xchat_hook_server)
#define xchat_hook_print ((XCHAT_PLUGIN_HANDLE)->xchat_hook_print)
#define xchat_hook_timer ((XCHAT_PLUGIN_HANDLE)->xchat_hook_timer)
#define xchat_hook_fd ((XCHAT_PLUGIN_HANDLE)->xchat_hook_fd)
#define xchat_unhook ((XCHAT_PLUGIN_HANDLE)->xchat_unhook)
#define xchat_print ((XCHAT_PLUGIN_HANDLE)->xchat_print)
#define xchat_printf ((XCHAT_PLUGIN_HANDLE)->xchat_printf)
#define xchat_command ((XCHAT_PLUGIN_HANDLE)->xchat_command)
#define xchat_commandf ((XCHAT_PLUGIN_HANDLE)->xchat_commandf)
#define xchat_nickcmp ((XCHAT_PLUGIN_HANDLE)->xchat_nickcmp)
#define xchat_set_context ((XCHAT_PLUGIN_HANDLE)->xchat_set_context)
#define xchat_find_context ((XCHAT_PLUGIN_HANDLE)->xchat_find_context)
#define xchat_get_context ((XCHAT_PLUGIN_HANDLE)->xchat_get_context)
#define xchat_get_info ((XCHAT_PLUGIN_HANDLE)->xchat_get_info)
#define xchat_get_prefs ((XCHAT_PLUGIN_HANDLE)->xchat_get_prefs)
#define xchat_list_get ((XCHAT_PLUGIN_HANDLE)->xchat_list_get)
#define xchat_list_free ((XCHAT_PLUGIN_HANDLE)->xchat_list_free)
#define xchat_list_fields ((XCHAT_PLUGIN_HANDLE)->xchat_list_fields)
#define xchat_list_str ((XCHAT_PLUGIN_HANDLE)->xchat_list_str)
#define xchat_list_int ((XCHAT_PLUGIN_HANDLE)->xchat_list_int)
#define xchat_list_time ((XCHAT_PLUGIN_HANDLE)->xchat_list_time)
#define xchat_list_next ((XCHAT_PLUGIN_HANDLE)->xchat_list_next)
#define xchat_plugingui_add ((XCHAT_PLUGIN_HANDLE)->xchat_plugingui_add)
#define xchat_plugingui_remove ((XCHAT_PLUGIN_HANDLE)->xchat_plugingui_remove)
#define xchat_emit_print ((XCHAT_PLUGIN_HANDLE)->xchat_emit_print)
#define xchat_gettext ((XCHAT_PLUGIN_HANDLE)->xchat_gettext)
#define xchat_send_modes ((XCHAT_PLUGIN_HANDLE)->xchat_send_modes)
#define xchat_strip ((XCHAT_PLUGIN_HANDLE)->xchat_strip)
#define xchat_free ((XCHAT_PLUGIN_HANDLE)->xchat_free)
#endif

#ifdef __cplusplus
}
#endif
#endif
