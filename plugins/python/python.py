from __future__ import print_function

import importlib
import os
import pydoc
import signal
import sys
import traceback
import weakref
from contextlib import contextmanager

from _hexchat_embedded import ffi, lib

if sys.version_info < (3, 0):
    from io import BytesIO as HelpEater
else:
    from io import StringIO as HelpEater

if not hasattr(sys, 'argv'):
    sys.argv = ['<hexchat>']

VERSION = b'2.0'  # Sync with hexchat.__version__
PLUGIN_NAME = ffi.new('char[]', b'Python')
PLUGIN_DESC = ffi.new('char[]', b'Python %d.%d scripting interface' % (sys.version_info[0], sys.version_info[1]))
PLUGIN_VERSION = ffi.new('char[]', VERSION)

# TODO: Constants should be screaming snake case
hexchat = None
local_interp = None
hexchat_stdout = None
plugins = set()


@contextmanager
def redirected_stdout():
    sys.stdout = sys.__stdout__
    sys.stderr = sys.__stderr__
    yield
    sys.stdout = hexchat_stdout
    sys.stderr = hexchat_stdout


if os.getenv('HEXCHAT_LOG_PYTHON'):
    def log(*args):
        with redirected_stdout():
            print(*args)

else:
    def log(*args):
        pass


class Stdout:
    def __init__(self):
        self.buffer = bytearray()

    def write(self, string):
        string = string.encode()
        idx = string.rfind(b'\n')
        if idx != -1:
            self.buffer += string[:idx]
            lib.hexchat_print(lib.ph, bytes(self.buffer))
            self.buffer = bytearray(string[idx + 1:])
        else:
            self.buffer += string

    def isatty(self):
        return False


class Attribute:
    def __init__(self):
        self.time = 0

    def __repr__(self):
        return '<Attribute object at {}>'.format(id(self))


class Hook:
    def __init__(self, plugin, callback, userdata, is_unload):
        self.is_unload = is_unload
        self.plugin = weakref.proxy(plugin)
        self.callback = callback
        self.userdata = userdata
        self.hexchat_hook = None
        self.handle = ffi.new_handle(weakref.proxy(self))

    def __del__(self):
        log('Removing hook', id(self))
        if self.is_unload is False:
            assert self.hexchat_hook is not None
            lib.hexchat_unhook(lib.ph, self.hexchat_hook)


if sys.version_info[0] == 2:
    def compile_file(data, filename):
        return compile(data, filename, 'exec', dont_inherit=True)


    def compile_line(string):
        try:
            return compile(string, '<string>', 'eval', dont_inherit=True)

        except SyntaxError:
            # For some reason `print` is invalid for eval
            # This will hide any return value though
            return compile(string, '<string>', 'exec', dont_inherit=True)
else:
    def compile_file(data, filename):
        return compile(data, filename, 'exec', optimize=2, dont_inherit=True)


    def compile_line(string):
        # newline appended to solve unexpected EOF issues
        return compile(string + '\n', '<string>', 'single', optimize=2, dont_inherit=True)


class Plugin:
    def __init__(self):
        self.ph = None
        self.name = ''
        self.filename = ''
        self.version = ''
        self.description = ''
        self.hooks = set()
        self.globals = {
            '__plugin': weakref.proxy(self),
            '__name__': '__main__',
        }

    def add_hook(self, callback, userdata, is_unload=False):
        hook = Hook(self, callback, userdata, is_unload=is_unload)
        self.hooks.add(hook)
        return hook

    def remove_hook(self, hook):
        for h in self.hooks:
            if id(h) == hook:
                ud = h.userdata
                self.hooks.remove(h)
                return ud

        log('Hook not found')
        return None

    def loadfile(self, filename):
        try:
            self.filename = filename
            with open(filename, 'rb') as f:
                data = f.read().decode('utf-8')
            compiled = compile_file(data, filename)
            exec(compiled, self.globals)

            try:
                self.name = self.globals['__module_name__']

            except KeyError:
                lib.hexchat_print(lib.ph, b'Failed to load module: __module_name__ must be set')

                return False

            self.version = self.globals.get('__module_version__', '')
            self.description = self.globals.get('__module_description__', '')
            self.ph = lib.hexchat_plugingui_add(lib.ph, filename.encode(), self.name.encode(),
                                                self.description.encode(), self.version.encode(), ffi.NULL)

        except Exception as e:
            lib.hexchat_print(lib.ph, 'Failed to load module: {}'.format(e).encode())
            traceback.print_exc()
            return False

        return True

    def __del__(self):
        log('unloading', self.filename)
        for hook in self.hooks:
            if hook.is_unload is True:
                try:
                    hook.callback(hook.userdata)

                except Exception as e:
                    log('Failed to run hook:', e)
                    traceback.print_exc()

        del self.hooks
        if self.ph is not None:
            lib.hexchat_plugingui_remove(lib.ph, self.ph)


if sys.version_info[0] == 2:
    def __decode(string):
        return string

else:
    def __decode(string):
        return string.decode()


# There can be empty entries between non-empty ones so find the actual last value
def wordlist_len(words):
    for i in range(31, 0, -1):
        if ffi.string(words[i]):
            return i

    return 0


def create_wordlist(words):
    size = wordlist_len(words)
    return [__decode(ffi.string(words[i])) for i in range(1, size + 1)]


# This function only exists for compat reasons with the C plugin
# It turns the word list from print hooks into a word_eol list
# This makes no sense to do...
def create_wordeollist(words):
    words = reversed(words)
    accum = None
    ret = []
    for word in words:
        if accum is None:
            accum = word

        elif word:
            last = accum
            accum = ' '.join((word, last))

        ret.insert(0, accum)

    return ret


def to_cb_ret(value):
    if value is None:
        return 0

    return int(value)


@ffi.def_extern()
def _on_command_hook(word, word_eol, userdata):
    hook = ffi.from_handle(userdata)
    word = create_wordlist(word)
    word_eol = create_wordlist(word_eol)
    return to_cb_ret(hook.callback(word, word_eol, hook.userdata))


@ffi.def_extern()
def _on_print_hook(word, userdata):
    hook = ffi.from_handle(userdata)
    word = create_wordlist(word)
    word_eol = create_wordeollist(word)
    return to_cb_ret(hook.callback(word, word_eol, hook.userdata))


@ffi.def_extern()
def _on_print_attrs_hook(word, attrs, userdata):
    hook = ffi.from_handle(userdata)
    word = create_wordlist(word)
    word_eol = create_wordeollist(word)
    attr = Attribute()
    attr.time = attrs.server_time_utc
    return to_cb_ret(hook.callback(word, word_eol, hook.userdata, attr))


@ffi.def_extern()
def _on_server_hook(word, word_eol, userdata):
    hook = ffi.from_handle(userdata)
    word = create_wordlist(word)
    word_eol = create_wordlist(word_eol)
    return to_cb_ret(hook.callback(word, word_eol, hook.userdata))


@ffi.def_extern()
def _on_server_attrs_hook(word, word_eol, attrs, userdata):
    hook = ffi.from_handle(userdata)
    word = create_wordlist(word)
    word_eol = create_wordlist(word_eol)
    attr = Attribute()
    attr.time = attrs.server_time_utc
    return to_cb_ret(hook.callback(word, word_eol, hook.userdata, attr))


@ffi.def_extern()
def _on_timer_hook(userdata):
    hook = ffi.from_handle(userdata)
    if hook.callback(hook.userdata) == True:
        return 1

    hook.is_unload = True  # Don't unhook
    for h in hook.plugin.hooks:
        if h == hook:
            hook.plugin.hooks.remove(h)
            break

    return 0


@ffi.def_extern(error=3)
def _on_say_command(word, word_eol, userdata):
    channel = ffi.string(lib.hexchat_get_info(lib.ph, b'channel'))
    if channel == b'>>python<<':
        python = ffi.string(word_eol[1])
        lib.hexchat_print(lib.ph, b'>>> ' + python)
        exec_in_interp(__decode(python))
        return 1

    return 0


def load_filename(filename):
    filename = os.path.expanduser(filename)
    if not os.path.isabs(filename):
        configdir = __decode(ffi.string(lib.hexchat_get_info(lib.ph, b'configdir')))

        filename = os.path.join(configdir, 'addons', filename)

    if filename and not any(plugin.filename == filename for plugin in plugins):
        plugin = Plugin()
        if plugin.loadfile(filename):
            plugins.add(plugin)
            return True

    return False


def unload_name(name):
    if name:
        for plugin in plugins:
            if name in (plugin.name, plugin.filename, os.path.basename(plugin.filename)):
                plugins.remove(plugin)
                return True

    return False


def reload_name(name):
    if name:
        for plugin in plugins:
            if name in (plugin.name, plugin.filename, os.path.basename(plugin.filename)):
                filename = plugin.filename
                plugins.remove(plugin)
                return load_filename(filename)

    return False


@contextmanager
def change_cwd(path):
    old_cwd = os.getcwd()
    os.chdir(path)
    yield
    os.chdir(old_cwd)


def autoload():
    configdir = __decode(ffi.string(lib.hexchat_get_info(lib.ph, b'configdir')))
    addondir = os.path.join(configdir, 'addons')
    try:
        with change_cwd(addondir):  # Maintaining old behavior
            for f in os.listdir(addondir):
                if f.endswith('.py'):
                    log('Autoloading', f)
                    # TODO: Set cwd
                    load_filename(os.path.join(addondir, f))

    except FileNotFoundError as e:
        log('Autoload failed', e)


def list_plugins():
    if not plugins:
        lib.hexchat_print(lib.ph, b'No python modules loaded')
        return

    tbl_headers = [b'Name', b'Version', b'Filename', b'Description']
    tbl = [
        tbl_headers,
        [(b'-' * len(s)) for s in tbl_headers]
    ]

    for plugin in plugins:
        basename = os.path.basename(plugin.filename).encode()
        name = plugin.name.encode()
        version = plugin.version.encode() if plugin.version else b'<none>'
        description = plugin.description.encode() if plugin.description else b'<none>'
        tbl.append((name, version, basename, description))

    column_sizes = [
        max(len(item) for item in column)
        for column in zip(*tbl)
    ]

    for row in tbl:
        lib.hexchat_print(lib.ph, b' '.join(item.ljust(column_sizes[i])
                                            for i, item in enumerate(row)))
    lib.hexchat_print(lib.ph, b'')


def exec_in_interp(python):
    global local_interp

    if not python:
        return

    if local_interp is None:
        local_interp = Plugin()
        local_interp.locals = {}
        local_interp.globals['hexchat'] = hexchat

    code = compile_line(python)
    try:
        ret = eval(code, local_interp.globals, local_interp.locals)
        if ret is not None:
            lib.hexchat_print(lib.ph, '{}'.format(ret).encode())

    except Exception as e:
        traceback.print_exc(file=hexchat_stdout)


@ffi.def_extern()
def _on_load_command(word, word_eol, userdata):
    filename = ffi.string(word[2])
    if filename.endswith(b'.py'):
        load_filename(__decode(filename))
        return 3

    return 0


@ffi.def_extern()
def _on_unload_command(word, word_eol, userdata):
    filename = ffi.string(word[2])
    if filename.endswith(b'.py'):
        unload_name(__decode(filename))
        return 3

    return 0


@ffi.def_extern()
def _on_reload_command(word, word_eol, userdata):
    filename = ffi.string(word[2])
    if filename.endswith(b'.py'):
        reload_name(__decode(filename))
        return 3

    return 0


@ffi.def_extern(error=3)
def _on_py_command(word, word_eol, userdata):
    subcmd = __decode(ffi.string(word[2])).lower()

    if subcmd == 'exec':
        python = __decode(ffi.string(word_eol[3]))
        exec_in_interp(python)

    elif subcmd == 'load':
        filename = __decode(ffi.string(word[3]))
        load_filename(filename)

    elif subcmd == 'unload':
        name = __decode(ffi.string(word[3]))
        if not unload_name(name):
            lib.hexchat_print(lib.ph, b'Can\'t find a python plugin with that name')

    elif subcmd == 'reload':
        name = __decode(ffi.string(word[3]))
        if not reload_name(name):
            lib.hexchat_print(lib.ph, b'Can\'t find a python plugin with that name')

    elif subcmd == 'console':
        lib.hexchat_command(lib.ph, b'QUERY >>python<<')

    elif subcmd == 'list':
        list_plugins()

    elif subcmd == 'about':
        lib.hexchat_print(lib.ph, b'HexChat Python interface version ' + VERSION)

    else:
        lib.hexchat_command(lib.ph, b'HELP PY')

    return 3


@ffi.def_extern()
def _on_plugin_init(plugin_name, plugin_desc, plugin_version, arg, libdir):
    global hexchat
    global hexchat_stdout

    signal.signal(signal.SIGINT, signal.SIG_DFL)

    plugin_name[0] = PLUGIN_NAME
    plugin_desc[0] = PLUGIN_DESC
    plugin_version[0] = PLUGIN_VERSION

    try:
        libdir = __decode(ffi.string(libdir))
        modpath = os.path.join(libdir, '..', 'python')
        sys.path.append(os.path.abspath(modpath))
        hexchat = importlib.import_module('hexchat')

    except (UnicodeDecodeError, ImportError) as e:
        lib.hexchat_print(lib.ph, b'Failed to import module: ' + repr(e).encode())

        return 0

    hexchat_stdout = Stdout()
    sys.stdout = hexchat_stdout
    sys.stderr = hexchat_stdout
    pydoc.help = pydoc.Helper(HelpEater(), HelpEater())

    lib.hexchat_hook_command(lib.ph, b'', 0, lib._on_say_command, ffi.NULL, ffi.NULL)
    lib.hexchat_hook_command(lib.ph, b'LOAD', 0, lib._on_load_command, ffi.NULL, ffi.NULL)
    lib.hexchat_hook_command(lib.ph, b'UNLOAD', 0, lib._on_unload_command, ffi.NULL, ffi.NULL)
    lib.hexchat_hook_command(lib.ph, b'RELOAD', 0, lib._on_reload_command, ffi.NULL, ffi.NULL)
    lib.hexchat_hook_command(lib.ph, b'PY', 0, lib._on_py_command, b'''Usage: /PY LOAD   <filename>
           UNLOAD <filename|name>
           RELOAD <filename|name>
           LIST
           EXEC <command>
           CONSOLE
           ABOUT''', ffi.NULL)

    lib.hexchat_print(lib.ph, b'Python interface loaded')
    autoload()
    return 1


@ffi.def_extern()
def _on_plugin_deinit():
    global local_interp
    global hexchat
    global hexchat_stdout
    global plugins

    plugins = set()
    local_interp = None
    hexchat = None
    hexchat_stdout = None
    sys.stdout = sys.__stdout__
    sys.stderr = sys.__stderr__
    pydoc.help = pydoc.Helper()

    for mod in ('_hexchat', 'hexchat', 'xchat', '_hexchat_embedded'):
        try:
            del sys.modules[mod]

        except KeyError:
            pass

    return 1
