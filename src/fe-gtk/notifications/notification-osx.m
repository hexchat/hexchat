/* HexChat
 * Copyright (C) 2015 Patrick Griffis.
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

#import <Cocoa/Cocoa.h>
#include <gtkosxapplication.h>

void
notification_backend_show (const char *title, const char *text)
{
	NSString *str_title = [[NSString alloc] initWithUTF8String:title];
	NSString *str_text = [[NSString alloc] initWithUTF8String:text];

	NSUserNotification *userNotification = [NSUserNotification new];
	userNotification.title = str_title;
	userNotification.informativeText = str_text;

	NSUserNotificationCenter *center = [NSUserNotificationCenter defaultUserNotificationCenter];
	[center scheduleNotification:userNotification];

	[str_title release];
	[str_text release];
}

int
notification_backend_init (const char **error)
{
	return 1;
}

void
notification_backend_deinit (void)
{
}

int
notification_backend_supported (void)
{
	return gtkosx_application_get_bundle_id () != NULL;
}
