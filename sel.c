/*
 * sel.c – the `sel` utility
 * -------------------------
 * Queries the X11 PRIMARY selection and prints it to standard output
 * as UTF-8 text. The implementation is intentionally minimal and keeps
 * the X11 interaction isolated so the program can be used easily in
 * scripts or combined with other utilities.
 *
 * Build & usage instructions can be found in the accompanying README.
 * For licensing details see the LICENSE file distributed with this
 * source code.
 */
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Size (in 8-bit units) of each chunk retrieved from the X server.
 * 4 KiB is a sensible default: small enough to avoid excessive memory
 * usage, yet large enough to minimise the number of round-trips when
 * fetching large selections. Tune this value if you have special
 * performance requirements.
 */
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

  /*
   * Ask the X server to convert the PRIMARY selection to UTF8_STRING
   * and store the result in the window property identified by
   * `prop_atom`.  The SelectionNotify event that follows lets us know
   * when the data is ready to be fetched with XGetWindowProperty.
   */
  XConvertSelection(dpy, XA_PRIMARY, utf8_string, prop_atom, win, CurrentTime);
  XFlush(dpy);

  /*
   * Block until we receive the SelectionNotify event.  XNextEvent is
   * used here instead of an async approach because sel is intended to
   * be a simple, synchronous filter that exits immediately after
   * writing the selection.
   */
  do {
    XNextEvent(dpy, &ev);
  } while (ev.type != SelectionNotify);

  if (ev.xselection.property == None) {
    /* Conversion failed – the owner either does not support
     * UTF8_STRING or declined the request. Return NULL so the caller
     * can handle the error gracefully.
     */
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

  /*
   * Open a connection to the default X display.  We do this once and
   * reuse the handle for the entire runtime to avoid the overhead of
   * reconnecting on each chunk.
   */
  dpy = XOpenDisplay(NULL);
  if (!dpy) {
    fprintf(stderr, "Cannot open display\n");
    exit(EXIT_FAILURE);
  }

  /*
   * Create an unmapped helper window.  It never becomes visible but is
   * necessary because the X11 selection mechanism requires a window to
   * own properties that temporarily hold the transferred data.
   */
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

  /*
   * Tidy up any X11 resources we created before exiting so that we
   * don't leak server-side objects.
   */
  XDestroyWindow(dpy, win);
  XCloseDisplay(dpy);
  return 0;
}
