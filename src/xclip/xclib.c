/*
 *  xclib.c - xclip library to look after xlib mechanics for xclip
 *  Copyright (C) 2001 Kim Saunders
 *  Copyright (C) 2007-2022 Peter Åstrand
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 *  2022-2024 Modified by H. Thevindu J. Wijesekera
 */

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <globals.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/utils.h>
#include <xclip/xclib.h>

#define MAX_BUF_LEN 16777216UL

/* wrapper for malloc that checks for errors */
void *xcmalloc(size_t size) {
    if (!size) error_exit("malloc zero");
    if (size > MAX_BUF_LEN && size > configuration.max_text_length) error_exit("malloc too large");

    void *mem = malloc(size);
    if (!mem) error_exit("malloc failed");

    return mem;
}

/* wrapper for realloc that checks for errors */
void *xcrealloc(void *ptr, size_t size) {
    if (!size) {
        if (ptr) free(ptr);
        error_exit("realloc zero");
    }
    if (size > MAX_BUF_LEN && size > configuration.max_text_length) {
        if (ptr) free(ptr);
        error_exit("realloc too large");
    }

    void *mem = realloc(ptr, size);
    if (!mem) {
        free(ptr);
        error_exit("realloc failed");
    }

    return mem;
}

/* Returns the machine-specific number of bytes per data element
 * returned by XGetWindowProperty */
static size_t mach_itemsize(int format) {
    if (format == 8) return sizeof(char);
    if (format == 16) return sizeof(short);
    if (format == 32) return sizeof(long);
    return 0;
}

/* Retrieves the contents of a selections. Arguments are:
 *
 * A display that has been opened.
 *
 * A window
 *
 * An event to process
 *
 * The selection to return
 *
 * The target(UTF8_STRING or XA_STRING) to return
 *
 * A pointer to an atom that receives the type of the data
 *
 * A pointer to a char array to put the selection into.
 *
 * A pointer to a long to record the length of the char array
 *
 * A pointer to an int to record the context in which to process the event
 *
 * Return value is 1 if the retrieval of the selection data is complete,
 * otherwise it's 0.
 */
int xcout(Display *dpy, Window win, XEvent evt, Atom sel, Atom target, Atom *type, unsigned char **txt_p,
          unsigned long *len_p, unsigned int *context) {
    /* a property for other windows to put their selection into */
    static Atom pty;
    static Atom inc;
    int pty_format;

    /* buffer for XGetWindowProperty to dump data into */
    unsigned char *buffer;
    unsigned long pty_size;
    unsigned long pty_items;
    unsigned long pty_machsize;

    /* local buffer of text to return */
    unsigned char *ltxt = *txt_p;

    if (!pty) {
        pty = XInternAtom(dpy, "XCLIP_OUT", False);
    }

    if (!inc) {
        inc = XInternAtom(dpy, "INCR", False);
    }

    switch (*context) {
        /* there is no context, do an XConvertSelection() */
        case XCLIB_XCOUT_NONE: {
            /* initialise return length to 0 */
            if (*len_p > 0) {
                free(*txt_p);
                *len_p = 0;
            }

            /* send a selection request */
            XConvertSelection(dpy, sel, target, pty, win, CurrentTime);
            *context = XCLIB_XCOUT_SENTCONVSEL;
            return 0;
        }
        case XCLIB_XCOUT_SENTCONVSEL: {
            if (evt.type != SelectionNotify) return 0;

            /* return failure when the current target failed */
            if (evt.xselection.property == None) {
                *context = XCLIB_XCOUT_BAD_TARGET;
                return 0;
            }

            /* find the size and format of the data in property */
            XGetWindowProperty(dpy, win, pty, 0, 0, False, AnyPropertyType, type, &pty_format, &pty_items, &pty_size,
                               &buffer);
            if (buffer) XFree(buffer);

            if (*type == inc) {
                /* start INCR mechanism by deleting property */
                XDeleteProperty(dpy, win, pty);
                XFlush(dpy);
                *context = XCLIB_XCOUT_INCR;
                return 0;
            }

            /* not using INCR mechanism, just read the property */
            XGetWindowProperty(dpy, win, pty, 0, (long)pty_size, False, AnyPropertyType, type, &pty_format, &pty_items,
                               &pty_size, &buffer);

            /* finished with property, delete it */
            XDeleteProperty(dpy, win, pty);

            /* compute the size of the data buffer we received */
            pty_machsize = pty_items * mach_itemsize(pty_format);

            /* copy the buffer to the pointer for returned data */
            if (pty_machsize > 0) {
                ltxt = (unsigned char *)xcmalloc(pty_machsize);
                memcpy(ltxt, buffer, pty_machsize);
            } else {
                ltxt = NULL;
            }

            /* set the length of the returned data */
            *len_p = pty_machsize;
            *txt_p = ltxt;

            /* free the buffer */
            if (buffer) XFree(buffer);

            *context = XCLIB_XCOUT_NONE;

            /* complete contents of selection fetched, return 1 */
            return 1;
        }
        case XCLIB_XCOUT_INCR: {
            /* To use the INCR method, we basically delete the
             * property with the selection in it, wait for an
             * event indicating that the property has been created,
             * then read it, delete it, etc.
             */

            /* make sure that the event is relevant */
            if (evt.type != PropertyNotify) return 0;

            /* skip unless the property has a new value */
            if (evt.xproperty.state != PropertyNewValue) return 0;

            /* check size and format of the property */
            XGetWindowProperty(dpy, win, pty, 0, 0, False, AnyPropertyType, type, &pty_format, &pty_items, &pty_size,
                               &buffer);

            if (pty_size == 0) {
                /* no more data, exit from loop */
                if (buffer) XFree(buffer);
                XDeleteProperty(dpy, win, pty);
                *context = XCLIB_XCOUT_NONE;

                /* this means that an INCR transfer is now
                 * complete, return 1
                 */
                return 1;
            }

            if (buffer) XFree(buffer);

            /* if we have come this far, the propery contains
             * text, we know the size.
             */
            XGetWindowProperty(dpy, win, pty, 0, (long)pty_size, False, AnyPropertyType, type, &pty_format, &pty_items,
                               &pty_size, &buffer);

            /* compute the size of the data buffer we received */
            pty_machsize = pty_items * mach_itemsize(pty_format);

            /* allocate memory to accommodate data in *txt */
            if (pty_machsize > 0) {
                if (*len_p == 0) {
                    *len_p = pty_machsize;
                    ltxt = (unsigned char *)xcmalloc(*len_p);
                } else {
                    *len_p += pty_machsize;
                    ltxt = (unsigned char *)xcrealloc(ltxt, *len_p);
                }

                /* add data to ltxt */
                memcpy(&ltxt[*len_p - pty_machsize], buffer, pty_machsize);
            }

            *txt_p = ltxt;
            if (buffer) XFree(buffer);

            /* delete property to get the next item */
            XDeleteProperty(dpy, win, pty);
            XFlush(dpy);
            return 0;
        }
        default:
            break;
    }

    return 0;
}

/* put data into a selection, in response to a SelecionRequest event from
 * another window (and any subsequent events relating to an INCR transfer).
 *
 * Arguments are:
 *
 * A display
 *
 * A window
 *
 * The event to respond to
 *
 * A pointer to an Atom. This gets set to the property nominated by the other
 * app in it's SelectionRequest. Things are likely to break if you change the
 * value of this yourself.
 *
 * The target(UTF8_STRING or XA_STRING) to respond to
 *
 * A pointer to an array of chars to read selection data from.
 *
 * The length of the array of chars.
 *
 * In the case of an INCR transfer, the position within the array of chars
 * that is being processed.
 *
 * The context that event is the be processed within.
 */
int xcin(Display *dpy, Window *win, XEvent evt, Atom *pty, Atom target, const unsigned char *txt, unsigned long len,
         unsigned long *pos, unsigned int *context) {
    unsigned long chunk_len;  // length of current chunk (for incr transfers only)
    XEvent res;               // response to event
    static Atom inc;
    static Atom targets;
    static long chunk_size;

    if (!targets) {
        targets = XInternAtom(dpy, "TARGETS", False);
    }

    if (!inc) {
        inc = XInternAtom(dpy, "INCR", False);
    }

    /* We consider selections larger than a quarter of the maximum
       request size to be "large". See ICCCM section 2.5 */
    if (!chunk_size) {
        chunk_size = XExtendedMaxRequestSize(dpy) / 4;
        if (!chunk_size) {
            chunk_size = XMaxRequestSize(dpy) / 4;
        }
    }

    switch (*context) {
        case XCLIB_XCIN_NONE: {
            if (evt.type != SelectionRequest) return 0;

            /* set the window and property that is being used */
            *win = evt.xselectionrequest.requestor;
            *pty = evt.xselectionrequest.property;

            /* reset position to 0 */
            *pos = 0;

            /* put the data into an property */
            if (evt.xselectionrequest.target == targets) {
                Atom types[2] = {targets, target};

                /* send data all at once (not using INCR) */
                XChangeProperty(dpy, *win, *pty, XA_ATOM, 32, PropModeReplace, (unsigned char *)types,
                                (int)(sizeof(types) / sizeof(Atom)));
            } else if ((long)len > chunk_size) {
                /* send INCR response */
                XChangeProperty(dpy, *win, *pty, inc, 32, PropModeReplace, 0, 0);

                /* With the INCR mechanism, we need to know
                 * when the requestor window changes (deletes)
                 * its properties
                 */
                XSelectInput(dpy, *win, PropertyChangeMask);

                *context = XCLIB_XCIN_INCR;
            } else {
                /* send data all at once (not using INCR) */
                XChangeProperty(dpy, *win, *pty, target, 8, PropModeReplace, txt, (int)len);
            }

            /* Perhaps FIXME: According to ICCCM section 2.5, we should
       confirm that XChangeProperty succeeded without any Alloc
       errors before replying with SelectionNotify. However, doing
       so would require an error handler which modifies a global
       variable, plus doing XSync after each XChangeProperty. */

            /* set values for the response event */
            res.xselection.property = *pty;
            res.xselection.type = SelectionNotify;
            res.xselection.display = evt.xselectionrequest.display;
            res.xselection.requestor = *win;
            res.xselection.selection = evt.xselectionrequest.selection;
            res.xselection.target = evt.xselectionrequest.target;
            res.xselection.time = evt.xselectionrequest.time;

            /* send the response event */
            XSendEvent(dpy, evt.xselectionrequest.requestor, 0, 0, &res);
            XFlush(dpy);

            /* don't treat TARGETS request as contents request */
            if (evt.xselectionrequest.target == targets) return 0;

            /* if len < chunk_size, then the data was sent all at
             * once and the transfer is now complete, return 1
             */
            if ((long)len > chunk_size)
                return 0;
            else
                return 1;
        }
        case XCLIB_XCIN_INCR: {
            /* length of current chunk */

            /* ignore non-property events */
            if (evt.type != PropertyNotify) return 0;

            /* ignore the event unless it's to report that the
             * property has been deleted
             */
            if (evt.xproperty.state != PropertyDelete) return 0;

            /* set the chunk length to the maximum size */
            chunk_len = (unsigned long)chunk_size;

            /* if a chunk length of maximum size would extend
             * beyond the end of txt, set the length to be the
             * remaining length of txt
             */
            if ((*pos + chunk_len) > len) chunk_len = len - *pos;

            /* if the start of the chunk is beyond the end of txt,
             * then we've already sent all the data, so set the
             * length to be zero
             */
            if (*pos > len) chunk_len = 0;

            if (chunk_len) {
                /* put the chunk into the property */
                XChangeProperty(dpy, *win, *pty, target, 8, PropModeReplace, &txt[*pos], (int)chunk_len);
            } else {
                /* make an empty property to show we've
                 * finished the transfer
                 */
                XChangeProperty(dpy, *win, *pty, target, 8, PropModeReplace, 0, 0);
            }
            XFlush(dpy);

            /* all data has been sent, break out of the loop */
            if (!chunk_len) *context = XCLIB_XCIN_NONE;

            *pos += (unsigned long)chunk_size;

            /* if chunk_len == 0, we just finished the transfer,
             * return 1
             */
            if (chunk_len > 0)
                return 0;
            else
                return 1;
        }
        default:
            break;
    }
    return 0;
}
