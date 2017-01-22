/*
 * delegate.m
 * Copyright (C) 2016-2017 Alexei Frolov
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

#import <string.h>

#import "application.h"
#import "delegate.h"
#import "display.h"
#import "kbm.h"
#import "keymap.h"
#import "menu.h"
#import "parser.h"

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
	NSMenu *menu;

	KBM_UNUSED(notification);

	self.status = [[NSStatusBar systemStatusBar]
			statusItemWithLength: NSVariableStatusItemLength];
	_status.image = [NSImage imageNamed:@"kbm"];
	_status.toolTip = @"kbm";

	menu = [[NSMenu alloc] init];
	[menu addItemWithTitle:@"Notifications"
		action:@selector(toggleNotifications:)
		keyEquivalent:@""];
	[menu addItemWithTitle:@"Open File"
		action:@selector(openFile)
		keyEquivalent:@""];
	[menu addItemWithTitle:@"Quit"
		action:@selector(menuQuit)
		keyEquivalent:@""];

	if (kbm_info.notifications)
		[[menu itemWithTitle:@"Notifications"] setState: NSOnState];

	_status.menu = menu;

	/* start listening for window changes */
	[[[NSWorkspace sharedWorkspace] notificationCenter]
		addObserver:self selector:@selector(windowChange:)
		name:NSWorkspaceDidActivateApplicationNotification
		object:nil];

	start_listening();
}

- (void)menuQuit
{
	[NSApp terminate:nil];
}

#define MAX_PATH 2048

- (void)openFile
{
	NSOpenPanel *panel;
	NSURL *file;
	FILE *f;
	char buf[MAX_PATH], err_file[MAX_PATH];
	static char path[MAX_PATH];

	panel = [[NSOpenPanel alloc] init];
	panel.canChooseFiles = true;
	panel.allowsMultipleSelection = false;
	[panel setAllowedFileTypes:[NSArray arrayWithObject:@"kbm"]];

	if ([panel runModal] == NSFileHandlingPanelOKButton) {
		file = [panel URLs][0];
		path[0] = '\0';
		strncat(path, [file fileSystemRepresentation], 1024);
		snprintf(err_file, MAX_PATH, "%s/.kbm_errlog", getenv("HOME"));
		f = fopen(err_file, "a");
		free_windows(&kbm_info.map);
		if (parse_file(path, &kbm_info.map, f) != 0) {
			snprintf(buf, MAX_PATH, "Could not parse keys from the "
						"file\n%s.\nErrors written "
						"to\n%s", path, err_file);
			osx_alert(2, "Failed to parse key map file", buf);
			goto cleanup;
		}

		unload_keys();
		load_keys(kbm_info.map.keys);
		kbm_info.curr_file = basename(path);

cleanup:
		fclose(f);
	} else {
		fprintf(stderr, "failed to get file\n");
	}
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

/*
 * windowChange:
 * Called when the active window changes. Check to see if it is in the
 * active windows array and process accordingly.
 */
- (void)windowChange:(NSNotification *)notification
{
	const char *window;
	char **s;
	NSWorkspace *ws = [notification object];

	if (!(kbm_info.map.flags & KBM_ACTIVEWIN))
		return;

	window = [[[ws frontmostApplication] localizedName] UTF8String];

	for (s = kbm_info.map.windows; *s; ++s) {
		if (strcmp(*s, window) == 0) {
			PRINT_DEBUG("ACTIVE window %s - keys enabled\n",
			            window);
			break;
		}
	}
	if (!*s) {
		PRINT_DEBUG("window %s - keys disabled\n", window);
	}
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
	KBM_UNUSED(notification);

	unload_keys();
	close_display();
	free_windows(&kbm_info.map);
	keymap_free();
}

@end
