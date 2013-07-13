/* HexChat
 * Copyright (C) 1998-2010 Peter Zelezny.
 * Copyright (C) 2009-2013 Berke Viktor.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* You can distribute this header with your plugins for easy compilation */
#ifndef HEXCHAT_PLUGIN_H
#define HEXCHAT_PLUGIN_H

#include <time.h>

#define HEXCHAT_PRI_HIGHEST	127
#define HEXCHAT_PRI_HIGH		64
#define HEXCHAT_PRI_NORM		0
#define HEXCHAT_PRI_LOW		(-64)
#define HEXCHAT_PRI_LOWEST	(-128)

#define HEXCHAT_FD_READ		1
#define HEXCHAT_FD_WRITE		2
#define HEXCHAT_FD_EXCEPTION	4
#define HEXCHAT_FD_NOTSOCKET	8

#define HEXCHAT_EAT_NONE		0	/* pass it on through! */
#define HEXCHAT_EAT_HEXCHAT		1	/* don't let HexChat see this event */
#define HEXCHAT_EAT_PLUGIN	2	/* don't let other plugins see this event */
#define HEXCHAT_EAT_ALL		(HEXCHAT_EAT_HEXCHAT|HEXCHAT_EAT_PLUGIN)	/* don't let anything see this event */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _hexchat_plugin hexchat_plugin;
typedef struct _hexchat_list hexchat_list;
typedef struct _hexchat_hook hexchat_hook;
#ifndef PLUGIN_C
typedef struct _hexchat_context hexchat_context;
#endif
typedef struct
{
	time_t server_time_utc; /* 0 if not used */
} hexchat_event_attrs;

#ifndef PLUGIN_C
struct _hexchat_plugin
{
	/* these are only used on win32 */
	hexchat_hook *(*hexchat_hook_command) (hexchat_plugin *ph,
		    const char *name,
		    int pri,
		    int (*callback) (char *word[], char *word_eol[], void *user_data),
		    const char *help_text,
		    void *userdata);
	hexchat_hook *(*hexchat_hook_server) (hexchat_plugin *ph,
		   const char *name,
		   int pri,
		   int (*callback) (char *word[], char *word_eol[], void *user_data),
		   void *userdata);
	hexchat_hook *(*hexchat_hook_print) (hexchat_plugin *ph,
		  const char *name,
		  int pri,
		  int (*callback) (char *word[], void *user_data),
		  void *userdata);
	hexchat_hook *(*hexchat_hook_timer) (hexchat_plugin *ph,
		  int timeout,
		  int (*callback) (void *user_data),
		  void *userdata);
	hexchat_hook *(*hexchat_hook_fd) (hexchat_plugin *ph,
		   int fd,
		   int flags,
		   int (*callback) (int fd, int flags, void *user_data),
		   void *userdata);
	void *(*hexchat_unhook) (hexchat_plugin *ph,
	      hexchat_hook *hook);
	void (*hexchat_print) (hexchat_plugin *ph,
	     const char *text);
	void (*hexchat_printf) (hexchat_plugin *ph,
	      const char *format, ...);
	void (*hexchat_command) (hexchat_plugin *ph,
	       const char *command);
	void (*hexchat_commandf) (hexchat_plugin *ph,
		const char *format, ...);
	int (*hexchat_nickcmp) (hexchat_plugin *ph,
	       const char *s1,
	       const char *s2);
	int (*hexchat_set_context) (hexchat_plugin *ph,
		   hexchat_context *ctx);
	hexchat_context *(*hexchat_find_context) (hexchat_plugin *ph,
		    const char *servname,
		    const char *channel);
	hexchat_context *(*hexchat_get_context) (hexchat_plugin *ph);
	const char *(*hexchat_get_info) (hexchat_plugin *ph,
		const char *id);
	int (*hexchat_get_prefs) (hexchat_plugin *ph,
		 const char *name,
		 const char **string,
		 int *integer);
	hexchat_list * (*hexchat_list_get) (hexchat_plugin *ph,
		const char *name);
	void (*hexchat_list_free) (hexchat_plugin *ph,
		 hexchat_list *xlist);
	const char * const * (*hexchat_list_fields) (hexchat_plugin *ph,
		   const char *name);
	int (*hexchat_list_next) (hexchat_plugin *ph,
		 hexchat_list *xlist);
	const char * (*hexchat_list_str) (hexchat_plugin *ph,
		hexchat_list *xlist,
		const char *name);
	int (*hexchat_list_int) (hexchat_plugin *ph,
		hexchat_list *xlist,
		const char *name);
	void * (*hexchat_plugingui_add) (hexchat_plugin *ph,
		     const char *filename,
		     const char *name,
		     const char *desc,
		     const char *version,
		     char *reserved);
	void (*hexchat_plugingui_remove) (hexchat_plugin *ph,
			void *handle);
	int (*hexchat_emit_print) (hexchat_plugin *ph,
			const char *event_name, ...);
	int (*hexchat_read_fd) (hexchat_plugin *ph,
			void *src,
			char *buf,
			int *len);
	time_t (*hexchat_list_time) (hexchat_plugin *ph,
		hexchat_list *xlist,
		const char *name);
	char *(*hexchat_gettext) (hexchat_plugin *ph,
		const char *msgid);
	void (*hexchat_send_modes) (hexchat_plugin *ph,
		  const char **targets,
		  int ntargets,
		  int modes_per_line,
		  char sign,
		  char mode);
	char *(*hexchat_strip) (hexchat_plugin *ph,
	     const char *str,
	     int len,
	     int flags);
	void (*hexchat_free) (hexchat_plugin *ph,
	    void *ptr);
	int (*hexchat_pluginpref_set_str) (hexchat_plugin *ph,
		const char *var,
		const char *value);
	int (*hexchat_pluginpref_get_str) (hexchat_plugin *ph,
		const char *var,
		char *dest);
	int (*hexchat_pluginpref_set_int) (hexchat_plugin *ph,
		const char *var,
		int value);
	int (*hexchat_pluginpref_get_int) (hexchat_plugin *ph,
		const char *var);
	int (*hexchat_pluginpref_delete) (hexchat_plugin *ph,
		const char *var);
	int (*hexchat_pluginpref_list) (hexchat_plugin *ph,
		char *dest);
	hexchat_hook *(*hexchat_hook_server_attrs) (hexchat_plugin *ph,
		   const char *name,
		   int pri,
		   int (*callback) (char *word[], char *word_eol[],
							hexchat_event_attrs *attrs, void *user_data),
		   void *userdata);
	hexchat_hook *(*hexchat_hook_print_attrs) (hexchat_plugin *ph,
		  const char *name,
		  int pri,
		  int (*callback) (char *word[], hexchat_event_attrs *attrs,
						   void *user_data),
		  void *userdata);
	int (*hexchat_emit_print_attrs) (hexchat_plugin *ph, hexchat_event_attrs *attrs,
									 const char *event_name, ...);
	hexchat_event_attrs *(*hexchat_event_attrs_create) (hexchat_plugin *ph);
	void (*hexchat_event_attrs_free) (hexchat_plugin *ph,
									  hexchat_event_attrs *attrs);
};
#endif


hexchat_hook *
hexchat_hook_command (hexchat_plugin *ph,
		    const char *name,
		    int pri,
		    int (*callback) (char *word[], char *word_eol[], void *user_data),
		    const char *help_text,
		    void *userdata);

hexchat_event_attrs *hexchat_event_attrs_create (hexchat_plugin *ph);

void hexchat_event_attrs_free (hexchat_plugin *ph, hexchat_event_attrs *attrs);

hexchat_hook *
hexchat_hook_server (hexchat_plugin *ph,
		   const char *name,
		   int pri,
		   int (*callback) (char *word[], char *word_eol[], void *user_data),
		   void *userdata);

hexchat_hook *
hexchat_hook_server_attrs (hexchat_plugin *ph,
		   const char *name,
		   int pri,
		   int (*callback) (char *word[], char *word_eol[],
							hexchat_event_attrs *attrs, void *user_data),
		   void *userdata);

hexchat_hook *
hexchat_hook_print (hexchat_plugin *ph,
		  const char *name,
		  int pri,
		  int (*callback) (char *word[], void *user_data),
		  void *userdata);

hexchat_hook *
hexchat_hook_print_attrs (hexchat_plugin *ph,
		  const char *name,
		  int pri,
		  int (*callback) (char *word[], hexchat_event_attrs *attrs,
						   void *user_data),
		  void *userdata);

hexchat_hook *
hexchat_hook_timer (hexchat_plugin *ph,
		  int timeout,
		  int (*callback) (void *user_data),
		  void *userdata);

hexchat_hook *
hexchat_hook_fd (hexchat_plugin *ph,
		int fd,
		int flags,
		int (*callback) (int fd, int flags, void *user_data),
		void *userdata);

void *
hexchat_unhook (hexchat_plugin *ph,
	      hexchat_hook *hook);

void
hexchat_print (hexchat_plugin *ph,
	     const char *text);

void
hexchat_printf (hexchat_plugin *ph,
	      const char *format, ...);

void
hexchat_command (hexchat_plugin *ph,
	       const char *command);

void
hexchat_commandf (hexchat_plugin *ph,
		const char *format, ...);

int
hexchat_nickcmp (hexchat_plugin *ph,
	       const char *s1,
	       const char *s2);

int
hexchat_set_context (hexchat_plugin *ph,
		   hexchat_context *ctx);

hexchat_context *
hexchat_find_context (hexchat_plugin *ph,
		    const char *servname,
		    const char *channel);

hexchat_context *
hexchat_get_context (hexchat_plugin *ph);

const char *
hexchat_get_info (hexchat_plugin *ph,
		const char *id);

int
hexchat_get_prefs (hexchat_plugin *ph,
		 const char *name,
		 const char **string,
		 int *integer);

hexchat_list *
hexchat_list_get (hexchat_plugin *ph,
		const char *name);

void
hexchat_list_free (hexchat_plugin *ph,
		 hexchat_list *xlist);

const char * const *
hexchat_list_fields (hexchat_plugin *ph,
		   const char *name);

int
hexchat_list_next (hexchat_plugin *ph,
		 hexchat_list *xlist);

const char *
hexchat_list_str (hexchat_plugin *ph,
		hexchat_list *xlist,
		const char *name);

int
hexchat_list_int (hexchat_plugin *ph,
		hexchat_list *xlist,
		const char *name);

time_t
hexchat_list_time (hexchat_plugin *ph,
		 hexchat_list *xlist,
		 const char *name);

void *
hexchat_plugingui_add (hexchat_plugin *ph,
		     const char *filename,
		     const char *name,
		     const char *desc,
		     const char *version,
		     char *reserved);

void
hexchat_plugingui_remove (hexchat_plugin *ph,
			void *handle);

int 
hexchat_emit_print (hexchat_plugin *ph,
		  const char *event_name, ...);

int 
hexchat_emit_print_attrs (hexchat_plugin *ph, hexchat_event_attrs *attrs,
						  const char *event_name, ...);

char *
hexchat_gettext (hexchat_plugin *ph,
	       const char *msgid);

void
hexchat_send_modes (hexchat_plugin *ph,
		  const char **targets,
		  int ntargets,
		  int modes_per_line,
		  char sign,
		  char mode);

char *
hexchat_strip (hexchat_plugin *ph,
	     const char *str,
	     int len,
	     int flags);

void
hexchat_free (hexchat_plugin *ph,
	    void *ptr);

int
hexchat_pluginpref_set_str (hexchat_plugin *ph,
		const char *var,
		const char *value);

int
hexchat_pluginpref_get_str (hexchat_plugin *ph,
		const char *var,
		char *dest);

int
hexchat_pluginpref_set_int (hexchat_plugin *ph,
		const char *var,
		int value);
int
hexchat_pluginpref_get_int (hexchat_plugin *ph,
		const char *var);

int
hexchat_pluginpref_delete (hexchat_plugin *ph,
		const char *var);

int
hexchat_pluginpref_list (hexchat_plugin *ph,
		char *dest);

#if !defined(PLUGIN_C) && defined(WIN32)
#ifndef HEXCHAT_PLUGIN_HANDLE
#define HEXCHAT_PLUGIN_HANDLE (ph)
#endif
#define hexchat_hook_command ((HEXCHAT_PLUGIN_HANDLE)->hexchat_hook_command)
#define hexchat_event_attrs_create ((HEXCHAT_PLUGIN_HANDLE)->hexchat_event_attrs_create)
#define hexchat_event_attrs_free ((HEXCHAT_PLUGIN_HANDLE)->hexchat_event_attrs_free)
#define hexchat_hook_server ((HEXCHAT_PLUGIN_HANDLE)->hexchat_hook_server)
#define hexchat_hook_server_attrs ((HEXCHAT_PLUGIN_HANDLE)->hexchat_hook_server_attrs)
#define hexchat_hook_print ((HEXCHAT_PLUGIN_HANDLE)->hexchat_hook_print)
#define hexchat_hook_print_attrs ((HEXCHAT_PLUGIN_HANDLE)->hexchat_hook_print_attrs)
#define hexchat_hook_timer ((HEXCHAT_PLUGIN_HANDLE)->hexchat_hook_timer)
#define hexchat_hook_fd ((HEXCHAT_PLUGIN_HANDLE)->hexchat_hook_fd)
#define hexchat_unhook ((HEXCHAT_PLUGIN_HANDLE)->hexchat_unhook)
#define hexchat_print ((HEXCHAT_PLUGIN_HANDLE)->hexchat_print)
#define hexchat_printf ((HEXCHAT_PLUGIN_HANDLE)->hexchat_printf)
#define hexchat_command ((HEXCHAT_PLUGIN_HANDLE)->hexchat_command)
#define hexchat_commandf ((HEXCHAT_PLUGIN_HANDLE)->hexchat_commandf)
#define hexchat_nickcmp ((HEXCHAT_PLUGIN_HANDLE)->hexchat_nickcmp)
#define hexchat_set_context ((HEXCHAT_PLUGIN_HANDLE)->hexchat_set_context)
#define hexchat_find_context ((HEXCHAT_PLUGIN_HANDLE)->hexchat_find_context)
#define hexchat_get_context ((HEXCHAT_PLUGIN_HANDLE)->hexchat_get_context)
#define hexchat_get_info ((HEXCHAT_PLUGIN_HANDLE)->hexchat_get_info)
#define hexchat_get_prefs ((HEXCHAT_PLUGIN_HANDLE)->hexchat_get_prefs)
#define hexchat_list_get ((HEXCHAT_PLUGIN_HANDLE)->hexchat_list_get)
#define hexchat_list_free ((HEXCHAT_PLUGIN_HANDLE)->hexchat_list_free)
#define hexchat_list_fields ((HEXCHAT_PLUGIN_HANDLE)->hexchat_list_fields)
#define hexchat_list_next ((HEXCHAT_PLUGIN_HANDLE)->hexchat_list_next)
#define hexchat_list_str ((HEXCHAT_PLUGIN_HANDLE)->hexchat_list_str)
#define hexchat_list_int ((HEXCHAT_PLUGIN_HANDLE)->hexchat_list_int)
#define hexchat_plugingui_add ((HEXCHAT_PLUGIN_HANDLE)->hexchat_plugingui_add)
#define hexchat_plugingui_remove ((HEXCHAT_PLUGIN_HANDLE)->hexchat_plugingui_remove)
#define hexchat_emit_print ((HEXCHAT_PLUGIN_HANDLE)->hexchat_emit_print)
#define hexchat_emit_print_attrs ((HEXCHAT_PLUGIN_HANDLE)->hexchat_emit_print_attrs)
#define hexchat_list_time ((HEXCHAT_PLUGIN_HANDLE)->hexchat_list_time)
#define hexchat_gettext ((HEXCHAT_PLUGIN_HANDLE)->hexchat_gettext)
#define hexchat_send_modes ((HEXCHAT_PLUGIN_HANDLE)->hexchat_send_modes)
#define hexchat_strip ((HEXCHAT_PLUGIN_HANDLE)->hexchat_strip)
#define hexchat_free ((HEXCHAT_PLUGIN_HANDLE)->hexchat_free)
#define hexchat_pluginpref_set_str ((HEXCHAT_PLUGIN_HANDLE)->hexchat_pluginpref_set_str)
#define hexchat_pluginpref_get_str ((HEXCHAT_PLUGIN_HANDLE)->hexchat_pluginpref_get_str)
#define hexchat_pluginpref_set_int ((HEXCHAT_PLUGIN_HANDLE)->hexchat_pluginpref_set_int)
#define hexchat_pluginpref_get_int ((HEXCHAT_PLUGIN_HANDLE)->hexchat_pluginpref_get_int)
#define hexchat_pluginpref_delete ((HEXCHAT_PLUGIN_HANDLE)->hexchat_pluginpref_delete)
#define hexchat_pluginpref_list ((HEXCHAT_PLUGIN_HANDLE)->hexchat_pluginpref_list)
#endif

#ifdef __cplusplus
}
#endif
#endif
