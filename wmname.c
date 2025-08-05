#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef VERSION
#define VERSION "not-specified"
#endif

static void die(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
  Display *dpy;
  Window root;
  Atom net_supporting_wm_check, net_wm_name, utf8_string, actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;

  /* Command-line argument processing */
  if (argc > 2)
    die("usage: wmname [name] [-v]\n");
  else if (argc == 2 && strcmp(argv[1], "-v") == 0) {
    printf("wmname-%s, © 2025 Michael Garcia\n", VERSION);
    return EXIT_SUCCESS;
  }

  if (!(dpy = XOpenDisplay(NULL)))
    die("wmname: cannot open display\n");
  root = DefaultRootWindow(dpy);

  /* Intern atoms for properties we want to use */
  net_supporting_wm_check = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
  net_wm_name = XInternAtom(dpy, "_NET_WM_NAME", False);
  utf8_string = XInternAtom(dpy, "UTF8_STRING", False);

  if (argc == 1) {
    /* Retrieve the WM name */
    int status = XGetWindowProperty(dpy, root, net_wm_name, 0L, (~0L), False,
                                    utf8_string, &actual_type, &actual_format,
                                    &nitems, &bytes_after, &prop);
    if (status == Success && prop != NULL) {
      printf("%s\n", prop);
      XFree(prop);
    } else {
      XCloseDisplay(dpy);
      die("wmname: unable to get window name property\n");
    }
  } else {
    /* Set both the supporting WM check and WM name properties */
    XChangeProperty(dpy, root, net_supporting_wm_check, XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&root, 1);
    XChangeProperty(dpy, root, net_wm_name, utf8_string, 8, PropModeReplace,
                    (unsigned char *)argv[1], (int)strlen(argv[1]));
  }
  XSync(dpy, False);
  XCloseDisplay(dpy);
  return EXIT_SUCCESS;
}
