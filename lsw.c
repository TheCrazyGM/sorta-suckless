#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Atom netwmname;
static Display *dpy;

/* getname() returns a window's name using _NET_WM_NAME when available,
 * falling back to XGetWMName if necessary. */
static char *getname(Window win) {
  static char buf[BUFSIZ];
  char **list = NULL;
  int list_count = 0;
  XTextProperty prop;

  /* First try _NET_WM_NAME */
  if (!XGetTextProperty(dpy, win, &prop, netwmname) || !prop.nitems) {
    /* Fall back to the legacy WM_NAME */
    if (!XGetWMName(dpy, win, &prop) || !prop.nitems)
      return "";
  }

  if (XmbTextPropertyToTextList(dpy, &prop, &list, &list_count) == Success &&
      list_count > 0 && list != NULL) {
    strncpy(buf, list[0], sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
    XFreeStringList(list);
  } else {
    strncpy(buf, (char *)prop.value, sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
  }
  XFree(prop.value);

  return buf;
}

/* lsw() lists the visible subwindows (children) of a window.
 * It uses XQueryTree to get the list of child windows and then prints
 * those which are viewable and not override-redirect.
 */
static void lsw(Window win) {
  unsigned int num_children;
  Window *child_windows;
  Window root_return;
  XWindowAttributes wa;

  if (!XQueryTree(dpy, win, &root_return, &root_return, &child_windows,
                  &num_children) ||
      num_children == 0)
    return;

  /* Loop backwards from the last child to the first child */
  for (unsigned int i = num_children; i-- > 0;) {
    if (XGetWindowAttributes(dpy, child_windows[i], &wa) &&
        !wa.override_redirect && wa.map_state == IsViewable) {
      printf("0x%07lx %s\n", child_windows[i], getname(child_windows[i]));
    }
  }
  XFree(child_windows);
}

int main(int argc, char *argv[]) {
  if ((dpy = XOpenDisplay(NULL)) == NULL) {
    fprintf(stderr, "%s: cannot open display\n", argv[0]);
    return EXIT_FAILURE;
  }
  netwmname = XInternAtom(dpy, "_NET_WM_NAME", False);

  if (argc < 2) {
    lsw(DefaultRootWindow(dpy));
  } else {
    for (int i = 1; i < argc; i++) {
      Window w = (Window)strtoul(argv[i], NULL, 0);
      lsw(w);
    }
  }

  XCloseDisplay(dpy);
  return EXIT_SUCCESS;
}
