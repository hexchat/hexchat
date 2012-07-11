/* 
 * X-Chat 2.0 LUA Plugin
 *
 * Copyright (c) 2007 Hanno Hecker
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */
/*
 *	$Id: lua.c 91 2007-06-09 18:44:03Z vetinari $
 *	$Revision: 91 $
 *	$Date: 2007-06-09 20:44:03 +0200 (Szo, 09 jún. 2007) $
 */
/* 
 * TODO:
 *   * compile (was OK)/run on IRIX
 *   ? localize error msgs? ... maybe later
 *   ? make xchat.print() like print() which does an tostring() on 
 *     everything it gets?
 *   ? add /LUA -s <code>? ... add a new script from cmdline... this state
 *        is not removed after the pcall(), but prints a name, which may
 *        be used to unload this virtual script. ... no xchat_register(),
 *        xchat_init() should be needed
 *        ... don't disable xchat.hook_* for this
 *   ? timer name per state/script and not per plugin?
 */
#define LXC_NAME "Lua"
#define LXC_DESC "Lua scripting interface"
#define LXC_VERSION "0.7 (r91)"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#ifdef _WIN32
#include <direct.h>	/* for getcwd */
#include "../../src/dirent/dirent-win32.h"
#endif

#if !( defined(_WIN32) || defined(LXC_XCHAT_GETTEXT) )
#  include <libintl.h>
#endif

#ifndef PATH_MAX /* hurd */
# define PATH_MAX 1024 
#endif

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define lua_pop(L,n)  lua_settop(L, -(n)-1)

#include "xchat-plugin.h"

static xchat_plugin *ph; /* plugin handle */

#define LXC_STRIP_COLOR 1
#define LXC_STRIP_ATTR  2
#define LXC_STRIP_ALL   (LXC_STRIP_COLOR|LXC_STRIP_ATTR)

/* registered hooks */
struct lxc_hooks {
	const char *name;
	xchat_hook *hook;
	struct lxc_hooks *next;
};

/* single linked list of all lua states^Wscripts ;-)  */
struct lxc_States {
	lua_State *state;          /* the lua state of the script  */
	char file[PATH_MAX+1];     /* the file name of the script  */
	struct lxc_hooks *hooks;   /* all hooks this script registered */
	void *gui;				/* the gui entry in windows->plugins and scripts... */
	struct lxc_States *next;
};

static struct lxc_States *lxc_states = NULL;

/* user/script supplied data for a callback */
struct lxc_userdata {
	int idx;					/* table index */
	int type;				/* lua type:                          */	
	const char *string;  /* only strings, ...                  */
	double num;          /* numbers and booleans are supported */
	struct lxc_userdata *next;	
};

/* callback data */
struct lxc_cbdata {
	lua_State *state;
	const char *func;
	xchat_hook *hook; /* timer ... */
	struct lxc_userdata *data;
};

static char lxc_event_name[1024] = "\0";

static int lxc_run_hook(char *word[], char *word_eol[], void *data);
static int lxc_run_print(char *word[], void *data);
static int lxc_run_timer(void *data);

static int lxc_hook_command(lua_State *L);
static int lxc_hook_server(lua_State *L);
static int lxc_hook_print(lua_State *L);
static int lxc_event(lua_State *L);
static int lxc_hook_timer(lua_State *L);
static int lxc_unhook(lua_State *L);

static int lxc_command(lua_State *L);
static int lxc_print(lua_State *L);
static int lxc_emit_print(lua_State *L);
static int lxc_send_modes(lua_State *L);
static int lxc_find_context(lua_State *L);
static int lxc_get_context(lua_State *L);
static int lxc_get_info(lua_State *L);
static int lxc_get_prefs(lua_State *L);
static int lxc_set_context(lua_State *L);
static int lxc_nickcmp(lua_State *L);

static int lxc_list_get(lua_State *L);
static int lxc_list_fields(lua_State *L);
static int lxc_gettext(lua_State *L);

static int lxc_bits(lua_State *L);

static luaL_reg lxc_functions[] = {
	{"hook_command",		lxc_hook_command },
/* TODO:
	{"hook_fd",				lxc_hook_fd      },
*/
	{"hook_print",			lxc_hook_print   },
	{"hook_server",		lxc_hook_server  },
	{"hook_timer",			lxc_hook_timer  },
	{"unhook",				lxc_unhook  },

	{"event",				lxc_event   },

	{"command",				lxc_command  	  },
	{"print", 				lxc_print     	  },
	{"emit_print",			lxc_emit_print },
	{"send_modes",			lxc_send_modes },
	{"find_context",		lxc_find_context },
	{"get_context",		lxc_get_context },
	{"get_info",			lxc_get_info },
	{"get_prefs",			lxc_get_prefs },
	{"set_context",		lxc_set_context },

	{"nickcmp", 			lxc_nickcmp 	},

	{"list_get",			lxc_list_get },
 	{"list_fields",		lxc_list_fields }, 

   {"gettext",				lxc_gettext},
/* helper function for bit flags */
	{"bits",					lxc_bits },
	{NULL, NULL}
};

static struct {
	const char *name;
	long value;
} lxc_consts[] = {
	{"EAT_NONE", 	XCHAT_EAT_NONE},
	{"EAT_XCHAT", 	XCHAT_EAT_XCHAT},
	{"EAT_PLUGIN",	XCHAT_EAT_PLUGIN},
	{"EAT_ALL",		XCHAT_EAT_ALL},

/* unused until hook_fd is done 
	{"FD_READ",			XCHAT_FD_READ},
	{"FD_WRITE",		XCHAT_FD_WRITE},
	{"FD_EXCEPTION",	XCHAT_FD_EXCEPTION},
	{"FD_NOTSOCKET",	XCHAT_FD_NOTSOCKET},
   */

	{"PRI_HIGHEST", 	XCHAT_PRI_HIGHEST},
	{"PRI_HIGH", 		XCHAT_PRI_HIGH},
	{"PRI_NORM", 		XCHAT_PRI_NORM},
	{"PRI_LOW", 		XCHAT_PRI_LOW},
	{"PRI_LOWEST", 	XCHAT_PRI_LOWEST},

	/* for: clean = xchat.strip(dirty, xchat.STRIP_ALL) */
	{"STRIP_COLOR",	LXC_STRIP_COLOR},
	{"STRIP_ATTR",    LXC_STRIP_ATTR},
	{"STRIP_ALL",     LXC_STRIP_ALL},

   /* for xchat.commandf("GUI COLOR %d", xchat.TAB_HILIGHT) */
	{"TAB_DEFAULT",  0},
	{"TAB_NEWDATA",  1},
	{"TAB_NEWMSG",   2},
	{"TAB_HILIGHT",  3},

	{NULL,				0}
};


#ifdef DEBUG
static void stackDump (lua_State *L, const char *msg) {
	int i, t;
	int top = lua_gettop(L);

	fprintf(stderr, "%s\n", msg);
	for (i = 1; i <= top; i++) {  /* repeat for each level */
	 t = lua_type(L, i);
	 switch (t) {

		case LUA_TSTRING:  /* strings */
		  fprintf(stderr, "`%s'", lua_tostring(L, i));
		  break;

		case LUA_TBOOLEAN:  /* booleans */
		  fprintf(stderr, lua_toboolean(L, i) ? "true" : "false");
		  break;

		case LUA_TNUMBER:  /* numbers */
		  fprintf(stderr, "%g", lua_tonumber(L, i));
		  break;

		default:  /* other values */
		  fprintf(stderr, "%s", lua_typename(L, t));
		  break;

	 }
	 fprintf(stderr, "  ");  /* put a separator */
  }
  fprintf(stderr, "\n");  /* end the listing */
}
#endif /* DEBUG */

static int lxc__newindex(lua_State *L)
{
	int i;
	const char *name = lua_tostring(L, 2);

	luaL_getmetatable(L, "xchat"); /* 4 */

	lua_pushnil(L);                /* 5 */
	while (lua_next(L, 4) != 0) {
		if ((lua_type(L, -2) == LUA_TSTRING) 
					&& strcmp("__index", lua_tostring(L, -2)) == 0)
				break; /* now __index is 5, table 6 */	
		lua_pop(L, 1);
	}

	lua_pushnil(L);
	while (lua_next(L, 6) != 0) {
		if ((lua_type(L, -2) == LUA_TSTRING)
				&& strcmp(name, lua_tostring(L, -2)) == 0) {
			for (i=0; lxc_consts[i].name; i++) {
				if (strcmp(name, lxc_consts[i].name) == 0) {
					luaL_error(L, 
						"`xchat.%s' is a readonly constant", lua_tostring(L, 2));
					return 0;
				}
			}
		}
		lua_pop(L, 1);
	}

	lua_pushvalue(L, 2);
	lua_pushvalue(L, 3);
	lua_rawset(L, 6);

	lua_settop(L, 1);
	return 0;
}

static int luaopen_xchat(lua_State *L)
{
	int i;
/* 
 * wrappers for xchat.printf() and xchat.commandf() 
 * ... xchat.strip 
 */
#define LXC_WRAPPERS  "function xchat.printf(...)\n" \
						 "    xchat.print(string.format(unpack(arg)))\n" \
						 "end\n" \
						 "function xchat.commandf(...)\n" \
						 "    xchat.command(string.format(unpack(arg)))\n" \
						 "end\n" \
						 "function xchat.strip(str, flags)\n" \
						 "    if flags == nil then\n" \
						 "        flags = xchat.STRIP_ALL\n" \
						 "    end\n" \
						 "    local bits = xchat.bits(flags)\n" \
						 "    if bits[1] then\n" \
						 "        str = string.gsub(\n" \
						 "            string.gsub(str, \"\\3%d%d?,%d%d?\", \"\"),\n" \
						 "                \"\\3%d%d?\", \"\")\n" \
						 "    end\n" \
						 "    if bits[2] then\n" \
						 "        -- bold, beep, reset, reverse, underline\n" \
						 "        str = string.gsub(str,\n" \
						 "            \"[\\2\\7\\15\\22\\31]\", \"\")\n" \
						 "    end\n" \
						 "    return str\n" \
						 "end\n"

#if defined(LUA_VERSION_NUM) && (LUA_VERSION_NUM >= 501)
	luaL_register(L, "xchat", lxc_functions);
	(void)luaL_dostring(L, LXC_WRAPPERS);
#else
	luaL_openlib(L, "xchat", lxc_functions, 0);
	lua_dostring(L, LXC_WRAPPERS);
#endif
	
	luaL_newmetatable(L, "xchat");

	lua_pushliteral(L, "__index");
	lua_newtable(L); 

	lua_pushstring(L, "ARCH");
#ifdef _WIN32
	lua_pushstring(L, "Windows");
#else
	lua_pushstring(L, "Unix");
#endif
	lua_settable(L, -3); /* add to table __index */

	for (i=0; lxc_consts[i].name; i++) {
		lua_pushstring(L, lxc_consts[i].name);
		lua_pushnumber(L, lxc_consts[i].value);
		lua_settable(L, -3); /* add to table __index */
	}
	lua_settable(L, -3); /* add to metatable */

	lua_pushliteral(L, "__newindex");   
	lua_pushcfunction(L, lxc__newindex); 
	lua_settable(L, -3);                
/* 
	lua_pushliteral(L, "__metatable");
	lua_pushstring(L, "nothing to see here, move along");
	lua_settable(L, -3);               
*/
	lua_setmetatable(L, -2);
	lua_pop(L, 1);
	return 1;
}

lua_State *lxc_new_state() 
{
#if defined(LUA_VERSION_NUM) && (LUA_VERSION_NUM >= 501)
	lua_State *L = luaL_newstate();     /* opens Lua */
	luaL_openlibs(L);
#else
	lua_State *L = lua_open();     /* opens Lua */
	luaopen_base(L);    /* opens the basic library */
	luaopen_table(L);   /* opens the table library */
	luaopen_io(L);      /* opens the I/O library */
	luaopen_string(L);  /* opens the string lib. */
	luaopen_math(L);    /* opens the math lib. */
#endif

	luaopen_xchat(L);
	return L;
}

static int
lxc_load_file(const char *script)
{
	lua_State *L;
	struct lxc_States *state;  /* pointer to lua states list */
	struct lxc_States *st;  /* pointer to lua states list */

	L = lxc_new_state();
	state = malloc(sizeof(struct lxc_States));
	if (state == NULL) {
		xchat_printf(ph, "malloc() failed: %s\n", strerror(errno));
		lua_close(L);
		return 0;
	}

	state->state = L;
	snprintf(state->file, PATH_MAX, script);
	state->next  = NULL;
	state->hooks = NULL;
	state->gui   = NULL;

	if (luaL_loadfile(L, script) || lua_pcall(L, 0, 0, 0)) {
		xchat_printf(ph, "Lua plugin: error loading script %s", 	
							lua_tostring(L, -1));
		lua_close(L);
		free(state);
		return 0;
	}

	if (!lxc_states) 
		lxc_states = state;
	else {
		st = lxc_states;
		while (st->next)
			st = st->next;
		st->next = state;
	}

	return 1;
}

static void
lxc_autoload_from_path(const char *path)
{
	DIR *dir;
	struct dirent *ent;
	char *file;
	int len;
	/* xchat_printf(ph, "loading from %s\n", path); */
	dir = opendir(path);
	if (dir) {
		while ((ent = readdir(dir))) {
			len = strlen(ent->d_name);
			if (len > 4 && strcasecmp(".lua", ent->d_name + len - 4) == 0) {
				file = malloc(len + strlen(path) + 2);
				if (file == NULL) {
					xchat_printf(ph, "lxc_autoload_from_path(): malloc failed: %s",
						strerror(errno));
					break;
				}
				sprintf(file, "%s/%s", path, ent->d_name);
				(void)lxc_load_file((const char *)file);
				free(file);
			}
		}
		closedir(dir);
	}
}

void lxc_unload_script(struct lxc_States *state)
{
	struct lxc_hooks *hooks, *h;
	struct lxc_cbdata *cb;
	struct lxc_userdata *ud, *u;
	lua_State *L = state->state;

	lua_pushstring(L, "xchat_unload");
	lua_gettable(L, LUA_GLOBALSINDEX);
	if (lua_type(L, -1) == LUA_TFUNCTION) {
		if (lua_pcall(L, 0, 0, 0)) {
			xchat_printf(ph, "Lua plugin: error while unloading script %s", 	
								lua_tostring(L, -1));
			lua_pop(L, 1);
		}
	}

	if (state->gui)
		xchat_plugingui_remove(ph, state->gui);
	state->gui = NULL;

	hooks = state->hooks;
	while (hooks) {
		h     = hooks;
		hooks = hooks->next;

		cb    = xchat_unhook(ph, h->hook);
		if (cb) {
			ud    = cb->data;
			while (ud) {
				u  = ud;
				ud = ud->next;
				free(u);
			}
			free(cb);
		}

		free(h);
	}
	lua_close(state->state);
}


static int lxc_cb_load(char *word[], char *word_eol[], void *userdata)
{
	int len;
	struct lxc_States *state;
	lua_State *L;
	const char *name, *desc, *vers;
	const char *xdir = "";
	char  *buf;
	char file[PATH_MAX+1];
	struct stat *st;

	if (word_eol[2][0] == 0)
		return XCHAT_EAT_NONE;
	
	buf = malloc(PATH_MAX + 1);
	if (!buf) {
		xchat_printf(ph, "malloc() failed: %s\n", strerror(errno));
		return XCHAT_EAT_NONE;
	}

	st = malloc(sizeof(struct stat));
	if (!st) {
		xchat_printf(ph, "malloc() failed: %s\n", strerror(errno));
		free(buf);
		return XCHAT_EAT_NONE;
	}

 	len = strlen(word[2]);
	if (len > 4 && strcasecmp (".lua", word[2] + len - 4) == 0) {
#ifdef WIN32
		if (strrchr(word[2], '\\') != NULL)
#else
		if (strrchr(word[2], '/') != NULL)
#endif
			strncpy(file, word[2], PATH_MAX);
		else {
			if (stat(word[2], st) == 0)
				xdir = getcwd(buf, PATH_MAX);
			else {
				xdir = xchat_get_info(ph, "xchatdirfs");
				if (!xdir) /* xchatdirfs is new for 2.0.9, will fail on older */
					xdir = xchat_get_info (ph, "xchatdir");
			}
			snprintf(file, PATH_MAX, "%s/%s", xdir, word[2]);
		}

		if (lxc_load_file((const char *)file) == 0) {
			free(st);
			free(buf);
			return XCHAT_EAT_ALL;
		}

		state = lxc_states;
		while (state) {
			if (state->next == NULL) {
				L = state->state;

				lua_pushstring(L, "xchat_register");
				lua_gettable(L, LUA_GLOBALSINDEX);
				if (lua_pcall(L, 0, 3, 0)) {
					xchat_printf(ph, "Lua plugin: error registering script %s", 	
								lua_tostring(L, -1));
					lua_pop(L, 1);
					free(st);
					free(buf);
					return XCHAT_EAT_ALL;
				}

				name = lua_tostring(L, -3);
				desc = lua_tostring(L, -2);
				vers = lua_tostring(L, -1);
				lua_pop(L, 4); /* func + 3 ret value */
				state->gui = xchat_plugingui_add(ph, state->file, 
																 name, desc, vers, NULL
															);

				lua_pushstring(L, "xchat_init");
				lua_gettable(L, LUA_GLOBALSINDEX);
				if (lua_type(L, -1) != LUA_TFUNCTION) 
					lua_pop(L, 1);
				else {
					if (lua_pcall(L, 0, 0, 0)) {
						xchat_printf(ph, 
									"Lua plugin: error calling xchat_init() %s", 	
									lua_tostring(L, -1));
						lua_pop(L, 1);
					}
				}
				free(st);
				free(buf);
				return XCHAT_EAT_ALL;
			}
			state = state->next;
		}
	}
	free(st);
	free(buf);
	return XCHAT_EAT_NONE;
}

static int lxc_cb_unload(char *word[], char *word_eol[], void *userdata)
{
	int len;
	struct lxc_States *state;
	struct lxc_States *prev = NULL;
	char *file;

	if (word_eol[2][0] == 0)
		return XCHAT_EAT_NONE;

	len = strlen(word[2]);
	if (len > 4 && strcasecmp(".lua", word[2] + len - 4) == 0) {
		state = lxc_states;
		while (state) {
			/* 
			 * state->file is the full or relative path, always with a '/' inside,
			 * even if loaded via '/LOAD script.lua'. So strrchr() will never
			 * be NULL.
			 * ... we just inspect the script name w/o path to see if it's the 
			 * right one to unload
			 */
			file = strrchr(state->file, '/') + 1; 
			if ((strcmp(state->file, word[2]) == 0) 
					|| (strcasecmp(file, word[2]) == 0)) {
				lxc_unload_script(state);
				if (prev) 
					prev->next = state->next;
				else
					lxc_states = state->next;
				xchat_printf(ph, "Lua script %s unloaded", file);
				free(state);
				return XCHAT_EAT_ALL;
			}
			prev  = state;
			state = state->next;
		}
	}
	return XCHAT_EAT_NONE;
}

static int lxc_cb_lua(char *word[], char *word_eol[], void *userdata)
{
	lua_State *L = lxc_new_state();
	if (word[2][0] == '\0') {
		xchat_printf(ph, "LUA: Usage: /LUA LUA_CODE... execute LUA_CODE");
		return XCHAT_EAT_ALL;
	}
	if (luaL_loadbuffer(L, word_eol[2], strlen(word_eol[2]), "/LUA")) {
		xchat_printf(ph, "LUA: error loading line %s", lua_tostring(L, -1));
		lua_pop(L, 1);
	}

#define LXC_HOOK_DISABLE "xchat.hook_command = nil\n" \
								 "xchat.hook_server  = nil\n" \
								 "xchat.hook_print   = nil\n" \
								 "xchat.hook_timer   = nil\n"

#if defined(LUA_VERSION_NUM) && (LUA_VERSION_NUM >= 501)
	(void)luaL_dostring(L, LXC_HOOK_DISABLE);
#else
	lua_dostring(L, LXC_HOOK_DISABLE);
#endif

	if (lua_pcall(L, 0, 0, 0)) {
		xchat_printf(ph, "LUA: error executing line %s", lua_tostring(L, -1));
		lua_pop(L, 1);
	}

	lua_close(L);
	return XCHAT_EAT_ALL;
}

int xchat_plugin_init(xchat_plugin *plugin_handle,
                      char **plugin_name,
                      char **plugin_desc,
                      char **plugin_version,
                      char *arg)
{
	struct lxc_States	*state;	
	lua_State *L;
	const char *xdir;
	const char *name, *desc, *vers;
   /* we need to save this for use with any xchat_* functions */
   ph = plugin_handle;

   /* tell xchat our info */
   *plugin_name = LXC_NAME;
   *plugin_desc = LXC_DESC;
   *plugin_version = LXC_VERSION;

	xchat_hook_command(ph, "LOAD", XCHAT_PRI_NORM, lxc_cb_load, NULL, NULL);
	xchat_hook_command(ph, "UNLOAD", XCHAT_PRI_NORM, lxc_cb_unload, NULL, NULL);
	xchat_hook_command(ph, "LUA", XCHAT_PRI_NORM, lxc_cb_lua, "Usage: LUA <code>, executes <code> in a new lua state", NULL);

	xdir = xchat_get_info(ph, "xchatdirfs");
	if (!xdir) /* xchatdirfs is new for 2.0.9, will fail on older */
		xdir = xchat_get_info (ph, "xchatdir");
		
	lxc_autoload_from_path(xdir);

	if (!lxc_states) /* no scripts loaded */
		return 1;
	
	state = lxc_states;
	while (state) {
		L = state->state;
		lua_pushstring(L, "xchat_register");
		lua_gettable(L, LUA_GLOBALSINDEX);
		if (lua_pcall(L, 0, 3, 0)) {
			xchat_printf(ph, "Lua plugin: error registering script %s", 	
								lua_tostring(L, -1));
			lua_pop(L, 1);
			state = state->next;
			continue;
		}

		name = lua_tostring(L, -3);
		desc = lua_tostring(L, -2);
		vers = lua_tostring(L, -1);
		lua_pop(L, 4); /* func + 3 ret value */
		state->gui = xchat_plugingui_add(ph, state->file, name, desc, vers, NULL);

		lua_pushstring(L, "xchat_init");
		lua_gettable(L, LUA_GLOBALSINDEX);
		if (lua_type(L, -1) != LUA_TFUNCTION) 
			lua_pop(L, 1);
		else {
			if (lua_pcall(L, 0, 0, 0)) {
				xchat_printf(ph, "Lua plugin: error calling xchat_init() %s", 	
								lua_tostring(L, -1));
				lua_pop(L, 1);
			}
		}
		state = state->next;
	}
	xchat_printf(ph, "Lua interface (v%s) loaded", LXC_VERSION);
	return 1; 
}

int xchat_plugin_deinit(xchat_plugin *plug_handle) 
{
	struct lxc_States *state, *st;

	state = lxc_states;
	while (state) {
		lxc_unload_script(state);
		xchat_printf(ph, "Lua script %s unloaded", state->file);
		st    = state;
		state = state->next;
		free(st);
	}
	xchat_printf(plug_handle, "Lua plugin v%s removed", LXC_VERSION);
	return 1;
}

/*
 * lua:  func_name(word, word_eol, data)
 * desc: your previously hooked callback function for hook_command() and
 *       hook_server(), you must return one of the xchat.EAT_* constants
 * ret:  none
 * args: 
 *       * word (table): the incoming line split into words (max 32)
 *       * word_eol (table): 
 *         for both see 
 *          http://xchat.org/docs/plugin20.html#word
 *       * data (table): the data table you passed to the hook_command() /
 *         hook_server() as 5th arg
 */
static int lxc_run_hook(char *word[], char *word_eol[], void *data)
{
	struct lxc_cbdata *cb   = data;
	lua_State *L            = cb->state;
	struct lxc_userdata *ud = cb->data;
	struct lxc_userdata *u;
	int i;
	lua_pushstring(L, cb->func);
	lua_gettable(L, LUA_GLOBALSINDEX);

	strcpy(lxc_event_name, word[0]);
	lua_newtable(L);
	for (i=1; i<=31 && word[i][0]; i++) {
		lua_pushnumber(L, i);
		lua_pushstring(L, word[i]);
		lua_settable(L, -3);
	}

	lua_newtable(L);
	for (i=1; i<=31 && word_eol[i][0]; i++) {
		lua_pushnumber(L, i);
		lua_pushstring(L, word_eol[i]);
		lua_settable(L, -3);
	}

	lua_newtable(L);
	u = ud;
	while (u) {
		lua_pushnumber(L, u->idx);
		switch (u->type) {
			case LUA_TSTRING:
				lua_pushstring(L, u->string);
				break;
			case LUA_TNUMBER:
				lua_pushnumber(L, u->num);
				break;
			case LUA_TBOOLEAN:
				lua_pushboolean(L, (((int)u->num == 0) ? 0 : 1));
				break;
			default: /* LUA_TNIL or others */
				lua_pushnil(L);
				break;
		}
		lua_settable(L, -3);
		u = u->next;
	}

	if (lua_pcall(L, 3, 1, 0)) {
		xchat_printf(ph, "failed to call callback for '%s': %s", 
				word[1], lua_tostring(L, -1)
			);
		lua_pop(L, 1);
		return XCHAT_EAT_NONE;
	}

	if (lua_type(L, -1) != LUA_TNUMBER) {
		xchat_printf(ph, "callback for '%s' did not return number...", word[1]);
		return XCHAT_EAT_NONE;
	}

	i = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);
	return i;
}

static int lxc_get_userdata(int pos, struct lxc_cbdata *cb)
{
	struct lxc_userdata *ud, *u;
	lua_State *L = cb->state;
	int i, t;

	t = lua_type(L, pos);
	if (t == LUA_TNIL)
		return 1;
	if (t != LUA_TTABLE)
		return 0;

	i = 1;
	while (1) {
		lua_pushnumber(L, i);
		lua_gettable(L, -2);

		t = lua_type(L, -1);
		if (t == LUA_TNIL) {
			lua_pop(L, 1);
			break;
		}

		ud = malloc(sizeof(struct lxc_userdata));
		if (!ud) {
			xchat_printf(ph, "lxc_get_userdata(): failed to malloc: %s", 
																strerror(errno));
			if (cb->data != NULL) {
				ud = cb->data;
				while (ud) {
					u  = ud;
					ud = ud->next;
					free(u);
				}
			}
			/* free(cb); NO! */
			lua_pushnil(L);
			return 0;
		}
		ud->idx = i;
		ud->next = NULL;
		switch (t) {
			case LUA_TSTRING:
				ud->string = lua_tostring(L, -1);
				ud->type   = LUA_TSTRING;
				break;
			case LUA_TNUMBER:
				ud->num    = lua_tonumber(L, -1);
				ud->type   = LUA_TNUMBER;
				break;
			case LUA_TBOOLEAN:
				ud->num    = (double)lua_toboolean(L, -1);
				ud->type   = LUA_TBOOLEAN;
				break;
			default:
				ud->type   = LUA_TNIL;
				break;
		}
		lua_pop(L, 1);

		if (cb->data == NULL)
			cb->data = ud;
		else {
			u = cb->data;
			while (u->next)
				u = u->next;
			u->next = ud;
		}
		i++;
	} /* END while (1) */
	return 1;
}

/* 
 * lua:  xchat.hook_command(name, func_name, prio, help_str, data)
 * desc: Adds a new /command. This allows your program to handle commands 
 *       entered at the input box. To capture text without a "/" at the start 
 *       (non-commands), you may hook a special name of "". i.e 
 *           xchat.hook_command( "", ...)
 * 		Starting from version 2.6.8, commands hooked that begin with a 
 * 		period ('.') will be hidden in /HELP and /HELP -l. 
 * ret:  true... or false if something went wrong while registering hook
 * args: 
 *       * name (string): the name of the new command
 *       * func_name (string): the lua function to be called when command is
 *          entered
 *       * prio (number): use one of the xchat.PRIO_*
 *       * help_str (string): help for the new command... use nil for no help
 *       * data (table): table with strings, numbers and booleans, which will
 *         be passed to func_name as last argument.
 */
static int lxc_hook_command(lua_State *L)
{
	xchat_hook *hook;
	const char *help, *command, *func;
	double prio;
	struct lxc_hooks *hooks, *h;
	struct lxc_States *st;
	struct lxc_cbdata *cb;


	if (lua_gettop(L) < 5) /* expand to five args if necessary */
		lua_settop(L, 5);

	cb = malloc(sizeof(struct lxc_cbdata));
	if (!cb) {
		xchat_printf(ph, "lxc_hook_command(): failed to malloc: %s", 
																	strerror(errno));
		lua_pushboolean(L, 0);
		return 1;
	}

	cb->state = L;
	cb->data = NULL;

	command 	= luaL_checkstring(L, 1);
	func     = luaL_checkstring(L, 2);
	cb->func = func;
	cb->hook = NULL;

	if (lua_type(L, 3) == LUA_TNIL)
		prio = XCHAT_PRI_NORM;
	else
		prio = luaL_checknumber(L, 3);

	if (lua_type(L, 4) == LUA_TSTRING) {
		help = luaL_checkstring(L, 4);
		if (strlen(help) == 0)
			help = NULL;
	}
	else
		help = NULL;
	
	if (lxc_get_userdata(5, cb) == 0) 
		lua_pushboolean(L, 0);
	else {
		h = malloc(sizeof(struct lxc_hooks));
		if (!h) {
			xchat_printf(ph, "lxc_hook_command(): failed to malloc: %s", 
																	strerror(errno));
			lua_pushboolean(L, 0);
			return 1;
		}
		hook    = xchat_hook_command(ph, command, prio, lxc_run_hook, help, cb);
		h->hook = hook;
		h->name = command;
		h->next = NULL;
		st      = lxc_states;
		while (st) {
			if (st->state == L) {
				if (!st->hooks) 
					st->hooks = h;
				else {
					hooks     = st->hooks;
					while (hooks->next) 
						hooks = hooks->next;
					hooks->next = h;
				}
				break;
			}
			st = st->next;
		}
		lua_pushboolean(L, 1);
	}
	return 1;
}

/*
 * lua:  func_name(word, data)
 * desc: your previously hooked callback function for hook_print(), 
 *       you must return one of the xchat.EAT_* constants
 * ret:  none
 * args: 
 *       * word (table): the incoming line split into words (max 32)
 *         (see http://xchat.org/docs/plugin20.html#word)
 *       * data (table): the data table you passed to the hook_print() /
 *         as 4th arg
 */
static int lxc_run_print(char *word[], void *data)
{
	struct lxc_cbdata *cb = data;
	lua_State *L          = cb->state;
	int i;

	lua_pushstring(L, cb->func);
	lua_gettable(L, LUA_GLOBALSINDEX);

	strcpy(lxc_event_name, word[0]);
	lua_newtable(L);
	for (i=1; i<=31 && word[i][0]; i++) {
		lua_pushnumber(L, i);
		lua_pushstring(L, word[i]);
		lua_settable(L, -3);
	}

	if (lua_pcall(L, 1, 1, 0)) {
		xchat_printf(ph, "failed to call callback for '%s': %s", 
			word[1], lua_tostring(L, -1));
		lua_pop(L, 1);
		return 0;
	}

	if (lua_type(L, -1) != LUA_TNUMBER) {
		xchat_printf(ph, "callback for '%s' didn't return number...", word[1]);
		return XCHAT_EAT_NONE;
	}
	i = (int)lua_tonumber(L, -1);
	lua_pop(L, 1);
	return i;
}

/* 
 * lua:  xchat.hook_print(name, func_name, prio, data)
 * desc: Registers a function to trap any print events. The event names may 
 *       be any available in the "Advanced > Text Events" window. There are 
 *       also some extra "special" events you may hook using this function,
 *       see: http://xchat.org/docs/plugin20.html#xchat_hook_print
 * ret:  true... or false if something went wrong while registering hook
 * args: 
 *       * name (string): the name of the new command
 *       * prio (number): use one of the xchat.PRIO_*
 *       * func_name (string): the lua function to be called when command is
 *         entered
 *       * data (table): table with strings, numbers and booleans, which will
 *         be passed to func_name as last argument.
 */
static int lxc_hook_print(lua_State *L)
{
	xchat_hook *hook;
	struct lxc_hooks *hooks, *h;
	struct lxc_States *st;
	struct lxc_cbdata *cb = malloc(sizeof(struct lxc_cbdata));
	const char *name, *func;
	double prio;

	if (!cb) {
		luaL_error(L, "lxc_hook_print(): failed to malloc: %s", strerror(errno));
		return 0;
	}

	if (lua_gettop(L) < 4) /* expand to 4 args if necessary */
		lua_settop(L, 4);

	name = luaL_checkstring(L, 1);
	func = luaL_checkstring(L, 2);
	if (lua_type(L, 3) == LUA_TNIL)
		prio = XCHAT_PRI_NORM;
	else
		prio = luaL_checknumber(L, 3);

	cb->state = L;
	cb->func  = func;
	cb->data  = NULL;
	cb->hook  = NULL;

	if (lxc_get_userdata(4, cb) == 0) 
		lua_pushboolean(L, 0);
	else {
		h = malloc(sizeof(struct lxc_hooks));
		if (!h) {
			xchat_printf(ph, "lxc_hook_print(): failed to malloc: %s", 
																	strerror(errno));
			lua_pushboolean(L, 0);
			return 1;
		}
		hook 	  = xchat_hook_print(ph, name, prio, lxc_run_print, cb); 
		h->hook = hook;
		h->name = name;
		h->next = NULL;
		st      = lxc_states;
		while (st) {
			if (st->state == L) {
				if (!st->hooks)
					st->hooks = h;
				else {
					hooks     = st->hooks;
					while (hooks->next) 
						hooks = hooks->next;
					hooks->next = h;
				}
				break;
			}
			st = st->next;
		}
		lua_pushboolean(L, 1);
	}
	return 1;
}

/*
 * lua:  xchat.hook_server(name, func_name, prio, data)
 * desc: Registers a function to be called when a certain server event 
 *       occurs. You can use this to trap PRIVMSG, NOTICE, PART, a server 
 *       numeric etc... If you want to hook every line that comes from the 
 *       IRC server, you may use the special name of "RAW LINE".
 * ret:  true... or false if something went wrong while registering
 * args: 
 *       * name (string): the event name / numeric (yes, also as a string)
 *       * prio (number): one of the xchat.PRIO_* constants
 *       * func_name (string): the function to be called, when the event
 *           happens
 *       * data (table)... see xchat.hook_command()
 */
static int lxc_hook_server(lua_State *L)
{
	xchat_hook *hook;
	struct lxc_hooks *hooks, *h;
	struct lxc_States *st;
	const char *name, *func;
	double prio;

	struct lxc_cbdata *cb = malloc(sizeof(struct lxc_cbdata));
	if (!cb) {
		xchat_printf(ph, "lxc_hook_server(): failed to malloc: %s", 
																	 strerror(errno));
		lua_pushnil(L);
		return 1;
	}

	if (lua_gettop(L) < 4) /* expand to 4 args if necessary */
		lua_settop(L, 4);

	name = luaL_checkstring(L, 1);
	func = luaL_checkstring(L, 2);
	if (lua_type(L, 3) == LUA_TNIL)
		prio = XCHAT_PRI_NORM;
	else
		prio = luaL_checknumber(L, 3);

	cb->state = L;
	cb->func = func;
	cb->data = NULL;
	cb->hook = NULL;

	if (lxc_get_userdata(4, cb) == 0) 
		lua_pushboolean(L, 0);
	else {
		h = malloc(sizeof(struct lxc_hooks));
		if (!h) {
			xchat_printf(ph, "lxc_hook_server(): failed to malloc: %s", 
																	strerror(errno));
			lua_pushboolean(L, 0);
			return 1;
		}
		hook    = xchat_hook_server(ph, name, prio, lxc_run_hook, cb); 
		h->hook = hook;
		h->name = name;
		h->next = NULL;
		st      = lxc_states;
		while (st) {
			if (st->state == L) {
				if (!st->hooks)
					st->hooks = h;
				else {
					hooks     = st->hooks;
					while (hooks->next) 
						hooks = hooks->next;
					hooks->next = h;
				}
				break;
			}
			st = st->next;
		}
		lua_pushboolean(L, 1);
	}
	return 1;
}

/* 
 * lua:  xchat.hook_timer(timeout, func_name, data)
 * desc: Registers a function to be called every "timeout" milliseconds.
 * ret:  true (or false on error while registering)
 * args: 
 *       * timeout (number): Timeout in milliseconds (1000 is 1 second). 
 *       * func_name (string): Callback function. This will be called 
 *           every "timeout" milliseconds. 
 *       * data (table): see xchat.hook_command()
 */

static unsigned long long lxc_timer_count = 0;

static int lxc_hook_timer(lua_State *L)
{
	xchat_hook *hook;
	struct lxc_hooks *hooks, *h;
	struct lxc_States *st;
	double timeout;
	const char *func;
	char name[32];

	struct lxc_cbdata *cb = malloc(sizeof(struct lxc_cbdata));
	if (!cb) {
		luaL_error(L, "lxc_hook_timer(): failed to malloc: %s", strerror(errno));
		lua_pushnil(L);
		return 1;
	}

	if (lua_gettop(L) < 3) /* expand to 3 args if necessary */
		lua_settop(L, 3);

	timeout = luaL_checknumber(L, 1);
	func    = luaL_checkstring(L, 2);

	cb->state = L;
	cb->func  = func;
	cb->data  = NULL;

	if (lxc_get_userdata(3, cb) == 0) 
		lua_pushnil(L);
	else {
		h = malloc(sizeof(struct lxc_hooks));
		if (!h) {
			luaL_error(L, "lxc_hook_timer(): failed to malloc: %s", 
				strerror(errno));
			return 0;
		}
		hook 	   = xchat_hook_timer(ph, timeout, lxc_run_timer, cb); 
		cb->hook = hook;
		h->hook  = hook;
		h->next  = NULL;
		snprintf(name, 31, "timer%llu", lxc_timer_count++);
		h->name  = name;
		lua_pushstring(L, name);

		st       = lxc_states;
		while (st) {
			if (st->state == L) {
				if (!st->hooks)
					st->hooks = h;
				else {
					hooks     = st->hooks;
					while (hooks->next)
						hooks = hooks->next;
					hooks->next = h;
				}
				break;
			}
			st = st->next;
		}
	}
	return 1;
}

static void lxc_unhook_timer(lua_State *L, xchat_hook *hook)
{
	struct lxc_States *state;
	struct lxc_hooks *hooks, *h, *prev_hook;
	struct lxc_cbdata *cb;
	struct lxc_userdata *ud, *u;

	prev_hook = NULL;
	state = lxc_states;
	while (state) {
		if (state->state == L) {
			hooks = state->hooks;
			while (hooks) {
				if (hooks->hook == hook) {
					h  = hooks;
					if (prev_hook)
						prev_hook->next = hooks->next;
					else
						state->hooks = hooks->next;

					cb = xchat_unhook(ph, h->hook);
					if (cb) {
						 ud = cb->data;
						 while (ud) {
							 u  = ud;
							 ud = ud->next;
							 free(u);
						 }
						 free(cb);
					}

					free(h);
					return;
				}
				prev_hook = hooks;
				hooks = hooks->next;
			}
			break;
		}
		state = state->next;
	}
}

/* 
 * lua:  func_name(data)
 * desc: the callback function for the registered timer hook, return
 *       true to keep this timer going, false to stop it
 * ret:  none
 * args: 
 *       * data (table): the table you gave the hook_timer() as last
 *           argument
 */
 static int lxc_run_timer(void *data)
{
	int ret;
	struct lxc_cbdata *cb = data;
	xchat_hook *hook      = cb->hook;
	lua_State *L          = cb->state;

	lua_pushstring(L, cb->func);
	lua_gettable(L, LUA_GLOBALSINDEX);

	if (lua_pcall(L, 0, 1, 0)) {
		xchat_printf(ph, "failed to call timer callback for '%s': %s", 
			cb->func, lua_tostring(L, -1));
		lua_pop(L, 1);
		lxc_unhook_timer(L, hook);
		return 0;
	}

	if (lua_type(L, -1) != LUA_TBOOLEAN) {
		xchat_printf(ph, 
			"timer callback for '%s' didn't return a boolean", cb->func);
		lua_pop(L, 1);
		lxc_unhook_timer(L, hook);
		return 0;
	}

	ret = (lua_toboolean(L, -1) == 0) ? 0 : 1;
	lua_pop(L, 1);

	if (ret == 0)
		lxc_unhook_timer(L, hook);

	return ret;
}

/*
 * lua:  xchat.unhook(name)
 * desc: unhooks a previously hooked hook 
 * ret:  true if the hook existed, else false..
 * args: 
 *       * name (string): name of a registered hook (e.g. with 
 *         xchat.hook_command("whois", ... ) you would unhook "whois"
 *         ... see timer warnings... there's currently just one "timer"
 *         to unhook
 */
static int lxc_unhook(lua_State *L)
{
	struct lxc_States *state;
	struct lxc_hooks *hooks, *h, *prev_hook;
	struct lxc_cbdata *cb;
	struct lxc_userdata *ud, *u;
	int done = 0;
	const char *name = luaL_checkstring(L, 1);

	prev_hook = NULL;
	state = lxc_states;
	while (state) {
		if (state->state == L) {
			hooks = state->hooks;
			while (hooks) {
				if (strcasecmp(hooks->name, name) == 0) {
					h  = hooks;
					if (prev_hook)
						prev_hook->next = hooks->next;
					else
						state->hooks = hooks->next;

					cb = xchat_unhook(ph, h->hook);
					if (cb) {
						ud = cb->data;
						while (ud) {
							u  = ud;
							ud = ud->next;
							free(u);
						}
						free(cb);
					}

					free(h);
					done = 1;
					break;
				}
				prev_hook = hooks;
				hooks = hooks->next;
			}
			break;
		}
		state = state->next;
	}
	lua_pushboolean(L, done);
	return 1;
}

static int lxc_event(lua_State *L)
{
	lua_pushstring(L, lxc_event_name);
	return 1;
}

/* 
 * lua:  xchat.command(command) 
 * desc: executes a command as if it were typed in xchat's input box. 
 * ret:  none
 * args: 
 *       * command (string): command to execute, without the forward slash "/". 
 */
static int lxc_command(lua_State *L)
{
	const char *command = luaL_checkstring(L, 1);
	xchat_command(ph, command);
	return 0;
}

/* 
 * lua:  xchat.print(text)
 * desc: Prints some text to the current tab/window.
 * ret:  none
 * args: 
 *       * text (string): the text to print
 */
static int lxc_print(lua_State *L)
{
	const char *txt = luaL_checkstring(L, 1);
	// FIXME? const char *txt = lua_tostring(L, 1);
	xchat_print(ph, txt);
	return 0;
}

/*
 * lua: xchat.emit_print(event, text, [text2, ...])
 * desc: Generates a print event. This can be any event found in the 
 *       Preferences > Advanced > Text Events window. The vararg parameter 
 *       list MUST be no longer than four (4) parameters. 
 *       Special care should be taken when calling this function inside a 
 *       print callback (from xchat.hook_print()), as not to cause endless 
 *       recursion. 
 * ret:  true on success, false on error
 * args: 
 *       * event (string): the event name from the references > Advanced > 
 *         Text Events window
 *       * text (string)
 *         text2 (string)
 *         ... (string(s)):
 *           parameters for the given event
 */
static int lxc_emit_print(lua_State *L)
{

	int n = lua_gettop(L);
	const char *text[5];
	const char *event;
	int i = 2;

	if (n > 6)
		luaL_error(L, "too many arguments to xchat.emit_print()");

	event = luaL_checkstring(L, 1);
	while (i <= n) {
		text[i-2] = luaL_checkstring(L, i);
		i++;
	}
	switch (n-1) {
		case 0:
			i = xchat_emit_print(ph, event, NULL);
			break;
		case 1:
			i = xchat_emit_print(ph, event, text[0], NULL);
			break;
		case 2:
			i = xchat_emit_print(ph, event, text[0], text[1], NULL);
			break;
		case 3:
			i = xchat_emit_print(ph, event, text[0], text[1], text[2], NULL);
			break;
		case 4:
			i = xchat_emit_print(ph, event, text[0], text[1], text[2], text[3], NULL);
			break;
	}
	lua_pushboolean(L, (i == 0) ? 0 : 1);
	return 1;
}

/* 
 * lua:  xchat.send_modes(targets, sign, mode [, modes_per_line])
 * desc: Sends a number of channel mode changes to the current channel. 
 *       For example, you can Op a whole group of people in one go. It may 
 *       send multiple MODE lines if the request doesn't fit on one. Pass 0 
 *       for modes_per_line to use the current server's maximum possible. 
 *       This function should only be called while in a channel context. 
 * ret:  none
 * args: 
 *       * targets (table): list of names
 *       * sign (string): mode sign, i.e. "+" or "-", only the first char of
 *            this is used (currently unchecked if it's really "+" or "-")
 *       * mode (string): mode char, i.e. "o" for opping, only the first 
 *            char of this is used (currently unchecked, what char)
 *       * modes_per_line (number): [optional] number of modes per line
 */
static int lxc_send_modes(lua_State *L)
{
	int i = 1;
	const char *name, *mode, *sign;
	const char *targets[4096];
	int num = 0; /* modes per line */

	if (!lua_istable(L, 1)) {
		luaL_error(L, 
			"xchat.send_modes(): first argument is not a table: %s", 
			lua_typename(L, lua_type(L, 1)));
		return 0;
	}

	while (1) {
		lua_pushnumber(L, i); /* push index on stack */
		lua_gettable(L, 1);   /* and get the element @ index */
		if (lua_isnil(L, -1)) { /* end of table */
			lua_pop(L, 1);
			break;
		}

		if (lua_type(L, -1) != LUA_TSTRING) { /* oops, something wrong */
			luaL_error(L, 
				"lua: xchat.send_modes(): table element #%d not a string: %s",
				i, lua_typename(L, lua_type(L, -1)));
			lua_pop(L, 1);
			return 0;
		}

		name = lua_tostring(L, -1);
		if (name == NULL) { /* this should not happen, but... */
			lua_pop(L, 1);
			break;
		}

		targets[i-1] = name;
		lua_pop(L, 1); /* take index from stack */
		++i;
	}

	sign = luaL_checkstring(L, 2);
	if (sign[0] == '\0' || sign[1] != '\0') {
		luaL_error(L, "argument #2 (mode sign) does not have length 1");
		return 0;
	}
	if ((sign[0] != '+') && (sign[0] != '-')) {
		luaL_error(L, "argument #2 (mode sign) is not '+' or '-'");
		return 0;
	}

	mode = luaL_checkstring(L, 3);
	if (mode[0] == '\0' || mode[1] != '\0') {
		luaL_error(L, "argument #3 (mode char) does not have length 1");
		return 0;
	}
	if (!isalpha((int)mode[0]) || !isascii((int)mode[0])) {
		luaL_error(L, "argument #3 is not a valid mode character");
		return 0;
	}

	if (lua_gettop(L) == 4)
		num = luaL_checknumber(L, 4);
	
	xchat_send_modes(ph, targets, i-1, num, sign[0], mode[0]);
	return 0;
}

/*
 * lua:  xchat.find_context(srv, chan)
 * desc: Finds a context based on a channel and servername. If servname is nil,
 *       it finds any channel (or query) by the given name. If channel is nil, 
 *       it finds the front-most tab/window of the given servname. If nil is 
 *       given for both arguments, the currently focused tab/window will be 
 *       returned.
 *       Changed in 2.6.1. If servname is nil, it finds the channel (or query)
 *       by the given name in the same server group as the current context. 
 *       If that doesn't exists then find any by the given name. 
 * ret:  context number (DON'T modify)
 * args: 
 *       * srv (string or nil): server name
 *       * chan (string or nil): channel / query name
 */
static int lxc_find_context(lua_State *L)
{
	const char *srv, *chan;
	long ctx;
	xchat_context *ptr;

	if (lua_type(L, 1) == LUA_TSTRING) {
		srv = lua_tostring(L, 1);
		if (srv[0] == '\0')
			srv = NULL;
	}
	else
		srv = NULL;
	
	if (lua_type(L, 2) == LUA_TSTRING) {
		chan = lua_tostring(L, 2);
		if (chan[0] == '\0')
			chan = NULL;
	}
	else
		chan = NULL;
	
	ptr = xchat_find_context(ph, srv, chan);
	ctx = (long)ptr;
#ifdef DEBUG
	fprintf(stderr, "find_context(): %#lx\n", (long)ptr);
#endif
	lua_pushnumber(L, (double)ctx);
	return 1;
}

/* 
 * lua:  xchat.get_context()
 * desc: Returns the current context for your plugin. You can use this later 
 *       with xchat_set_context. 
 * ret:  context number ... DON'T modifiy
 * args: none
 */
static int lxc_get_context(lua_State *L)
{
	long ptr;
	xchat_context *ctx = xchat_get_context(ph);
	ptr = (long)ctx;
#ifdef DEBUG
	fprintf(stderr, "get_context(): %#lx\n", ptr);
#endif
	lua_pushnumber(L, (double)ptr);
	return 1;
}

/* 
 * lua:  xchat.get_info(id)
 * desc: Returns information based on your current context.
 * ret:  the requested string or nil on error
 * args: 
 *       * id (string): the wanted information
 */
static int lxc_get_info(lua_State *L)
{
	const char *id    = luaL_checkstring(L, 1);
	const char *value = xchat_get_info(ph, id);
	if (value == NULL)
		lua_pushnil(L);
	else
		lua_pushstring(L, value);
	return 1;
}

/*
 * lua:  xchat.get_prefs(name)
 * desc: Provides xchat's setting information (that which is available 
 *       through the /set command). A few extra bits of information are 
 *       available that don't appear in the /set list, currently they are:
 *       * state_cursor: Current input-box cursor position (characters, 
 *                       not bytes). Since 2.4.2.
 *       *id:            Unique server id. Since 2.6.1.
 * ret:  returns the string/number/boolean for the given config var
 *       or nil on error
 * args:
 *       * name (string): the wanted setting's name
 */
static int lxc_get_prefs(lua_State *L)
{
	int i;
	const char *str;
	const char *name = luaL_checkstring(L, 1);
	/* 
	 * luckily we can store anything in a lua var... this makes the 
	 * xchat lua api more user friendly ;-) 
	 */
	switch (xchat_get_prefs(ph, name, &str, &i)) { 
		case 0: /* request failed */
			lua_pushnil(L);
			break;
		case 1: 
			lua_pushstring(L, str);
			break;
		case 2:
			lua_pushnumber(L, (double)i);
			break;
		case 3:
			lua_pushboolean(L, i);
			break;
		default: /* doesn't happen if xchat's C-API doesn't change ;-) */
			lua_pushnil(L);
			break;
	}
	return 1;
}

/*
 * lua:  xchat.set_context(ctx)
 * desc: Changes your current context to the one given. 
 * ret:  true or false 
 * args: 
 *       * ctx (number): the context (e.g. from xchat.get_context())
 */
static int lxc_set_context(lua_State *L)
{
	double ctx = luaL_checknumber(L, 1);
#ifdef DEBUG
	fprintf(stderr, "set_context(): %#lx\n", (long)ctx);
#endif
	xchat_context *xc = (void *)(long)ctx;
	lua_pushboolean(L, xchat_set_context(ph, xc));
	return 1;
}

/*
 * lua:  xchat.nickcmp(name1, name2)
 * desc: Performs a nick name comparision, based on the current server 
 *       connection. This might be a RFC1459 compliant string compare, or 
 *       plain ascii (in the case of DALNet). Use this to compare channels 
 *       and nicknames. The function works the same way as strcasecmp. 
 * ret:  number ess than, equal to, or greater than zero if name1 is found, 
 *       respectively, to be less than, to match, or be greater than name2. 
 * args:
 *       * name1 (string): nick or channel name
 *       * name2 (string): nick or channel name
 */
static int lxc_nickcmp(lua_State *L)
{
	const char *n1 = luaL_checkstring(L, 1);
	const char *n2 = luaL_checkstring(L, 2);
	lua_pushnumber(L, (double)xchat_nickcmp(ph, n1, n2));
	return 1;
}

/* 
 * lua:  xchat.list_get(name)
 * desc: http://xchat.org/docs/plugin20.html#lists :)
 *       time_t values are stored as number => e.g.
 *         os.date("%Y-%m-%d, %H:%M:%S", time_t_value)
 *       pointers (channel -> context) as number... untested if this works
 * ret:  table with tables with all keys=Name, value=(Type)... or nil on error
 * args: 
 *       * name (string): the wanted list
 */
static int lxc_list_get(lua_State *L)
{
	const char *name          = luaL_checkstring(L, 1); 
	int i; /* item index */
	int l; /* list index */
	const char *str;
	double      num;
	time_t     date;
	long        ptr;
	const char *const *fields = xchat_list_fields(ph, name);
	xchat_list *list          = xchat_list_get(ph, name);

	if (!list) {
		lua_pushnil(L);
		return 1;
	}
	lua_newtable(L);
	/* this is like the perl plugin does it ;-) */
	l = 1;
	while (xchat_list_next(ph, list)) {
		i = 0;
		lua_pushnumber(L, l);
		lua_newtable(L);
		while (fields[i] != NULL) {
			switch (fields[i][0]) {
				case 's':
					str = xchat_list_str(ph, list, fields [i] + 1);
					lua_pushstring(L, fields[i]+1);
					if (str != NULL)
						lua_pushstring(L, str);
					else 
						lua_pushnil(L);
					lua_settable(L, -3);
					break;
				case 'p':
					ptr = (long)xchat_list_str(ph, list, fields [i] + 1);
					num = (double)ptr;
					lua_pushstring(L, fields[i]+1);
					lua_pushnumber(L, num);
					lua_settable(L, -3);
					break;
				case 'i':
					num = (double)xchat_list_int(ph, list, fields[i] + 1);
					lua_pushstring(L, fields[i]+1);
					lua_pushnumber(L, num);
					lua_settable(L, -3);
					break;
				case 't':
					date = xchat_list_time(ph, list, fields[i] + 1);
					lua_pushstring(L, fields[i]+1);
					lua_pushnumber(L, (double)date);
					lua_settable(L, -3);
					break;
			}
			i++;
		}
		lua_settable(L, -3);
		l++;
	}
	xchat_list_free(ph, list);
	return 1;
}

/*
 * lua:  xchat.list_fields(name)
 * desc: returns the possible keys for name as table
 * ret:  table ;->
 * args: 
 *       * name (string): name of the wanted list ("channels", "dcc", 
 *            "ignore", "notify", "users")
 */
static int lxc_list_fields(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	const char *const *fields = xchat_list_fields(ph, name);
	int i;

	lua_newtable(L);
	i = 0;
	while (fields[i] != NULL) {
		 lua_pushnumber(L, i);
		 /* first char is the type ... */
		 lua_pushstring(L, fields[i]+1);
		 lua_settable(L, -3);
	}
	return 1;
}

/*
 * lua:  xchat.gettext(str)
 * desc: 
 */
static int lxc_gettext(lua_State *L)
{
#if defined(_WIN32) || defined(LXC_XCHAT_GETTEXT)
	lua_pushstring(L, xchat_gettext(ph, luaL_checkstring(L, 1)));
#else
	const char *dom;
	const char *msgid = luaL_checkstring(L, 1);
	if (lua_type(L,2) == LUA_TSTRING)
			dom = lua_tostring(L, 2);
	else
			dom = "xchat";
	lua_pushstring(L, dgettext(dom, msgid));
#endif
	return 1;
}

/*
 * lua:  xchat.bits(flags)
 * desc: returns a table of booleans if the bit at index (err... index-1) is
 *       set
 * ret:  table of booleans
 * args: 
 *       * flags (number)
  */
static int lxc_bits(lua_State *L)
{
	int flags = luaL_checknumber(L, 1);
	int i;
	lua_pop(L, 1);
	lua_newtable(L);
	for (i=0; i<16; i++) { /* at time of writing, the highest index was 9 ... */
		lua_pushnumber(L, i+1);
		lua_pushboolean(L, ((1<<i) & flags) == 0 ? 0 : 1);
		lua_settable(L, -3);
	}
	return 1;
}
/* 
 * vim: ts=3 noexpandtab
 */
