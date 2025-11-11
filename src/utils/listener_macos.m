/*
 * utils/listener_macos.c - implementation of clipboard listener for macOS
 * Copyright (C) 2025 H. Thevindu J. Wijesekera
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

#import <Cocoa/Cocoa.h>
#import <stdlib.h>
#import <utils/clipboard_listener.h>
#import <utils/utils.h>

#if !__has_feature(objc_arc)
#error This file must be compiled with ARC.
#endif

ListenerCallback clip_callback = NULL;

@interface ClipboardListener : NSObject
@property NSPasteboard *pasteboard;
@property NSInteger changeCount;
@property NSArray *classes;
@property NSDictionary *options;
@end

@implementation ClipboardListener

- (instancetype)init {
    self = [super init];
    if (self) {
        self.pasteboard = [NSPasteboard generalPasteboard];
        self.changeCount = [self.pasteboard changeCount];
        self.classes = [NSArray arrayWithObject:[NSURL class]];
        self.options = [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:YES]
                                                   forKey:NSPasteboardURLReadingFileURLsOnlyKey];
        [NSTimer scheduledTimerWithTimeInterval:0.2
                                         target:self
                                       selector:@selector(checkClipboard)
                                       userInfo:nil
                                        repeats:YES];
    }
    return self;
}

- (void)checkClipboard {
    if (self.pasteboard.changeCount == self.changeCount) {
        return;
    }
    self.changeCount = self.pasteboard.changeCount;

    if (check_and_delete_temp_file()) {
        return;
    }

    int copied_type = COPIED_TYPE_NONE;
    @autoreleasepool {
        NSArray *fileURLs = [self.pasteboard readObjectsForClasses:self.classes options:self.options];
        if (fileURLs && [fileURLs count]) {
            copied_type = COPIED_TYPE_FILE;
        }
        NSString *text =
            (copied_type == COPIED_TYPE_NONE) ? [self.pasteboard stringForType:NSPasteboardTypeString] : NULL;
        if (text) {
            copied_type = COPIED_TYPE_TEXT;
        }
    }
    if (copied_type != COPIED_TYPE_NONE) {
        clip_callback(copied_type);
    }
}

@end

int clipboard_listen(ListenerCallback callback) {
    clip_callback = callback;
    @autoreleasepool {
        ClipboardListener *listener = [[ClipboardListener alloc] init];
        (void)listener;
        [[NSRunLoop currentRunLoop] run];
    }
    return EXIT_SUCCESS;
}

void cleanup_listener(void) {}

#endif
