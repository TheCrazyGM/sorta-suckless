/* See LICENSE file for license details. */
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK_SIZE 4096L

/*
 * getsel_chunk()
 *
 * Requests a chunk of the primary selection (via UTF8_STRING conversion)
 * and returns a freshly allocated buffer of exactly 'len' bytes (if any).
 *
 * It accepts the display and window already opened in main so that we do
 * not repeatedly open/close the X connection. The selection request is made
 * and XNextEvent waits for the SelectionNotify.
 */
static unsigned char *getsel_chunk(Display *dpy, Window win,
                                   unsigned long offset, unsigned long *len,
                                   unsigned long *remain) {
  Atom utf8_string, prop_atom;
  Atom typeret;
  int format;
  unsigned char *data = NULL;
  unsigned char *result = NULL;
  XEvent ev;

  utf8_string = XInternAtom(dpy, "UTF8_STRING", False);
  prop_atom = XInternAtom(dpy, "_SEL_STRING", False);

  /* Request the selection */
  XConvertSelection(dpy, XA_PRIMARY, utf8_string, prop_atom, win, CurrentTime);
  XFlush(dpy);

  /* Wait for the SelectionNotify event */
  do {
    XNextEvent(dpy, &ev);
  } while (ev.type != SelectionNotify);

  if (ev.xselection.property == None) {
    /* The selection conversion failed */
    return NULL;
  }

  if (XGetWindowProperty(dpy, win, ev.xselection.property, offset, CHUNK_SIZE,
                         False, AnyPropertyType, &typeret, &format, len, remain,
                         &data) != Success) {
    fprintf(stderr, "XGetWindowProperty failed\n");
    return NULL;
  }

  if (*len && data) {
    result = malloc(*len);
    if (result) {
      memcpy(result, data, *len);
    } else {
      fprintf(stderr, "malloc failed\n");
    }
    XFree(data);
  }

  XDeleteProperty(dpy, win, ev.xselection.property);
  return result;
}

int main(int argc, char **argv) {
  unsigned char *data;
  unsigned long offset = 0, len = 0, remain = 0;
  Display *dpy;
  Window win;

  if ((argc > 1) && !strncmp(argv[1], "-v", 3)) {
    fputs("sel-" VERSION ", Copyright © 2025 Michael Garcia\n", stdout);
    exit(EXIT_SUCCESS);
  }

  /* Open display once */
  dpy = XOpenDisplay(NULL);
  if (!dpy) {
    fprintf(stderr, "Cannot open display\n");
    exit(EXIT_FAILURE);
  }

  /* Create a simple helper window */
  win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 10, 10, 200, 200, 1,
                            CopyFromParent, CopyFromParent);

  /*
   * Loop: the selection might be larger than CHUNK_SIZE.
   * We repeatedly request a chunk (starting at offset) until
   * there is no remaining data.
   */
  do {
    data = getsel_chunk(dpy, win, offset, &len, &remain);
    if (data) {
      fwrite(data, 1, len, stdout);
      free(data);
    }
    offset += len;
  } while (remain);

  if (offset) {
    putchar('\n');
  }

  /* Clean up */
  XDestroyWindow(dpy, win);
  XCloseDisplay(dpy);
  return 0;
}
