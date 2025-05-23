/*
 * utils/mac_utils.h - utility functions for macOS
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
#import <AppKit/NSPasteboard.h>
#import <globals.h>
#import <objc/Object.h>
#import <stddef.h>
#import <stdint.h>
#import <string.h>
#import <utils/utils.h>

#if !__has_feature(objc_arc)
#error This file must be compiled with ARC.
#endif

int get_clipboard_text(char **bufptr, uint32_t *lenptr) {
    NSPasteboard *pasteBoard = [NSPasteboard generalPasteboard];
    NSString *copiedString = [pasteBoard stringForType:NSPasteboardTypeString];
    if (!copiedString) {
        *lenptr = 0;
        if (*bufptr) free(*bufptr);
        *bufptr = NULL;
        return EXIT_FAILURE;
    }
    const char *cstring = [copiedString UTF8String];
    *bufptr = strndup(cstring, configuration.max_text_length);
    *lenptr = (uint32_t)strnlen(*bufptr, configuration.max_text_length + 1);
    (*bufptr)[*lenptr] = 0;
    return EXIT_SUCCESS;
}

int put_clipboard_text(char *data, uint32_t len) {
    char c = data[len];
    data[len] = 0;
    NSString *str_data = @(data);
    data[len] = c;
    NSPasteboard *pasteBoard = [NSPasteboard generalPasteboard];
    [pasteBoard clearContents];
    BOOL status = [pasteBoard setString:str_data forType:NSPasteboardTypeString];
    if (status != YES) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

char *get_copied_files_as_str(int *offset) {
    *offset = 0;
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSArray *classes = [NSArray arrayWithObject:[NSURL class]];
    NSDictionary *options = [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:YES]
                                                        forKey:NSPasteboardURLReadingFileURLsOnlyKey];
    NSArray *fileURLs = [pasteboard readObjectsForClasses:classes options:options];
    size_t tot_len = 0;
    for (NSURL *fileURL in fileURLs) {
        NSURL *pathURL = [fileURL filePathURL];
        if (!pathURL) continue;
        NSString *absString = [pathURL absoluteString];
        if (!absString) continue;
        const char *cstring = [absString UTF8String];
        tot_len += strnlen(cstring, 2047) + 1;  // +1 for separator (\n) or terminator (\0)
    }

    char *all_files = malloc(tot_len);
    if (!all_files) return NULL;
    char *ptr = all_files;
    for (NSURL *fileURL in fileURLs) {
        NSURL *pathURL = [fileURL filePathURL];
        if (!pathURL) continue;
        NSString *absString = [pathURL absoluteString];
        if (!absString) continue;
        const char *cstring = [absString UTF8String];
        strncpy(ptr, cstring, MIN(tot_len, 2047));
        size_t url_len = strnlen(cstring, 2047);
        ptr += strnlen(cstring, 2047);
        *ptr = '\n';
        ptr++;
        tot_len -= url_len + 1;
    }
    ptr--;
    *ptr = 0;
#ifdef DEBUG_MODE
    puts(all_files);
#endif
    return all_files;
}

#endif
