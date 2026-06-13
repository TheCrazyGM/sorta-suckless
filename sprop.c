#include <X11/Xatom.h>
#include <X11/Xlib.h>
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

int main(int argc, char **argv) {
  if (argc > 1 && strcmp(argv[1], "-v") == 0) {
    printf("sprop-%s, © 2026 Michael Garcia\n", VERSION);
    return EXIT_SUCCESS;
  }

  if (argc < 3 || argc > 4)
    die("usage: sprop <window-id> <property-name> [value]\n");

  char *endp = NULL;
  unsigned long id = strtoul(argv[1], &endp, 0);
  if (!id || (endp && *endp))
    die("sprop: invalid window id\n");

  Display *dpy = XOpenDisplay(NULL);
  if (!dpy)
    die("sprop: cannot open display\n");

  Window w = (Window)id;
  const char *prop_name = argv[2];
  Atom prop_atom = XInternAtom(dpy, prop_name, False);

  if (argc == 3) {
    /* Get and print property */
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop = NULL;

    if (XGetWindowProperty(dpy, w, prop_atom, 0L, ~0L, False, AnyPropertyType,
                           &actual_type, &actual_format, &nitems, &bytes_after,
                           &prop) == Success && prop) {
      if (actual_format == 8) {
        fwrite(prop, 1, nitems, stdout);
        putchar('\n');
      } else if (actual_format == 16) {
        unsigned short *s = (unsigned short *)prop;
        for (unsigned long i = 0; i < nitems; i++) {
          printf("%u%c", s[i], (i == nitems - 1) ? '\n' : ' ');
        }
      } else if (actual_format == 32) {
        unsigned long *l = (unsigned long *)prop;
        for (unsigned long i = 0; i < nitems; i++) {
          printf("0x%lx%c", l[i], (i == nitems - 1) ? '\n' : ' ');
        }
      }
      XFree(prop);
    } else {
      XCloseDisplay(dpy);
      die("sprop: property not found or unable to read\n");
    }
  } else {
    /* Set property (as UTF8_STRING, format 8) */
    Atom utf8 = XInternAtom(dpy, "UTF8_STRING", False);
    const unsigned char *val = (const unsigned char *)argv[3];
    XChangeProperty(dpy, w, prop_atom, utf8, 8, PropModeReplace, val,
                    (int)strlen((const char *)val));
    XSync(dpy, False);
  }

  XCloseDisplay(dpy);
  return EXIT_SUCCESS;
}
