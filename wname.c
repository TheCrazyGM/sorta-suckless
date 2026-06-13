#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef VERSION
#define VERSION "not-specified"
#endif

static void die(const char *msg) {
  fputs(msg, stderr);
  exit(EXIT_FAILURE);
}

static char *get_utf8_name(Display *dpy, Window win) {
  static char buf[BUFSIZ];
  Atom net_wm_name = XInternAtom(dpy, "_NET_WM_NAME", False);
  Atom utf8 = XInternAtom(dpy, "UTF8_STRING", False);
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;

  buf[0] = '\0';

  if (XGetWindowProperty(dpy, win, net_wm_name, 0L, (~0L), False, utf8,
                         &actual_type, &actual_format, &nitems, &bytes_after,
                         &prop) == Success &&
      prop) {
    if (actual_format == 8) {
      unsigned long len = (nitems < sizeof(buf) - 1) ? nitems : (sizeof(buf) - 1);
      memcpy(buf, prop, len);
      buf[len] = '\0';
    } else {
      buf[0] = '\0';
    }
    XFree(prop);
    return buf;
  }

  /* Fallback to legacy WM_NAME */
  XTextProperty prop_name;
  char **list = NULL;
  int count = 0;
  if (XGetWMName(dpy, win, &prop_name) && prop_name.nitems) {
    if (XmbTextPropertyToTextList(dpy, &prop_name, &list, &count) == Success &&
        list && count > 0) {
      strncpy(buf, list[0], sizeof(buf));
      buf[sizeof(buf) - 1] = '\0';
      XFreeStringList(list);
    } else if (prop_name.value) {
      unsigned long len = (prop_name.nitems < sizeof(buf) - 1) ? prop_name.nitems : (sizeof(buf) - 1);
      memcpy(buf, prop_name.value, len);
      buf[len] = '\0';
    }
    if (prop_name.value)
      XFree(prop_name.value);
  }
  return buf;
}

int main(int argc, char **argv) {
  if (argc > 1 && strncmp(argv[1], "-v", 3) == 0) {
    printf("wname-%s, © 2025 Michael Garcia\n", VERSION);
    return EXIT_SUCCESS;
  }

  if (argc < 2 || argc > 3)
    die("usage: wname <window-id> [new-title]\n");

  char *endp = NULL;
  unsigned long id = strtoul(argv[1], &endp, 0);
  if (!id || (endp && *endp))
    die("wname: invalid window id\n");

  Display *dpy = XOpenDisplay(NULL);
  if (!dpy)
    die("wname: cannot open display\n");

  Window w = (Window)id;

  if (argc == 2) {
    /* Print current name */
    printf("%s\n", get_utf8_name(dpy, w));
  } else {
    /* Set new UTF8 name */
    Atom net_wm_name = XInternAtom(dpy, "_NET_WM_NAME", False);
    Atom utf8 = XInternAtom(dpy, "UTF8_STRING", False);
    const unsigned char *val = (const unsigned char *)argv[2];
    XChangeProperty(dpy, w, net_wm_name, utf8, 8, PropModeReplace, val,
                    (int)strlen((const char *)val));
    XSync(dpy, False);
  }

  XCloseDisplay(dpy);
  return EXIT_SUCCESS;
}
