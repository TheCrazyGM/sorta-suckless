#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <stdint.h>
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

static int get_opacity(Display *dpy, Window w, uint32_t *out) {
  Atom opacity = XInternAtom(dpy, "_NET_WM_WINDOW_OPACITY", False);
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;

  if (XGetWindowProperty(dpy, w, opacity, 0, 1, False, XA_CARDINAL,
                         &actual_type, &actual_format, &nitems, &bytes_after,
                         &prop) != Success ||
      !prop || nitems < 1) {
    if (prop)
      XFree(prop);
    return 0;
  }

  /* Property is 32-bit cardinal */
  uint32_t val = *(uint32_t *)prop;
  XFree(prop);
  *out = val;
  return 1;
}

static void set_opacity(Display *dpy, Window w, uint32_t val) {
  Atom opacity = XInternAtom(dpy, "_NET_WM_WINDOW_OPACITY", False);
  XChangeProperty(dpy, w, opacity, XA_CARDINAL, 32, PropModeReplace,
                  (unsigned char *)&val, 1);
  XSync(dpy, False);
}

int main(int argc, char **argv) {
  if (argc > 1 && strncmp(argv[1], "-v", 3) == 0) {
    printf("opacity-%s, © 2025 Michael Garcia\n", VERSION);
    return EXIT_SUCCESS;
  }

  if (argc < 2 || argc > 3)
    die("usage: opacity <window-id> [0..1]\n");

  char *endp = NULL;
  unsigned long id = strtoul(argv[1], &endp, 0);
  if (!id || (endp && *endp))
    die("opacity: invalid window id\n");

  Display *dpy = XOpenDisplay(NULL);
  if (!dpy)
    die("opacity: cannot open display\n");

  Window w = (Window)id;

  if (argc == 2) {
    uint32_t raw;
    if (get_opacity(dpy, w, &raw)) {
      double f = (double)raw / 4294967295.0; /* 0xFFFFFFFF */
      printf("%f (0x%08x)\n", f, raw);
    } else {
      /* No opacity set usually means fully opaque */
      printf("1.000000 (unset)\n");
    }
  } else {
    char *endp = NULL;
    double f = strtod(argv[2], &endp);
    if (endp == argv[2] || (*endp != '\0' && *endp != '\n')) {
      die("opacity: invalid opacity value\n");
    }
    if (f < 0.0)
      f = 0.0;
    if (f > 1.0)
      f = 1.0;
    uint32_t raw = (uint32_t)(f * 4294967295.0);
    set_opacity(dpy, w, raw);
  }

  XCloseDisplay(dpy);
  return EXIT_SUCCESS;
}
