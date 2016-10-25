/*
 * application.m
 * Copyright (C) 2016 Alexei Frolov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#import <Cocoa/Cocoa.h>
#import "kbm.h"

/* osx_notify: send user notification with message msg */
void osx_notify(const char *msg)
{
	NSUserNotification *n;

	n = [[NSUserNotification alloc] init];
	n.title = @PROGRAM_NAME;
	n.informativeText = [NSString stringWithUTF8String: msg];

	[[NSUserNotificationCenter defaultUserNotificationCenter]
			deliverNotification: n];
}

/* osx_alert: create a message box with message msg */
void osx_alert(int style, const char *title, const char *msg)
{
	NSAlert *alert;

	alert = [[NSAlert alloc] init];
	alert.alertStyle = style;
	alert.messageText = [NSString stringWithUTF8String: title];
	alert.informativeText = [NSString stringWithUTF8String: msg];
	[alert addButtonWithTitle:@"Ok"];
	[alert runModal];
}

void terminate_app(void)
{
	[NSApp terminate:nil];
}
