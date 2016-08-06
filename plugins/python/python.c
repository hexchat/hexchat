/*
* Copyright (c) 2002-2003  Gustavo Niemeyer <niemeyer@conectiva.com>
*
* XChat Python Plugin Interface
*
* Xchat Python Plugin Interface is free software; you can redistribute
* it and/or modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2 of the
* License, or (at your option) any later version.
*
* pybot is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this file; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
*/

/* Thread support
 * ==============
 *
 * The python interpreter has a global interpreter lock. Any thread
 * executing must acquire it before working with data accessible from
 * python code. Here we must also care about xchat not being
 * thread-safe. We do this by using an xchat lock, which protects
 * xchat instructions from being executed out of time (when this
 * plugin is not "active").
 *
 * When xchat calls python code:
 *   - Change the current_plugin for the executing plugin;
 *   - Release xchat lock
 *   - Acquire the global interpreter lock
 *   - Make the python call
 *   - Release the global interpreter lock
 *   - Acquire xchat lock
 *
 * When python code calls xchat:
 *   - Release the global interpreter lock
 *   - Acquire xchat lock
 *   - Restore context, if necessary
 *   - Make the xchat call
 *   - Release xchat lock
 *   - Acquire the global interpreter lock
 *
 * Inside a timer, so that individual threads have a chance to run:
 *   - Release xchat lock
 *   - Go ahead threads. Have a nice time!
 *   - Acquire xchat lock
 *
 */

#include "config.h"

#include <glib.h>
#include <glib/gstdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef WIN32
#include <direct.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif

#include "hexchat-plugin.h"
#undef _POSIX_C_SOURCE	/* Avoid warnings from /usr/include/features.h */
#undef _XOPEN_SOURCE
#include <Python.h>
#include <structmember.h>
#include <pythread.h>

/* Macros to convert version macros into string literals.
 * The indirect macro is a well-known preprocessor trick to force X to be evaluated before the # operator acts to make it a string literal.
 * If STRINGIZE were to be directly defined as #X instead, VERSION would be "VERSION_MAJOR" instead of "1".
 */
#define STRINGIZE2(X) #X
#define STRINGIZE(X) STRINGIZE2(X)

/* Version number macros */
#define VERSION_MAJOR 1
#define VERSION_MINOR 0

/* Version string macro e.g 1.0/3.3 */
#define VERSION STRINGIZE(VERSION_MAJOR) "." STRINGIZE(VERSION_MINOR) "/" \
				STRINGIZE(PY_MAJOR_VERSION) "." STRINGIZE (PY_MINOR_VERSION)

/* #define's for Python 2 */
#if PY_MAJOR_VERSION == 2
#undef PyLong_Check
#define PyLong_Check PyInt_Check
#define PyLong_AsLong PyInt_AsLong
#define PyLong_FromLong PyInt_FromLong

#undef PyUnicode_Check
#undef PyUnicode_FromString
#undef PyUnicode_FromFormat
#define PyUnicode_Check PyString_Check
#define PyUnicode_AsFormat PyString_AsFormat
#define PyUnicode_FromFormat PyString_FromFormat
#define PyUnicode_FromString PyString_FromString
#define PyUnicode_AsUTF8 PyString_AsString

#ifdef WIN32
#undef WITH_THREAD
#endif
#endif

/* #define for Python 3 */
#if PY_MAJOR_VERSION == 3
#define IS_PY3K
#endif

#define NONE 0
#define ALLOW_THREADS 1
#define RESTORE_CONTEXT 2

#ifdef WITH_THREAD
#define ACQUIRE_XCHAT_LOCK() PyThread_acquire_lock(xchat_lock, 1)
#define RELEASE_XCHAT_LOCK() PyThread_release_lock(xchat_lock)
#define BEGIN_XCHAT_CALLS(x) \
	do { \
		PyObject *calls_plugin = NULL; \
		PyThreadState *calls_thread; \
		if ((x) & RESTORE_CONTEXT) \
			calls_plugin = Plugin_GetCurrent(); \
		calls_thread = PyEval_SaveThread(); \
		ACQUIRE_XCHAT_LOCK(); \
		if (!((x) & ALLOW_THREADS)) { \
			PyEval_RestoreThread(calls_thread); \
			calls_thread = NULL; \
		} \
		if (calls_plugin) \
			hexchat_set_context(ph, \
				Plugin_GetContext(calls_plugin)); \
		while (0)
#define END_XCHAT_CALLS() \
		RELEASE_XCHAT_LOCK(); \
		if (calls_thread) \
			PyEval_RestoreThread(calls_thread); \
	} while(0)
#else
#define ACQUIRE_XCHAT_LOCK()
#define RELEASE_XCHAT_LOCK()
#define BEGIN_XCHAT_CALLS(x)
#define END_XCHAT_CALLS()
#endif

#ifdef WITH_THREAD

#define BEGIN_PLUGIN(plg) \
	do { \
	hexchat_context *begin_plugin_ctx = hexchat_get_context(ph); \
	RELEASE_XCHAT_LOCK(); \
	Plugin_AcquireThread(plg); \
	Plugin_SetContext(plg, begin_plugin_ctx); \
	} while (0)
#define END_PLUGIN(plg) \
	do { \
	Plugin_ReleaseThread(plg); \
	ACQUIRE_XCHAT_LOCK(); \
	} while (0)

#else /* !WITH_THREAD (win32) */

static PyThreadState *pTempThread;

#define BEGIN_PLUGIN(plg) \
	do { \
	hexchat_context *begin_plugin_ctx = hexchat_get_context(ph); \
	RELEASE_XCHAT_LOCK(); \
	PyEval_AcquireLock(); \
	pTempThread = PyThreadState_Swap(((PluginObject *)(plg))->tstate); \
	Plugin_SetContext(plg, begin_plugin_ctx); \
	} while (0)
#define END_PLUGIN(plg) \
	do { \
	((PluginObject *)(plg))->tstate = PyThreadState_Swap(pTempThread); \
	PyEval_ReleaseLock(); \
	ACQUIRE_XCHAT_LOCK(); \
	} while (0)

#endif /* !WITH_THREAD */

#define Plugin_Swap(x) \
	PyThreadState_Swap(((PluginObject *)(x))->tstate)
#define Plugin_AcquireThread(x) \
	PyEval_AcquireThread(((PluginObject *)(x))->tstate)
#define Plugin_ReleaseThread(x) \
	Util_ReleaseThread(((PluginObject *)(x))->tstate)
#define Plugin_GetFilename(x) \
	(((PluginObject *)(x))->filename)
#define Plugin_GetName(x) \
	(((PluginObject *)(x))->name)
#define Plugin_GetVersion(x) \
	(((PluginObject *)(x))->version)
#define Plugin_GetDesc(x) \
	(((PluginObject *)(x))->description)
#define Plugin_GetHooks(x) \
	(((PluginObject *)(x))->hooks)
#define Plugin_GetContext(x) \
	(((PluginObject *)(x))->context)
#define Plugin_SetFilename(x, y) \
	((PluginObject *)(x))->filename = (y);
#define Plugin_SetName(x, y) \
	((PluginObject *)(x))->name = (y);
#define Plugin_SetVersion(x, y) \
	((PluginObject *)(x))->version = (y);
#define Plugin_SetDescription(x, y) \
	((PluginObject *)(x))->description = (y);
#define Plugin_SetHooks(x, y) \
	((PluginObject *)(x))->hooks = (y);
#define Plugin_SetContext(x, y) \
	((PluginObject *)(x))->context = (y);
#define Plugin_SetGui(x, y) \
	((PluginObject *)(x))->gui = (y);

#define HOOK_XCHAT  1
#define HOOK_XCHAT_ATTR 2
#define HOOK_UNLOAD 3

/* ===================================================================== */
/* Object definitions */

typedef struct {
	PyObject_HEAD
	int softspace; /* We need it for print support. */
} XChatOutObject;

typedef struct {
	PyObject_HEAD
	hexchat_context *context;
} ContextObject;

typedef struct {
	PyObject_HEAD
	PyObject *time;
} AttributeObject;

typedef struct {
	PyObject_HEAD
	const char *listname;
	PyObject *dict;
} ListItemObject;

typedef struct {
	PyObject_HEAD
	char *name;
	char *version;
	char *filename;
	char *description;
	GSList *hooks;
	PyThreadState *tstate;
	hexchat_context *context;
	void *gui;
} PluginObject;

typedef struct {
	int type;
	PyObject *plugin;
	PyObject *callback;
	PyObject *userdata;
	char *name;
	void *data; /* A handle, when type == HOOK_XCHAT */
} Hook;


/* ===================================================================== */
/* Function declarations */

static PyObject *Util_BuildList(char *word[]);
static PyObject *Util_BuildEOLList(char *word[]);
static void Util_Autoload(void);
static char *Util_Expand(char *filename);

static int Callback_Server(char *word[], char *word_eol[], hexchat_event_attrs *attrs, void *userdata);
static int Callback_Command(char *word[], char *word_eol[], void *userdata);
static int Callback_Print_Attrs(char *word[], hexchat_event_attrs *attrs, void *userdata);
static int Callback_Print(char *word[], void *userdata);
static int Callback_Timer(void *userdata);
static int Callback_ThreadTimer(void *userdata);

static PyObject *XChatOut_New(void);
static PyObject *XChatOut_write(PyObject *self, PyObject *args);
static void XChatOut_dealloc(PyObject *self);

static PyObject *Attribute_New(hexchat_event_attrs *attrs);

static void Context_dealloc(PyObject *self);
static PyObject *Context_set(ContextObject *self, PyObject *args);
static PyObject *Context_command(ContextObject *self, PyObject *args);
static PyObject *Context_prnt(ContextObject *self, PyObject *args);
static PyObject *Context_get_info(ContextObject *self, PyObject *args);
static PyObject *Context_get_list(ContextObject *self, PyObject *args);
static PyObject *Context_compare(ContextObject *a, ContextObject *b, int op);
static PyObject *Context_FromContext(hexchat_context *context);
static PyObject *Context_FromServerAndChannel(char *server, char *channel);

static PyObject *Plugin_New(char *filename, PyObject *xcoobj);
static PyObject *Plugin_GetCurrent(void);
static PluginObject *Plugin_ByString(char *str);
static Hook *Plugin_AddHook(int type, PyObject *plugin, PyObject *callback,
			    PyObject *userdata, char *name, void *data);
static Hook *Plugin_FindHook(PyObject *plugin, char *name);
static void Plugin_RemoveHook(PyObject *plugin, Hook *hook);
static void Plugin_RemoveAllHooks(PyObject *plugin);

static PyObject *Module_hexchat_command(PyObject *self, PyObject *args);
static PyObject *Module_xchat_prnt(PyObject *self, PyObject *args);
static PyObject *Module_hexchat_get_context(PyObject *self, PyObject *args);
static PyObject *Module_hexchat_find_context(PyObject *self, PyObject *args,
					   PyObject *kwargs);
static PyObject *Module_hexchat_get_info(PyObject *self, PyObject *args);
static PyObject *Module_hexchat_hook_command(PyObject *self, PyObject *args,
					   PyObject *kwargs);
static PyObject *Module_hexchat_hook_server(PyObject *self, PyObject *args,
					  PyObject *kwargs);
static PyObject *Module_hexchat_hook_print(PyObject *self, PyObject *args,
					 PyObject *kwargs);
static PyObject *Module_hexchat_hook_timer(PyObject *self, PyObject *args,
					 PyObject *kwargs);
static PyObject *Module_hexchat_unhook(PyObject *self, PyObject *args);
static PyObject *Module_hexchat_get_info(PyObject *self, PyObject *args);
static PyObject *Module_xchat_get_list(PyObject *self, PyObject *args);
static PyObject *Module_xchat_get_lists(PyObject *self, PyObject *args);
static PyObject *Module_hexchat_nickcmp(PyObject *self, PyObject *args);
static PyObject *Module_hexchat_strip(PyObject *self, PyObject *args);
static PyObject *Module_hexchat_pluginpref_set(PyObject *self, PyObject *args);
static PyObject *Module_hexchat_pluginpref_get(PyObject *self, PyObject *args);
static PyObject *Module_hexchat_pluginpref_delete(PyObject *self, PyObject *args);
static PyObject *Module_hexchat_pluginpref_list(PyObject *self, PyObject *args);

static void IInterp_Exec(char *command);
static int IInterp_Cmd(char *word[], char *word_eol[], void *userdata);

static void Command_PyList(void);
static void Command_PyLoad(char *filename);
static void Command_PyUnload(char *name);
static void Command_PyReload(char *name);
static void Command_PyAbout(void);
static int Command_Py(char *word[], char *word_eol[], void *userdata);

/* ===================================================================== */
/* Static declarations and definitions */

static PyTypeObject Plugin_Type;
static PyTypeObject XChatOut_Type;
static PyTypeObject Context_Type;
static PyTypeObject ListItem_Type;
static PyTypeObject Attribute_Type;

static PyThreadState *main_tstate = NULL;
static void *thread_timer = NULL;

static hexchat_plugin *ph;
static GSList *plugin_list = NULL;

static PyObject *interp_plugin = NULL;
static PyObject *xchatout = NULL;

#ifdef WITH_THREAD
static PyThread_type_lock xchat_lock = NULL;
#endif

static const char usage[] = "\
Usage: /PY LOAD   <filename>\n\
           UNLOAD <filename|name>\n\
           RELOAD <filename|name>\n\
           LIST\n\
           EXEC <command>\n\
           CONSOLE\n\
           ABOUT\n\
\n";

static const char about[] = "HexChat Python interface version " VERSION "\n";

/* ===================================================================== */
/* Utility functions */

static PyObject *
Util_BuildList(char *word[])
{
	PyObject *list;
	int listsize = 31;
	int i;
	/* Find the last valid array member; there may be intermediate NULLs that
	 * would otherwise cause us to drop some members. */
	while (listsize > 0 &&
	       (word[listsize] == NULL || word[listsize][0] == 0))
		listsize--;
	list = PyList_New(listsize);
	if (list == NULL) {
		PyErr_Print();
		return NULL;
	}
	for (i = 1; i <= listsize; i++) {
		PyObject *o;
		if (word[i] == NULL) {
			Py_INCREF(Py_None);
			o = Py_None;
		} else {
			/* This handles word[i][0] == 0 automatically. */
			o = PyUnicode_FromString(word[i]);
		}
		PyList_SetItem(list, i - 1, o);
	}
	return list;
}

static PyObject *
Util_BuildEOLList(char *word[])
{
	PyObject *list;
	int listsize = 31;
	int i;
	char *accum = NULL;
	char *last = NULL;

	/* Find the last valid array member; there may be intermediate NULLs that
	 * would otherwise cause us to drop some members. */
	while (listsize > 0 &&
	       (word[listsize] == NULL || word[listsize][0] == 0))
		listsize--;
	list = PyList_New(listsize);
	if (list == NULL) {
		PyErr_Print();
		return NULL;
	}
	for (i = listsize; i > 0; i--) {
		char *part = word[i];
		PyObject *uni_part;
		if (accum == NULL) {
			accum = g_strdup (part);
		} else if (part != NULL && part[0] != 0) {
			last = accum;
			accum = g_strjoin(" ", part, last, NULL);
			g_free (last);
			last = NULL;

			if (accum == NULL) {
				Py_DECREF(list);
				hexchat_print(ph, "Not enough memory to alloc accum"
				              "for python plugin callback");
				return NULL;
			}
		}
		uni_part = PyUnicode_FromString(accum);
		PyList_SetItem(list, i - 1, uni_part);
	}

	g_free (last);
	g_free (accum);

	return list;
}

static void
Util_Autoload_from (const char *dir_name)
{
	gchar *oldcwd;
	const char *entry_name;
	GDir *dir;

	oldcwd = g_get_current_dir ();
	if (oldcwd == NULL)
		return;
	if (g_chdir(dir_name) != 0)
	{
		g_free (oldcwd);
		return;
	}
	dir = g_dir_open (".", 0, NULL);
	if (dir == NULL)
	{
		g_free (oldcwd);
		return;
	}
	while ((entry_name = g_dir_read_name (dir)))
	{
		if (g_str_has_suffix (entry_name, ".py"))
			Command_PyLoad((char*)entry_name);
	}
	g_dir_close (dir);
	g_chdir (oldcwd);
}

static void
Util_Autoload()
{
	const char *xdir;
	char *sub_dir;
	/* we need local filesystem encoding for g_chdir, g_dir_open etc */

	xdir = hexchat_get_info(ph, "configdir");

	/* auto-load from subdirectory addons */
	sub_dir = g_build_filename (xdir, "addons", NULL);
	Util_Autoload_from(sub_dir);
	g_free (sub_dir);
}

static char *
Util_Expand(char *filename)
{
	char *expanded;

	/* Check if this is an absolute path. */
	if (g_path_is_absolute(filename)) {
		if (g_file_test(filename, G_FILE_TEST_EXISTS))
			return g_strdup(filename);
		else
			return NULL;
	}

	/* Check if it starts with ~/ and expand the home if positive. */
	if (*filename == '~' && *(filename+1) == '/') {
		expanded = g_build_filename(g_get_home_dir(),
					    filename+2, NULL);
		if (g_file_test(expanded, G_FILE_TEST_EXISTS))
			return expanded;
		else {
			g_free(expanded);
			return NULL;
		}
	}

	/* Check if it's in the current directory. */
	expanded = g_build_filename(g_get_current_dir(),
				    filename, NULL);
	if (g_file_test(expanded, G_FILE_TEST_EXISTS))
		return expanded;
	g_free(expanded);

	/* Check if ~/.config/hexchat/addons/<filename> exists. */
	expanded = g_build_filename(hexchat_get_info(ph, "configdir"),
				    "addons", filename, NULL);
	if (g_file_test(expanded, G_FILE_TEST_EXISTS))
		return expanded;
	g_free(expanded);

	return NULL;
}

/* Similar to PyEval_ReleaseThread, but accepts NULL thread states. */
static void
Util_ReleaseThread(PyThreadState *tstate)
{
	PyThreadState *old_tstate;
	if (tstate == NULL)
		Py_FatalError("PyEval_ReleaseThread: NULL thread state");
	old_tstate = PyThreadState_Swap(NULL);
	if (old_tstate != tstate && old_tstate != NULL)
		Py_FatalError("PyEval_ReleaseThread: wrong thread state");
	PyEval_ReleaseLock();
}

/* ===================================================================== */
/* Hookable functions. These are the entry points to python code, besides
 * the load function, and the hooks for interactive interpreter. */

static int
Callback_Server(char *word[], char *word_eol[], hexchat_event_attrs *attrs, void *userdata)
{
	Hook *hook = (Hook *) userdata;
	PyObject *retobj;
	PyObject *word_list, *word_eol_list;
	PyObject *attributes;
	int ret = 0;
	PyObject *plugin;

	plugin = hook->plugin;
	BEGIN_PLUGIN(plugin);

	word_list = Util_BuildList(word);
	if (word_list == NULL) {
		END_PLUGIN(plugin);
		return 0;
	}
	word_eol_list = Util_BuildList(word_eol);
	if (word_eol_list == NULL) {
		Py_DECREF(word_list);
		END_PLUGIN(plugin);
		return 0;
	}

	attributes = Attribute_New(attrs);

	if (hook->type == HOOK_XCHAT_ATTR)
		retobj = PyObject_CallFunction(hook->callback, "(OOOO)", word_list,
					       word_eol_list, hook->userdata, attributes);
	else
		retobj = PyObject_CallFunction(hook->callback, "(OOO)", word_list,
					       word_eol_list, hook->userdata);
	Py_DECREF(word_list);
	Py_DECREF(word_eol_list);
	Py_DECREF(attributes);

	if (retobj == Py_None) {
		ret = HEXCHAT_EAT_NONE;
		Py_DECREF(retobj);
	} else if (retobj) {
		ret = PyLong_AsLong(retobj);
		Py_DECREF(retobj);
	} else {
		PyErr_Print();
	}

	END_PLUGIN(plugin);

	return ret;
}

static int
Callback_Command(char *word[], char *word_eol[], void *userdata)
{
	Hook *hook = (Hook *) userdata;
	PyObject *retobj;
	PyObject *word_list, *word_eol_list;
	int ret = 0;
	PyObject *plugin;

	plugin = hook->plugin;
	BEGIN_PLUGIN(plugin);

	word_list = Util_BuildList(word);
	if (word_list == NULL) {
		END_PLUGIN(plugin);
		return 0;
	}
	word_eol_list = Util_BuildList(word_eol);
	if (word_eol_list == NULL) {
		Py_DECREF(word_list);
		END_PLUGIN(plugin);
		return 0;
	}

	retobj = PyObject_CallFunction(hook->callback, "(OOO)", word_list,
				       word_eol_list, hook->userdata);
	Py_DECREF(word_list);
	Py_DECREF(word_eol_list);

	if (retobj == Py_None) {
		ret = HEXCHAT_EAT_NONE;
		Py_DECREF(retobj);
	} else if (retobj) {
		ret = PyLong_AsLong(retobj);
		Py_DECREF(retobj);
	} else {
		PyErr_Print();
	}

	END_PLUGIN(plugin);

	return ret;
}

static int
Callback_Print_Attrs(char *word[], hexchat_event_attrs *attrs, void *userdata)
{
	Hook *hook = (Hook *) userdata;
	PyObject *retobj;
	PyObject *word_list;
	PyObject *word_eol_list;
	PyObject *attributes;
	int ret = 0;
	PyObject *plugin;

	plugin = hook->plugin;
	BEGIN_PLUGIN(plugin);

	word_list = Util_BuildList(word);
	if (word_list == NULL) {
		END_PLUGIN(plugin);
		return 0;
	}
	word_eol_list = Util_BuildEOLList(word);
	if (word_eol_list == NULL) {
		Py_DECREF(word_list);
		END_PLUGIN(plugin);
		return 0;
	}

	attributes = Attribute_New(attrs);

	retobj = PyObject_CallFunction(hook->callback, "(OOOO)", word_list,
					    word_eol_list, hook->userdata, attributes);

	Py_DECREF(word_list);
	Py_DECREF(word_eol_list);
	Py_DECREF(attributes);

	if (retobj == Py_None) {
		ret = HEXCHAT_EAT_NONE;
		Py_DECREF(retobj);
	} else if (retobj) {
		ret = PyLong_AsLong(retobj);
		Py_DECREF(retobj);
	} else {
		PyErr_Print();
	}

	END_PLUGIN(plugin);

	return ret;
}

static int
Callback_Print(char *word[], void *userdata)
{
	Hook *hook = (Hook *) userdata;
	PyObject *retobj;
	PyObject *word_list;
	PyObject *word_eol_list;
	int ret = 0;
	PyObject *plugin;

	plugin = hook->plugin;
	BEGIN_PLUGIN(plugin);

	word_list = Util_BuildList(word);
	if (word_list == NULL) {
		END_PLUGIN(plugin);
		return 0;
	}
	word_eol_list = Util_BuildEOLList(word);
	if (word_eol_list == NULL) {
		Py_DECREF(word_list);
		END_PLUGIN(plugin);
		return 0;
	}

	retobj = PyObject_CallFunction(hook->callback, "(OOO)", word_list,
					       word_eol_list, hook->userdata);

	Py_DECREF(word_list);
	Py_DECREF(word_eol_list);

	if (retobj == Py_None) {
		ret = HEXCHAT_EAT_NONE;
		Py_DECREF(retobj);
	} else if (retobj) {
		ret = PyLong_AsLong(retobj);
		Py_DECREF(retobj);
	} else {
		PyErr_Print();
	}

	END_PLUGIN(plugin);

	return ret;
}

static int
Callback_Timer(void *userdata)
{
	Hook *hook = (Hook *) userdata;
	PyObject *retobj;
	int ret = 0;
	PyObject *plugin;

	plugin = hook->plugin;

	BEGIN_PLUGIN(hook->plugin);

	retobj = PyObject_CallFunction(hook->callback, "(O)", hook->userdata);

	if (retobj) {
		ret = PyObject_IsTrue(retobj);
		Py_DECREF(retobj);
	} else {
		PyErr_Print();
	}

	/* Returning 0 for this callback unhooks itself. */
	if (ret == 0)
		Plugin_RemoveHook(plugin, hook);

	END_PLUGIN(plugin);

	return ret;
}

#ifdef WITH_THREAD
static int
Callback_ThreadTimer(void *userdata)
{
	RELEASE_XCHAT_LOCK();
#ifndef WIN32
	usleep(1);
#endif
	ACQUIRE_XCHAT_LOCK();
	return 1;
}
#endif

/* ===================================================================== */
/* XChatOut object */

/* We keep this information global, so we can reset it when the
 * deinit function is called. */
/* XXX This should be somehow bound to the printing context. */
static GString *xchatout_buffer = NULL;

static PyObject *
XChatOut_New()
{
	XChatOutObject *xcoobj;
	xcoobj = PyObject_New(XChatOutObject, &XChatOut_Type);
	if (xcoobj != NULL)
		xcoobj->softspace = 0;
	return (PyObject *) xcoobj;
}

static void
XChatOut_dealloc(PyObject *self)
{
	Py_TYPE(self)->tp_free((PyObject *)self);
}

/* This is a little bit complex because we have to buffer data
 * until a \n is received, since xchat breaks the line automatically.
 * We also crop the last \n for this reason. */
static PyObject *
XChatOut_write(PyObject *self, PyObject *args)
{
	gboolean add_space;
	char *data, *pos;

	if (!PyArg_ParseTuple(args, "s:write", &data))
		return NULL;
	if (!data || !*data) {
		Py_RETURN_NONE;
	}
	BEGIN_XCHAT_CALLS(RESTORE_CONTEXT|ALLOW_THREADS);
	if (((XChatOutObject *)self)->softspace) {
		add_space = TRUE;
		((XChatOutObject *)self)->softspace = 0;
	} else {
		add_space = FALSE;
	}

	g_string_append (xchatout_buffer, data);

	/* If not end of line add space to continue buffer later */
	if (add_space && xchatout_buffer->str[xchatout_buffer->len - 1] != '\n')
	{
		g_string_append_c (xchatout_buffer, ' ');
	}

	/* If there is an end of line print up to that */
	if ((pos = strrchr (xchatout_buffer->str, '\n')))
	{
		*pos = '\0';
		hexchat_print (ph, xchatout_buffer->str);

		/* Then remove it from buffer */
		g_string_erase (xchatout_buffer, 0, pos - xchatout_buffer->str + 1);
	}

	END_XCHAT_CALLS();
	Py_RETURN_NONE;
}

#define OFF(x) offsetof(XChatOutObject, x)

static PyMemberDef XChatOut_members[] = {
	{"softspace", T_INT, OFF(softspace), 0},
	{0}
};

static PyMethodDef XChatOut_methods[] = {
	{"write", XChatOut_write, METH_VARARGS},
	{NULL, NULL}
};

static PyTypeObject XChatOut_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"hexchat.XChatOut",	/*tp_name*/
	sizeof(XChatOutObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	XChatOut_dealloc,	/*tp_dealloc*/
	0,			/*tp_print*/
	0,			/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
        0,                      /*tp_call*/
        0,                      /*tp_str*/
        PyObject_GenericGetAttr,/*tp_getattro*/
        PyObject_GenericSetAttr,/*tp_setattro*/
        0,                      /*tp_as_buffer*/
        Py_TPFLAGS_DEFAULT,     /*tp_flags*/
        0,                      /*tp_doc*/
        0,                      /*tp_traverse*/
        0,                      /*tp_clear*/
        0,                      /*tp_richcompare*/
        0,                      /*tp_weaklistoffset*/
        0,                      /*tp_iter*/
        0,                      /*tp_iternext*/
        XChatOut_methods,       /*tp_methods*/
        XChatOut_members,       /*tp_members*/
        0,                      /*tp_getset*/
        0,                      /*tp_base*/
        0,                      /*tp_dict*/
        0,                      /*tp_descr_get*/
        0,                      /*tp_descr_set*/
        0,                      /*tp_dictoffset*/
        0,                      /*tp_init*/
        PyType_GenericAlloc,    /*tp_alloc*/
        PyType_GenericNew,      /*tp_new*/
      	PyObject_Del,          /*tp_free*/
        0,                      /*tp_is_gc*/
};


/* ===================================================================== */
/* Attribute object */

#undef OFF
#define OFF(x) offsetof(AttributeObject, x)

static PyMemberDef Attribute_members[] = {
	{"time", T_OBJECT, OFF(time), 0},
	{0}
};

static void
Attribute_dealloc(PyObject *self)
{
	Py_DECREF(((AttributeObject*)self)->time);
	Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
Attribute_repr(PyObject *self)
{
	return PyUnicode_FromFormat("<Attribute object at %p>", self);
}

static PyTypeObject Attribute_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"hexchat.Attribute",	/*tp_name*/
	sizeof(AttributeObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	Attribute_dealloc,	/*tp_dealloc*/
	0,			/*tp_print*/
	0,			/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	Attribute_repr,		/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
        0,                      /*tp_call*/
        0,                      /*tp_str*/
        PyObject_GenericGetAttr,/*tp_getattro*/
        PyObject_GenericSetAttr,/*tp_setattro*/
        0,                      /*tp_as_buffer*/
        Py_TPFLAGS_DEFAULT,     /*tp_flags*/
        0,                      /*tp_doc*/
        0,                      /*tp_traverse*/
        0,                      /*tp_clear*/
        0,                      /*tp_richcompare*/
        0,                      /*tp_weaklistoffset*/
        0,                      /*tp_iter*/
        0,                      /*tp_iternext*/
        0,                      /*tp_methods*/
        Attribute_members,		/*tp_members*/
        0,                      /*tp_getset*/
        0,                      /*tp_base*/
        0,                      /*tp_dict*/
        0,                      /*tp_descr_get*/
        0,                      /*tp_descr_set*/
        0,						/*tp_dictoffset*/
        0,                      /*tp_init*/
        PyType_GenericAlloc,    /*tp_alloc*/
        PyType_GenericNew,      /*tp_new*/
      	PyObject_Del,          /*tp_free*/
        0,                      /*tp_is_gc*/
};

static PyObject *
Attribute_New(hexchat_event_attrs *attrs)
{
	AttributeObject *attr;
	attr = PyObject_New(AttributeObject, &Attribute_Type);
	if (attr != NULL) {
		attr->time = PyLong_FromLong((long)attrs->server_time_utc);
	}
	return (PyObject *) attr;
}


/* ===================================================================== */
/* Context object */

static void
Context_dealloc(PyObject *self)
{
	Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
Context_set(ContextObject *self, PyObject *args)
{
	PyObject *plugin = Plugin_GetCurrent();
	Plugin_SetContext(plugin, self->context);
	Py_RETURN_NONE;
}

static PyObject *
Context_command(ContextObject *self, PyObject *args)
{
	char *text;
	if (!PyArg_ParseTuple(args, "s:command", &text))
		return NULL;
	BEGIN_XCHAT_CALLS(ALLOW_THREADS);
	hexchat_set_context(ph, self->context);
	hexchat_command(ph, text);
	END_XCHAT_CALLS();
	Py_RETURN_NONE;
}

static PyObject *
Context_prnt(ContextObject *self, PyObject *args)
{
	char *text;
	if (!PyArg_ParseTuple(args, "s:prnt", &text))
		return NULL;
	BEGIN_XCHAT_CALLS(ALLOW_THREADS);
	hexchat_set_context(ph, self->context);
	hexchat_print(ph, text);
	END_XCHAT_CALLS();
	Py_RETURN_NONE;
}

static PyObject *
Context_emit_print(ContextObject *self, PyObject *args, PyObject *kwargs)
{
	char *argv[6];
	char *name;
	int res;
	long time = 0;
	hexchat_event_attrs *attrs;
	char *kwlist[] = {"name", "arg1", "arg2", "arg3",
					"arg4", "arg5", "arg6", 
					"time", NULL};
	memset(&argv, 0, sizeof(char*)*6);
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|ssssssl:print_event", kwlist, &name,
			      &argv[0], &argv[1], &argv[2],
			      &argv[3], &argv[4], &argv[5],
				  &time))
		return NULL;
	BEGIN_XCHAT_CALLS(ALLOW_THREADS);
	hexchat_set_context(ph, self->context);
	attrs = hexchat_event_attrs_create(ph);
	attrs->server_time_utc = (time_t)time; 
	
	res = hexchat_emit_print_attrs(ph, attrs, name, argv[0], argv[1], argv[2],
					 argv[3], argv[4], argv[5], NULL);

	hexchat_event_attrs_free(ph, attrs);
	END_XCHAT_CALLS();
	return PyLong_FromLong(res);
}

static PyObject *
Context_get_info(ContextObject *self, PyObject *args)
{
	const char *info;
	char *name;
	if (!PyArg_ParseTuple(args, "s:get_info", &name))
		return NULL;
	BEGIN_XCHAT_CALLS(NONE);
	hexchat_set_context(ph, self->context);
	info = hexchat_get_info(ph, name);
	END_XCHAT_CALLS();
	if (info == NULL) {
		Py_RETURN_NONE;
	}
	return PyUnicode_FromString(info);
}

static PyObject *
Context_get_list(ContextObject *self, PyObject *args)
{
	PyObject *plugin = Plugin_GetCurrent();
	hexchat_context *saved_context = Plugin_GetContext(plugin);
	PyObject *ret;
	Plugin_SetContext(plugin, self->context);
	ret = Module_xchat_get_list((PyObject*)self, args);
	Plugin_SetContext(plugin, saved_context);
	return ret;
}

/* needed to make context1 == context2 work */
static PyObject *
Context_compare(ContextObject *a, ContextObject *b, int op)
{
	PyObject *ret;
	/* check for == */
	if (op == Py_EQ)
		ret = (a->context == b->context ? Py_True : Py_False);
	/* check for != */
	else if (op == Py_NE)
		ret = (a->context != b->context ? Py_True : Py_False);
	/* only makes sense as == and != */
	else
	{
		PyErr_SetString(PyExc_TypeError, "contexts are either equal or not equal");
		ret = Py_None;
	}

	Py_INCREF(ret);
	return ret;
}

static PyMethodDef Context_methods[] = {
	{"set", (PyCFunction) Context_set, METH_NOARGS},
	{"command", (PyCFunction) Context_command, METH_VARARGS},
	{"prnt", (PyCFunction) Context_prnt, METH_VARARGS},
	{"emit_print", (PyCFunction) Context_emit_print, METH_VARARGS|METH_KEYWORDS},
	{"get_info", (PyCFunction) Context_get_info, METH_VARARGS},
	{"get_list", (PyCFunction) Context_get_list, METH_VARARGS},
	{NULL, NULL}
};

static PyTypeObject Context_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"hexchat.Context",	/*tp_name*/
	sizeof(ContextObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	Context_dealloc,        /*tp_dealloc*/
	0,			/*tp_print*/
	0,			/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
        0,                      /*tp_call*/
        0,                      /*tp_str*/
        PyObject_GenericGetAttr,/*tp_getattro*/
        PyObject_GenericSetAttr,/*tp_setattro*/
        0,                      /*tp_as_buffer*/
        Py_TPFLAGS_DEFAULT,     /*tp_flags*/
        0,                      /*tp_doc*/
        0,                      /*tp_traverse*/
        0,                      /*tp_clear*/
        (richcmpfunc)Context_compare,    /*tp_richcompare*/
        0,                      /*tp_weaklistoffset*/
        0,                      /*tp_iter*/
        0,                      /*tp_iternext*/
        Context_methods,        /*tp_methods*/
        0,                      /*tp_members*/
        0,                      /*tp_getset*/
        0,                      /*tp_base*/
        0,                      /*tp_dict*/
        0,                      /*tp_descr_get*/
        0,                      /*tp_descr_set*/
        0,                      /*tp_dictoffset*/
        0,                      /*tp_init*/
        PyType_GenericAlloc,    /*tp_alloc*/
        PyType_GenericNew,      /*tp_new*/
      	PyObject_Del,          /*tp_free*/
        0,                      /*tp_is_gc*/
};

static PyObject *
Context_FromContext(hexchat_context *context)
{
	ContextObject *ctxobj = PyObject_New(ContextObject, &Context_Type);
	if (ctxobj != NULL)
		ctxobj->context = context;
	return (PyObject *) ctxobj;
}

static PyObject *
Context_FromServerAndChannel(char *server, char *channel)
{
	ContextObject *ctxobj;
	hexchat_context *context;
	BEGIN_XCHAT_CALLS(NONE);
	context = hexchat_find_context(ph, server, channel);
	END_XCHAT_CALLS();
	if (context == NULL)
		return NULL;
	ctxobj = PyObject_New(ContextObject, &Context_Type);
	if (ctxobj == NULL)
		return NULL;
	ctxobj->context = context;
	return (PyObject *) ctxobj;
}


/* ===================================================================== */
/* ListItem object */

#undef OFF
#define OFF(x) offsetof(ListItemObject, x)

static PyMemberDef ListItem_members[] = {
	{"__dict__", T_OBJECT, OFF(dict), 0},
	{0}
};

static void
ListItem_dealloc(PyObject *self)
{
	Py_DECREF(((ListItemObject*)self)->dict);
	Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
ListItem_repr(PyObject *self)
{
	return PyUnicode_FromFormat("<%s list item at %p>",
			    	   ((ListItemObject*)self)->listname, self);
}

static PyTypeObject ListItem_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"hexchat.ListItem",	/*tp_name*/
	sizeof(ListItemObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	ListItem_dealloc,	/*tp_dealloc*/
	0,			/*tp_print*/
	0,			/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	ListItem_repr,		/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
        0,                      /*tp_call*/
        0,                      /*tp_str*/
        PyObject_GenericGetAttr,/*tp_getattro*/
        PyObject_GenericSetAttr,/*tp_setattro*/
        0,                      /*tp_as_buffer*/
        Py_TPFLAGS_DEFAULT,     /*tp_flags*/
        0,                      /*tp_doc*/
        0,                      /*tp_traverse*/
        0,                      /*tp_clear*/
        0,                      /*tp_richcompare*/
        0,                      /*tp_weaklistoffset*/
        0,                      /*tp_iter*/
        0,                      /*tp_iternext*/
        0,                      /*tp_methods*/
        ListItem_members,       /*tp_members*/
        0,                      /*tp_getset*/
        0,                      /*tp_base*/
        0,                      /*tp_dict*/
        0,                      /*tp_descr_get*/
        0,                      /*tp_descr_set*/
        OFF(dict),              /*tp_dictoffset*/
        0,                      /*tp_init*/
        PyType_GenericAlloc,    /*tp_alloc*/
        PyType_GenericNew,      /*tp_new*/
      	PyObject_Del,          /*tp_free*/
        0,                      /*tp_is_gc*/
};

static PyObject *
ListItem_New(const char *listname)
{
	ListItemObject *item;
	item = PyObject_New(ListItemObject, &ListItem_Type);
	if (item != NULL) {
		/* listname parameter must be statically allocated. */
		item->listname = listname;
		item->dict = PyDict_New();
		if (item->dict == NULL) {
			Py_DECREF(item);
			item = NULL;
		}
	}
	return (PyObject *) item;
}


/* ===================================================================== */
/* Plugin object */

#define GET_MODULE_DATA(x, force) \
	o = PyObject_GetAttrString(m, "__module_" #x "__"); \
	if (o == NULL) { \
		if (force) { \
			hexchat_print(ph, "Module has no __module_" #x "__ " \
					"defined"); \
			goto error; \
		} \
		plugin->x = g_strdup(""); \
	} else {\
		if (!PyUnicode_Check(o)) { \
			hexchat_print(ph, "Variable __module_" #x "__ " \
					"must be a string"); \
			goto error; \
		} \
		plugin->x = g_strdup(PyUnicode_AsUTF8(o)); \
		if (plugin->x == NULL) { \
			hexchat_print(ph, "Not enough memory to allocate " #x); \
			goto error; \
		} \
	}

static PyObject *
Plugin_GetCurrent()
{
	PyObject *plugin;
	plugin = PySys_GetObject("__plugin__");
	if (plugin == NULL)
		PyErr_SetString(PyExc_RuntimeError, "lost sys.__plugin__");
	return plugin;
}

static hexchat_plugin *
Plugin_GetHandle(PluginObject *plugin)
{
	/* This works but the issue is that the script must be ran to get
	 * the name of it thus upon first use it will use the wrong handler
	 * work around would be to run a fake script once to get name? */
#if 0
	/* return fake handle for pluginpref */
	if (plugin->gui != NULL)
		return plugin->gui;
	else
#endif
		return ph;
}

static PluginObject *
Plugin_ByString(char *str)
{
	GSList *list;
	PluginObject *plugin;
	char *basename;
	list = plugin_list;
	while (list != NULL) {
		plugin = (PluginObject *) list->data;
		basename = g_path_get_basename(plugin->filename);
		if (basename == NULL)
			break;
		if (strcasecmp(plugin->name, str) == 0 ||
		    strcasecmp(plugin->filename, str) == 0 ||
		    strcasecmp(basename, str) == 0) {
			g_free(basename);
			return plugin;
		}
		g_free(basename);
		list = list->next;
	}
	return NULL;
}

static Hook *
Plugin_AddHook(int type, PyObject *plugin, PyObject *callback,
	       PyObject *userdata, char *name, void *data)
{
	Hook *hook = g_new(Hook, 1);
	hook->type = type;
	hook->plugin = plugin;
	Py_INCREF(callback);
	hook->callback = callback;
	Py_INCREF(userdata);
	hook->userdata = userdata;
	hook->name = g_strdup (name);
	hook->data = NULL;
	Plugin_SetHooks(plugin, g_slist_append(Plugin_GetHooks(plugin),
					       hook));

	return hook;
}

static Hook *
Plugin_FindHook(PyObject *plugin, char *name)
{
	Hook *hook = NULL;
	GSList *plugin_hooks = Plugin_GetHooks(plugin);
	
	while (plugin_hooks)
	{
		if (g_strcmp0 (((Hook *)plugin_hooks->data)->name, name) == 0)
		{
			hook = (Hook *)plugin_hooks->data;
			break;
		}
		
		plugin_hooks = g_slist_next(plugin_hooks);
	}

	return hook;
}

static void
Plugin_RemoveHook(PyObject *plugin, Hook *hook)
{
	GSList *list;
	/* Is this really a hook of the running plugin? */
	list = g_slist_find(Plugin_GetHooks(plugin), hook);
	if (list) {
		/* Ok, unhook it. */
		if (hook->type != HOOK_UNLOAD) {
			/* This is an xchat hook. Unregister it. */
			BEGIN_XCHAT_CALLS(NONE);
			hexchat_unhook(ph, (hexchat_hook*)hook->data);
			END_XCHAT_CALLS();
		}
		Plugin_SetHooks(plugin,
				g_slist_remove(Plugin_GetHooks(plugin),
					       hook));
		Py_DECREF(hook->callback);
		Py_DECREF(hook->userdata);
		g_free(hook->name);
		g_free(hook);
	}
}

static void
Plugin_RemoveAllHooks(PyObject *plugin)
{
	GSList *list = Plugin_GetHooks(plugin);
	while (list) {
		Hook *hook = (Hook *) list->data;
		if (hook->type != HOOK_UNLOAD) {
			/* This is an xchat hook. Unregister it. */
			BEGIN_XCHAT_CALLS(NONE);
			hexchat_unhook(ph, (hexchat_hook*)hook->data);
			END_XCHAT_CALLS();
		}
		Py_DECREF(hook->callback);
		Py_DECREF(hook->userdata);
		g_free(hook->name);
		g_free(hook);
		list = list->next;
	}
	Plugin_SetHooks(plugin, NULL);
}

static void
Plugin_Delete(PyObject *plugin)
{
	PyThreadState *tstate = ((PluginObject*)plugin)->tstate;
	GSList *list = Plugin_GetHooks(plugin);
	while (list) {
		Hook *hook = (Hook *) list->data;
		if (hook->type == HOOK_UNLOAD) {
			PyObject *retobj;
			retobj = PyObject_CallFunction(hook->callback, "(O)",
						       hook->userdata);
			if (retobj) {
				Py_DECREF(retobj);
			} else {
				PyErr_Print();
				PyErr_Clear();
			}
		}
		list = list->next;
	}
	Plugin_RemoveAllHooks(plugin);
	if (((PluginObject *)plugin)->gui != NULL)
		hexchat_plugingui_remove(ph, ((PluginObject *)plugin)->gui);
	Py_DECREF(plugin);
	/*PyThreadState_Swap(tstate); needed? */
	Py_EndInterpreter(tstate);
}

static PyObject *
Plugin_New(char *filename, PyObject *xcoobj)
{
	PluginObject *plugin = NULL;
	PyObject *m, *o;
#ifdef IS_PY3K
	wchar_t *argv[] = { L"<hexchat>", 0 };
#else
	char *argv[] = { "<hexchat>", 0 };
#endif

	if (filename) {
		char *old_filename = filename;
		filename = Util_Expand(filename);
		if (filename == NULL) {
			hexchat_printf(ph, "File not found: %s", old_filename);
			return NULL;
		}
	}

	/* Allocate plugin structure. */
	plugin = PyObject_New(PluginObject, &Plugin_Type);
	if (plugin == NULL) {
		hexchat_print(ph, "Can't create plugin object");
		goto error;
	}

	Plugin_SetName(plugin, NULL);
	Plugin_SetVersion(plugin, NULL);
	Plugin_SetFilename(plugin, NULL);
	Plugin_SetDescription(plugin, NULL);
	Plugin_SetHooks(plugin, NULL);
	Plugin_SetContext(plugin, hexchat_get_context(ph));
	Plugin_SetGui(plugin, NULL);

	/* Start a new interpreter environment for this plugin. */
	PyEval_AcquireThread(main_tstate);
	plugin->tstate = Py_NewInterpreter();
	if (plugin->tstate == NULL) {
		hexchat_print(ph, "Can't create interpreter state");
		goto error;
	}

	PySys_SetArgv(1, argv);
	PySys_SetObject("__plugin__", (PyObject *) plugin);

	/* Set stdout and stderr to xchatout. */
	Py_INCREF(xcoobj);
	PySys_SetObject("stdout", xcoobj);
	Py_INCREF(xcoobj);
	PySys_SetObject("stderr", xcoobj);

	if (filename) {
#ifdef WIN32
		char *file;
		if (!g_file_get_contents_utf8(filename, &file, NULL, NULL)) {
			hexchat_printf(ph, "Can't open file %s: %s\n",
				     filename, strerror(errno));
			goto error;
		}

		if (PyRun_SimpleString(file) != 0) {
			hexchat_printf(ph, "Error loading module %s\n",
				     filename);
			g_free (file);
			goto error;
		}

		plugin->filename = filename;
		filename = NULL;
		g_free (file);
#else
		FILE *fp;
		plugin->filename = filename;

		/* It's now owned by the plugin. */
		filename = NULL;

		/* Open the plugin file. */
		fp = fopen(plugin->filename, "r");
		if (fp == NULL) {
			hexchat_printf(ph, "Can't open file %s: %s\n",
				     plugin->filename, strerror(errno));
			goto error;
		}

		/* Run the plugin. */
		if (PyRun_SimpleFile(fp, plugin->filename) != 0) {
			hexchat_printf(ph, "Error loading module %s\n",
				     plugin->filename);
			fclose(fp);
			goto error;
		}
		fclose(fp);
#endif
		m = PyDict_GetItemString(PyImport_GetModuleDict(),
					 "__main__");
		if (m == NULL) {
			hexchat_print(ph, "Can't get __main__ module");
			goto error;
		}
		GET_MODULE_DATA(name, 1);
		GET_MODULE_DATA(version, 0);
		GET_MODULE_DATA(description, 0);
		plugin->gui = hexchat_plugingui_add(ph, plugin->filename,
						  plugin->name,
						  plugin->description,
						  plugin->version, NULL);
	}

	PyEval_ReleaseThread(plugin->tstate);

	return (PyObject *) plugin;

error:
	g_free(filename);

	if (plugin) {
		if (plugin->tstate)
			Plugin_Delete((PyObject *)plugin);
		else
			Py_DECREF(plugin);
	}
	PyEval_ReleaseLock();

	return NULL;
}

static void
Plugin_dealloc(PluginObject *self)
{
	g_free(self->filename);
	g_free(self->name);
	g_free(self->version);
	g_free(self->description);
	Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyTypeObject Plugin_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"hexchat.Plugin",		/*tp_name*/
	sizeof(PluginObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	(destructor)Plugin_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	0,			/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
        0,                      /*tp_call*/
        0,                      /*tp_str*/
        PyObject_GenericGetAttr,/*tp_getattro*/
        PyObject_GenericSetAttr,/*tp_setattro*/
        0,                      /*tp_as_buffer*/
        Py_TPFLAGS_DEFAULT,     /*tp_flags*/
        0,                      /*tp_doc*/
        0,                      /*tp_traverse*/
        0,                      /*tp_clear*/
        0,                      /*tp_richcompare*/
        0,                      /*tp_weaklistoffset*/
        0,                      /*tp_iter*/
        0,                      /*tp_iternext*/
        0,                      /*tp_methods*/
        0,                      /*tp_members*/
        0,                      /*tp_getset*/
        0,                      /*tp_base*/
        0,                      /*tp_dict*/
        0,                      /*tp_descr_get*/
        0,                      /*tp_descr_set*/
        0,                      /*tp_dictoffset*/
        0,                      /*tp_init*/
        PyType_GenericAlloc,    /*tp_alloc*/
        PyType_GenericNew,      /*tp_new*/
      	PyObject_Del,          /*tp_free*/
        0,                      /*tp_is_gc*/
};


/* ===================================================================== */
/* XChat module */

static PyObject *
Module_hexchat_command(PyObject *self, PyObject *args)
{
	char *text;
	if (!PyArg_ParseTuple(args, "s:command", &text))
		return NULL;
	BEGIN_XCHAT_CALLS(RESTORE_CONTEXT|ALLOW_THREADS);
	hexchat_command(ph, text);
	END_XCHAT_CALLS();
	Py_RETURN_NONE;
}

static PyObject *
Module_xchat_prnt(PyObject *self, PyObject *args)
{
	char *text;
	if (!PyArg_ParseTuple(args, "s:prnt", &text))
		return NULL;
	BEGIN_XCHAT_CALLS(RESTORE_CONTEXT|ALLOW_THREADS);
	hexchat_print(ph, text);
	END_XCHAT_CALLS();
	Py_RETURN_NONE;
}

static PyObject *
Module_hexchat_emit_print(PyObject *self, PyObject *args, PyObject *kwargs)
{
	char *argv[6];
	char *name;
	int res;
	long time = 0;
	hexchat_event_attrs *attrs;
	char *kwlist[] = {"name", "arg1", "arg2", "arg3",
					"arg4", "arg5", "arg6", 
					"time", NULL};
	memset(&argv, 0, sizeof(char*)*6);
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|ssssssl:print_event", kwlist, &name,
			      &argv[0], &argv[1], &argv[2],
			      &argv[3], &argv[4], &argv[5],
				  &time))
		return NULL;
	BEGIN_XCHAT_CALLS(RESTORE_CONTEXT|ALLOW_THREADS);
	attrs = hexchat_event_attrs_create(ph);
	attrs->server_time_utc = (time_t)time; 
	
	res = hexchat_emit_print_attrs(ph, attrs, name, argv[0], argv[1], argv[2],
					 argv[3], argv[4], argv[5], NULL);

	hexchat_event_attrs_free(ph, attrs);
	END_XCHAT_CALLS();
	return PyLong_FromLong(res);
}

static PyObject *
Module_hexchat_get_info(PyObject *self, PyObject *args)
{
	const char *info;
	char *name;
	if (!PyArg_ParseTuple(args, "s:get_info", &name))
		return NULL;
	BEGIN_XCHAT_CALLS(RESTORE_CONTEXT);
	info = hexchat_get_info(ph, name);
	END_XCHAT_CALLS();
	if (info == NULL) {
		Py_RETURN_NONE;
	}
	if (strcmp (name, "gtkwin_ptr") == 0 || strcmp (name, "win_ptr") == 0)
		return PyUnicode_FromFormat("%p", info); /* format as pointer */
	else
		return PyUnicode_FromString(info);
}

static PyObject *
Module_xchat_get_prefs(PyObject *self, PyObject *args)
{
	PyObject *res;
	const char *info;
	int integer;
	char *name;
	int type;
	if (!PyArg_ParseTuple(args, "s:get_prefs", &name))
		return NULL;
	BEGIN_XCHAT_CALLS(NONE);
	type = hexchat_get_prefs(ph, name, &info, &integer);
	END_XCHAT_CALLS();
	switch (type) {
		case 0:
			Py_INCREF(Py_None);
			res = Py_None;
			break;
		case 1:
			res = PyUnicode_FromString((char*)info);
			break;
		case 2:
		case 3:
			res = PyLong_FromLong(integer);
			break;
		default:
			PyErr_Format(PyExc_RuntimeError,
				     "unknown get_prefs type (%d), "
				     "please report", type);
			res = NULL;
			break;
	}
	return res;
}

static PyObject *
Module_hexchat_get_context(PyObject *self, PyObject *args)
{
	PyObject *plugin;
	PyObject *ctxobj;
	plugin = Plugin_GetCurrent();
	if (plugin == NULL)
		return NULL;
	ctxobj = Context_FromContext(Plugin_GetContext(plugin));
	if (ctxobj == NULL) {
		Py_RETURN_NONE;
	}
	return ctxobj;
}

static PyObject *
Module_hexchat_find_context(PyObject *self, PyObject *args, PyObject *kwargs)
{
	char *server = NULL;
	char *channel = NULL;
	PyObject *ctxobj;
	char *kwlist[] = {"server", "channel", 0};
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|zz:find_context",
					 kwlist, &server, &channel))
		return NULL;
	ctxobj = Context_FromServerAndChannel(server, channel);
	if (ctxobj == NULL) {
		Py_RETURN_NONE;
	}
	return ctxobj;
}

static PyObject *
Module_hexchat_pluginpref_set(PyObject *self, PyObject *args)
{
	PluginObject *plugin = (PluginObject*)Plugin_GetCurrent();
	hexchat_plugin *prefph = Plugin_GetHandle(plugin);
	int result;
	char *var;
	PyObject *value;
		
	if (!PyArg_ParseTuple(args, "sO:set_pluginpref", &var, &value))
		return NULL;
	if (PyLong_Check(value)) {
		int intvalue = PyLong_AsLong(value);
		BEGIN_XCHAT_CALLS(NONE);
		result = hexchat_pluginpref_set_int(prefph, var, intvalue);
		END_XCHAT_CALLS();
	}
	else if (PyUnicode_Check(value)) {
		char *charvalue = PyUnicode_AsUTF8(value);
		BEGIN_XCHAT_CALLS(NONE);
		result = hexchat_pluginpref_set_str(prefph, var, charvalue);
		END_XCHAT_CALLS();
	}
	else
		result = 0;
	return PyBool_FromLong(result);
}

static PyObject *
Module_hexchat_pluginpref_get(PyObject *self, PyObject *args)
{
	PluginObject *plugin = (PluginObject*)Plugin_GetCurrent();
	hexchat_plugin *prefph = Plugin_GetHandle(plugin);
	PyObject *ret;
	char *var;
	char retstr[512];
	int retint;
	int result;
	if (!PyArg_ParseTuple(args, "s:get_pluginpref", &var))
		return NULL;
		
	/* This will always return numbers as integers. */
	BEGIN_XCHAT_CALLS(NONE);
	result = hexchat_pluginpref_get_str(prefph, var, retstr);
	END_XCHAT_CALLS();
	if (result) {
		if (strlen (retstr) <= 12) {
			BEGIN_XCHAT_CALLS(NONE);
			retint = hexchat_pluginpref_get_int(prefph, var);
			END_XCHAT_CALLS();
			if ((retint == -1) && (strcmp(retstr, "-1") != 0))
				ret = PyUnicode_FromString(retstr);
			else
				ret = PyLong_FromLong(retint);
		} else
			ret = PyUnicode_FromString(retstr);
	}
	else
	{
		Py_INCREF(Py_None);
		ret = Py_None;
	}
	return ret;
}

static PyObject *
Module_hexchat_pluginpref_delete(PyObject *self, PyObject *args)
{
	PluginObject *plugin = (PluginObject*)Plugin_GetCurrent();
	hexchat_plugin *prefph = Plugin_GetHandle(plugin);
	char *var;
	int result;
	if (!PyArg_ParseTuple(args, "s:del_pluginpref", &var))
		return NULL;
	BEGIN_XCHAT_CALLS(NONE);
	result = hexchat_pluginpref_delete(prefph, var);
	END_XCHAT_CALLS();
	return PyBool_FromLong(result);
}

static PyObject *
Module_hexchat_pluginpref_list(PyObject *self, PyObject *args)
{
	PluginObject *plugin = (PluginObject*)Plugin_GetCurrent();
	hexchat_plugin *prefph = Plugin_GetHandle(plugin);
	char list[4096];
	char* token;
	int result;
	PyObject *pylist;
	pylist = PyList_New(0);
	BEGIN_XCHAT_CALLS(NONE);
	result = hexchat_pluginpref_list(prefph, list);
	END_XCHAT_CALLS();
	if (result) {
		token = strtok(list, ",");
		while (token != NULL) {
			PyList_Append(pylist, PyUnicode_FromString(token));
			token = strtok (NULL, ",");
		}
	}
	return pylist;
}

static PyObject *
Module_hexchat_hook_command(PyObject *self, PyObject *args, PyObject *kwargs)
{
	char *name;
	PyObject *callback;
	PyObject *userdata = Py_None;
	int priority = HEXCHAT_PRI_NORM;
	char *help = NULL;
	PyObject *plugin;
	Hook *hook;
	char *kwlist[] = {"name", "callback", "userdata",
			  "priority", "help", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO|Oiz:hook_command",
					 kwlist, &name, &callback, &userdata,
					 &priority, &help))
		return NULL;

	plugin = Plugin_GetCurrent();
	if (plugin == NULL)
		return NULL;
	if (!PyCallable_Check(callback)) {
		PyErr_SetString(PyExc_TypeError, "callback is not callable");
		return NULL;
	}

	hook = Plugin_AddHook(HOOK_XCHAT, plugin, callback, userdata, name, NULL);
	if (hook == NULL)
		return NULL;

	BEGIN_XCHAT_CALLS(NONE);
	hook->data = (void*)hexchat_hook_command(ph, name, priority,
					       Callback_Command, help, hook);
	END_XCHAT_CALLS();

	return PyLong_FromVoidPtr(hook);
}

static PyObject *
Module_hexchat_hook_server(PyObject *self, PyObject *args, PyObject *kwargs)
{
	char *name;
	PyObject *callback;
	PyObject *userdata = Py_None;
	int priority = HEXCHAT_PRI_NORM;
	PyObject *plugin;
	Hook *hook;
	char *kwlist[] = {"name", "callback", "userdata", "priority", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO|Oi:hook_server",
					 kwlist, &name, &callback, &userdata,
					 &priority))
		return NULL;

	plugin = Plugin_GetCurrent();
	if (plugin == NULL)
		return NULL;
	if (!PyCallable_Check(callback)) {
		PyErr_SetString(PyExc_TypeError, "callback is not callable");
		return NULL;
	}

	hook = Plugin_AddHook(HOOK_XCHAT, plugin, callback, userdata, NULL, NULL);
	if (hook == NULL)
		return NULL;

	BEGIN_XCHAT_CALLS(NONE);
	hook->data = (void*)hexchat_hook_server_attrs(ph, name, priority,
					      Callback_Server, hook);
	END_XCHAT_CALLS();

	return PyLong_FromVoidPtr(hook);
}

static PyObject *
Module_hexchat_hook_server_attrs(PyObject *self, PyObject *args, PyObject *kwargs)
{
	char *name;
	PyObject *callback;
	PyObject *userdata = Py_None;
	int priority = HEXCHAT_PRI_NORM;
	PyObject *plugin;
	Hook *hook;
	char *kwlist[] = {"name", "callback", "userdata", "priority", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO|Oi:hook_server",
					 kwlist, &name, &callback, &userdata,
					 &priority))
		return NULL;

	plugin = Plugin_GetCurrent();
	if (plugin == NULL)
		return NULL;
	if (!PyCallable_Check(callback)) {
		PyErr_SetString(PyExc_TypeError, "callback is not callable");
		return NULL;
	}

	hook = Plugin_AddHook(HOOK_XCHAT_ATTR, plugin, callback, userdata, NULL, NULL);
	if (hook == NULL)
		return NULL;

	BEGIN_XCHAT_CALLS(NONE);
	hook->data = (void*)hexchat_hook_server_attrs(ph, name, priority,
					      Callback_Server, hook);
	END_XCHAT_CALLS();

	return PyLong_FromVoidPtr(hook);
}

static PyObject *
Module_hexchat_hook_print(PyObject *self, PyObject *args, PyObject *kwargs)
{
	char *name;
	PyObject *callback;
	PyObject *userdata = Py_None;
	int priority = HEXCHAT_PRI_NORM;
	PyObject *plugin;
	Hook *hook;
	char *kwlist[] = {"name", "callback", "userdata", "priority", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO|Oi:hook_print",
					 kwlist, &name, &callback, &userdata,
					 &priority))
		return NULL;

	plugin = Plugin_GetCurrent();
	if (plugin == NULL)
		return NULL;
	if (!PyCallable_Check(callback)) {
		PyErr_SetString(PyExc_TypeError, "callback is not callable");
		return NULL;
	}

	hook = Plugin_AddHook(HOOK_XCHAT, plugin, callback, userdata, name, NULL);
	if (hook == NULL)
		return NULL;

	BEGIN_XCHAT_CALLS(NONE);
	hook->data = (void*)hexchat_hook_print(ph, name, priority,
					     Callback_Print, hook);
	END_XCHAT_CALLS();

	return PyLong_FromVoidPtr(hook);
}

static PyObject *
Module_hexchat_hook_print_attrs(PyObject *self, PyObject *args, PyObject *kwargs)
{
	char *name;
	PyObject *callback;
	PyObject *userdata = Py_None;
	int priority = HEXCHAT_PRI_NORM;
	PyObject *plugin;
	Hook *hook;
	char *kwlist[] = {"name", "callback", "userdata", "priority", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sO|Oi:hook_print_attrs",
					 kwlist, &name, &callback, &userdata,
					 &priority))
		return NULL;

	plugin = Plugin_GetCurrent();
	if (plugin == NULL)
		return NULL;
	if (!PyCallable_Check(callback)) {
		PyErr_SetString(PyExc_TypeError, "callback is not callable");
		return NULL;
	}

	hook = Plugin_AddHook(HOOK_XCHAT_ATTR, plugin, callback, userdata, name, NULL);
	if (hook == NULL)
		return NULL;

	BEGIN_XCHAT_CALLS(NONE);
	hook->data = (void*)hexchat_hook_print_attrs(ph, name, priority,
					     Callback_Print_Attrs, hook);
	END_XCHAT_CALLS();

	return PyLong_FromVoidPtr(hook);
}

static PyObject *
Module_hexchat_hook_timer(PyObject *self, PyObject *args, PyObject *kwargs)
{
	int timeout;
	PyObject *callback;
	PyObject *userdata = Py_None;
	PyObject *plugin;
	Hook *hook;
	char *kwlist[] = {"timeout", "callback", "userdata", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "iO|O:hook_timer",
					 kwlist, &timeout, &callback,
					 &userdata))
		return NULL;

	plugin = Plugin_GetCurrent();
	if (plugin == NULL)
		return NULL;
	if (!PyCallable_Check(callback)) {
		PyErr_SetString(PyExc_TypeError, "callback is not callable");
		return NULL;
	}

	hook = Plugin_AddHook(HOOK_XCHAT, plugin, callback, userdata, NULL, NULL);
	if (hook == NULL)
		return NULL;

	BEGIN_XCHAT_CALLS(NONE);
	hook->data = (void*)hexchat_hook_timer(ph, timeout,
					     Callback_Timer, hook);
	END_XCHAT_CALLS();

	return PyLong_FromVoidPtr(hook);
}

static PyObject *
Module_hexchat_hook_unload(PyObject *self, PyObject *args, PyObject *kwargs)
{
	PyObject *callback;
	PyObject *userdata = Py_None;
	PyObject *plugin;
	Hook *hook;
	char *kwlist[] = {"callback", "userdata", 0};

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:hook_unload",
					 kwlist, &callback, &userdata))
		return NULL;

	plugin = Plugin_GetCurrent();
	if (plugin == NULL)
		return NULL;
	if (!PyCallable_Check(callback)) {
		PyErr_SetString(PyExc_TypeError, "callback is not callable");
		return NULL;
	}

	hook = Plugin_AddHook(HOOK_UNLOAD, plugin, callback, userdata, NULL, NULL);
	if (hook == NULL)
		return NULL;

	return PyLong_FromVoidPtr(hook);
}

static PyObject *
Module_hexchat_unhook(PyObject *self, PyObject *args)
{
	PyObject *plugin;
	PyObject *obj;
	Hook *hook;
	if (!PyArg_ParseTuple(args, "O:unhook", &obj))
		return NULL;
	plugin = Plugin_GetCurrent();
	if (plugin == NULL)
		return NULL;

	if (PyUnicode_Check (obj))
	{
		hook = Plugin_FindHook(plugin, PyUnicode_AsUTF8 (obj));
		while (hook)
		{
			Plugin_RemoveHook(plugin, hook);
			hook = Plugin_FindHook(plugin, PyUnicode_AsUTF8 (obj));
		}
	}
	else
	{
		hook = (Hook *)PyLong_AsVoidPtr(obj);
		Plugin_RemoveHook(plugin, hook);
	}	

	Py_RETURN_NONE;
}

static PyObject *
Module_xchat_get_list(PyObject *self, PyObject *args)
{
	hexchat_list *list;
	PyObject *l;
	const char *name;
	const char *const *fields;
	int i;

	if (!PyArg_ParseTuple(args, "s:get_list", &name))
		return NULL;
	/* This function is thread safe, and returns statically
	 * allocated data. */
	fields = hexchat_list_fields(ph, "lists");
	for (i = 0; fields[i]; i++) {
		if (strcmp(fields[i], name) == 0) {
			/* Use the static allocated one. */
			name = fields[i];
			break;
		}
	}
	if (fields[i] == NULL) {
		PyErr_SetString(PyExc_KeyError, "list not available");
		return NULL;
	}
	l = PyList_New(0);
	if (l == NULL)
		return NULL;
	BEGIN_XCHAT_CALLS(RESTORE_CONTEXT);
	list = hexchat_list_get(ph, (char*)name);
	if (list == NULL)
		goto error;
	fields = hexchat_list_fields(ph, (char*)name);
	while (hexchat_list_next(ph, list)) {
		PyObject *o = ListItem_New(name);
		if (o == NULL || PyList_Append(l, o) == -1) {
			Py_XDECREF(o);
			goto error;
		}
		Py_DECREF(o); /* l is holding a reference */
		for (i = 0; fields[i]; i++) {
			const char *fld = fields[i]+1;
			PyObject *attr = NULL;
			const char *sattr;
			int iattr;
			time_t tattr;
			switch(fields[i][0]) {
			case 's':
				sattr = hexchat_list_str(ph, list, (char*)fld);
				attr = PyUnicode_FromString(sattr?sattr:"");
				break;
			case 'i':
				iattr = hexchat_list_int(ph, list, (char*)fld);
				attr = PyLong_FromLong((long)iattr);
				break;
			case 't':
				tattr = hexchat_list_time(ph, list, (char*)fld);
				attr = PyLong_FromLong((long)tattr);
				break;
			case 'p':
				sattr = hexchat_list_str(ph, list, (char*)fld);
				if (strcmp(fld, "context") == 0) {
					attr = Context_FromContext(
						(hexchat_context*)sattr);
					break;
				}
			default: /* ignore unknown (newly added?) types */
				continue;
			}
			if (attr == NULL)
				goto error;
			PyObject_SetAttrString(o, (char*)fld, attr); /* add reference on attr in o */
			Py_DECREF(attr); /* make o own attr */
		}
	}
	hexchat_list_free(ph, list);
	goto exit;
error:
	if (list)
		hexchat_list_free(ph, list);
	Py_DECREF(l);
	l = NULL;

exit:
	END_XCHAT_CALLS();
	return l;
}

static PyObject *
Module_xchat_get_lists(PyObject *self, PyObject *args)
{
	PyObject *l, *o;
	const char *const *fields;
	int i;
	/* This function is thread safe, and returns statically
	 * allocated data. */
	fields = hexchat_list_fields(ph, "lists");
	l = PyList_New(0);
	if (l == NULL)
		return NULL;
	for (i = 0; fields[i]; i++) {
		o = PyUnicode_FromString(fields[i]);
		if (o == NULL || PyList_Append(l, o) == -1) {
			Py_DECREF(l);
			Py_XDECREF(o);
			return NULL;
		}
		Py_DECREF(o); /* l is holding a reference */
	}
	return l;
}

static PyObject *
Module_hexchat_nickcmp(PyObject *self, PyObject *args)
{
	char *s1, *s2;
	if (!PyArg_ParseTuple(args, "ss:nickcmp", &s1, &s2))
		return NULL;
	return PyLong_FromLong((long) hexchat_nickcmp(ph, s1, s2));
}

static PyObject *
Module_hexchat_strip(PyObject *self, PyObject *args)
{
	PyObject *result;
	char *str, *str2;
	int len = -1, flags = 1 | 2;
	if (!PyArg_ParseTuple(args, "s|ii:strip", &str, &len, &flags))
		return NULL;
	str2 = hexchat_strip(ph, str, len, flags);
	result = PyUnicode_FromString(str2);
	hexchat_free(ph, str2);
	return result;
}

static PyMethodDef Module_xchat_methods[] = {
	{"command",		Module_hexchat_command,
		METH_VARARGS},
	{"prnt",		Module_xchat_prnt,
		METH_VARARGS},
	{"emit_print",		(PyCFunction)Module_hexchat_emit_print,
		METH_VARARGS|METH_KEYWORDS},
	{"get_info",		Module_hexchat_get_info,
		METH_VARARGS},
	{"get_prefs",		Module_xchat_get_prefs,
		METH_VARARGS},
	{"get_context",		Module_hexchat_get_context,
		METH_NOARGS},
	{"find_context",	(PyCFunction)Module_hexchat_find_context,
		METH_VARARGS|METH_KEYWORDS},
	{"set_pluginpref", Module_hexchat_pluginpref_set,
		METH_VARARGS},
	{"get_pluginpref", Module_hexchat_pluginpref_get,
		METH_VARARGS},
	{"del_pluginpref", Module_hexchat_pluginpref_delete,
		METH_VARARGS},
	{"list_pluginpref", Module_hexchat_pluginpref_list,
		METH_VARARGS},
	{"hook_command",	(PyCFunction)Module_hexchat_hook_command,
		METH_VARARGS|METH_KEYWORDS},
	{"hook_server",		(PyCFunction)Module_hexchat_hook_server,
		METH_VARARGS|METH_KEYWORDS},
	{"hook_server_attrs",		(PyCFunction)Module_hexchat_hook_server_attrs,
		METH_VARARGS|METH_KEYWORDS},
	{"hook_print",		(PyCFunction)Module_hexchat_hook_print,
		METH_VARARGS|METH_KEYWORDS},
	{"hook_print_attrs",		(PyCFunction)Module_hexchat_hook_print_attrs,
		METH_VARARGS|METH_KEYWORDS},
	{"hook_timer",		(PyCFunction)Module_hexchat_hook_timer,
		METH_VARARGS|METH_KEYWORDS},
	{"hook_unload",		(PyCFunction)Module_hexchat_hook_unload,
		METH_VARARGS|METH_KEYWORDS},
	{"unhook",		Module_hexchat_unhook,
		METH_VARARGS},
	{"get_list",		Module_xchat_get_list,
		METH_VARARGS},
	{"get_lists",		Module_xchat_get_lists,
		METH_NOARGS},
	{"nickcmp",		Module_hexchat_nickcmp,
		METH_VARARGS},
	{"strip",		Module_hexchat_strip,
		METH_VARARGS},
	{NULL, NULL}
};

#ifdef IS_PY3K
static struct PyModuleDef moduledef = {
	PyModuleDef_HEAD_INIT,
	"hexchat",     /* m_name */
	"HexChat Scripting Interface",  /* m_doc */
	-1,                  /* m_size */
	Module_xchat_methods,    /* m_methods */
	NULL,                /* m_reload */
	NULL,                /* m_traverse */
	NULL,                /* m_clear */
	NULL,                /* m_free */
};

static struct PyModuleDef xchat_moduledef = {
	PyModuleDef_HEAD_INIT,
	"xchat",     /* m_name */
	"HexChat Scripting Interface",  /* m_doc */
	-1,                  /* m_size */
	Module_xchat_methods,    /* m_methods */
	NULL,                /* m_reload */
	NULL,                /* m_traverse */
	NULL,                /* m_clear */
	NULL,                /* m_free */
};
#endif

static PyObject *
moduleinit_hexchat(void)
{
	PyObject *hm;
#ifdef IS_PY3K
		hm = PyModule_Create(&moduledef);
#else
    hm = Py_InitModule3("hexchat", Module_xchat_methods, "HexChat Scripting Interface");
#endif

	PyModule_AddIntConstant(hm, "EAT_NONE", HEXCHAT_EAT_NONE);
	PyModule_AddIntConstant(hm, "EAT_HEXCHAT", HEXCHAT_EAT_HEXCHAT);
	PyModule_AddIntConstant(hm, "EAT_XCHAT", HEXCHAT_EAT_HEXCHAT); /* for compat */
	PyModule_AddIntConstant(hm, "EAT_PLUGIN", HEXCHAT_EAT_PLUGIN);
	PyModule_AddIntConstant(hm, "EAT_ALL", HEXCHAT_EAT_ALL);
	PyModule_AddIntConstant(hm, "PRI_HIGHEST", HEXCHAT_PRI_HIGHEST);
	PyModule_AddIntConstant(hm, "PRI_HIGH", HEXCHAT_PRI_HIGH);
	PyModule_AddIntConstant(hm, "PRI_NORM", HEXCHAT_PRI_NORM);
	PyModule_AddIntConstant(hm, "PRI_LOW", HEXCHAT_PRI_LOW);
	PyModule_AddIntConstant(hm, "PRI_LOWEST", HEXCHAT_PRI_LOWEST);

	PyObject_SetAttrString(hm, "__version__", Py_BuildValue("(ii)", VERSION_MAJOR, VERSION_MINOR));

	return hm;
}

static PyObject *
moduleinit_xchat(void)
{
	PyObject *xm;
#ifdef IS_PY3K
		xm = PyModule_Create(&xchat_moduledef);
#else
    xm = Py_InitModule3("xchat", Module_xchat_methods, "HexChat Scripting Interface");
#endif

	PyModule_AddIntConstant(xm, "EAT_NONE", HEXCHAT_EAT_NONE);
	PyModule_AddIntConstant(xm, "EAT_XCHAT", HEXCHAT_EAT_HEXCHAT);
	PyModule_AddIntConstant(xm, "EAT_PLUGIN", HEXCHAT_EAT_PLUGIN);
	PyModule_AddIntConstant(xm, "EAT_ALL", HEXCHAT_EAT_ALL);
	PyModule_AddIntConstant(xm, "PRI_HIGHEST", HEXCHAT_PRI_HIGHEST);
	PyModule_AddIntConstant(xm, "PRI_HIGH", HEXCHAT_PRI_HIGH);
	PyModule_AddIntConstant(xm, "PRI_NORM", HEXCHAT_PRI_NORM);
	PyModule_AddIntConstant(xm, "PRI_LOW", HEXCHAT_PRI_LOW);
	PyModule_AddIntConstant(xm, "PRI_LOWEST", HEXCHAT_PRI_LOWEST);

	PyObject_SetAttrString(xm, "__version__", Py_BuildValue("(ii)", VERSION_MAJOR, VERSION_MINOR));

	return xm;
}

#ifdef IS_PY3K
PyMODINIT_FUNC
PyInit_hexchat(void)
{
    return moduleinit_hexchat();
}
PyMODINIT_FUNC
PyInit_xchat(void)
{
    return moduleinit_xchat();
}
#else
PyMODINIT_FUNC
inithexchat(void)
{
		moduleinit_hexchat();
}
PyMODINIT_FUNC
initxchat(void)
{
		moduleinit_xchat();
}
#endif

/* ===================================================================== */
/* Python interactive interpreter functions */

static void
IInterp_Exec(char *command)
{
	PyObject *m, *d, *o;
	char *buffer;
	int len;

	BEGIN_PLUGIN(interp_plugin);

	m = PyImport_AddModule("__main__");
	if (m == NULL) {
		hexchat_print(ph, "Can't get __main__ module");
		goto fail;
	}
	d = PyModule_GetDict(m);
	len = strlen(command);

	buffer = g_malloc(len + 2);
	memcpy(buffer, command, len);
	buffer[len] = '\n';
	buffer[len+1] = 0;
	PyRun_SimpleString("import hexchat");
	o = PyRun_StringFlags(buffer, Py_single_input, d, d, NULL);
	g_free(buffer);
	if (o == NULL) {
		PyErr_Print();
		goto fail;
	}
	Py_DECREF(o);

fail:
	END_PLUGIN(interp_plugin);
	return;
}

static int
IInterp_Cmd(char *word[], char *word_eol[], void *userdata)
{
	char *channel = (char *) hexchat_get_info(ph, "channel");
	if (channel && channel[0] == '>' && strcmp(channel, ">>python<<") == 0) {
		hexchat_printf(ph, ">>> %s\n", word_eol[1]);
		IInterp_Exec(word_eol[1]);
		return 1;
	}
	return 0;
}


/* ===================================================================== */
/* Python command handling */

static void
Command_PyList(void)
{
	GSList *list;
	list = plugin_list;
	if (list == NULL) {
		hexchat_print(ph, "No python modules loaded");
	} else {
		hexchat_print(ph,
		   "Name         Version  Filename             Description\n"
		   "----         -------  --------             -----------\n");
		while (list != NULL) {
			PluginObject *plg = (PluginObject *) list->data;
			char *basename = g_path_get_basename(plg->filename);
			hexchat_printf(ph, "%-12s %-8s %-20s %-10s\n",
				     plg->name,
				     *plg->version ? plg->version
				     		  : "<none>",
				     basename,
				     *plg->description ? plg->description
				     		      : "<none>");
			g_free(basename);
			list = list->next;
		}
		hexchat_print(ph, "\n");
	}
}

static void
Command_PyLoad(char *filename)
{
	PyObject *plugin;
	RELEASE_XCHAT_LOCK();
	plugin = Plugin_New(filename, xchatout);
	ACQUIRE_XCHAT_LOCK();
	if (plugin)
		plugin_list = g_slist_append(plugin_list, plugin);
}

static void
Command_PyUnload(char *name)
{
	PluginObject *plugin = Plugin_ByString(name);
	if (!plugin) {
		hexchat_print(ph, "Can't find a python plugin with that name");
	} else {
		BEGIN_PLUGIN(plugin);
		Plugin_Delete((PyObject*)plugin);
		END_PLUGIN(plugin);
		plugin_list = g_slist_remove(plugin_list, plugin);
	}
}

static void
Command_PyReload(char *name)
{
	PluginObject *plugin = Plugin_ByString(name);
	if (!plugin) {
		hexchat_print(ph, "Can't find a python plugin with that name");
	} else {
		char *filename = g_strdup(plugin->filename);
		Command_PyUnload(filename);
		Command_PyLoad(filename);
		g_free(filename);
	}
}

static void
Command_PyAbout(void)
{
	hexchat_print(ph, about);
}

static int
Command_Py(char *word[], char *word_eol[], void *userdata)
{
	char *cmd = word[2];
	int ok = 0;
	if (strcasecmp(cmd, "LIST") == 0) {
		ok = 1;
		Command_PyList();
	} else if (strcasecmp(cmd, "EXEC") == 0) {
		if (word[3][0]) {
			ok = 1;
			IInterp_Exec(word_eol[3]);
		}
	} else if (strcasecmp(cmd, "LOAD") == 0) {
		if (word[3][0]) {
			ok = 1;
			Command_PyLoad(word[3]);
		}
	} else if (strcasecmp(cmd, "UNLOAD") == 0) {
		if (word[3][0]) {
			ok = 1;
			Command_PyUnload(word[3]);
		}
	} else if (strcasecmp(cmd, "RELOAD") == 0) {
		if (word[3][0]) {
			ok = 1;
			Command_PyReload(word[3]);
		}
	} else if (strcasecmp(cmd, "CONSOLE") == 0) {
		ok = 1;
		hexchat_command(ph, "QUERY >>python<<");
	} else if (strcasecmp(cmd, "ABOUT") == 0) {
		ok = 1;
		Command_PyAbout();
	}
	if (!ok)
		hexchat_print(ph, usage);
	return HEXCHAT_EAT_ALL;
}

static int
Command_Load(char *word[], char *word_eol[], void *userdata)
{
	int len = strlen(word[2]);
	if (len > 3 && strcasecmp(".py", word[2]+len-3) == 0) {
		Command_PyLoad(word[2]);
		return HEXCHAT_EAT_HEXCHAT;
	}
	return HEXCHAT_EAT_NONE;
}

static int
Command_Reload(char *word[], char *word_eol[], void *userdata)
{
	int len = strlen(word[2]);
	if (len > 3 && strcasecmp(".py", word[2]+len-3) == 0) {
	Command_PyReload(word[2]);
	return HEXCHAT_EAT_HEXCHAT;
	}
	return HEXCHAT_EAT_NONE;
}

static int
Command_Unload(char *word[], char *word_eol[], void *userdata)
{
	int len = strlen(word[2]);
	if (len > 3 && strcasecmp(".py", word[2]+len-3) == 0) {
		Command_PyUnload(word[2]);
		return HEXCHAT_EAT_HEXCHAT;
	}
	return HEXCHAT_EAT_NONE;
}

/* ===================================================================== */
/* Autoload function */

/* ===================================================================== */
/* (De)initialization functions */

static int initialized = 0;
static int reinit_tried = 0;

void
hexchat_plugin_get_info(char **name, char **desc, char **version, void **reserved)
{
	*name = "Python";
	*version = VERSION;
	*desc = "Python scripting interface";
   if (reserved)
      *reserved = NULL;
}

int
hexchat_plugin_init(hexchat_plugin *plugin_handle,
		  char **plugin_name,
		  char **plugin_desc,
		  char **plugin_version,
		  char *arg)
{
#ifdef IS_PY3K
	wchar_t *argv[] = { L"<hexchat>", 0 };
#else
	char *argv[] = { "<hexchat>", 0 };
#endif

	ph = plugin_handle;

	/* Block double initalization. */
	if (initialized != 0) {
		hexchat_print(ph, "Python interface already loaded");
		/* deinit is called even when init fails, so keep track
		 * of a reinit failure. */
		reinit_tried++;
		return 0;
	}
	initialized = 1;

	*plugin_name = "Python";
	*plugin_version = VERSION;

	/* FIXME You can't free this since it's used as long as the plugin's
	 * loaded, but if you unload it, everything belonging to the plugin is
	 * supposed to be freed anyway.
	 */
	*plugin_desc = g_strdup_printf ("Python %d scripting interface", PY_MAJOR_VERSION);

	/* Initialize python. */
#ifdef IS_PY3K
	Py_SetProgramName(L"hexchat");
	PyImport_AppendInittab("hexchat", PyInit_hexchat);
	PyImport_AppendInittab("xchat", PyInit_xchat);
#else
	Py_SetProgramName("hexchat");
	PyImport_AppendInittab("hexchat", inithexchat);
	PyImport_AppendInittab("xchat", initxchat);
#endif
	Py_Initialize();
	PySys_SetArgv(1, argv);

	xchatout_buffer = g_string_new (NULL);
	xchatout = XChatOut_New();
	if (xchatout == NULL) {
		hexchat_print(ph, "Can't allocate xchatout object");
		return 0;
	}

#ifdef WITH_THREAD
	PyEval_InitThreads();
	xchat_lock = PyThread_allocate_lock();
	if (xchat_lock == NULL) {
		hexchat_print(ph, "Can't allocate hexchat lock");
		Py_DECREF(xchatout);
		xchatout = NULL;
		return 0;
	}
#endif

	main_tstate = PyEval_SaveThread();

	interp_plugin = Plugin_New(NULL, xchatout);
	if (interp_plugin == NULL) {
		hexchat_print(ph, "Plugin_New() failed.\n");
#ifdef WITH_THREAD
		PyThread_free_lock(xchat_lock);
#endif
		Py_DECREF(xchatout);
		xchatout = NULL;
		return 0;
	}


	hexchat_hook_command(ph, "", HEXCHAT_PRI_NORM, IInterp_Cmd, 0, 0);
	hexchat_hook_command(ph, "PY", HEXCHAT_PRI_NORM, Command_Py, usage, 0);
	hexchat_hook_command(ph, "LOAD", HEXCHAT_PRI_NORM, Command_Load, 0, 0);
	hexchat_hook_command(ph, "UNLOAD", HEXCHAT_PRI_NORM, Command_Unload, 0, 0);
	hexchat_hook_command(ph, "RELOAD", HEXCHAT_PRI_NORM, Command_Reload, 0, 0);
#ifdef WITH_THREAD
	thread_timer = hexchat_hook_timer(ph, 300, Callback_ThreadTimer, NULL);
#endif

	hexchat_print(ph, "Python interface loaded\n");

	Util_Autoload();
	return 1;
}

int
hexchat_plugin_deinit(void)
{
	GSList *list;

	/* A reinitialization was tried. Just give up and live the
	 * environment as is. We are still alive. */
	if (reinit_tried) {
		reinit_tried--;
		return 1;
	}

	list = plugin_list;
	while (list != NULL) {
		PyObject *plugin = (PyObject *) list->data;
		BEGIN_PLUGIN(plugin);
		Plugin_Delete(plugin);
		END_PLUGIN(plugin);
		list = list->next;
	}
	g_slist_free(plugin_list);
	plugin_list = NULL;

	/* Reset xchatout buffer. */
	g_string_free (xchatout_buffer, TRUE);
	xchatout_buffer = NULL;

	if (interp_plugin) {
		Py_DECREF(interp_plugin);
		interp_plugin = NULL;
	}

	/* Switch back to the main thread state. */
	if (main_tstate) {
		PyEval_RestoreThread(main_tstate);
		PyThreadState_Swap(main_tstate);
		main_tstate = NULL;
	}
	Py_Finalize();

#ifdef WITH_THREAD
	if (thread_timer != NULL) {
		hexchat_unhook(ph, thread_timer);
		thread_timer = NULL;
	}
	PyThread_free_lock(xchat_lock);
#endif

	hexchat_print(ph, "Python interface unloaded\n");
	initialized = 0;

	return 1;
}

