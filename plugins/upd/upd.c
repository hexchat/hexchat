/* HexChat
 * Copyright (c) 2010-2012 Berke Viktor.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <winsparkle.h>

#include "hexchat-plugin.h"

#define APPCAST_URL "https://hexchat.github.io/appcast.xml"

static hexchat_plugin *ph;   /* plugin handle */
static char name[] = "Update Checker";
static char desc[] = "Check for HexChat updates automatically";
static char version[] = "5.0";
static const char upd_help[] = "Update Checker Usage:\n  /UPDCHK, check for HexChat updates\n";

static int
check_cmd (char *word[], char *word_eol[], void *userdata)
{
	win_sparkle_check_update_with_ui ();

	return HEXCHAT_EAT_ALL;
}

int
hexchat_plugin_init (hexchat_plugin *plugin_handle, char **plugin_name, char **plugin_desc, char **plugin_version, char *arg)
{
	ph = plugin_handle;

	*plugin_name = name;
	*plugin_desc = desc;
	*plugin_version = version;

	win_sparkle_set_appcast_url (APPCAST_URL);
	win_sparkle_init ();

	hexchat_hook_command (ph, "UPDCHK", HEXCHAT_PRI_NORM, check_cmd, upd_help, NULL);
	hexchat_command (ph, "MENU -ishare\\download.png ADD \"Help/Check for Updates\" \"UPDCHK\"");
	hexchat_printf (ph, "%s plugin loaded\n", name);

	return 1;
}

int
hexchat_plugin_deinit (void)
{
	win_sparkle_cleanup ();

	hexchat_command (ph, "MENU DEL \"Help/Check for updates\"");
	hexchat_printf (ph, "%s plugin unloaded\n", name);
	return 1;
}
