#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef VERSION
#define VERSION "not-specified"
#endif

/* We request CLIPBOARD as UTF8_STRING and handle both normal and INCR modes. */
static int print_clipboard(Display *dpy, Window win) {
  Atom utf8 = XInternAtom(dpy, "UTF8_STRING", False);
  Atom prop = XInternAtom(dpy, "_SEL_STRING", False);
  Atom sel = XInternAtom(dpy, "CLIPBOARD", False);
  Atom incr = XInternAtom(dpy, "INCR", False);
  Atom type;
  int format;
  unsigned long nitems, bytes_after;
  unsigned char *data = NULL;
  XEvent ev;

  /* Receive PropertyNotify while using INCR */
  XSelectInput(dpy, win, PropertyChangeMask);

  XConvertSelection(dpy, sel, utf8, prop, win, CurrentTime);
  XFlush(dpy);

  /* Wait for SelectionNotify */
  do {
    XNextEvent(dpy, &ev);
  } while (ev.type != SelectionNotify);

  if (ev.xselection.property == None)
    return 0;

  /* Peek at the property */
  if (XGetWindowProperty(dpy, win, prop, 0, (~0L), False, AnyPropertyType,
                         &type, &format, &nitems, &bytes_after,
                         &data) != Success ||
      type == None) {
    if (data)
      XFree(data);
    return 0;
  }

  if (type == incr) {
    /* INCR protocol: data initially contains the estimated size (we ignore).
       Delete and then read chunks on PropertyNotify until zero-length. */
    if (data)
      XFree(data);
    XDeleteProperty(dpy, win, prop);

    for (;;) {
      XEvent pe;
      do {
        XNextEvent(dpy, &pe);
      } while (!(pe.type == PropertyNotify && pe.xproperty.window == win &&
                 pe.xproperty.atom == prop &&
                 pe.xproperty.state == PropertyNewValue));

      if (XGetWindowProperty(dpy, win, prop, 0, (~0L), True, AnyPropertyType,
                             &type, &format, &nitems, &bytes_after,
                             &data) != Success) {
        return 0;
      }
      if (type == None) {
        /* Property deleted between events; continue. */
        if (data)
          XFree(data);
        continue;
      }

      if (nitems == 0) {
        if (data)
          XFree(data);
        break; /* Done */
      }

      if (format == 8 && data) {
        fwrite(data, 1, nitems, stdout);
      }
      if (data)
        XFree(data);
    }
  } else {
    /* Normal (non-INCR) transfer */
    if (format == 8 && data)
      fwrite(data, 1, nitems, stdout);
    if (data)
      XFree(data);
    XDeleteProperty(dpy, win, prop);
  }

  return 1;
}

int main(int argc, char **argv) {
  if ((argc > 1) && !strncmp(argv[1], "-v", 3)) {
    printf("clip-%s, © 2025 Michael Garcia\n", VERSION);
    return EXIT_SUCCESS;
  }
  if ((argc > 1) && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help") ||
                     !strcmp(argv[1], "-help"))) {
    puts("usage: clip [-v]\n\nReads CLIPBOARD selection as UTF-8 to stdout.");
    return EXIT_SUCCESS;
  }

  Display *dpy = XOpenDisplay(NULL);
  if (!dpy) {
    fprintf(stderr, "Cannot open display\n");
    return EXIT_FAILURE;
  }

  Window win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 1, 1, 0,
                                   CopyFromParent, CopyFromParent);

  if (!print_clipboard(dpy, win)) {
    /* If CLIPBOARD is empty/unavailable, exit quietly with non-zero? */
    /* Keep consistent with sel.c behavior: non-fatal if empty. */
  }

  XDestroyWindow(dpy, win);
  XCloseDisplay(dpy);
  return EXIT_SUCCESS;
}
