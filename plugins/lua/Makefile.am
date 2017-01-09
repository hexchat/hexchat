libdir = $(hexchatlibdir)

lib_LTLIBRARIES = lua.la
lua_la_SOURCES = lua.c
lua_la_LDFLAGS = $(PLUGIN_LDFLAGS) -module
lua_la_LIBADD = $(LUA_LIBS) $(GLIB_LIBS)
lua_la_CPPFLAGS = -I$(top_srcdir)/src/common
lua_la_CFLAGS = $(GLIB_CFLAGS) $(LUA_CFLAGS)

