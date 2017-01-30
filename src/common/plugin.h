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

#ifndef HEXCHAT_COMMONPLUGIN_H
#define HEXCHAT_COMMONPLUGIN_H

#ifdef PLUGIN_C
struct _hexchat_plugin
{
	/* Keep these in sync with hexchat-plugin.h */
	/* !!don't change the order, to keep binary compat!! */
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
	void *(*hexchat_read_fd) (hexchat_plugin *ph);
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

	/* PRIVATE FIELDS! */
	void *handle;		/* from dlopen */
	char *filename;	/* loaded from */
	char *name;
	char *desc;
	char *version;
	session *context;
	void *deinit_callback;	/* pointer to hexchat_plugin_deinit */
	unsigned int fake:1;		/* fake plugin. Added by hexchat_plugingui_add() */
	unsigned int free_strings:1;		/* free name,desc,version? */
};
#endif

GModule *module_load (char *filename);
char *plugin_load (session *sess, char *filename, char *arg);
int plugin_reload (session *sess, char *name, int by_filename);
void plugin_add (session *sess, char *filename, void *handle, void *init_func, void *deinit_func, char *arg, int fake);
int plugin_kill (char *name, int by_filename);
void plugin_kill_all (void);
void plugin_auto_load (session *sess);
int plugin_emit_command (session *sess, char *name, char *word[], char *word_eol[]);
int plugin_emit_server (session *sess, char *name, char *word[], char *word_eol[],
						time_t server_time);
int plugin_emit_print (session *sess, char *word[], time_t server_time);
int plugin_emit_dummy_print (session *sess, char *name);
int plugin_emit_keypress (session *sess, unsigned int state, unsigned int keyval, gunichar key);
GList* plugin_command_list(GList *tmp_list);
int plugin_show_help (session *sess, char *cmd);
void plugin_command_foreach (session *sess, void *userdata, void (*cb) (session *sess, void *userdata, char *name, char *usage));
session *plugin_find_context (const char *servname, const char *channel, server *current_server);

#endif
