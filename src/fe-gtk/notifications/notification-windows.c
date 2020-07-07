/* HexChat
 * Copyright (C) 2015 Arnav Singh.
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

#include "hexchat.h"
#include "plugin.h"

#include <gmodule.h>
#include <windows.h>

void (*winrt_notification_backend_show) (const char *title, const char *text) = NULL;
int (*winrt_notification_backend_init) (const char **error) = NULL;
void (*winrt_notification_backend_deinit) (void) = NULL;
int (*winrt_notification_backend_supported) (void) = NULL;

void
notification_backend_show (const char *title, const char *text)
{
	if (winrt_notification_backend_show == NULL)
	{
		return;
	}

	winrt_notification_backend_show (title, text);
}

int
notification_backend_init (const char **error)
{
	UINT original_error_mode;
	GModule *module;

	/* Temporarily suppress the "DLL could not be loaded" dialog box before trying to load hcnotifications-winrt.dll */
	original_error_mode = GetErrorMode ();
	SetErrorMode(SEM_FAILCRITICALERRORS);
	module = module_load (HEXCHATLIBDIR "\\hcnotifications-winrt.dll");
	SetErrorMode (original_error_mode);

	if (module == NULL)
	{
		*error = "hcnotifications-winrt not found.";
		return 0;
	}

	g_module_symbol (module, "notification_backend_show", (gpointer *) &winrt_notification_backend_show);
	g_module_symbol (module, "notification_backend_init", (gpointer *) &winrt_notification_backend_init);
	g_module_symbol (module, "notification_backend_deinit", (gpointer *) &winrt_notification_backend_deinit);
	g_module_symbol (module, "notification_backend_supported", (gpointer *) &winrt_notification_backend_supported);

	return winrt_notification_backend_init (error);
}

void
notification_backend_deinit (void)
{
	if (winrt_notification_backend_deinit == NULL)
	{
		return;
	}

	winrt_notification_backend_deinit ();
}

int
notification_backend_supported (void)
{
	if (winrt_notification_backend_supported == NULL)
	{
		return 0;
	}

	return winrt_notification_backend_supported ();
}
