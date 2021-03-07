import inspect
import sys
from contextlib import contextmanager

from _hexchat_embedded import ffi, lib

__all__ = [
    'EAT_ALL', 'EAT_HEXCHAT', 'EAT_NONE', 'EAT_PLUGIN', 'EAT_XCHAT',
    'PRI_HIGH', 'PRI_HIGHEST', 'PRI_LOW', 'PRI_LOWEST', 'PRI_NORM',
    '__doc__', '__version__', 'command', 'del_pluginpref', 'emit_print',
    'find_context', 'get_context', 'get_info',
    'get_list', 'get_lists', 'get_pluginpref', 'get_prefs', 'hook_command',
    'hook_print', 'hook_print_attrs', 'hook_server', 'hook_server_attrs',
    'hook_timer', 'hook_unload', 'list_pluginpref', 'nickcmp', 'prnt',
    'set_pluginpref', 'strip', 'unhook',
]

__doc__ = 'HexChat Scripting Interface'
__version__ = (2, 0)
__license__ = 'GPL-2.0+'

EAT_NONE = 0
EAT_HEXCHAT = 1
EAT_XCHAT = EAT_HEXCHAT
EAT_PLUGIN = 2
EAT_ALL = EAT_HEXCHAT | EAT_PLUGIN

PRI_LOWEST = -128
PRI_LOW = -64
PRI_NORM = 0
PRI_HIGH = 64
PRI_HIGHEST = 127


# We need each module to be able to reference their parent plugin
# which is a bit tricky since they all share the exact same module.
# Simply navigating up to what module called it seems to actually
# be a fairly reliable and simple method of doing so if ugly.
def __get_current_plugin():
    frame = inspect.stack()[1][0]
    while '__plugin' not in frame.f_globals:
        frame = frame.f_back
        assert frame is not None

    return frame.f_globals['__plugin']


# Keeping API compat
if sys.version_info[0] == 2:
    def __decode(string):
        return string

else:
    def __decode(string):
        return string.decode()


# ------------ API ------------
def prnt(string):
    lib.hexchat_print(lib.ph, string.encode())


def emit_print(event_name, *args, **kwargs):
    time = kwargs.pop('time', 0)  # For py2 compat
    cargs = []
    for i in range(4):
        arg = args[i].encode() if len(args) > i else b''
        cstring = ffi.new('char[]', arg)
        cargs.append(cstring)

    if time == 0:
        return lib.hexchat_emit_print(lib.ph, event_name.encode(), *cargs)

    attrs = lib.hexchat_event_attrs_create(lib.ph)
    attrs.server_time_utc = time
    ret = lib.hexchat_emit_print_attrs(lib.ph, attrs, event_name.encode(), *cargs)
    lib.hexchat_event_attrs_free(lib.ph, attrs)
    return ret


# TODO: this shadows itself. command should be changed to cmd
def command(command):
    lib.hexchat_command(lib.ph, command.encode())


def nickcmp(string1, string2):
    return lib.hexchat_nickcmp(lib.ph, string1.encode(), string2.encode())


def strip(text, length=-1, flags=3):
    stripped = lib.hexchat_strip(lib.ph, text.encode(), length, flags)
    ret = __decode(ffi.string(stripped))
    lib.hexchat_free(lib.ph, stripped)
    return ret


def get_info(name):
    ret = lib.hexchat_get_info(lib.ph, name.encode())
    if ret == ffi.NULL:
        return None
    if name in ('gtkwin_ptr', 'win_ptr'):
        # Surely there is a less dumb way?
        ptr = repr(ret).rsplit(' ', 1)[1][:-1]
        return ptr

    return __decode(ffi.string(ret))


def get_prefs(name):
    string_out = ffi.new('char**')
    int_out = ffi.new('int*')
    _type = lib.hexchat_get_prefs(lib.ph, name.encode(), string_out, int_out)
    if _type == 0:
        return None

    if _type == 1:
        return __decode(ffi.string(string_out[0]))

    if _type in (2, 3):  # XXX: 3 should be a bool, but keeps API
        return int_out[0]

    raise AssertionError('Out of bounds pref storage')


def __cstrarray_to_list(arr):
    i = 0
    ret = []
    while arr[i] != ffi.NULL:
        ret.append(ffi.string(arr[i]))
        i += 1

    return ret


__FIELD_CACHE = {}


def __get_fields(name):
    return __FIELD_CACHE.setdefault(name, __cstrarray_to_list(lib.hexchat_list_fields(lib.ph, name)))


__FIELD_PROPERTY_CACHE = {}


def __cached_decoded_str(string):
    return __FIELD_PROPERTY_CACHE.setdefault(string, __decode(string))


def get_lists():
    return [__cached_decoded_str(field) for field in __get_fields(b'lists')]


class ListItem:
    def __init__(self, name):
        self._listname = name

    def __repr__(self):
        return '<{} list item at {}>'.format(self._listname, id(self))


# done this way for speed
if sys.version_info[0] == 2:
    def get_getter(name):
        return ord(name[0])

else:
    def get_getter(name):
        return name[0]


def get_list(name):
    # XXX: This function is extremely inefficient and could be interators and
    # lazily loaded properties, but for API compat we stay slow
    orig_name = name
    name = name.encode()

    if name not in __get_fields(b'lists'):
        raise KeyError('list not available')

    list_ = lib.hexchat_list_get(lib.ph, name)
    if list_ == ffi.NULL:
        return None

    ret = []
    fields = __get_fields(name)

    def string_getter(field):
        string = lib.hexchat_list_str(lib.ph, list_, field)
        if string != ffi.NULL:
            return __decode(ffi.string(string))

        return ''

    def ptr_getter(field):
        if field == b'context':
            ptr = lib.hexchat_list_str(lib.ph, list_, field)
            ctx = ffi.cast('hexchat_context*', ptr)
            return Context(ctx)

        return None

    getters = {
        ord('s'): string_getter,
        ord('i'): lambda field: lib.hexchat_list_int(lib.ph, list_, field),
        ord('t'): lambda field: lib.hexchat_list_time(lib.ph, list_, field),
        ord('p'): ptr_getter,
    }

    while lib.hexchat_list_next(lib.ph, list_) == 1:
        item = ListItem(orig_name)
        for _field in fields:
            getter = getters.get(get_getter(_field))
            if getter is not None:
                field_name = _field[1:]
                setattr(item, __cached_decoded_str(field_name), getter(field_name))

        ret.append(item)

    lib.hexchat_list_free(lib.ph, list_)
    return ret


# TODO: 'command' here shadows command above, and should be renamed to cmd
def hook_command(command, callback, userdata=None, priority=PRI_NORM, help=None):
    plugin = __get_current_plugin()
    hook = plugin.add_hook(callback, userdata)
    handle = lib.hexchat_hook_command(lib.ph, command.encode(), priority, lib._on_command_hook,
                                      help.encode() if help is not None else ffi.NULL, hook.handle)

    hook.hexchat_hook = handle
    return id(hook)


def hook_print(name, callback, userdata=None, priority=PRI_NORM):
    plugin = __get_current_plugin()
    hook = plugin.add_hook(callback, userdata)
    handle = lib.hexchat_hook_print(lib.ph, name.encode(), priority, lib._on_print_hook, hook.handle)
    hook.hexchat_hook = handle
    return id(hook)


def hook_print_attrs(name, callback, userdata=None, priority=PRI_NORM):
    plugin = __get_current_plugin()
    hook = plugin.add_hook(callback, userdata)
    handle = lib.hexchat_hook_print_attrs(lib.ph, name.encode(), priority, lib._on_print_attrs_hook, hook.handle)
    hook.hexchat_hook = handle
    return id(hook)


def hook_server(name, callback, userdata=None, priority=PRI_NORM):
    plugin = __get_current_plugin()
    hook = plugin.add_hook(callback, userdata)
    handle = lib.hexchat_hook_server(lib.ph, name.encode(), priority, lib._on_server_hook, hook.handle)
    hook.hexchat_hook = handle
    return id(hook)


def hook_server_attrs(name, callback, userdata=None, priority=PRI_NORM):
    plugin = __get_current_plugin()
    hook = plugin.add_hook(callback, userdata)
    handle = lib.hexchat_hook_server_attrs(lib.ph, name.encode(), priority, lib._on_server_attrs_hook, hook.handle)
    hook.hexchat_hook = handle
    return id(hook)


def hook_timer(timeout, callback, userdata=None):
    plugin = __get_current_plugin()
    hook = plugin.add_hook(callback, userdata)
    handle = lib.hexchat_hook_timer(lib.ph, timeout, lib._on_timer_hook, hook.handle)
    hook.hexchat_hook = handle
    return id(hook)


def hook_unload(callback, userdata=None):
    plugin = __get_current_plugin()
    hook = plugin.add_hook(callback, userdata, is_unload=True)
    return id(hook)


def unhook(handle):
    plugin = __get_current_plugin()
    return plugin.remove_hook(handle)


def set_pluginpref(name, value):
    if isinstance(value, str):
        return bool(lib.hexchat_pluginpref_set_str(lib.ph, name.encode(), value.encode()))

    if isinstance(value, int):
        return bool(lib.hexchat_pluginpref_set_int(lib.ph, name.encode(), value))

    # XXX: This should probably raise but this keeps API
    return False


def get_pluginpref(name):
    name = name.encode()
    string_out = ffi.new('char[512]')
    if lib.hexchat_pluginpref_get_str(lib.ph, name, string_out) != 1:
        return None

    string = ffi.string(string_out)
    # This API stores everything as a string so we have to figure out what
    # its actual type was supposed to be.
    if len(string) > 12:  # Can't be a number
        return __decode(string)

    number = lib.hexchat_pluginpref_get_int(lib.ph, name)
    if number == -1 and string != b'-1':
        return __decode(string)

    return number


def del_pluginpref(name):
    return bool(lib.hexchat_pluginpref_delete(lib.ph, name.encode()))


def list_pluginpref():
    prefs_str = ffi.new('char[4096]')
    if lib.hexchat_pluginpref_list(lib.ph, prefs_str) == 1:
        return __decode(ffi.string(prefs_str)).split(',')

    return []


class Context:
    def __init__(self, ctx):
        self._ctx = ctx

    def __eq__(self, value):
        if not isinstance(value, Context):
            return False

        return self._ctx == value._ctx

    @contextmanager
    def __change_context(self):
        old_ctx = lib.hexchat_get_context(lib.ph)
        if not self.set():
            # XXX: Behavior change, previously used wrong context
            lib.hexchat_print(lib.ph, b'Context object refers to closed context, ignoring call')
            return

        yield
        lib.hexchat_set_context(lib.ph, old_ctx)

    def set(self):
        # XXX: API addition, C plugin silently ignored failure
        return bool(lib.hexchat_set_context(lib.ph, self._ctx))

    def prnt(self, string):
        with self.__change_context():
            prnt(string)

    def emit_print(self, event_name, *args, **kwargs):
        time = kwargs.pop('time', 0)  # For py2 compat
        with self.__change_context():
            return emit_print(event_name, *args, time=time)

    def command(self, string):
        with self.__change_context():
            command(string)

    def get_info(self, name):
        with self.__change_context():
            return get_info(name)

    def get_list(self, name):
        with self.__change_context():
            return get_list(name)


def get_context():
    ctx = lib.hexchat_get_context(lib.ph)
    return Context(ctx)


def find_context(server=None, channel=None):
    server = server.encode() if server is not None else ffi.NULL
    channel = channel.encode() if channel is not None else ffi.NULL
    ctx = lib.hexchat_find_context(lib.ph, server, channel)
    if ctx == ffi.NULL:
        return None

    return Context(ctx)
