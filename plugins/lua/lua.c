/*
 * Copyright (c) 2015-2016 mniip
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 * associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <glib.h>
#include <gmodule.h>

#ifndef G_OS_WIN32
#include <pwd.h>
#endif

#include <hexchat-plugin.h>

static char plugin_name[] = "Lua";
static char plugin_description[] = "Lua scripting interface";
static char plugin_version[16] = "1.3";

static char console_tab[] = ">>lua<<";
static char command_help[] =
	"Usage: /lua load <filename>\n"
	"            unload <filename>\n"
	"            reload <filename>\n"
	"            exec <code>\n"
	"            inject <filename> <code>\n"
	"            reset\n"
	"            list\n"
	"            console";

static char registry_field[] = "plugin";

static hexchat_plugin *ph;

#if LUA_VERSION_NUM < 502
#define lua_rawlen lua_objlen
#define luaL_setfuncs(L, r, n) luaL_register(L, NULL, r)
#endif

typedef struct
{
	hexchat_hook *hook;
	lua_State *state;
	int ref;
}
hook_info;

typedef struct
{
	char *name;
	char *description;
	char *version;
	hexchat_plugin *handle;
	char *filename;
	lua_State *state;
	GPtrArray *hooks;
	GPtrArray *unload_hooks;
	int traceback;
	int status;
}
script_info;

#define STATUS_ACTIVE 1
#define STATUS_DEFERRED_UNLOAD 2
#define STATUS_DEFERRED_RELOAD 4

static void check_deferred(script_info *info);

static inline script_info *get_info(lua_State *L)
{
	script_info *info;

	lua_getfield(L, LUA_REGISTRYINDEX, registry_field);
	info = lua_touserdata(L, -1);
	lua_pop(L, 1);

	return info;
}

static int api_hexchat_register(lua_State *L)
{
	char const *name, *version, *description;
	script_info *info = get_info(L);
	if(info->name)
		return luaL_error(L, "script is already registered as '%s'", info->name);

	name = luaL_checkstring(L, 1);
	version = luaL_checkstring(L, 2);
	description = luaL_checkstring(L, 3);

	info->name = g_strdup(name);
	info->description = g_strdup(description);
	info->version = g_strdup(version);
	info->handle = hexchat_plugingui_add(ph, info->filename, info->name, info->description, info->version, NULL);

	return 0;
}

static int api_hexchat_command(lua_State *L)
{
	hexchat_command(ph, luaL_checkstring(L, 1));
	return 0;
}

static int tostring(lua_State *L, int n)
{
	luaL_checkany(L, n);
	switch (lua_type(L, n))
	{
		case LUA_TNUMBER:
			lua_pushstring(L, lua_tostring(L, n));
			break;
		case LUA_TSTRING:
			lua_pushvalue(L, n);
			break;
		case LUA_TBOOLEAN:
			lua_pushstring(L, (lua_toboolean(L, n) ? "true" : "false"));
			break;
		case LUA_TNIL:
			lua_pushliteral(L, "nil");
			break;
		default:
			lua_pushfstring(L, "%s: %p", luaL_typename(L, n), lua_topointer(L, n));
			break;
	}
	return 1;
}

static int api_hexchat_print(lua_State *L)
{
	int i, args = lua_gettop(L);
	luaL_Buffer b;
	luaL_buffinit(L, &b);
	for(i = 1; i <= args; i++)
	{
		if(i != 1)
			luaL_addstring(&b, " ");
		tostring(L, i);
		luaL_addvalue(&b);
	}
	luaL_pushresult(&b);
	hexchat_print(ph, lua_tostring(L, -1));
	return 0;
}

static int api_hexchat_emit_print(lua_State *L)
{
	hexchat_emit_print(ph, luaL_checkstring(L, 1), luaL_optstring(L, 2, NULL), luaL_optstring(L, 3, NULL), luaL_optstring(L, 4, NULL), luaL_optstring(L, 5, NULL), luaL_optstring(L, 6, NULL), NULL);
	return 0;
}

static int api_hexchat_emit_print_attrs(lua_State *L)
{
	hexchat_event_attrs *attrs = *(hexchat_event_attrs **)luaL_checkudata(L, 1, "attrs");
	hexchat_emit_print_attrs(ph, attrs, luaL_checkstring(L, 2), luaL_optstring(L, 3, NULL), luaL_optstring(L, 4, NULL), luaL_optstring(L, 5, NULL), luaL_optstring(L, 6, NULL), luaL_optstring(L, 7, NULL), NULL);
	return 0;
}

static int api_hexchat_send_modes(lua_State *L)
{
	size_t i, n;
	int modes;
	char const *mode;
	char const **targets;

	luaL_checktype(L, 1, LUA_TTABLE);
	n = lua_rawlen(L, 1);
	mode = luaL_checkstring(L, 2);
	if(strlen(mode) != 2)
		return luaL_argerror(L, 2, "expected sign followed by a mode letter");
	modes = luaL_optinteger(L, 3, 0);
	targets = g_new(char const *, n);

	for(i = 0; i < n; i++)
	{
		lua_rawgeti(L, 1, i + 1);
		if(lua_type(L, -1) != LUA_TSTRING)
		{
			g_free(targets);
			return luaL_argerror(L, 1, "expected an array of strings");
		}
		targets[i] = lua_tostring(L, -1);
		lua_pop(L, 1);
	}
	hexchat_send_modes(ph, targets, n, modes, mode[0], mode[1]);
	g_free(targets);
	return 0;
}

static int api_hexchat_nickcmp(lua_State *L)
{
	lua_pushinteger(L, hexchat_nickcmp(ph, luaL_checkstring(L, 1), luaL_checkstring(L, 2)));
	return 1;
}

static int api_hexchat_strip(lua_State *L)
{
	size_t len;
	char const *text;
	gboolean leave_colors, leave_attrs;
	char *result;

	luaL_checktype(L, 1, LUA_TSTRING);
	text = lua_tolstring(L, 1, &len);
	leave_colors = lua_toboolean(L, 2);
	leave_attrs = lua_toboolean(L, 3);
	result = hexchat_strip(ph, text, len, (leave_colors ? 0 : 1) | (leave_attrs ? 0 : 2));
	if(result)
	{
		lua_pushstring(L, result);
		hexchat_free(ph, result);
		return 1;
	}
	return 0;
}

static void register_hook(hook_info *hook)
{
	script_info *info = get_info(hook->state);
	g_ptr_array_add(info->hooks, hook);
}

static void free_hook(hook_info *hook)
{
	if(hook->state)
		luaL_unref(hook->state, LUA_REGISTRYINDEX, hook->ref);
	if(hook->hook)
		hexchat_unhook(ph, hook->hook);
	g_free(hook);
}

static int unregister_hook(hook_info *hook)
{
	script_info *info = get_info(hook->state);

	if(g_ptr_array_remove_fast(info->hooks, hook))
		return 1;

	if(g_ptr_array_remove_fast(info->unload_hooks, hook))
		return 1;

	return 0;
}

static int api_command_closure(char *word[], char *word_eol[], void *udata)
{
	int base, i, ret;
	hook_info *info = udata;
	lua_State *L = info->state;
	script_info *script = get_info(L);

	lua_rawgeti(L, LUA_REGISTRYINDEX, script->traceback);
	base = lua_gettop(L);
	lua_rawgeti(L, LUA_REGISTRYINDEX, info->ref);
	lua_newtable(L);
	for(i = 1; i < 32 && *word_eol[i]; i++)
	{
		lua_pushstring(L, word[i]);
		lua_rawseti(L, -2, i);
	}
	lua_newtable(L);
	for(i = 1; i < 32 && *word_eol[i]; i++)
	{
		lua_pushstring(L, word_eol[i]);
		lua_rawseti(L, -2, i);
	}
	script->status |= STATUS_ACTIVE;
	if(lua_pcall(L, 2, 1, base))
	{
		char const *error = lua_tostring(L, -1);
		lua_pop(L, 2);
		hexchat_printf(ph, "Lua error in command hook: %s", error ? error : "(non-string error)");
		check_deferred(script);
		return HEXCHAT_EAT_NONE;
	}
	ret = lua_tointeger(L, -1);
	lua_pop(L, 2);
	check_deferred(script);
	return ret;
}

static int api_hexchat_hook_command(lua_State *L)
{
	hook_info *info, **u;
	char const *command, *help;
	int ref, pri;

	command = luaL_optstring(L, 1, "");
	lua_pushvalue(L, 2);
	ref = luaL_ref(L, LUA_REGISTRYINDEX);
	help = luaL_optstring(L, 3, NULL);
	pri = luaL_optinteger(L, 4, HEXCHAT_PRI_NORM);
	info = g_new(hook_info, 1);
	info->state = L;
	info->ref = ref;
	info->hook = hexchat_hook_command(ph, command, pri, api_command_closure, help, info);
	u = lua_newuserdata(L, sizeof(hook_info *));
	*u = info;
	luaL_newmetatable(L, "hook");
	lua_setmetatable(L, -2);
	register_hook(info);
	return 1;
}

static int api_print_closure(char *word[], void *udata)
{
	hook_info *info = udata;
	lua_State *L = info->state;
	script_info *script = get_info(L);
	int i, j, base, ret;

	lua_rawgeti(L, LUA_REGISTRYINDEX, script->traceback);
	base = lua_gettop(L);
	lua_rawgeti(L, LUA_REGISTRYINDEX, info->ref);

	for(j = 31; j >= 1; j--)
	{
		if(*word[j])
			break;
	}
	lua_newtable(L);
	for(i = 1; i <= j; i++)
	{
		lua_pushstring(L, word[i]);
		lua_rawseti(L, -2, i);
	}
	script->status |= STATUS_ACTIVE;
	if(lua_pcall(L, 1, 1, base))
	{
		char const *error = lua_tostring(L, -1);
		lua_pop(L, 2);
		hexchat_printf(ph, "Lua error in print hook: %s", error ? error : "(non-string error)");
		check_deferred(script);
		return HEXCHAT_EAT_NONE;
	}
	ret = lua_tointeger(L, -1);
	lua_pop(L, 2);
	check_deferred(script);
	return ret;
}

static int api_hexchat_hook_print(lua_State *L)
{
	char const *event = luaL_checkstring(L, 1);
	hook_info *info, **u;
	int ref, pri;

	lua_pushvalue(L, 2);
	ref = luaL_ref(L, LUA_REGISTRYINDEX);
	pri = luaL_optinteger(L, 3, HEXCHAT_PRI_NORM);
	info = g_new(hook_info, 1);
	info->state = L;
	info->ref = ref;
	info->hook = hexchat_hook_print(ph, event, pri, api_print_closure, info);
	u = lua_newuserdata(L, sizeof(hook_info *));
	*u = info;
	luaL_newmetatable(L, "hook");
	lua_setmetatable(L, -2);
	register_hook(info);
	return 1;
}

static hexchat_event_attrs *event_attrs_copy(const hexchat_event_attrs *attrs)
{
	hexchat_event_attrs *copy = hexchat_event_attrs_create(ph);
	copy->server_time_utc = attrs->server_time_utc;
	return copy;
}

static int api_print_attrs_closure(char *word[], hexchat_event_attrs *attrs, void *udata)
{
	hook_info *info = udata;
	lua_State *L = info->state;
	script_info *script = get_info(L);
	int base, i, j, ret;
	hexchat_event_attrs **u;

	lua_rawgeti(L, LUA_REGISTRYINDEX, script->traceback);
	base = lua_gettop(L);
	lua_rawgeti(L, LUA_REGISTRYINDEX, info->ref);
	for(j = 31; j >= 1; j--)
	{
		if(*word[j])
			break;
	}
	lua_newtable(L);
	for(i = 1; i <= j; i++)
	{
		lua_pushstring(L, word[i]);
		lua_rawseti(L, -2, i);
	}
	u = lua_newuserdata(L, sizeof(hexchat_event_attrs *));
	*u = event_attrs_copy(attrs);
	luaL_newmetatable(L, "attrs");
	lua_setmetatable(L, -2);
	script->status |= STATUS_ACTIVE;
	if(lua_pcall(L, 2, 1, base))
	{
		char const *error = lua_tostring(L, -1);
		lua_pop(L, 2);
		hexchat_printf(ph, "Lua error in print_attrs hook: %s", error ? error : "(non-string error)");
		check_deferred(script);
		return HEXCHAT_EAT_NONE;
	}
	ret = lua_tointeger(L, -1);
	lua_pop(L, 2);
	check_deferred(script);
	return ret;
}

static int api_hexchat_hook_print_attrs(lua_State *L)
{
	hook_info *info, **u;
	int ref, pri;
	char const *event = luaL_checkstring(L, 1);

	lua_pushvalue(L, 2);
	ref = luaL_ref(L, LUA_REGISTRYINDEX);
	pri = luaL_optinteger(L, 3, HEXCHAT_PRI_NORM);
	info = g_new(hook_info, 1);
	info->state = L;
	info->ref = ref;
	info->hook = hexchat_hook_print_attrs(ph, event, pri, api_print_attrs_closure, info);
	u = lua_newuserdata(L, sizeof(hook_info *));
	*u = info;
	luaL_newmetatable(L, "hook");
	lua_setmetatable(L, -2);
	register_hook(info);
	return 1;
}

static int api_server_closure(char *word[], char *word_eol[], void *udata)
{
	hook_info *info = udata;
	lua_State *L = info->state;
	script_info *script = get_info(L);
	int base, i, ret;

	lua_rawgeti(L, LUA_REGISTRYINDEX, script->traceback);
	base = lua_gettop(L);
	lua_rawgeti(L, LUA_REGISTRYINDEX, info->ref);
	lua_newtable(L);
	for(i = 1; i < 32 && *word_eol[i]; i++)
	{
		lua_pushstring(L, word[i]);
		lua_rawseti(L, -2, i);
	}
	lua_newtable(L);
	for(i = 1; i < 32 && *word_eol[i]; i++)
	{
		lua_pushstring(L, word_eol[i]);
		lua_rawseti(L, -2, i);
	}
	script->status |= STATUS_ACTIVE;
	if(lua_pcall(L, 2, 1, base))
	{
		char const *error = lua_tostring(L, -1);
		lua_pop(L, 2);
		hexchat_printf(ph, "Lua error in server hook: %s", error ? error : "(non-string error)");
		check_deferred(script);
		return HEXCHAT_EAT_NONE;
	}
	ret = lua_tointeger(L, -1);
	lua_pop(L, 2);
	check_deferred(script);
	return ret;
}

static int api_hexchat_hook_server(lua_State *L)
{
	char const *command = luaL_optstring(L, 1, "RAW LINE");
	hook_info *info, **u;
	int ref, pri;

	lua_pushvalue(L, 2);
	ref = luaL_ref(L, LUA_REGISTRYINDEX);
	pri = luaL_optinteger(L, 3, HEXCHAT_PRI_NORM);
	info = g_new(hook_info, 1);
	info->state = L;
	info->ref = ref;
	info->hook = hexchat_hook_server(ph, command, pri, api_server_closure, info);
	u = lua_newuserdata(L, sizeof(hook_info *));
	*u = info;
	luaL_newmetatable(L, "hook");
	lua_setmetatable(L, -2);
	register_hook(info);
	return 1;
}

static int api_server_attrs_closure(char *word[], char *word_eol[], hexchat_event_attrs *attrs, void *udata)
{
	hook_info *info = udata;
	lua_State *L = info->state;
	script_info *script = get_info(L);
	int base, i, ret;
	hexchat_event_attrs **u;

	lua_rawgeti(L, LUA_REGISTRYINDEX, script->traceback);
	base = lua_gettop(L);
	lua_rawgeti(L, LUA_REGISTRYINDEX, info->ref);
	lua_newtable(L);
	for(i = 1; i < 32 && *word_eol[i]; i++)
	{
		lua_pushstring(L, word[i]);
		lua_rawseti(L, -2, i);
	}
	lua_newtable(L);
	for(i = 1; i < 32 && *word_eol[i]; i++)
	{
		lua_pushstring(L, word_eol[i]);
		lua_rawseti(L, -2, i);
	}

	u = lua_newuserdata(L, sizeof(hexchat_event_attrs *));
	*u = event_attrs_copy(attrs);
	luaL_newmetatable(L, "attrs");
	lua_setmetatable(L, -2);
	script->status |= STATUS_ACTIVE;
	if(lua_pcall(L, 3, 1, base))
	{
		char const *error = lua_tostring(L, -1);
		lua_pop(L, 2);
		hexchat_printf(ph, "Lua error in server_attrs hook: %s", error ? error : "(non-string error)");
		check_deferred(script);
		return HEXCHAT_EAT_NONE;
	}
	ret = lua_tointeger(L, -1);
	lua_pop(L, 2);
	check_deferred(script);
	return ret;
}

static int api_hexchat_hook_server_attrs(lua_State *L)
{
	char const *command = luaL_optstring(L, 1, "RAW LINE");
	int ref, pri;
	hook_info *info, **u;

	lua_pushvalue(L, 2);
	ref = luaL_ref(L, LUA_REGISTRYINDEX);
	pri = luaL_optinteger(L, 3, HEXCHAT_PRI_NORM);
	info = g_new(hook_info, 1);
	info->state = L;
	info->ref = ref;
	info->hook = hexchat_hook_server_attrs(ph, command, pri, api_server_attrs_closure, info);
	u = lua_newuserdata(L, sizeof(hook_info *));
	*u = info;
	luaL_newmetatable(L, "hook");
	lua_setmetatable(L, -2);
	register_hook(info);
	return 1;
}

static int api_timer_closure(void *udata)
{
	hook_info *info = udata;
	lua_State *L = info->state;
	script_info *script = get_info(L);
	int base, ret;

	lua_rawgeti(L, LUA_REGISTRYINDEX, script->traceback);
	base = lua_gettop(L);
	lua_rawgeti(L, LUA_REGISTRYINDEX, info->ref);
	script->status |= STATUS_ACTIVE;
	if(lua_pcall(L, 0, 1, base))
	{
		char const *error = lua_tostring(L, -1);
		lua_pop(L, 2);
		hexchat_printf(ph, "Lua error in timer hook: %s", error ? error : "(non-string error)");
		check_deferred(script);
		return 0;
	}
	ret = lua_toboolean(L, -1);
	lua_pop(L, 2);
	check_deferred(script);
	return ret;
}

static int api_hexchat_hook_timer(lua_State *L)
{
	int ref, timeout = luaL_checkinteger (L, 1);
	hook_info *info, **u;

	lua_pushvalue(L, 2);
	ref = luaL_ref(L, LUA_REGISTRYINDEX);
	info = g_new(hook_info, 1);
	info->state = L;
	info->ref = ref;
	info->hook = hexchat_hook_timer(ph, timeout, api_timer_closure, info);
	u = lua_newuserdata(L, sizeof(hook_info *));
	*u = info;
	luaL_newmetatable(L, "hook");
	lua_setmetatable(L, -2);
	register_hook(info);
	return 1;
}

static int api_hexchat_hook_unload(lua_State *L)
{
	script_info *script;
	hook_info *info, **u;
	int ref;

	lua_pushvalue(L, 1);
	ref = luaL_ref(L, LUA_REGISTRYINDEX);
	info = g_new(hook_info, 1);
	info->state = L;
	info->ref = ref;
	info->hook = NULL;
	u = lua_newuserdata(L, sizeof(hook_info *));
	*u = info;
	luaL_newmetatable(L, "hook");
	lua_setmetatable(L, -2);
	script = get_info(info->state);

	g_ptr_array_add(script->unload_hooks, info);
	return 1;
}

static int api_hexchat_unhook(lua_State *L)
{
	hook_info **info = (hook_info **)luaL_checkudata(L, 1, "hook");
	if(*info)
	{
		unregister_hook(*info);
		*info = 0;
		return 0;
	}
	else
	{
		tostring(L, 1);
		return luaL_error(L, "hook %s is already unhooked", lua_tostring(L, -1));
	}
}

static int api_hexchat_find_context(lua_State *L)
{
	char const *server = luaL_optstring(L, 1, NULL);
	char const *channel = luaL_optstring(L, 2, NULL);
	hexchat_context *context = hexchat_find_context(ph, server, channel);
	if(context)
	{
		hexchat_context **u = lua_newuserdata(L, sizeof(hexchat_context *));
		*u = context;
		luaL_newmetatable(L, "context");
		lua_setmetatable(L, -2);
		return 1;
	}
	else
	{
		lua_pushnil(L);
		return 1;
	}
}

static int api_hexchat_get_context(lua_State *L)
{
	hexchat_context *context = hexchat_get_context(ph);
	hexchat_context **u = lua_newuserdata(L, sizeof(hexchat_context *));
	*u = context;
	luaL_newmetatable(L, "context");
	lua_setmetatable(L, -2);
	return 1;
}

static int api_hexchat_set_context(lua_State *L)
{
	hexchat_context *context = *(hexchat_context **)luaL_checkudata(L, 1, "context");
	int success = hexchat_set_context(ph, context);
	lua_pushboolean(L, success);
	return 1;
}

static int wrap_context_closure(lua_State *L)
{
	hexchat_context *old, *context = *(hexchat_context **)luaL_checkudata(L, 1, "context");
	lua_pushvalue(L, lua_upvalueindex(1));
	lua_replace(L, 1);
	old = hexchat_get_context(ph);
	if(!hexchat_set_context(ph, context))
		return luaL_error(L, "could not switch into context");
	lua_call(L, lua_gettop(L) - 1, LUA_MULTRET);
	hexchat_set_context(ph, old);
	return lua_gettop(L);
}

static inline void wrap_context(lua_State *L, char const *field, lua_CFunction func)
{
	lua_pushcfunction(L, func);
	lua_pushcclosure(L, wrap_context_closure, 1);
	lua_setfield(L, -2, field);
}

static int api_hexchat_context_meta_eq(lua_State *L)
{
	hexchat_context *this = *(hexchat_context **)luaL_checkudata(L, 1, "context");
	hexchat_context *that = *(hexchat_context **)luaL_checkudata(L, 2, "context");
	lua_pushboolean(L, this == that);
	return 1;
}

static int api_hexchat_get_info(lua_State *L)
{
	char const *key = luaL_checkstring(L, 1);
	char const *data = hexchat_get_info(ph, key);
	if(data)
	{
		if(!strcmp(key, "gtkwin_ptr") || !strcmp(key, "win_ptr"))
			lua_pushlightuserdata(L, (void *)data);
		else
			lua_pushstring(L, data);
		return 1;
	}
	lua_pushnil(L);
	return 1;
}

static int api_hexchat_attrs(lua_State *L)
{
	hexchat_event_attrs *attrs = hexchat_event_attrs_create(ph);
	hexchat_event_attrs **u = lua_newuserdata(L, sizeof(hexchat_event_attrs *));
	*u = attrs;
	luaL_newmetatable(L, "attrs");
	lua_setmetatable(L, -2);
	return 1;
}

static int api_iterate_closure(lua_State *L)
{
	hexchat_list *list = *(hexchat_list **)luaL_checkudata(L, lua_upvalueindex(1), "list");
	if(hexchat_list_next(ph, list))
	{
		lua_pushvalue(L, lua_upvalueindex(1));
		return 1;
	}
	else
		return 0;
}

static int api_hexchat_iterate(lua_State *L)
{
	char const *name = luaL_checkstring(L, 1);
	hexchat_list *list = hexchat_list_get(ph, name);
	if(list)
	{
		hexchat_list **u = lua_newuserdata(L, sizeof(hexchat_list *));
		*u = list;
		luaL_newmetatable(L, "list");
		lua_setmetatable(L, -2);
		lua_pushcclosure(L, api_iterate_closure, 1);
		return 1;
	}
	else
		return luaL_argerror(L, 1, "invalid list name");
}

static int api_hexchat_prefs_meta_index(lua_State *L)
{
	char const *key = luaL_checkstring(L, 2);
	char const *string;
	int number;
	int ret = hexchat_get_prefs(ph, key, &string, &number);
	switch(ret)
	{
		case 0:
			lua_pushnil(L);
			return 1;
		case 1:
			lua_pushstring(L, string);
			return 1;
		case 2:
			lua_pushnumber(L, number);
			return 1;
		case 3:
			lua_pushboolean(L, number);
			return 1;
		default:
			return 0;
	}
}

static int api_hexchat_prefs_meta_newindex(lua_State *L)
{
	return luaL_error(L, "hexchat.prefs is read-only");
}

static inline int list_marshal(lua_State *L, const char *key, hexchat_list *list)
{
	char const *str = hexchat_list_str(ph, list, key);
	int number;
	if(str)
	{
		if(!strcmp(key, "context"))
		{
			hexchat_context **u = lua_newuserdata(L, sizeof(hexchat_context *));
			*u = (hexchat_context *)str;
			luaL_newmetatable(L, "context");
			lua_setmetatable(L, -2);
			return 1;
		}
		lua_pushstring(L, str);
		return 1;
	}
	number = hexchat_list_int(ph, list, key);
	if(number != -1)
	{
		lua_pushinteger(L, number);
		return 1;
	}
	if (list != NULL)
	{
		time_t tm = hexchat_list_time(ph, list, key);
		if(tm != -1)
		{
			lua_pushinteger(L, tm);
			return 1;
		}
	}

	lua_pushnil(L);
	return 1;
}

static int api_hexchat_props_meta_index(lua_State *L)
{
	char const *key = luaL_checkstring(L, 2);
	return list_marshal(L, key, NULL);
}

static int api_hexchat_props_meta_newindex(lua_State *L)
{
	return luaL_error(L, "hexchat.props is read-only");
}

static int api_hexchat_pluginprefs_meta_index(lua_State *L)
{
	script_info *script = get_info(L);
	const char *key;
	hexchat_plugin *h;
	char str[512];
	int r;

	if(!script->name)
		return luaL_error(L, "cannot use hexchat.pluginprefs before registering with hexchat.register");

	key = luaL_checkstring(L, 2);
	h = script->handle;
	r = hexchat_pluginpref_get_int(h, key);
	if(r != -1)
	{
		lua_pushinteger(L, r);
		return 1;
	}
	if(hexchat_pluginpref_get_str(h, key, str))
	{
		/* Wasn't actually a failure */
		if (!strcmp(str, "-1"))
			lua_pushinteger(L, r);
		else
			lua_pushstring(L, str);
		return 1;
	}
	lua_pushnil(L);
	return 1;
}

static int api_hexchat_pluginprefs_meta_newindex(lua_State *L)
{
	script_info *script = get_info(L);
	const char *key;
	hexchat_plugin *h;

	if(!script->name)
		return luaL_error(L, "cannot use hexchat.pluginprefs before registering with hexchat.register");

	key = luaL_checkstring(L, 2);
	h = script->handle;
	switch(lua_type(L, 3))
	{
		case LUA_TSTRING:
			hexchat_pluginpref_set_str(h, key, lua_tostring(L, 3));
			return 0;
		case LUA_TNUMBER:
			hexchat_pluginpref_set_int(h, key, lua_tointeger(L, 3));
			return 0;
		case LUA_TNIL: case LUA_TNONE:
			hexchat_pluginpref_delete(h, key);
			return 0;
		default:
			return luaL_argerror(L, 3, "expected string, number, or nil");
	}
}

static int api_hexchat_pluginprefs_meta_pairs_closure(lua_State *L)
{
	char *dest = lua_touserdata(L, lua_upvalueindex(1));
	hexchat_plugin *h = get_info(L)->handle;

	if(dest && *dest)
	{
		int r;
		char str[512];
		char *key = dest;

		dest = strchr(dest, ',');
		if(dest)
			*(dest++) = 0;
		lua_pushlightuserdata(L, dest);
		lua_replace(L, lua_upvalueindex(1));
		lua_pushstring(L, key);
		r = hexchat_pluginpref_get_int(h, key);
		if(r != -1)
		{
			lua_pushinteger(L, r);
			return 2;
		}
		if(hexchat_pluginpref_get_str(h, key, str))
		{
			lua_pushstring(L, str);
			return 2;
		}
		lua_pushnil(L);
		return 2;
	}
	else
		return 0;
}

static int api_hexchat_pluginprefs_meta_pairs(lua_State *L)
{
	script_info *script = get_info(L);
	char *dest;
	hexchat_plugin *h;

	if(!script->name)
		return luaL_error(L, "cannot use hexchat.pluginprefs before registering with hexchat.register");

	dest = lua_newuserdata(L, 4096);
	h = script->handle;
	if(!hexchat_pluginpref_list(h, dest))
		strcpy(dest, "");
	lua_pushlightuserdata(L, dest);
	lua_pushlightuserdata(L, dest);
	lua_pushcclosure(L, api_hexchat_pluginprefs_meta_pairs_closure, 2);
	return 1;
}

static int api_attrs_meta_index(lua_State *L)
{
	hexchat_event_attrs *attrs = *(hexchat_event_attrs **)luaL_checkudata(L, 1, "attrs");
	char const *key = luaL_checkstring(L, 2);
	if(!strcmp(key, "server_time_utc"))
	{
		lua_pushinteger(L, attrs->server_time_utc);
		return 1;
	}
	else
	{
		lua_pushnil(L);
		return 1;
	}
}

static int api_attrs_meta_newindex(lua_State *L)
{
	hexchat_event_attrs *attrs = *(hexchat_event_attrs **)luaL_checkudata(L, 1, "attrs");
	char const *key = luaL_checkstring(L, 2);
	if(!strcmp(key, "server_time_utc"))
	{
		attrs->server_time_utc = luaL_checkinteger(L, 3);
		return 0;
	}
	else
		return 0;
}

static int api_attrs_meta_gc(lua_State *L)
{
	hexchat_event_attrs *attrs = *(hexchat_event_attrs **)luaL_checkudata(L, 1, "attrs");
	hexchat_event_attrs_free(ph, attrs);
	return 0;
}

static int api_list_meta_index(lua_State *L)
{
	hexchat_list *list = *(hexchat_list **)luaL_checkudata(L, 1, "list");
	char const *key = luaL_checkstring(L, 2);
	return list_marshal(L, key, list);
}

static int api_list_meta_newindex(lua_State *L)
{
	return luaL_error(L, "hexchat.iterate list is read-only");
}

static int api_list_meta_gc(lua_State *L)
{
	hexchat_list *list = *(hexchat_list **)luaL_checkudata(L, 1, "list");
	hexchat_list_free(ph, list);
	return 0;
}

static luaL_Reg api_hexchat[] = {
	{"register", api_hexchat_register},
	{"command", api_hexchat_command},
	{"print", api_hexchat_print},
	{"emit_print", api_hexchat_emit_print},
	{"emit_print_attrs", api_hexchat_emit_print_attrs},
	{"send_modes", api_hexchat_send_modes},
	{"nickcmp", api_hexchat_nickcmp},
	{"strip", api_hexchat_strip},
	{"get_info", api_hexchat_get_info},
	{"hook_command", api_hexchat_hook_command},
	{"hook_print", api_hexchat_hook_print},
	{"hook_print_attrs", api_hexchat_hook_print_attrs},
	{"hook_server", api_hexchat_hook_server},
	{"hook_server_attrs", api_hexchat_hook_server_attrs},
	{"hook_timer", api_hexchat_hook_timer},
	{"hook_unload", api_hexchat_hook_unload},
	{"unhook", api_hexchat_unhook},
	{"get_context", api_hexchat_get_context},
	{"find_context", api_hexchat_find_context},
	{"set_context", api_hexchat_set_context},
	{"attrs", api_hexchat_attrs},
	{"iterate", api_hexchat_iterate},
	{NULL, NULL}
};

static luaL_Reg api_hexchat_props_meta[] = {
	{"__index", api_hexchat_props_meta_index},
	{"__newindex", api_hexchat_props_meta_newindex},
	{NULL, NULL}
};

static luaL_Reg api_hexchat_prefs_meta[] = {
	{"__index", api_hexchat_prefs_meta_index},
	{"__newindex", api_hexchat_prefs_meta_newindex},
	{NULL, NULL}
};

static luaL_Reg api_hexchat_pluginprefs_meta[] = {
	{"__index", api_hexchat_pluginprefs_meta_index},
	{"__newindex", api_hexchat_pluginprefs_meta_newindex},
	{"__pairs", api_hexchat_pluginprefs_meta_pairs},
	{NULL, NULL}
};

static luaL_Reg api_hook_meta_index[] = {
	{"unhook", api_hexchat_unhook},
	{NULL, NULL}
};

static luaL_Reg api_attrs_meta[] = {
	{"__index", api_attrs_meta_index},
	{"__newindex", api_attrs_meta_newindex},
	{"__gc", api_attrs_meta_gc},
	{NULL, NULL}
};

static luaL_Reg api_list_meta[] = {
	{"__index", api_list_meta_index},
	{"__newindex", api_list_meta_newindex},
	{"__gc", api_list_meta_gc},
	{NULL, NULL}
};

static int luaopen_hexchat(lua_State *L)
{
	lua_newtable(L);
	luaL_setfuncs(L, api_hexchat, 0);

	lua_pushinteger(L, HEXCHAT_PRI_HIGHEST); lua_setfield(L, -2, "PRI_HIGHEST");
	lua_pushinteger(L, HEXCHAT_PRI_HIGH); lua_setfield(L, -2, "PRI_HIGH");
	lua_pushinteger(L, HEXCHAT_PRI_NORM); lua_setfield(L, -2, "PRI_NORM");
	lua_pushinteger(L, HEXCHAT_PRI_LOW); lua_setfield(L, -2, "PRI_LOW");
	lua_pushinteger(L, HEXCHAT_PRI_LOWEST); lua_setfield(L, -2, "PRI_LOWEST");
	lua_pushinteger(L, HEXCHAT_EAT_NONE); lua_setfield(L, -2, "EAT_NONE");
	lua_pushinteger(L, HEXCHAT_EAT_HEXCHAT); lua_setfield(L, -2, "EAT_HEXCHAT");
	lua_pushinteger(L, HEXCHAT_EAT_PLUGIN); lua_setfield(L, -2, "EAT_PLUGIN");
	lua_pushinteger(L, HEXCHAT_EAT_ALL); lua_setfield(L, -2, "EAT_ALL");

	lua_newtable(L);
	lua_newtable(L);
	luaL_setfuncs(L, api_hexchat_prefs_meta, 0);
	lua_setmetatable(L, -2);
	lua_setfield(L, -2, "prefs");

	lua_newtable(L);
	lua_newtable(L);
	luaL_setfuncs(L, api_hexchat_props_meta, 0);
	lua_setmetatable(L, -2);
	lua_setfield(L, -2, "props");

	lua_newtable(L);
	lua_newtable(L);
	luaL_setfuncs(L, api_hexchat_pluginprefs_meta, 0);
	lua_setmetatable(L, -2);
	lua_setfield(L, -2, "pluginprefs");

	luaL_newmetatable(L, "hook");
	lua_newtable(L);
	luaL_setfuncs(L, api_hook_meta_index, 0);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);

	luaL_newmetatable(L, "context");
	lua_newtable(L);
	lua_pushcfunction(L, api_hexchat_set_context);
	lua_setfield(L, -2, "set");
	wrap_context(L, "find_context", api_hexchat_find_context);
	wrap_context(L, "print", api_hexchat_print);
	wrap_context(L, "emit_print", api_hexchat_emit_print);
	wrap_context(L, "emit_print_attrs", api_hexchat_emit_print_attrs);
	wrap_context(L, "command", api_hexchat_command);
	wrap_context(L, "nickcmp", api_hexchat_nickcmp);
	wrap_context(L, "get_info", api_hexchat_get_info);
	wrap_context(L, "iterate", api_hexchat_iterate);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, api_hexchat_context_meta_eq);
	lua_setfield(L, -2, "__eq");
	lua_pop(L, 1);


	luaL_newmetatable(L, "attrs");
	luaL_setfuncs(L, api_attrs_meta, 0);
	lua_pop(L, 1);

	luaL_newmetatable(L, "list");
	luaL_setfuncs(L, api_list_meta, 0);
	lua_pop(L, 1);

	return 1;
}

static int pairs_closure(lua_State *L)
{
	lua_settop(L, 1);
	if(luaL_getmetafield(L, 1, "__pairs"))
	{
		lua_insert(L, 1);
		lua_call(L, 1, LUA_MULTRET);
		return lua_gettop(L);
	}
	else
	{
		lua_pushvalue(L, lua_upvalueindex(1));
		lua_insert(L, 1);
		lua_call(L, 1, LUA_MULTRET);
		return lua_gettop(L);
	}
}

static void patch_pairs(lua_State *L)
{
	lua_getglobal(L, "pairs");
	lua_pushcclosure(L, pairs_closure, 1);
	lua_setglobal(L, "pairs");
}

static void patch_clibs(lua_State *L)
{
	lua_pushnil(L);
	while(lua_next(L, LUA_REGISTRYINDEX))
	{
		if(lua_type(L, -2) == LUA_TLIGHTUSERDATA && lua_type(L, -1) == LUA_TTABLE)
		{
			lua_setfield(L, LUA_REGISTRYINDEX, "_CLIBS");
			break;
		}
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
}

static GPtrArray *scripts;

static char *expand_buffer = NULL;
static char const *expand_path(char const *path)
{
	if(g_path_is_absolute(path))
		return path;
#ifndef G_OS_WIN32
	if(path[0] == '~')
	{
		if(!path[1] || path[1] == '/')
		{
			g_free(expand_buffer);
			expand_buffer = g_build_filename(g_get_home_dir(), path + 1, NULL);
			return expand_buffer;
		}
		else
		{
			char *user = g_strdup(path + 1);
			char *slash_pos = strchr(user, '/');
			struct passwd *pw;
			if(slash_pos)
				*slash_pos = 0;
			pw = getpwnam(user);
			g_free(user);
			if(pw)
			{
				slash_pos = strchr(path, '/');
				if(!slash_pos)
					return pw->pw_dir;

				g_free(expand_buffer);
				expand_buffer = g_strconcat(pw->pw_dir, slash_pos, NULL);
				return expand_buffer;
			}
			else
			{
				return path;
			}
		}
	}
	else
#endif
	{
		g_free(expand_buffer);
		expand_buffer = g_build_filename(hexchat_get_info(ph, "configdir"), "addons", path, NULL);
		return expand_buffer;
	}
}

static inline int is_lua_file(char const *file)
{
	return g_str_has_suffix(file, ".lua") || g_str_has_suffix(file, ".luac");
}

static void prepare_state(lua_State *L, script_info *info)
{
	luaL_openlibs(L);
	if(LUA_VERSION_NUM < 502)
		patch_pairs(L);
	if(LUA_VERSION_NUM > 502)
		patch_clibs(L);
	lua_getglobal(L, "debug");
	lua_getfield(L, -1, "traceback");
	info->traceback = luaL_ref(L, LUA_REGISTRYINDEX);
	lua_pop(L, 1);
	lua_pushlightuserdata(L, info);
	lua_setfield(L, LUA_REGISTRYINDEX, registry_field);
	luaopen_hexchat(L);
	lua_setglobal(L, "hexchat");
	lua_getglobal(L, "hexchat");
	lua_getfield(L, -1, "print");
	lua_setglobal(L, "print");
	lua_pop(L, 1);
}

static void run_unload_hook(hook_info *hook, lua_State *L)
{
	int base = lua_gettop(L);

	lua_rawgeti(L, LUA_REGISTRYINDEX, hook->ref);
	if(lua_pcall(L, 0, 0, base))
	{
		char const *error = lua_tostring(L, -1);
		hexchat_printf(ph, "Lua error in unload hook: %s", error ? error : "(non-string error)");
	}
	lua_settop(L, base);
}

static void run_unload_hooks(script_info *info, void *unused)
{
	lua_State *L = info->state;
	lua_rawgeti(L, LUA_REGISTRYINDEX, info->traceback);
	g_ptr_array_foreach(info->unload_hooks, (GFunc)run_unload_hook, L);
	lua_pop(L, 1);
}

static void destroy_script(script_info *info)
{
	if (info)
	{
		g_clear_pointer(&info->hooks, g_ptr_array_unref);
		g_clear_pointer(&info->unload_hooks, g_ptr_array_unref);
		g_clear_pointer(&info->state, lua_close);
		if (info->handle)
			hexchat_plugingui_remove(ph, info->handle);
		g_free(info->filename);
		g_free(info->name);
		g_free(info->description);
		g_free(info->version);
		g_free(info);
	}
}

static script_info *create_script(char const *file)
{
	int base;
	char *filename_fs;
	lua_State *L;
	script_info *info = g_new0(script_info, 1);
	info->hooks = g_ptr_array_new_with_free_func((GDestroyNotify)free_hook);
	info->unload_hooks = g_ptr_array_new_with_free_func((GDestroyNotify)free_hook);
	info->filename = g_strdup(expand_path(file));
	L = luaL_newstate();
	info->state = L;
	if(!L)
	{
		hexchat_print(ph, "\00304Could not allocate memory for the script");
		destroy_script(info);
		return NULL;
	}
	prepare_state(L, info);
	lua_rawgeti(L, LUA_REGISTRYINDEX, info->traceback);
	base = lua_gettop(L);
	filename_fs = g_filename_from_utf8(info->filename, -1, NULL, NULL, NULL);
	if(!filename_fs)
	{
		hexchat_printf(ph, "Invalid filename: %s", info->filename);
		destroy_script(info);
		return NULL;
	}
	if(luaL_loadfile(L, filename_fs))
	{
		g_free(filename_fs);
		hexchat_printf(ph, "Lua syntax error: %s", luaL_optstring(L, -1, ""));
		destroy_script(info);
		return NULL;
	}
	g_free(filename_fs);
	info->status |= STATUS_ACTIVE;
	if(lua_pcall(L, 0, 0, base))
	{
		char const *error = lua_tostring(L, -1);
		hexchat_printf(ph, "Lua error: %s", error ? error : "(non-string error)");
		destroy_script(info);
		return NULL;
	}
	lua_pop(L, 1);
	if(!info->name)
	{
		hexchat_printf(ph, "Lua script didn't register with hexchat.register");
		destroy_script(info);
		return NULL;
	}
	return info;
}

static void load_script(char const *file)
{
	script_info *info = create_script(file);
	if(info)
	{
		g_ptr_array_add(scripts, info);
		check_deferred(info);
	}
}

static script_info *get_script_by_file(char const *filename)
{
	char const *expanded = expand_path(filename);
	guint i;
	for(i = 0; i < scripts->len; i++)
	{
		script_info *script = scripts->pdata[i];
		if(!strcmp(script->filename, expanded))
		{
			return script;
		}
	}

	return NULL;
}

static int unload_script(char const *filename)
{
	script_info *script = get_script_by_file(filename);

	if (!script)
		return 0;

	if(script->status & STATUS_ACTIVE)
		script->status |= STATUS_DEFERRED_UNLOAD;
	else
	{
		run_unload_hooks(script, NULL);
		g_ptr_array_remove_fast(scripts, script);
	}

	return 1;

}

static int reload_script(char const *filename)
{
	script_info *script = get_script_by_file(filename);

	if (!script)
		return 0;

	if(script->status & STATUS_ACTIVE)
	{
		script->status |= STATUS_DEFERRED_RELOAD;
	}
	else
	{
		char *filename = g_strdup(script->filename);
		run_unload_hooks(script, NULL);
		g_ptr_array_remove_fast(scripts, script);
		load_script(filename);
		g_free(filename);
	}

	return 1;
}

static void autoload_scripts(void)
{
	char *path = g_build_filename(hexchat_get_info(ph, "configdir"), "addons", NULL);
	GDir *dir = g_dir_open(path, 0, NULL);
	if(dir)
	{
		char const *filename;
		while((filename = g_dir_read_name(dir)))
		{
			if(is_lua_file(filename))
				load_script(filename);
		}
		g_dir_close(dir);
	}
	g_free(path);
}

static script_info *interp = NULL;
static void create_interpreter(void)
{
	lua_State *L;
	interp = g_new0(script_info, 1);
	interp->hooks = g_ptr_array_new_with_free_func((GDestroyNotify)free_hook);
	interp->unload_hooks = g_ptr_array_new_with_free_func((GDestroyNotify)free_hook);
	interp->name = "lua interpreter";
	interp->description = "";
	interp->version = "";
	interp->handle = ph;
	interp->filename = "";
	L = luaL_newstate();
	interp->state = L;
	if(!L)
	{
		hexchat_print(ph, "\00304Could not allocate memory for the interpreter");
		g_free(interp);
		interp = NULL;
		return;
	}
	prepare_state(L, interp);
}

static void destroy_interpreter(void)
{
	if(interp)
	{
		g_clear_pointer(&interp->hooks, g_ptr_array_unref);
		g_clear_pointer(&interp->unload_hooks, g_ptr_array_unref);
		g_clear_pointer(&interp->state, lua_close);
		g_clear_pointer(&interp, g_free);
	}
}

static void inject_string(script_info *info, char const *line)
{
	lua_State *L = info->state;
	int base, top;
	char *ret_line;
	gboolean force_ret = FALSE;

	if(line[0] == '=')
	{
		line++;
		force_ret = TRUE;
	}
	ret_line = g_strconcat("return ", line, NULL);

	lua_rawgeti(L, LUA_REGISTRYINDEX, info->traceback);
	base = lua_gettop(L);
	if(luaL_loadbuffer(L, ret_line, strlen(ret_line), "@interpreter"))
	{
		if(!force_ret)
			lua_pop(L, 1);
		if(force_ret || luaL_loadbuffer(L, line, strlen(line), "@interpreter"))
		{
			hexchat_printf(ph, "Lua syntax error: %s", luaL_optstring(L, -1, ""));
			lua_pop(L, 2);
			g_free(ret_line);
			return;
		}
	}
	g_free(ret_line);
	info->status |= STATUS_ACTIVE;
	if(lua_pcall(L, 0, LUA_MULTRET, base))
	{
		char const *error = lua_tostring(L, -1);
		lua_pop(L, 2);
		hexchat_printf(ph, "Lua error: %s", error ? error : "(non-string error)");
		return;
	}
	top = lua_gettop(L);
	if(top > base)
	{
		int i;
		luaL_Buffer b;
		luaL_buffinit(L, &b);
		for(i = base + 1; i <= top; i++)
		{
			if(i != base + 1)
				luaL_addstring(&b, " ");
			tostring(L, i);
			luaL_addvalue(&b);
		}
		luaL_pushresult(&b);
		hexchat_print(ph, lua_tostring(L, -1));
		lua_pop(L, top - base + 1);
	}
	lua_pop(L, 1);
	check_deferred(info);
}

static int command_load(char *word[], char *word_eol[], void *userdata)
{
	if(is_lua_file(word[2]))
	{
		load_script(word[2]);
		return HEXCHAT_EAT_ALL;
	}
	else
		return HEXCHAT_EAT_NONE;
}

static int command_unload(char *word[], char *word_eol[], void *userdata)
{
	if(unload_script(word[2]))
		return HEXCHAT_EAT_ALL;
	else
		return HEXCHAT_EAT_NONE;
}

static int command_reload(char *word[], char *word_eol[], void *userdata)
{
	if(reload_script(word[2]))
		return HEXCHAT_EAT_ALL;
	else
		return HEXCHAT_EAT_NONE;
}

static int command_console_exec(char *word[], char *word_eol[], void *userdata)
{
	char const *channel = hexchat_get_info(ph, "channel");
	if(channel && !strcmp(channel, console_tab))
	{
		if(interp)
		{
			hexchat_printf(ph, "> %s", word_eol[1]);
			inject_string(interp, word_eol[1]);
		}
		return HEXCHAT_EAT_ALL;
	}
	return HEXCHAT_EAT_NONE;
}

static void check_deferred(script_info *info)
{
	info->status &= ~STATUS_ACTIVE;
	if(info->status & STATUS_DEFERRED_UNLOAD)
	{
		run_unload_hooks(info, NULL);
		g_ptr_array_remove_fast(scripts, info);
	}
	else if(info->status & STATUS_DEFERRED_RELOAD)
	{
		if(info == interp)
		{
			run_unload_hooks(interp, NULL);
			destroy_interpreter();
			create_interpreter();
		}
		else
		{
			char *filename = g_strdup(info->filename);
			run_unload_hooks(info, NULL);
			g_ptr_array_remove_fast(scripts, info);
			load_script(filename);
			g_free(filename);
		}
	}
}

static int command_lua(char *word[], char *word_eol[], void *userdata)
{
	if(!strcmp(word[2], "load"))
	{
		load_script(word[3]);
	}
	else if(!strcmp(word[2], "unload"))
	{
		if(!unload_script(word[3]))
			hexchat_printf(ph, "Could not find a script by the name '%s'", word[3]);
	}
	else if(!strcmp(word[2], "reload"))
	{
		if(!reload_script(word[3]))
			hexchat_printf(ph, "Could not find a script by the name '%s'", word[3]);
	}
	else if(!strcmp(word[2], "exec"))
	{
		if(interp)
			inject_string(interp, word_eol[3]);
	}
	else if(!strcmp(word[2], "inject"))
	{
		script_info *script = get_script_by_file(word[3]);
		if (script)
		{
			inject_string(script, word_eol[4]);
		}
		else
		{
			hexchat_printf(ph, "Could not find a script by the name '%s'", word[3]);
		}
	}
	else if(!strcmp(word[2], "reset"))
	{
		if(interp)
		{
			if(interp->status & STATUS_ACTIVE)
			{
				interp->status |= STATUS_DEFERRED_RELOAD;
			}
			else
			{
				run_unload_hooks(interp, NULL);
				destroy_interpreter();
				create_interpreter();
			}
		}
	}
	else if(!strcmp(word[2], "list"))
	{
		guint i;
		hexchat_print(ph,
		   "Name             Version  Filename             Description\n"
		   "----             -------  --------             -----------\n");
		for(i = 0; i < scripts->len; i++)
		{
			script_info *info = scripts->pdata[i];
			char *basename = g_path_get_basename(info->filename);
			hexchat_printf(ph, "%-16s %-8s %-20s %-10s\n", info->name, info->version,
						   basename, info->description);
			g_free(basename);
		}
		if(interp)
			hexchat_printf(ph, "%-16s %-8s", interp->name, plugin_version);
	}
	else if(!strcmp(word[2], "console"))
	{
		hexchat_commandf(ph, "query %s", console_tab);
	}
	else
	{
		hexchat_command(ph, "help lua");
	}
	return HEXCHAT_EAT_ALL;
}

G_MODULE_EXPORT int hexchat_plugin_init(hexchat_plugin *plugin_handle, char **name, char **description, char **version, char *arg)
{
	if (g_str_has_prefix(LUA_VERSION, "Lua "))
	{
		strcat(plugin_version, "/");
		g_strlcat(plugin_version, LUA_VERSION + 4, sizeof(plugin_version));
	}

	*name = plugin_name;
	*description = plugin_description;
	*version = plugin_version;

	ph = plugin_handle;

	hexchat_hook_command(ph, "", HEXCHAT_PRI_NORM, command_console_exec, NULL, NULL);
	hexchat_hook_command(ph, "LOAD", HEXCHAT_PRI_NORM, command_load, NULL, NULL);
	hexchat_hook_command(ph, "UNLOAD", HEXCHAT_PRI_NORM, command_unload, NULL, NULL);
	hexchat_hook_command(ph, "RELOAD", HEXCHAT_PRI_NORM, command_reload, NULL, NULL);
	hexchat_hook_command(ph, "lua", HEXCHAT_PRI_NORM, command_lua, command_help, NULL);

	hexchat_printf(ph, "%s version %s loaded.\n", plugin_name, plugin_version);

	scripts = g_ptr_array_new_with_free_func((GDestroyNotify)destroy_script);
	create_interpreter();

	if(!arg)
		autoload_scripts();
	return 1;
}

G_MODULE_EXPORT int hexchat_plugin_deinit(hexchat_plugin *plugin_handle)
{
	guint i;
	gboolean active = FALSE;
	for(i = 0; i < scripts->len; i++)
	{
		if(((script_info*)scripts->pdata[i])->status & STATUS_ACTIVE)
		{
			active = TRUE;
			break;
		}
	}
	if(interp && interp->status & STATUS_ACTIVE)
		active = TRUE;
	if(active)
	{
		hexchat_print(ph, "\00304Cannot unload the lua plugin while there are active states");
		return 0;
	}
	if(interp)
		run_unload_hooks(interp, NULL);
	destroy_interpreter();
	g_ptr_array_foreach(scripts, (GFunc)run_unload_hooks, NULL);
	g_clear_pointer(&scripts, g_ptr_array_unref);
	g_clear_pointer(&expand_buffer, g_free);
	return 1;
}

