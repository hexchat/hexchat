# HexChat Plugin Interface

## Introduction
Plugins for HexChat are written in C. The interface aims to keep 100%
binary compatability. This means that if you upgrade HexChat, you will
not need to recompile your plugins, they'll continue to work. The
interface doesn't depend on any structures and offsets, so compiler
versions shouldn't have an impact either. The only real requirement of
a HexChat plugin is that it define an _hexchat\_plugin\_init_ symbol. This
is your entry point function, see the example below. You should make
all your global variables and functions _static_, so that a symbol
is not exported. There is no harm in exporting these symbols, but they
are not necessary and only pollute the name-space. Plugins are compiled as shared objects
(.so files), for example:

Most UNIX systems:
> gcc -Wl,--export-dynamic -Wall -O1 -shared -fPIC myplugin.c -o myplugin.so

OS X:
> gcc -no-cpp-precomp -g -O2 -Wall -bundle -flat_namespace -undefined suppress -o myplugin.so myplugin.c

See the Windows section on how to compile a plugin using Visual Studio.

All strings passed to and from plugins are encoded in UTF-8, regardless
of locale.

## Sample plugin

This simple plugin auto-ops anyone who joins a channel you're in. It also
adds a new command _/AUTOOPTOGGLE_, which can be used to turn the feature ON
or OFF. Every HexChat plugin must define an _hexchat\_plugin\_init_ function, this
is the normal entry point. _hexchat\_plugin\_deinit_ is optional.

<pre>
#include "hexchat-plugin.h"

#define PNAME "AutoOp"
#define PDESC "Auto Ops anyone that joins"
#define PVERSION "0.1"

static hexchat_plugin *ph;		/* plugin handle */
static int enable = 1;

static int
join_cb (char *word[], void *userdata)
{
	if (enable)
	{
		/* Op ANYONE who joins */
		hexchat_commandf (ph, "OP %s", word[1]);
	}
	/* word[1] is the nickname, as in the Settings->Text Events window in HexChat */

	return HEXCHAT_EAT_NONE;		/* don't eat this event, HexChat needs to see it! */
}

static int
autooptoggle_cb (char *word[], char *word_eol[], void *userdata)
{
	if (!enable)
	{
		enable = 1;
		hexchat_print (ph, "AutoOping now enabled!\n");
	}
	else
	{
		enable = 0;
		hexchat_print (ph, "AutoOping now disabled!\n");
	}

	return HEXCHAT_EAT_ALL;		/* eat this command so HexChat and other plugins can't process it */
}

void
hexchat_plugin_get_info (char **name, char **desc, char **version, void **reserved)
{
	*name = PNAME;
	*desc = PDESC;
	*version = PVERSION;
}

int
hexchat_plugin_init (hexchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	/* we need to save this for use with any hexchat_* functions */
	ph = plugin_handle;

	/* tell HexChat our info */
	*plugin_name = PNAME;
	*plugin_desc = PDESC;
	*plugin_version = PVERSION;

	hexchat_hook_command (ph, "AutoOpToggle", HEXCHAT_PRI_NORM, autooptoggle_cb, "Usage: AUTOOPTOGGLE, Turns OFF/ON Auto Oping", 0);
	hexchat_hook_print (ph, "Join", HEXCHAT_PRI_NORM, join_cb, 0);

	hexchat_print (ph, "AutoOpPlugin loaded successfully!\n");

	return 1;		/* return 1 for success */
}
</pre>


## What's _word_ and _word\_eol_?

They are arrays of strings. They contain the parameters the user entered
for the particular command. For example, if you executed:

<pre>
/command NICK hi there

word[1] is command
word[2] is NICK
word[3] is hi
word[4] is there

word_eol[1] is command NICK hi there
word_eol[2] is NICK hi there
word_eol[3] is hi there
word_eol[4] is there
</pre>

These arrays are simply provided for your convenience. You are **not** allowed
to alter them. Both arrays are limited to 32 elements (index 31). _word[0]_ and
_word\_eol[0]_ are reserved and should not be read.

## Lists and Fields
Lists of information (DCCs, Channels, User list, etc.) can be retreived
with _hexchat\_list\_get_. All fields are **read only** and must be copied if
needed for a long time after calling _hexchat\_list\_str_. The types of lists and fields available are:

<blockquote>

"channels" - list of channels, querys and their servers.
<blockquote><table border=1>
<tr bgcolor="#dddddd"><td>Name</td><td>Description</td><td>Type</td></tr>
<tr><td>channel</td><td>Channel or query name</td><td>string</td></tr>
<tr><td>chantypes</td><td>Channel types e.g. "#!&amp;"<br><small>(Added in version 2.0.9. Older versions will return NULL)</small></td><td>string</td>
<tr><td>context</td><td>(hexchat_context *) pointer. Can be used with hexchat_set_context</td><td>string</td></tr>
<tr><td>flags</td><td>Server/Channel Bits:<br>
<table>
<tr><td>Bit #</td><td>Value</td><td>Description</td></tr>
<tr><td>0</td><td>1</td><td>Connected</td></tr>
<tr><td>1</td><td>2</td><td>Connecting in Progress</td></tr>
<tr><td>2</td><td>4</td><td>You are away</td></tr>
<tr><td>3</td><td>8</td><td>End of MOTD (Login complete)</td></tr>
<tr><td>4</td><td>16</td><td>Has WHOX (ircu)</td></tr>
<tr><td>5</td><td>32</td><td>Has IDMSG (FreeNode)</td></tr>
<tr><td>6</td><td>64</td><td>Hide Join/Part Messages</td></tr>
<tr><td>7</td><td>128</td><td>unused (was Color Paste in old versions)</td></tr>
<tr><td>8</td><td>256</td><td>Beep on Message</td></tr>
<tr><td>9</td><td>512</td><td>Blink Tray</td></tr>
<tr><td>10</td><td>1024</td><td>Blink Task Bar</td></tr>
</table>
<br><small>(Bits 0-5 added in 2.0.9. Bits 6-8 added in 2.6.6. Bit 9 added in 2.8.0. Bit 10 in 2.8.6)</small></td><td>int</td></tr>
<tr><td>id</td><td>Unique server ID<br><small>(Added in version 2.0.8. Older versions will return -1)</small></td><td>int</td></tr>
<tr><td>lag</td><td>Lag in milliseconds<br><small>(Added in version 2.6.8. Older versions will return -1)</small></td><td>int</td>
<tr><td>maxmodes</td><td>Maximum modes per line<br><small>(Added in version 2.0.9. Older versions will return -1)</small></td><td>int</td>
<tr><td>network</td><td>Network name to which this channel belongs<br><small>(Added in version 2.0.2. Older versions will return NULL)</small></td><td>string</td></tr>
<tr><td>nickprefixes</td><td>Nickname prefixes e.g. "@+"<br><small>(Added in version 2.0.9. Older versions will return NULL)</small></td><td>string</td>
<tr><td>nickmodes</td><td>Nickname mode chars e.g. "ov"<br><small>(Added in version 2.0.9. Older versions will return NULL)</small></td><td>string</td>
<tr><td>queue</td><td>Number of bytes in the send-queue<br><small>(Added in version 2.6.8. Older versions will return -1)</small></td><td>int</td>
<tr><td>server</td><td>Server name to which this channel belongs</td><td>string</td></tr>
<tr><td>type</td><td>Type of context this is: 1-Server 2-Channel 3-Dialog<br><small>(Added in version 2.0.2. Older versions will return -1)</small></td><td>int</td></tr>
<tr><td>users</td><td>Number of users in this channel<br><small>(Added in version 2.0.8. Older versions will return -1)</small></td><td>int</td></tr>
</table>
</blockquote>

"dcc" - list of DCC file transfers. Fields:
<blockquote> <table border=1>
<tr bgcolor="#dddddd"><td>Name</td><td>Description</td><td>Type</td></tr>
<tr><td>address32</td><td>Address of the remote user (ipv4 address)</td><td>int</td></tr>
<tr><td>cps</td><td>Bytes per second (speed)</td><td>int</td></tr>
<tr><td>destfile</td><td>Destination full pathname</td><td>string</td></tr>
<tr><td>file</td><td>File name</td><td>string</td></tr>
<tr><td>nick</td><td>Nickname of person who the file is from/to</td><td>string</td></tr>
<tr><td>port</td><td>TCP port number</td><td>int</td></tr>
<tr><td>pos</td><td>Bytes sent/received</td><td>int</td></tr>
<tr><td>poshigh</td><td>Bytes sent/received, high order 32 bits</td><td>int</td></tr>
<tr><td>resume</td><td>Point at which this file was resumed (or zero if it was not resumed)</td><td>int</td></tr>
<tr><td>resumehigh</td><td>Point at which this file was resumed, high order 32 bits</td><td>int</td></tr>
<tr><td>size</td><td>File size in bytes, low order 32 bits (cast it to unsigned)</td><td>int</td></tr>
<tr><td>sizehigh</td><td>File size in bytes, high order 32 bits</td><td>int</td></tr>
<tr><td>status</td><td>DCC Status: 0-Queued 1-Active 2-Failed 3-Done 4-Connecting 5-Aborted</td><td>int</td></tr>
<tr><td>type</td><td>DCC Type: 0-Send 1-Receive 2-ChatRecv 3-ChatSend</td><td>int</td></tr>
</table>
</blockquote>

"ignore" - current ignore list.
<blockquote> <table border=1>
<tr bgcolor="#dddddd"><td>Name</td><td>Description</td><td>Type</td></tr>
<tr><td>mask</td><td>Ignore mask. .e.g: *!*@*.aol.com</td><td>string</td></tr>
<tr><td>flags</td><td>Bit field of flags. 0=Private 1=Notice 2=Channel 3=Ctcp<br>
4=Invite 5=UnIgnore 6=NoSave 7=DCC</td><td>int</td></tr>
</table>
</blockquote>

"notify" - list of people on notify.
<blockquote> <table border=1>
<tr bgcolor="#dddddd"><td>Name</td><td>Description</td><td>Type</td></tr>
<tr><td>networks</td><td>Networks to which this nick applies. Comma separated. May be NULL.
<br><small>(Added in version 2.6.8)</small></td><td>string</td></tr>
<tr><td>nick</td><td>Nickname</td><td>string</td></tr>
<tr><td>flags</td><td>Bit field of flags. 0=Is online.</td><td>int</td></tr>
<tr><td>on</td><td>Time when user came online.</td><td>time_t</td></tr>
<tr><td>off</td><td>Time when user went offline.</td><td>time_t</td></tr>
<tr><td>seen</td><td>Time when user the user was last verified still online.</td><td>time_t</td></tr>
</table>
<small>Fields are only valid for the context when hexchat_list_get() was called
(i.e. you get information about the user ON THAT ONE SERVER ONLY). You
may cycle through the "channels" list to find notify information for every
server.</small>
</blockquote>

"users" - list of users in the current channel.
<blockquote> <table border=1>
<tr bgcolor="#dddddd"><td>Name</td><td>Description</td><td>Type</td></tr>
<tr><td>away</td><td>Away status (boolean)<br><small>(Added in version 2.0.6. Older versions will return -1)</small></td><td>int</td></tr>
<tr><td>lasttalk</td><td>Last time the user was seen talking<br><small>(Added in version 2.4.2. Older versions will return -1)</small></td><td>time_t</td></tr>
<tr><td>nick</td><td>Nick name</td><td>string</td></tr>
<tr><td>host</td><td>Host name in the form: user@host (or NULL if not known).</td><td>string</td></tr>
<tr><td>prefix</td><td>Prefix character, .e.g: @ or +. Points to a single char.</td><td>string</td></tr>
<tr><td>realname</td><td>Real name or NULL<br><small>(Added in version 2.8.6)</small></td><td>string</td></tr>
<tr><td>selected</td><td>Selected status in the user list, only works for retrieving the user list of the focused tab<br><small>(Added in version 2.6.1. Older versions will return -1)</small></td><td>int</td></tr>
</table>
</blockquote>

</blockquote>

Example:

<pre>
	list = hexchat_list_get (ph, "dcc");

	if (list)
	{
		hexchat_print (ph, "--- DCC LIST ------------------\nFile  To/From   KB/s   Position\n");

		while (hexchat_list_next (ph, list))
		{
			hexchat_printf (ph, "%6s %10s %.2f  %d\n",
				hexchat_list_str (ph, list, "file"),
				hexchat_list_str (ph, list, "nick"),
				hexchat_list_int (ph, list, "cps") / 1024,
				hexchat_list_int (ph, list, "pos"));
		}

		hexchat_list_free (ph, list);
	}
</pre>

## Plugins on Windows (Win32)

All you need is Visual Studio setup as explained in [Building](http://www.hexchat.org/developers/building). Your best bet is to use an existing plugin (such as the currently unused SASL plugin) in the HexChat solution as a starting point. You should have the following files:


 * [hexchat-plugin.h](https://github.com/hexchat/hexchat/blob/master/src/common/hexchat-plugin.h) - main plugin header
 * plugin.c - Your plugin, you need to write this one :)
 * plugin.def - A simple text file containing the following:

<pre>
	EXPORTS
	hexchat_plugin_init
	hexchat_plugin_deinit
	hexchat_plugin_get_info
</pre>

Leave out _hexchat\_plugin\_deinit_ if you don't intend to define that
function. Then compile your plugin in Visual Studio as usual.

**Caveat:** plugins compiled on Win32 **must** have a
global variable called _ph_, which is the _plugin\_handle_, much like
in the sample plugin above.

## Controlling the GUI

A simple way to perform basic GUI functions is to use the _/GUI_ command.
You can execute this command through the input box, or by calling _hexchat\_command (ph, "GUI .....");_.

 * **GUI ATTACH:** Same function as "Attach Window" in the HexChat menu.
 * **GUI DETACH:** Same function as "Detach Tab" in the HexChat menu.
 * **GUI APPLY:** Similar to clicking OK in the settings window. Execute this after /SET to activate GUI changes.
 * **GUI COLOR _n_:** Change the tab color of the current context, where n is a number from 0 to 3.
 * **GUI FOCUS:** Focus the current window or tab.
 * **GUI FLASH:** Flash the taskbar button. It will flash only if the window isn't focused and will stop when it is focused by the user.
 * **GUI HIDE:** Hide the main HexChat window completely.
 * **GUI ICONIFY:** Iconify (minimize to taskbar) the current HexChat window.
 * **GUI MSGBOX _text_:** Displays a asynchronous message box with your text.
 * **GUI SHOW:** Show the main HexChat window (if currently hidden).

You can add your own items to the menu bar. The menu command has this syntax:
<pre>
	MENU [-eX] [-i&lt;ICONFILE>] [-k&lt;mod>,&lt;key>] [-m] [-pX] [-rX,group] [-tX] {ADD|DEL} &lt;path> [command] [unselect command]
</pre>

For example:

<pre>
	MENU -p5 ADD FServe
	MENU ADD "FServe/Show File List" "fs list"
	MENU ADD FServe/-
	MENU -k4,101 -t1 ADD "FServe/Enabled" "fs on" "fs off"
	MENU -e0 ADD "FServe/Do Something" "fs action"
</pre>

In the example above, it would be recommended to execute _MENU DEL FServe_ inside your _hexchat\_plugin\_deinit_ function. The special item with name "-" will add a separator line.

Parameters and flags:

 * **-eX:** Set enable flag to X. -e0 for disable, -e1 for enable. This lets you create a disabled (shaded) item.
 * **-iFILE:** Use an icon filename FILE. Not supported for toggles or radio items.
 * **-k&lt;mod>,&lt;key>:** Specify a keyboard shortcut. "mod" is the modifier which is a bitwise OR of: 1-SHIFT 4- CTRL 8-ALT in decimal. "key" is the key value in decimal, e.g. -k5,101 would specify SHIFT-CTRL-E.
 * **-m:** Specify that this label should be treated as <a href="http://developer.gnome.org/doc/API/2.0/pango/PangoMarkupFormat.html">Pango Markup</a> language. Since forward slash ("/") is already used in menu paths, you should replace closing tags with an ASCII 003 instead e.g.: hexchat_command (ph, "MENU -m ADD \"&lt;b>Bold Menu&lt;\003b>\"");
 * **-pX:** Specify a menu item's position number. e.g. -p5 will cause the item to be inserted in the 5th place. If the position is a negative number, it will be used as an offset from the bottom/right-most item.
 * **-rX,group:** Specify a radio menu item, with initial state X and a group name. The group name should be the exact label of another menu item (without the path) that this item will be grouped with. For radio items, only a select command will be executed (no unselect command).
 * **-tX:** Specify a toggle menu item with an initial state. -t0 for an "unticked" item and -t1 for a "ticked" item.

If you want to change an item's toggle state or enabled flag,
just _ADD_ an item with exactly the same name and command and specify the _-tX -eX_ parameters you need.

It's also possible to add items to HexChat's existing menus, for example:

<pre>
	MENU ADD "Settings/Sub Menu"
	MENU -t0 ADD "Settings/Sub Menu/My Setting" myseton mysetoff
</pre>

However, internal names and layouts of HexChat's menu may change in the future, so use at own risk.

Here is an example of Radio items:

<pre>
	MENU ADD "Language"
	MENU -r1,"English" ADD "Language/English" cmd1
	MENU -r0,"English" ADD "Language/Spanish" cmd2
	MENU -r0,"English" ADD "Language/German" cmd3
</pre>

You can also change menus other than the main one (i.e popup menus). Currently they are:

<blockquote>
<table border=1 cellpadding=4 rules=all>
<tr bgcolor="#999999"><td>Root Name</td><td>Menu</td></tr>
<tr><td>$TAB</td><td>Tab menu (right click a channel/query tab or treeview row)</td></tr>
<tr><td>$TRAY</td><td>System Tray menu</td></tr>
<tr><td>$URL</td><td>URL link menu</td></tr>
<tr><td>$NICK</td><td>Userlist nick-name popup menu</td></tr>
<tr><td>$CHAN</td><td>Menu when clicking a channel in the text area (since 2.8.4)</td></tr>
</table>
</blockquote>

Example:

<pre>
	MENU -p0 ADD "$TAB/Cycle Channel" cycle
</pre>

You can manipulate HexChat's system tray icon using the _/TRAY_ command:

<pre>
	Usage:
	TRAY -f &lt;timeout> &lt;file1> [&lt;file2>] Flash tray between two icons. Leave off file2 to use default HexChat icon.
	TRAY -f &lt;filename>                  Set tray to a fixed icon.
	TRAY -i &lt;number>                    Flash tray with an internal icon.
	TRAY -t &lt;text>                      Set the tray tooltip.
	TRAY -b &lt;title> &lt;text>              Set the tray balloon.
</pre>

Icon numbers:

 * 2: Message
 * 5: Highlight
 * 8: Private
 * 11:File

For tray balloons on Linux, you'll need libnotify.

Filenames can be _ICO_ or _PNG_ format. _PNG_ format is supported on Linux/BSD and Windows XP. Set a timeout of -1 to use HexChat's default.

## Handling UTF-8/Unicode strings

The HexChat plugin API specifies that strings passed to and from HexChat must be encoded in UTF-8.

What does this mean for the plugin programmer? You just have to be a little careful when
passing strings obtained from IRC to system calls. For example, if you're writing a file-server
bot, someone might message you a filename. Can you pass this filename directly to open()? Maybe!
If you're lazy... The correct thing to do is to convert the string to "system locale encoding",
otherwise your plugin will fail on non-ascii characters.

Here are examples on how to do this conversion on Unix and Windows. In this example, someone will
CTCP you the message "SHOWFILE &lt;filename&gt;".

<pre>
static int
ctcp_cb (char *word[], char *word_eol[], void *userdata)
{
	if(strcmp(word[1], "SHOWFILE") == 0)
	{
		get_file_name (nick, word[2]);
	}

	return HEXCHAT_EAT_HEXCHAT;
}

static void
get_file_name (char *nick, char *fname)
{
	char buf[256];
	FILE *fp;

	/* the fname is in UTF-8, because it came from the HexChat API */

#ifdef _WIN32

	wchar_t wide_name[MAX_PATH];

	/* convert UTF-8 to WIDECHARs (aka UTF-16LE) */
	if (MultiByteToWideChar (CP_UTF8, 0, fname, -1, wide_name, MAX_PATH) &lt; 1)
	{
		return;
	}

	/* now we have WIDECHARs, so we can _wopen() or CreateFileW(). */
	/* _wfopen actually requires NT4, Win2000, XP or newer. */
	fp = _wfopen (wide_name, "r");

#else

	char *loc_name;

	/* convert UTF-8 to System Encoding */
	loc_name = g_filename_from_utf8 (fname, -1, 0, 0, 0);
	if(!loc_name)
	{
		return;
	}

	/* now open using the system's encoding */
	fp = fopen (loc_name, "r");
	g_free (loc_name);

#endif

	if (fp)
	{
		while (fgets (buf, sizeof (buf), fp))
		{
			/* send every line to the user that requested it */
			hexchat_commandf (ph, "QUOTE NOTICE %s :%s", nick, buf);
		}
		fclose (fp);
	}
}
</pre>

## Functions

***

### hexchat\_hook\_command ()

**Prototype:** hexchat\_hook \*hexchat\_hook\_command (hexchat\_plugin \*ph, const char \*name, int pri, hexchat\_cmd\_cb \*callb, const char \*help\_text, void \*userdata);

**Description:** Adds a new _/command_. This allows your program to
handle commands entered at the input box. To capture text without a "/" at
the start (non-commands), you may hook a special name of "". i.e _hexchat\_hook\_command (ph, "", ...);_.

Commands hooked that begin with a period ('.') will be hidden in _/HELP_ and _/HELP -l_.

**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).
 * **name:** Name of the command (without the forward slash).
 * **pri:** Priority of this command. Use _HEXCHAT\_PRI\_NORM_.
 * **callb:** Callback function. This will be called when the user executes the given command name.
 * **help\_text:** String of text to display when the user executes _/HELP_ for this command. May be NULL if you're lazy.
 * **userdata:** Pointer passed to the callback function.

**Returns:** Pointer to the hook. Can be passed to _hexchat\_unhook ()_.

**Example:**

<pre>
static int
onotice_cb (char *word[], char *word_eol[], void *userdata)
{
	if (word_eol[2][0] == 0)
	{
		hexchat_printf (ph, "Second arg must be the message!\n");
		return HEXCHAT_EAT_ALL;
	}

	hexchat_commandf (ph, "NOTICE @%s :%s", hexchat_get_info (ph, "channel"), word_eol[2]);
	return HEXCHAT_EAT_ALL;
}

hexchat_hook_command (ph, "ONOTICE", HEXCHAT_PRI_NORM, onotice_cb, "Usage: ONOTICE &lt;message> Sends a notice to all ops", NULL);
</pre>

***

### hexchat\_hook\_fd ()

**Prototype:** hexchat\_hook \*hexchat\_hook\_fd (hexchat\_plugin \*ph, int fd, int flags, hexchat\_fd\_cb \*callb, void \*userdata);

**Description:** Hooks a socket or file descriptor. WIN32: Passing a pipe from MSVCR71, MSVCR80 or other variations is not supported at this time.
**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).
 * **fd:** The file descriptor or socket.
 * **flags:** One or more of _HEXCHAT\_FD\_READ_, _HEXCHAT\_FD\_WRITE_, _HEXCHAT\_FD\_EXCEPTION_, _HEXCHAT\_FD\_NOTSOCKET_. Use bitwise OR to combine them. _HEXCHAT\_FD\_NOTSOCKET_ tells HexChat that the provided _fd__ is not a socket, but an "MSVCRT.DLL" pipe.
 * **callb:** Callback function. This will be called when the socket is available for reading/writing or exception (depending on your chosen _flags_)
 * **userdata:** Pointer passed to the callback function.

**Returns:** Pointer to the hook. Can be passed to _hexchat\_unhook ()_.

***

### hexchat\_hook\_print ()

**Prototype:** hexchat\_hook \*hexchat\_hook\_print (hexchat\_plugin \*ph, const char \*name, int pri, hexchat\_print\_cb \*callb, void \*userdata);

**Description:** Registers a function to trap any print events.
The event names may be any available in the "Advanced > Text Events" window.
There are also some extra "special" events you may hook using this function.
Currently they are:

 * "Open Context": Called when a new hexchat\_context is created.
 * "Close Context": Called when a hexchat\_context pointer is closed.
 * "Focus Tab": Called when a tab is brought to front.
 * "Focus Window": Called a toplevel window is focused, or the main tab-window is focused by the window manager.
 * "DCC Chat Text": Called when some text from a DCC Chat arrives. It provides these elements in the _word[]_ array:
<pre>
	word[1] Address
	word[2] Port
	word[3] Nick
	word[4] The Message
</pre>
 * "Key Press": Called when some keys are pressed in the input box. It provides these elements in the _word[]_ array:
<pre>
	word[1] Key Value
	word[2] State Bitfield (shift, capslock, alt)
	word[3] String version of the key
	word[4] Length of the string (may be 0 for unprintable keys)
</pre>

**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).
 * **name:** Name of the print event (as in _Edit Event Texts_ window).
 * **pri:** Priority of this command. Use HEXCHAT\_PRI\_NORM.
 * **callb:** Callback function. This will be called when this event name is printed.
 * **userdata:** Pointer passed to the callback function.

**Returns:** Pointer to the hook. Can be passed to _hexchat\_unhook ()_.

**Example:**

<pre>
static int
youpart_cb (char *word[], void *userdata)
{
	hexchat_printf (ph, "You have left channel %s\n", word[3]);
	return HEXCHAT_EAT_HEXCHAT;		/* dont let HexChat do its normal printing */
}

hexchat_hook_print (ph, "You Part", HEXCHAT_PRI_NORM, youpart_cb, NULL);
</pre>

***

### hexchat\_hook\_server ()

**Prototype:** hexchat\_hook \*hexchat\_hook\_server (hexchat\_plugin \*ph, const char \*name, int pri, hexchat\_serv\_cb \*callb, void \*userdata);

**Description:** Registers a function to be called when a certain server event occurs. You can
use this to trap _PRIVMSG_, _NOTICE_, _PART_, a server numeric, etc. If you want to
hook every line that comes from the IRC server, you may use the special name of _RAW LINE_.

**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).
 * **name:** Name of the server event.
 * **pri:** Priority of this command. Use HEXCHAT\_PRI\_NORM.
 * **callb:** Callback function. This will be called when this event is received from the server.
 * **userdata:** Pointer passed to the callback function.

**Returns:** Pointer to the hook. Can be passed to _hexchat\_unhook_.

**Example:**
<pre>
static int
kick_cb (char *word[], char *word_eol[], void *userdata)
{
	hexchat_printf (ph, "%s was kicked from %s (reason=%s)\n", word[4], word[3], word_eol[5]);
	return HEXCHAT_EAT_NONE;		/* don't eat this event, let other plugins and HexChat see it too */
}

hexchat_hook_server (ph, "KICK", HEXCHAT_PRI_NORM, kick_cb, NULL);
</pre>

***

### hexchat\_hook\_timer ()

**Prototype:** hexchat\_hook \*hexchat\_hook\_timer (hexchat\_plugin \*ph, int timeout, hexchat\_timer\_cb \*callb, void \*userdata);

**Description:** Registers a function to be called every "timeout" milliseconds.

**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).
 * **timeout:** Timeout in milliseconds (1000 is 1 second).
 * **callb:** Callback function. This will be called every "timeout" milliseconds.
 * **userdata:** Pointer passed to the callback function.

**Returns:** Pointer to the hook. Can be passed to hexchat_unhook.

**Example:**

<pre>
static hexchat_hook *myhook;

static int
stop_cb (char *word[], char *word_eol[], void *userdata)
{
	if (myhook != NULL)
	{
		hexchat_unhook (ph, myhook);
		myhook = NULL;
		hexchat_print (ph, "Timeout removed!\n");
	}

	return HEXCHAT_EAT_ALL;
}

static int
timeout_cb (void *userdata)
{
	hexchat_print (ph, "Annoying message every 5 seconds! Type /STOP to stop it.\n");
	return 1;		/* return 1 to keep the timeout going */
}

myhook = hexchat_hook_timer (ph, 5000, timeout_cb, NULL);
hexchat_hook_command (ph, "STOP", HEXCHAT_PRI_NORM, stop_cb, NULL, NULL);
</pre>

***

### hexchat\_unhook ()

**Prototype:** void \*hexchat\_unhook (hexchat\_plugin \*ph, hexchat\_hook \*hook);

**Description:** Unhooks any hook registered with hexchat\_hook\_print/server/timer/command. When plugins are unloaded, all of its hooks are automatically removed, so you don't need to call this within your hexchat\_plugin\_deinit () function.

**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).
 * **hook:** Pointer to the hook, as returned by hexchat\_hook\_*.

**Returns:** The userdata you originally gave to hexchat\_hook\_*.

***

### hexchat\_command ()

**Prototype:** void hexchat\_command (hexchat\_plugin \*ph, const char \*command);

**Description:** Executes a command as if it were typed in HexChat's input box.

**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).
 * **command:** Command to execute, without the forward slash "/".

***

### hexchat\_commandf ()

**Prototype:** void hexchat\_commandf (hexchat\_plugin \*ph, const char \*format, ...);

**Description:** Executes a command as if it were typed in HexChat's input box and provides string formatting like _printf ()_.

**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).
 * **format:** The format string.

***

### hexchat\_print ()

**Prototype:** void hexchat\_print (hexchat\_plugin \*ph, const char \*text);

**Description:** Prints some text to the current tab/window.

**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).
 * **text:** Text to print. May contain mIRC color codes.

***

### hexchat\_printf ()

**Prototype:** void hexchat\_printf (hexchat\_plugin \*ph, const char \*format, ...);

**Description:** Prints some text to the current tab/window and provides formatting like _printf ()_.

**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).
 * **format:** The format string.

***

### hexchat\_emit\_print ()
**Prototype:** int hexchat\_emit\_print (hexchat\_plugin \*ph, const char \*event\_name, ...);

**Description:** Generates a print event. This can be any event found in the Preferences > Advanced > Text Events window. The vararg parameter list **must** always be NULL terminated. Special care should be taken when calling this function inside a print callback (from hexchat\_hook\_print), as not to cause endless recursion.

**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).
 * **event_name:** Text event to print.

**Returns:**

 * 1: Success.
 * 0: Failure.

**Example:**

<pre>
hexchat_emit_print (ph, "Channel Message", "John", "Hi there", "@", NULL);
</pre>

***

### hexchat\_send\_modes ()

**Prototype:** void hexchat\_send\_modes (hexchat\_plugin \*ph, const char \*targets[], int ntargets, int modes_per_line, char sign, char mode)

**Description:** Sends a number of channel mode changes to the current channel. For example, you can Op a whole
group of people in one go. It may send multiple MODE lines if the request doesn't fit on one. Pass 0 for
_modes\_per\_line_ to use the current server's maximum possible. This function should only be called while
in a channel context.

**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).
 * **targets:** Array of targets (strings). The names of people whom the action will be performed on.
 * **ntargets:** Number of elements in the array given.
 * **modes_per_line:** Maximum modes to send per line.
 * **sign:** Mode sign, '-' or '+'.
 * **mode:** Mode char, e.g. 'o' for Ops.

**Example:** (Ops the three names given)

<pre>
const char *names_to_Op[] = {"John", "Jack", "Jill"};
hexchat_send_modes (ph, names_to_Op, 3, 0, '+', 'o');
</pre>

***

### hexchat\_find\_context ()

**Prototype:** hexchat\_context \*hexchat\_find\_context (hexchat\_plugin \*ph, const char \*servname, const char \*channel);

**Description:** Finds a context based on a channel and servername. If _servname_ is NULL, it finds any channel (or query) by the given name. If _channel_ is NULL, it finds the front-most tab/window of the given _servname_. If NULL is given for both arguments, the currently focused tab/window will be returned.

Changed in 2.6.1. If _servname_ is NULL, it finds the channel (or query) by the given name in the same server group as the current context. If that doesn't exists then find any by the given name.

**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).
 * **servname:** Server name or NULL.
 * **channel:** Channel name or NULL.

**Returns:** Context pointer (for use with _hexchat\_set\_context_) or NULL.

***

### hexchat\_get\_context ()

**Prototype:** hexchat\_context \*hexchat\_get\_context (hexchat\_plugin \*ph);

**Description:** Returns the current context for your plugin. You can use this later with _hexchat\_set\_context ()_.

**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).

**Returns:** Context pointer (for use with _hexchat\_set\_context_).

***

### hexchat\_get\_info ()

**Prototype:** const char \*hexchat\_get\_info (hexchat\_plugin \*ph, const char \*id);

**Description:** Returns information based on your current context.

**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).
 * **id:** ID of the information you want. Currently supported IDs are (case sensitive):
	* **away:** away reason or NULL if you are not away.
	* **channel:** current channel name.
	* **charset:** character-set used in the current context.
	* **configdir:** HexChat config directory, e.g.: `/home/user/.config/hexchat`. This string is encoded in UTF-8.
	* **event\_text &lt;name>:** text event format string for _name_.
	* **gtkwin\_ptr:** (GtkWindow \*).
	* **host:** real hostname of the server you connected to.
	* **inputbox:** the input-box contents, what the user has typed.
	* **libdirfs:** library directory. e.g. /usr/lib/hexchat. The same directory used for auto-loading plugins. This string isn't necessarily UTF-8, but local file system encoding.
	* **modes:** channel modes, if known, or NULL.
	* **network:** current network name or NULL.
	* **nick:** your current nick name.
	* **nickserv:** nickserv password for this network or NULL.
	* **server:** current server name (what the server claims to be). NULL if you are not connected.
	* **topic:** current channel topic.
	* **version:** HexChat version number.
	* **win\_ptr:** native window pointer. Unix: (GtkWindow *) Win32: HWND.
	* **win\_status:** window status: "active", "hidden" or "normal".

**Returns:** A string of the requested information, or NULL. This string must
not be freed and must be copied if needed after the call to _hexchat\_get\_info ()_.

***

### hexchat\_get\_prefs ()

**Prototype:** int hexchat\_get\_prefs (hexchat\_plugin \*ph, const char \*name, const char \*\*string, int \*integer);

**Description:** Provides HexChat's setting information (that which is available through the _/SET_ command).
A few extra bits of information are available that don't appear in the _/SET_ list, currently they are:

 * **state_cursor:** Current input box cursor position (characters, not bytes).
 * **id:** Unique server id

**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).
 * **name:** Setting name required.
 * **string:** Pointer-pointer which to set.
 * **integer:** Pointer to an integer to set, if setting is a boolean or integer type.

**Returns:**

 * 0: Failed.
 * 1: Returned a string.
 * 2: Returned an integer.
 * 3: Returned a boolean.

**Example:**

<pre>
{
	int i;
	const char *str;

	if (hexchat_get_prefs (ph, "irc_nick1", &amp;str, &amp;i) == 1)
	{
		hexchat_printf (ph, "Current nickname setting: %s\n", str);
	}
}
</pre>

***

### hexchat\_set\_context ()

**Prototype:** int hexchat\_set\_context (hexchat\_plugin \*ph, hexchat\_context \*ctx);

**Description:** Changes your current context to the one given.

**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).
 * **ctx:** Context to change to (obtained with _hexchat\_get\_context ()_ or _hexchat\_find\_context ()_).

**Returns:**

 * 1: Success.
 * 0: Failure.

***

### hexchat\_nickcmp ()

**Prototype:** int hexchat\_nickcmp (hexchat\_plugin \*ph, const char \*s1, const char \*s2);

**Description:** Performs a nick name comparision, based on the current server connection. This might be an RFC1459 compliant string compare, or plain ascii (in the case of DALNet). Use this to compare channels and nicknames. The function works the same way as _strcasecmp ()_.

**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).
 * **s1:** String to compare.
 * **s2:** String to compare _s1_ to.

**Quote from RFC1459:**
>Because of IRC's scandanavian origin, the characters {}| are
considered to be the lower case equivalents of the characters []\,
respectively. This is a critical issue when determining the
equivalence of two nicknames.

**Returns:** An integer less than, equal to, or greater than zero if _s1_ is found, respectively, to be less than, to match, or be greater than _s2_.

***

### hexchat\_strip ()

**Prototype:** char \*hexchat\_strip (hexchat\_plugin \*ph, const char \*str, int len, int flags);

**Description:** Strips mIRC color codes and/or text attributes (bold, underlined etc) from the given string and returns a newly allocated string.

**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).
 * **str:** String to strip.
 * **len:** Length of the string (or -1 for NULL terminated).
 * **flags:** Bit-field of flags:
	* 0: Strip mIRC colors.
	* 1: Strip text attributes.

**Returns:** A newly allocated string or NULL for failure. You must free this string with _hexchat\_free ()_.

**Example:**

<pre>
{
	char *new_text;

	/* strip both colors and attributes by using the 0 and 1 bits (1 BITWISE-OR 2) */
	new_text = hexchat_strip (ph, "\00312Blue\003 \002Bold!\002", -1, 1 | 2);

	if (new_text)
	{
		/* new_text should now contain only "Blue Bold!" */
		hexchat_printf (ph, "%s\n", new_text);
		hexchat_free (ph, new_text);
	}
}
</pre>

***

### hexchat\_free ()

**Prototype:** void hexchat\_free (hexchat\_plugin \*ph, void \*ptr);

**Description:** Frees a string returned by _hexchat\_*_ functions. Currently only used to free strings from _hexchat\_strip ()_.

**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).
 * **ptr:** Pointer to free.

***

### hexchat\_pluginpref\_set\_str ()

**Prototype:** int hexchat\_pluginpref\_set\_str (hexchat\_plugin \*ph, const char \*var, const char \*value);

**Description:** Saves a plugin-specific setting with string value to a plugin-specific config file.

**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).
 * **var:** Name of the setting to save.
 * **value:** String value of the the setting.

**Returns:**

 * 1: Success.
 * 0: Failure.

**Example:**

<pre>
int
hexchat_plugin_init (hexchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	ph = plugin_handle;
	*plugin_name = "Tester Thingie";
	*plugin_desc = "Testing stuff";
	*plugin_version = "1.0";

	hexchat_pluginpref_set_str (ph, "myvar1", "I want to save this string!");
	hexchat_pluginpref_set_str (ph, "myvar2", "This is important, too.");

	return 1;       /* return 1 for success */
}
</pre>

In the example above, the settings will be saved to the plugin_tester_thingie.conf file, and its content will be:
>myvar1 = I want to save this string!  
myvar2 = This is important, too.

You should never need to edit this file manually.

***

### hexchat\_pluginpref\_get\_str ()
**Prototype:** int hexchat_pluginpref_get_str (hexchat\_plugin \*ph, const char \*var, char \*dest);

**Description:** Loads a plugin-specific setting with string value from a plugin-specific config file.

**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).
 * **var:** Name of the setting to load.
 * **dest:** Array to save the loaded setting's string value to.

**Returns:**

 * 1: Success.
 * 0: Failure.

***

### hexchat\_pluginpref\_set\_int ()

**Prototype:** int hexchat\_pluginpref\_set\_int (hexchat\_plugin \*ph, const char \*var, int value);

**Description:** Saves a plugin-specific setting with decimal value to a plugin-specific config file.

**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).
 * **var:** Name of the setting to save.
 * **value:** Decimal value of the the setting.

**Returns:**

 * 1: Success.
 * 0: Failure.

**Example:**

<pre>
static int
saveint_cb (char *word[], char *word_eol[], void *user_data)
{
	int buffer = atoi (word[2]);

	if (buffer > 0 && buffer &lt; INT_MAX)
	{
		if (hexchat_pluginpref_set_int (ph, "myint1", buffer))
		{
			hexchat_printf (ph, "Setting successfully saved!\n");
		}
		else
		{
			hexchat_printf (ph, "Error while saving!\n");
		}
	}
	else
	{
		hexchat_printf (ph, "Invalid input!\n");
	}

	return HEXCHAT_EAT_HEXCHAT;
}
</pre>

You only need such complex checks if you're saving user input, which can be non-numeric.

***

### hexchat\_pluginpref\_get\_int ()

**Prototype:** int hexchat\_pluginpref\_get\_int (hexchat\_plugin \*ph, const char \*var);

**Description:** Loads a plugin-specific setting with decimal value from a plugin-specific config file.

**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).
 * **var:** Name of the setting to load.

**Returns:** The decimal value of the requested setting upon success, -1 for failure.

***

### hexchat\_pluginpref\_delete ()

**Prototype:** int hexchat\_pluginpref\_delete (hexchat\_plugin \*ph, const char \*var);

**Description:** Deletes a plugin-specific setting from a plugin-specific config file.

**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).
 * **var:** Name of the setting to delete.

**Returns:**

 * 1: Success.
 * 0: Failure.

If the given setting didn't exist, it also returns 1, so 1 only indicates that the setting won't exist after the call.

***

### hexchat\_pluginpref\_list ()

**Prototype:** int hexchat\_pluginpref\_list (hexchat\_plugin \*ph, char \*dest);

**Description:** Builds a comma-separated list of the currently saved settings from a plugin-specific config file.

**Arguments:**

 * **ph:** Plugin handle (as given to _hexchat\_plugin\_init ()_).
 * **dest:** Array to save the list to.

**Returns:**

 * 1: Success.
 * 0: Failure (nonexistent, empty or inaccessible config file).

**Example:**
<pre>
static void
list_settings ()
{
	char list[512];
	char buffer[512];
	char *token;

	hexchat_pluginpref_list (ph, list);
	hexchat_printf (ph, "Current Settings:\n");
	token = strtok (list, ",");

	while (token != NULL)
	{
		hexchat_pluginpref_get_str (ph, token, buffer);
		hexchat_printf (ph, "%s: %s\n", token, buffer);
		token = strtok (NULL, ",");
	}
}
</pre>

In the example above we query the list of currently stored settings, then print them one by one with their respective values. We always use _hexchat\_pluginpref\_get\_str ()_, and that's because we can read an integer as string (but not vice versa).
