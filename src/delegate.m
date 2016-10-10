/*
 * delegate.m
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

#import "application.h"
#import "delegate.h"
#import "display.h"
#import "kbm.h"
#import "keymap.h"
#import "menu.h"

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
	NSMenu *menu;

	self.status = [[NSStatusBar systemStatusBar]
			statusItemWithLength: NSVariableStatusItemLength];
	_status.image = [NSImage imageNamed:@"kbm"];
	_status.toolTip = @"kbm";

	menu = [[NSMenu alloc] init];
	[menu addItemWithTitle:@"Notifications"
		action:@selector(toggleNotifications:)
		keyEquivalent:@""];
	[menu addItemWithTitle:@"Quit"
		action:@selector(menuQuit)
		keyEquivalent:@""];

	if (kbm_info.notifications)
		[[menu itemWithTitle:@"Notifications"] setState: NSOnState];

	_status.menu = menu;

	start_listening();
}

- (void)menuQuit
{
	[NSApp terminate:nil];
}

/* toggleNotifications: enable/disable notifications */
- (void)toggleNotifications:(id)sender
{
	int state;
	NSMenuItem *m = (NSMenuItem *)sender;

	kbm_info.notifications = !kbm_info.notifications;
	state = kbm_info.notifications ? NSOnState : NSOffState;

	[m setState: state];
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
	unload_keys();
	close_display();
	keymap_free();
}

@end
