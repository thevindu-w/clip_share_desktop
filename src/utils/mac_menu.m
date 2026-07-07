/*
 * utils/mac_menu.m - show menu icon on macos
 * Copyright (C) 2024-2025 H. Thevindu J. Wijesekera
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef __APPLE__

#import <AppKit/AppKit.h>
#import <globals.h>
#import <utils/kill_others.h>
#include <utils/mac_menu.h>
#import <utils/utils.h>

#define OPEN_PATH "/usr/bin/open"

extern char icon_png[];
extern unsigned int icon_png_len;

const char *global_prog_name;

@implementation NSApplication (KillInstances)  // NOLINT

- (void)onQuitAction:(id)sender {
    kill_other_processes(global_prog_name);
    [NSApp terminate:nil];
}

- (void)onOpenBrowserAction:(id)sender {
    if (fork() == 0) {
        char url[24];
        snprintf(url, sizeof(url), "http://127.0.0.1:%hu", configuration.ports.web);
        execl(OPEN_PATH, "open", url, NULL);
        error_exit("Couldn't open default browser");
    }
}

@end

void show_menu_icon(void) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        NSStatusItem *statusItem = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];

        NSData *iconData = [NSData dataWithBytesNoCopy:icon_png length:icon_png_len freeWhenDone:NO];
        NSImage *iconImage = [[NSImage alloc] initWithData:iconData];
        if (!iconImage) {
#ifdef DEBUG_MODE
            error("Image allocation failed for menu icon");
#endif
            return;
        }
        [statusItem.button setImage:iconImage];

        NSMenu *menu = [[NSMenu alloc] init];
        if (!menu) {
#ifdef DEBUG_MODE
            error("Menu allocation or creation failed");
#endif
            return;
        }

        if (file_exists(OPEN_PATH)) {
            NSMenuItem *browserMenuItem = [[NSMenuItem alloc] initWithTitle:@"Open in Browser"
                                                                     action:@selector(onOpenBrowserAction:)
                                                              keyEquivalent:@"b"];
            if (!browserMenuItem) {
#ifdef DEBUG_MODE
                error("Menu item creation failed");
#endif
                return;
            }
            [menu addItem:browserMenuItem];
        }

        NSMenuItem *quitMenuItem = [[NSMenuItem alloc] initWithTitle:@"Quit"
                                                              action:@selector(onQuitAction:)
                                                       keyEquivalent:@"q"];
        if (!quitMenuItem) {
#ifdef DEBUG_MODE
            error("Menu item creation failed");
#endif
            return;
        }
        [menu addItem:quitMenuItem];
        statusItem.menu = menu;
        [quitMenuItem setTarget:NSApp];

        [app run];
    }
}

#endif
