# HexChat Perl Interface


## Introduction

This is the Perl interface for HexChat. If there are any problems, questions, comments or
suggestions please email them to the address on the bottom of this page.


## Constants


###Priorities

 * **`Xchat::PRI_HIGHEST`**
 * **`Xchat::PRI_HIGH`**
 * **`Xchat::PRI_NORM`**
 * **`Xchat::PRI_LOW`**
 * **`Xchat::PRI_LOWEST`**


### Return values

 * **`Xchat::EAT_NONE`** - pass the event along
 * **`Xchat::EAT_XCHAT`** - don't let HexChat see this event
 * **`Xchat::EAT_PLUGIN`** - don't let other scripts and plugins see this event but xchat will still see it
 * **`Xchat::EAT_ALL`** - don't let anything else see this event


#### Timer and fd hooks

 * **`Xchat::KEEP`** - keep the timer going or hook watching the handle
 * **`Xchat::REMOVE`** - remove the timer or hook watching the handle


### hook\_fd flags

 * **`Xchat::FD_READ`** - invoke the callback when the handle is ready for reading
 * **`Xchat::FD_WRITE`** - invoke the callback when the handle is ready for writing
 * **`Xchat::FD_EXCEPTION`** - invoke the callback if an exception occurs
 * **`Xchat::FD_NOTSOCKET`** - indicate that the handle being hooked is not a socket

## Functions

### `Xchat::register( $name, $version, [$description,[$callback]] )`

 * `$name` - The name of this script
 * `$version` - This script's version
 * `$description` - A description for this script
 * `$callback` - This is a function that will be called when the is script
                     unloaded. This can be either a reference to a
                     function or an anonymous sub reference.

This is the first thing to call in every script.

### `Xchat::hook_server( $message, $callback, [\%options] )`

### `Xchat::hook_command( $command, $callback, [\%options] )`

### `Xchat::hook_print( $event,$callback, [\%options] )`

### `Xchat::hook_timer( $timeout,$callback, [\%options | $data] )`

### `Xchat::hook_fd( $handle, $callback, [ \%options ] )`

These functions can be to intercept various events.
hook\_server can be used to intercept any incoming message from the IRC server.
hook\_command can be used to intercept any command, if the command doesn't currently exist then a new one is created.
hook\_print can be used to intercept any of the events listed in _Setttings_ `->` _Text Events_.
hook\_timer can be used to create a new timer

 * **`$message`** - server message to hook such as PRIVMSG
 * **`$command`** - command to intercept, without the leading /
 * **`$event`** - one of the events listed in _Settings_ `->` _Text Events_
 * **`$timeout`** - timeout in milliseconds
 * **`$handle`** - the I/O handle you want to monitor with hook\_fd. This must be something that has a fileno. See perldoc -f fileno or [fileno](http://perldoc.perl.org/functions/fileno.html)
 * **`$callback`** - callback function, this is called whenever
                  the hooked event is trigged, the following are
                  the conditions that will trigger the different hooks.
                  This can be either a reference to a
                  function or an anonymous sub reference.
 * **`\%options`** - a hash reference containing addional options for the hooks

Valid keys for \%options:

<table border="1">   <tr>
   <td>data</td>  <td>Additional data that is to be associated with the<br />
                  hook. For timer hooks this value can be provided either as<br />
                  <code>Xchat::hook_timer( $timeout, $cb,{data=&gt;$data})</code><br />
                  or <code>Xchat::hook_timer( $timeout, $cb, $data )</code>.<br />
                  However, this means that hook_timer cannot be provided<br />
                  with a hash reference containing data as a key.<br />                  example:<br />
                  my $options = { data =&gt; [@arrayOfStuff] };<br />
                  Xchat::hook_timer( $timeout, $cb, $options );<br />
                  <br />
                  In this example, the timer's data will be<br />
                  [@arrayOfStuff] and not { data =&gt; [@arrayOfStuff] }<br />
                  <br />
                  This key is valid for all of the hook functions.<br />
                  <br />
                  Default is undef.<br />
                  </td>
   </tr>   <tr>
      <td>priority</td> <td>Sets the priority for the hook.<br />
                        It can be set to one of the
                        <code>Xchat::PRI_*</code> constants.<br />
                        <br />
                        This key only applies to server, command
                        and print hooks.<br />
                        <br />
                        Default is <code>Xchat::PRI_NORM</code>.
                        </td>   </tr>   <tr>
      <td>help_text</td>   <td>Text displayed for /help $command.<br />
                           <br />
                           This key only applies to command hooks.<br />
                           <br />
                           Default is "".
                           </td>
   </tr>   <tr>
      <td>flags</td>   <td>Specify the flags for a fd hook.<br />
                       <br />
                       See <a href="#hook_fd_flags">hook fd flags</a> section for valid values.<br />
                       <br />
                       On Windows if the handle is a pipe you specify<br />
                       Xchat::FD_NOTSOCKET in addition to any other flags you might be using.<br />
                       <br />
                       This key only applies to fd hooks.<br />
                       Default is Xchat::FD_READ
                           </td>
   </tr></table><p>


#### When callbacks are invoked

Each of the hooks will be triggered at different times depending on the type
of hook.

<table border="1">   <tr style="background-color: #dddddd">
      <td>Hook Type</td>   <td>When the callback will be invoked</td>
   </tr>   <tr>
      <td>server hooks</td>   <td>a <code>$message</code> message is 
                              received from the server
                              </td>
   </tr>   <tr>
      <td>command hooks</td>  <td>the <code>$command</code> command is
                              executed, either by the user or from a script
                              </td>
   </tr>   <tr>
      <td>print hooks</td> <td>X-Chat is about to print the message for the
                           <code>$event</code> event
                           </td>
   </tr>   <tr>
      <td>timer hooks</td> <td>called every <code>$timeout</code> milliseconds
                           (1000 millisecond is 1 second)<br />
                           the callback will be executed in the same context where
                           the hook_timer was called, if the context no longer exists
                           then it will execute in a random context
                           </td>
   </tr>   <tr>
      <td>fd hooks</td> <td>depends on the flags that were passed to hook_fd<br />
                        See <a href="#hook_fd_flags">hook_fd flags</a> section.
                        </td>
   </tr>
</table>

The value return from these hook functions can be passed to `Xchat::unhook` to remove the hook.


#### Callback Arguments

All callback functions will receive their arguments in `@_` like every other Perl subroutine.

Server and command callbacks

`$_[0]` - array reference containing the IRC message or command and arguments broken into words
example:  
/command arg1 arg2 arg3  
`$_[0][0]` - command  
`$_[0][1]` - arg1  
`$_[0][2]` - arg2  
`$_[0][3]` - arg3

`$_[1]` - array reference containing the Nth word to the last word   
example:  
/command arg1 arg2 arg3  
`$_[1][0]` - command arg1 arg2 arg3  
`$_[1][1]` - arg1 arg2 arg3  
`$_[1][2]` - arg2 arg3  
`$_[1][3]` - arg3  

`$_[2]` - the data that was passed to the hook function

Print callbacks

`$_[0]` - array reference containing the values for the text event, see _Settings_ `->` _Text Events_  
`$_[1]` - the data that was passed to the hook function

Timer callbacks

`$_[0]` - the data that was passed to the hook function

fd callbacks

`$_[0]` - the handle that was passed to hook\_fd
`$_[1]` - flags indicating why the callback was called
`$_[2]` - the data that was passed to the hook function


#### Callback return values

All server, command and print  callbacks should return one of the `Xchat::EAT_*` constants.  
Timer callbacks can return `Xchat::REMOVE` to remove the timer or `Xchat::KEEP` to keep it going.


#### Miscellaneous Hook Related Information

For server hooks, if `$message` is "RAW LINE" then `$cb`> will be called for every IRC message that HexChat receives.

For command hooks if `$command` is "" then `$cb` will be called for messages entered by the user that is not a command.

For print hooks besides those events listed in _Settings_ `->` _Text Events_, these additional events can be used.

<table border="1">   <tr style="background-color: #dddddd">
      <td>Event</td> <td>Description</td>
   </tr>   <tr>
      <td>"Open Context"</td> <td>a new context is created</td>
   </tr>   <tr>
      <td>"Close Context"</td>   <td>a context has been close</td>
   </tr>   <tr>
      <td>"Focus Tab"</td> <td>when a tab is brought to the front</td>
   </tr>   <tr>
      <td>"Focus Window"</td> <td>when a top level window is focused or the
                              main tab window is focused by the window manager
                              </td>
   </tr>   <tr>
      <td>"DCC Chat Text"</td>   <td>when text from a DCC Chat arrives.
                                 <code>$_[0]</code> will have these values<br />
                                 <br />
                                 <code>$_[0][0]</code>   -  Address<br />
                                 <code>$_[0][1]</code>   -  Port<br />
                                 <code>$_[0][2]</code>   -  Nick<br />
                                 <code>$_[0][3]</code>   -  Message<br />
                                 </td>
   </tr>   <tr>
      <td>"Key Press"</td> <td>used for intercepting key presses<br />
			$_[0][0] - key value<br />
			$_[0][1] - state bitfield, 1 - shift, 4 - control, 8 - alt<br />
			$_[0][2] - string version of the key which might be empty for unprintable keys<br />
			$_[0][3] - length of the string in $_[0][2]<br />
		</td>
   </tr>
</table>


### `Xchat::unhook( $hook )`


 * **`$hook` - the hook that was previously returned by one of the `Xchat::hook_*` functions**

This function is used to removed a hook previously added with one of the `Xchat::hook_*` functions.

It returns the data that was passed to the `Xchat::hook_*` function when the hook was added.


### `Xchat::print( $text | \@lines, [$channel,[$server]] )`

 * **`$text` - the text to print**
 * **`\@lines` - array reference containing lines of text to be printed all the elements will be joined together before printing**
 * **`$channel` - channel or tab with the given name where `$text` will be printed**
 * **`$server` - specifies that the text will be printed in a channel or tab that is associated with `$server`**

The first argument can either be a string or an array reference of strings.
Either or both of `$channel` and `$server` can be undef.

If called as `Xchat::print( $text )`, it will always return true.
If called with either the channel or the channel and the server
specified then it will return true if a context is found and
false otherwise. The text will not be printed if the context
is not found.  The meaning of setting `$channel` or `$server` to
undef is the same as find\_context.


### `Xchat::printf( $format, LIST )`

 * **`$format` - a format string, see "perldoc -f [sprintf](http://perldoc.perl.org/functions/sprintf.html)" for further details**
 * **`LIST` - list of values for the format fields**


### `Xchat::command( $command | \@commands, [$channel,[$server]] )`

 * **`$command` - the command to execute, without the leading /**
 * **`\@commands` - array reference containing a list of commands to execute**
 * **`$channel` - channel or tab with the given name where `$command` will be executed**
 * **`$server` - specifies that the command will be executed in a channel or tab that is associated with `$server`**

The first argument can either be a string or an array reference of strings.
Either or both of `$channel` and `$server` can be undef.

If called as `Xchat::command( $command )`, it will always return true.
If called with either the channel or the channel and the server
specified then it will return true if a context is found and false
otherwise. The command will not be executed if the context is not found.
The meaning of setting `$channel` or `$server` to undef is the same as find\_context.


### `Xchat::commandf( $format, LIST )`

 * **`$format` - a format string, see "perldoc -f [sprintf](http://perldoc.perl.org/functions/sprintf.html)" for further details**
 * **`LIST` - list of values for the format fields**


### `Xchat::find_context( [$channel, [$server]] )`

 * **`$channel` - name of a channel**
 * **`$server` - name of a server**

Either or both of `$channel` and `$server` can be undef. Calling
`Xchat::find_context()` is the same as calling `Xchat::find_context( undef, undef)` and
`Xchat::find_context( $channel )` is the same as `Xchat::find_context( $channel, undef )`.

If `$server` is undef, find any channel named `$channel`. If `$channel` is undef, find the front most window
or tab named `$server`.If both `$channel` and `$server` are undef, find the currently focused tab or window.

Return the context found for one of the above situations or undef if such
a context cannot be found.


### `Xchat::get_context()`

Returns the current context.


### `Xchat::set_context( $context | $channel,[$server] )`

 * **`$context` - context value as returned from `get_context`, `find_context` or one
               of the fields in the list of hashrefs returned by `list_get`**
 * **`$channel` - name of a channel you want to switch context to**
 * **`$server` - name of a server you want to switch context to**

See `find_context` for more details on `$channel` and `$server`.

Returns true on success, false on failure.


### `Xchat::get_info( $id )`

 * **`$id` - one of the following case sensitive values**

<table border="1">   <tr style="background-color: #dddddd">
      <td>ID</td>
      <td>Return value</td>
      <td>Associated Command(s)</td>
   </tr>   <tr>
      <td>away</td>
      <td>away reason or undef if you are not away</td>
      <td>AWAY, BACK</td>
   </tr>   <tr>
      <td>channel</td>
      <td>current channel name</td>
      <td>SETTAB</td>
   </tr>   <tr>
      <td>charset</td>
      <td>character-set used in the current context</td>
      <td>CHARSET</td>
   </tr>
<tr>
   <td>configdir</td> <td>HexChat config directory encoded in UTF-8. Examples:<br />
                     /home/user/.config/hexchat<br />
                     C:\Users\user\Appdata\Roaming\HexChat
                     </td>
   <td></td>
</tr>
<tr>
      <td>event_text &lt;Event Name&gt;</td> <td>text event format string for &lt;Event name&gt;<br />
      Example:
   <div class="example synNormal"><div class='line_number'>
<div>1</div>
</div>
<div class='content'><pre><span class="synStatement">my</span> <span class="synIdentifier">$channel_msg_format</span> = Xchat::get_info( <span class="synStatement">&quot;</span><span class="synConstant">event_text Channel Message</span><span class="synStatement">&quot;</span> );
</pre></div>
</div>
   </td>
   <td></td>
</tr>
<tr>
   <td>host</td>
   <td>real hostname of the current server</td>
   <td></td>
</tr><tr>
   <td>id</td>
   <td>connection id</td>
   <td></td>
</tr><tr>
   <td>inputbox</td>
   <td>contents of the inputbox</td>
   <td>SETTEXT</td>
</tr><tr>
   <td>libdirfs</td>
   <td>the system wide directory where xchat will look for plugins.
   this string is in the same encoding as the local file system</td>
   <td></td>
</tr><tr>
   <td>modes</td>
   <td>the current channels modes or undef if not known</td>
   <td>MODE</td>
</tr><tr>
   <td>network</td>
   <td>current network name or undef, this value is taken from the Network List</td>
   <td></td>
</tr><tr>
   <td>nick</td>
   <td>current nick</td>
   <td>NICK</td>
</tr><tr>
   <td>nickserv</td>
   <td>nickserv password for this network or undef, this value is taken from the Network List</td>
   <td></td>
</tr><tr>
   <td>server</td>   <td>current server name <br />
                     (what the server claims to be) undef if not connected
                     </td>
   <td></td>
</tr><tr>
   <td>state\_cursor</td>
   <td>current inputbox cursor position in characters</td>
   <td>SETCURSOR</td>
</tr><tr>
   <td>topic</td>
   <td>current channel topic</td>
   <td>TOPIC</td>
</tr><tr>
   <td>version</td>
   <td>xchat version number</td>
   <td></td>
</tr><tr>
   <td>win_status</td>
   <td>status of the xchat window, possible values are "active", "hidden"
   and "normal"</td>
   <td>GUI</td>
</tr><tr>
  <td>win\_ptr</td> <td>native window pointer, GtkWindow * on Unix, HWND on Win32.<br />
  On Unix if you have the Glib module installed you can use my $window = Glib::Object->new\_from\_pointer( Xchat::get_info( "win\_ptr" ) ); to get a Gtk2::Window object.<br />
  Additionally when you have detached tabs, each of the windows will return a different win\_ptr for the different Gtk2::Window objects.<br />
  See <a href="http://xchat.cvs.sourceforge.net/viewvc/xchat/xchat2/plugins/perl/char_count.pl?view=markup">char\_count.pl</a> for a longer example of a script that uses this to show how many characters you currently have in your input box.
  </td>
  <td></td>
</tr>
<tr>
  <td>gtkwin_ptr</td>
  <td>similar to win_ptr except it will always be a GtkWindow *</td>
  <td></td>
</tr>
</table>

This function is used to retrieve certain information about the current
context. If there is an associated command then that command can be used
to change the value for a particular ID.


### `Xchat::get_prefs( $name )`

 * **`$name` - name of a HexChat setting (available through the /set command)**

This function provides a way to retrieve HexChat's setting information.

Returns `undef` if there is no setting called called `$name`.


### `Xchat::emit_print( $event, LIST )`

 * **`$event` - name from the Event column in _Settings_ `->` _Text Events_**
 * **`LIST` - this depends on the Description column on the bottom of _Settings_ `->` _Text Events_**

This functions is used to generate one of the events listed under _Settings_ `->` _Text Events_.

Note: when using this function you **must** return `Xchat::EAT_ALL` otherwise you will end up with duplicate events.
One is the original and the second is the one you emit.

Returns true on success, false on failure.


### `Xchat::send_modes( $target | \@targets, $sign, $mode, [ $modes_per_line ] )`

 * **`$target` - a single nick to set the mode on**
 * **`\@targets` - an array reference of the nicks to set the mode on**
 * **`$sign` - the mode sign, either '+' or '-'**
 * **`$mode` - the mode character such as 'o' and 'v', this can only be one character long**
 * **`$modes_per_line` - an optional argument maximum number of modes to send per at once, pass 0 use the current server's maximum (default)**

Send multiple mode changes for the current channel. It may send multiple MODE lines if the request doesn't fit on one.

Example:

<pre>
use strict;
use warning;
use Xchat qw(:all);

hook_command( "MODES", sub {
   my (undef, $who, $sign, $mode) = @{$_[0]};
   my @targets = split /,/, $who;
   if( @targets > 1 ) {
      send_modes( \@targets, $sign, $mode, 1 );
   } else {
      send_modes( $who, $sign, $mode );
   }
   return EAT_XCHAT;
});
</pre>

### `Xchat::nickcmp( $nick1, $nick2 )`

 * **`$nick1, $nick2` - the two nicks or channel names that are to be compared**

The comparsion is based on the current server. Either an [RFC1459](http://www.ietf.org/rfc/rfc1459.txt)
compliant string compare or plain ascii will be using depending on the server. The
comparison is case insensitive.

Returns a number less than, equal to or greater than zero if `$nick1` is
found respectively, to be less than, to match, or be greater than `$nick2`.


### `Xchat::get_list( $name )`

 * **`$name` -  name of the list, one of the following: "channels", "dcc", "ignore", "notify", "users"**

This function will return a list of hash references.  The hash references
will have different keys depend on the list.  An empty list is returned
if there is no such list.

"channels" - list of channels, querys and their server

<table border="1">   <tr style="background-color: #dddddd">
      <td>Key</td>   <td>Description</td>
   </tr>   <tr>
      <td>channel</td>  <td>tab name</td>
   </tr>   <tr>
      <td>chantypes</td>
      <td>channel types supported by the server, typically "#&amp;"</td>
   </tr>   <tr>
      <td>context</td>  <td>can be used with set_context</td>
   </tr>   <tr>
      <td>flags</td> <td>Server Bits:<br />
                     0 - Connected<br />
                     1 - Connecting<br />
                     2 - Away<br />
                     3 - EndOfMotd(Login complete)<br />
                     4 - Has WHOX<br />
                     5 - Has IDMSG (FreeNode)<br />
                    <br />
                    <p>The following correspond to the /chanopt command</p>
                    6  - Hide Join/Part Message (text_hidejoinpart)<br />
                    7  - unused (was for color paste)<br />
                    8  - Beep on message (alert_beep)<br />
                    9  - Blink Tray (alert_tray)<br />
                    10 - Blink Task Bar (alert_taskbar)<br />
<p>Example of checking if the current context has Hide Join/Part messages set:</p>
<div class="example synNormal"><div class='line_number'>
<div>1</div>
<div>2</div>
<div>3</div>
</div>
<div class='content'><pre><span class="synStatement">if</span>( Xchat::context_info-&gt;{flags} &amp; (<span class="synConstant">1</span> &lt;&lt; <span class="synConstant">6</span>) ) {
  Xchat::<span class="synStatement">print</span>( <span class="synStatement">&quot;</span><span class="synConstant">Hide Join/Part messages is enabled</span><span class="synStatement">&quot;</span> );
}
</pre></div>
</div>                     </td>
   </tr>   <tr>
      <td>id</td> <td>Unique server ID </td>
   </tr>
   
   <tr>
      <td>lag</td>
      <td>lag in milliseconds</td>
   </tr>   <tr>
      <td>maxmodes</td> <td>Maximum modes per line</td>
   </tr>   <tr>
      <td>network</td>  <td>network name to which this channel belongs</td>
   </tr>   <tr>
      <td>nickprefixes</td>   <td>Nickname prefixes e.g. "+@"</td>
   </tr>
   
   <tr>
      <td>nickmodes</td>   <td>Nickname mode chars e.g. "vo"</td>
   </tr>   <tr>
      <td>queue</td>
      <td>number of bytes in the send queue</td>
   </tr>
   
   <tr>
      <td>server</td>   <td>server name to which this channel belongs</td>
   </tr>   <tr>
      <td>type</td>  <td>the type of this context<br />
                     1 - server<br />
                     2 - channel<br />
                     3 - dialog<br />
                     4 - notices<br />
                     5 - server notices<br />
                     </td>
   </tr>   <tr>
      <td>users</td> <td>Number of users in this channel</td>
   </tr>
</table>

"dcc" - list of DCC file transfers

<table border="1">   <tr style="background-color: #dddddd">
      <td>Key</td>   <td>Value</td>
   </tr>   <tr>
      <td>address32</td>   <td>address of the remote user(ipv4 address)</td>
   </tr>   <tr>
      <td>cps</td>   <td>bytes per second(speed)</td>
   </tr>   <tr>
      <td>destfile</td> <td>destination full pathname</td>
   </tr>   <tr>
      <td>file</td>  <td>file name</td>
   </tr>   <tr>
      <td>nick</td>
      <td>nick of the person this DCC connection is connected to</td>
   </tr>   <tr>
      <td>port</td>  <td>TCP port number</td>
   </tr>   <tr>
      <td>pos</td>   <td>bytes sent/received</td>
   </tr>   <tr>
      <td>poshigh</td>   <td>bytes sent/received, high order 32 bits</td>
   </tr>   <tr>
      <td>resume</td>   <td>point at which this file was resumed<br />
                        (zero if it was not resumed)
                        </td>
   </tr>   <tr>
      <td>resumehigh</td>   <td>point at which this file was resumed, high order 32 bits<br />
                        </td>
   </tr>   <tr>
      <td>size</td>  <td>file size in bytes low order 32 bits</td>
   </tr>   <tr>
      <td>sizehigh</td>	<td>file size in bytes, high order 32 bits (when the files is > 4GB)</td>
	</tr>
	<tr>
      <td>status</td>   <td>DCC Status:<br />
                        0 - queued<br />
                        1 - active<br />
                        2 - failed<br />
                        3 - done<br />
                        4 - connecting<br />
                        5 - aborted
                        </td>
   </tr>   <tr>
      <td>type</td>  <td>DCC Type:<br />
                     0 - send<br />
                     1 - receive<br />
                     2 - chatrecv<br />
                     3 - chatsend
                     </td>
   </tr></table>

"ignore" - current ignore list

<table border="1">   <tr style="background-color: #dddddd">
      <td>Key</td> <td>Value</td>
   </tr>   <tr>
      <td>mask</td>  <td>ignore mask. e.g: *!*@*.aol.com</td>
   </tr>   <tr>
      <td>flags</td> <td>Bit field of flags.<br />
                     0 - private<br />
                     1 - notice<br />
                     2 - channel<br />
                     3 - ctcp<br />
                     4 - invite<br />
                     5 - unignore<br />
                     6 - nosave<br />
                     7 - dcc<br />
                     </td>
   </tr></table>

"notify" - list of people on notify

<table border="1">
   <tr style="background-color: #dddddd">
      <td>Key</td>   <td>Value</td>
   </tr>   <tr>
      <td>networks</td>
      <td>comma separated list of networks where you will be notfified about this user's online/offline status or undef if you will be notificed on every network you are connected to</td>
   </tr>   <tr>
      <td>nick</td>  <td>nickname</td>
   </tr>   <tr>
      <td>flags</td> <td>0 = is online</td>
   </tr>   <tr>
      <td>on</td> <td>time when user came online</td>
   </tr>   <tr>
      <td>off</td>   <td>time when user went offline</td>
   </tr>   <tr>
      <td>seen</td>  <td>time when user was last verified still online</td>
   </tr>
</table>

The values indexed by on, off and seen can be passed to localtime
and gmtime, see perldoc -f [localtime](http://perldoc.perl.org/functions/localtime.html) and
perldoc -f [gmtime](http://perldoc.perl.org/functions/gmtime.html) for more details.

"users" - list of users in the current channel

<table border="1">
<tr style="background-color: #dddddd">
      <td>Key</td>   <td>Value</td>
   </tr>   <tr>
      <td>away</td>  <td>away status(boolean)</td>
   </tr>   <tr>
      <td>lasttalk</td>
      <td>last time a user was seen talking, this is the an epoch time(number of seconds since a certain date, that date depends on the OS)</td>
   </tr>   <tr>
      <td>nick</td>  <td>nick name</td>
   </tr>   <tr>
      <td>host</td>
      <td>host name in the form: user@host or undef if not known</td>
   </tr>   <tr>
      <td>prefix</td>   <td>prefix character, .e.g: @ or +</td>
   </tr>   <tr>
      <td>realname</td>
       <td>Real name or undef</td>
   </tr>   <tr>
      <td>selected</td>
      <td>selected status in the user list, only works when retrieving the user list of the focused tab. You can use the /USELECT command to select the nicks</td>
   </tr>
</table>

"networks" - 	list of networks and the associated settings from network list

<table border="1">   <tr style="background-color: #dddddd">
      <td>Key</td>   <td>Value</td>
   </tr>
	
	<tr>
	<td>autojoins</td> <td>An object with the following methods:<br />
		<table>
			<tr>
				<td>Method</td>
				<td>Description</td>
			</tr>			<tr>
				<td>channels()</td>
				<td>returns a list of this networks' autojoin channels in list context, a count of the number autojoin channels in scalar context</td>
			</tr>			<tr>
				<td>keys()</td>
				<td>returns a list of the keys to go with the channels, the order is the same as the channels, if a channel doesn't  have a key, '' will be returned in it's place</td>
			</tr>			<tr>
				<td>pairs()</td>
				<td>a combination of channels() and keys(), returns a list of (channels, keys) pairs. This can be assigned to a hash for a mapping from channel to key.</td>
			</tr>			<tr>
				<td>as_hash()</td>
				<td>return the pairs as a hash reference</td>
			</tr>			<tr>
				<td>as_string()</td>
				<td>the original string that was used to construct this autojoin object, this can be used with the JOIN command to join all the channels in the autojoin list</td>
			</tr>			<tr>
				<td>as_array()</td>
				<td>return an array reference of hash references consisting of the keys "channel" and "key"</td>
			</tr>			<tr>
				<td>as_bool()</td>
				<td>returns true if the network has autojoins and false otherwise</td>
			</tr>
		</table>
	</td>
	</tr>
   
	<tr>
	<td>connect_commands</td> <td>An array reference containing the connect commands for a network. An empty array if there aren't any</td>
	</tr>	<tr>
	<td>encoding</td> <td>the encoding for the network</td>
	</tr>	<tr>
		<td>flags</td>
		<td>
			a hash reference corresponding to the checkboxes in the network edit window
			<table>
				<tr>
					<td>allow_invalid</td>
					<td>true if "Accept invalid SSL certificate" is checked</td>
				</tr>				<tr>
					<td>autoconnect</td>
					<td>true if "Auto connect to this network at startup" is checked</td>
				</tr>				<tr>
					<td>cycle</td>
					<td>true if "Connect to selected server only" is <strong>NOT</strong> checked</td>
				</tr>				<tr>
					<td>use_global</td>
					<td>true if "Use global user information" is checked</td>
				</tr>				<tr>
					<td>use_proxy</td>
					<td>true if "Bypass proxy server" is <strong>NOT</strong> checked</td>
				</tr>				<tr>
					<td>use_ssl</td>
					<td>true if "Use SSL for all the servers on this network" is checked</td>
				</tr>
			</table>
		</td>
	</tr>	<tr>
		<td>irc_nick1</td>
		<td>Corresponds with the "Nick name" field in the network edit window</td>
	</tr>	<tr>
		<td>irc_nick2</td>
		<td>Corresponds with the "Second choice" field in the network edit window</td>
	</tr>	<tr>
		<td>irc_real_name</td>
		<td>Corresponds with the "Real name" field in the network edit window</td>
	</tr>	<tr>
		<td>irc_user_name</td>
		<td>Corresponds with the "User name" field in the network edit window</td>
	</tr>	<tr>
		<td>network</td>
		<td>Name of the network</td>
	</tr>	<tr>
		<td>nickserv_password</td>
		<td>Corresponds with the "Nickserv password" field in the network edit window</td>
	</tr>	<tr>
		<td>selected</td>
		<td>Index into the list of servers in the "servers" key, this is used if the "cycle" flag is false</td>
	</tr>	<tr>
		<td>server_password</td>
		<td>Corresponds with the "Server password" field in the network edit window</td>
	</tr>	<tr>
		<td>servers</td>
		<td>An array reference of hash references with a "host" and "port" key. If a port is not specified then 6667 will be used.</td>
	</tr>
</table>


### `Xchat::user_info( [$nick] )`

 * **`$nick` - the nick to look for, if this is not given your own nick will be used as default**

This function is mainly intended to be used as a shortcut for when you need
to retrieve some information about only one user in a channel. Otherwise it
is better to use `get_list`. If `$nick` is found a hash reference containing
the same keys as those in the "users" list of `get_list` is returned otherwise
undef is returned. Since it relies on `get_list` this function can only be used in a
channel context.


### `Xchat::context_info( [$context] )`

 * **`$context` - context returned from `get_context`, `find_context` and `get_list`, this is the context that you want infomation about. If this is omitted, it will default to current context.**

This function will return the information normally retrieved with `get_info`, except this
is for the context that is passed in. The information will be returned in the form of a
hash. The keys of the hash are the `$id` you would normally supply to `get_info` as well
as all the keys that are valid for the items in the "channels" list from `get_list`. Use
of this function is more efficient than calling get_list( "channels" ) and
searching through the result.

Example:

<pre>
use strict;
use warnings;
use Xchat qw(:all); # imports all the functions documented on this page

register( "User Count", "0.1",
   "Print out the number of users on the current channel" );
hook_command( "UCOUNT", \&display_count );
sub display_count {
   prnt "There are " . context_info()->{users} . " users in this channel.";
   return EAT_XCHAT;
}
</pre>


### `Xchat::strip_code( $string )`

 * **`$string` - string to remove codes from**

This function will remove bold, color, beep, reset, reverse and underline codes from `$string`.
It will also remove ANSI escape codes which might get used by certain terminal based clients.
If it is called in void context `$string` will be modified otherwise a modified copy of `$string` is returned.


## Examples

### Asynchronous DNS resolution with hook\_fd

<pre>
use strict;
use warnings;
use Xchat qw(:all);
use Net::DNS;
   
hook_command( "BGDNS", sub {
   my $host = $_[0][1];
   my $resolver = Net::DNS::Resolver->new;
   my $sock = $resolver->bgsend( $host );
   
   hook_fd( $sock, sub {
      my $ready_sock = $_[0];
      my $packet = $resolver->bgread( $ready_sock );
      
      if( $packet->authority && (my @answers = $packet->answer ) ) {
         
         if( @answers ) {
            prnt "$host:";
            my $padding = " " x (length( $host ) + 2);
            for my $answer ( @answers ) {
               prnt $padding . $answer->rdatastr . ' ' . $answer->type;
            }
         }
      } else {
         prnt "Unable to resolve $host";
      }
      
      return REMOVE;
   },
   {
      flags => FD_READ,
   });
   
   return EAT_XCHAT;
});
</pre>


## Contact Information

Contact Lian Wan Situ at &lt;atmcmnky [at] yahoo.com> for questions, comments and
corrections about this page or the Perl plugin itself. You can also find me in #xchat
on freenode under the nick Khisanth.

<table border="0" width="100%" cellspacing="0" cellpadding="3">
<tr><td class="block" style="background-color: #cccccc" valign="middle">
<big><strong><span class="block">&nbsp;X-Chat 2 Perl Interface</span></strong></big>
</td></tr>
</table>
