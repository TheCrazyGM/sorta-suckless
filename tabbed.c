#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TAB_BAR_HEIGHT 26
#define MAX_TABS 32

#ifndef VERSION
#define VERSION "not-specified"
#endif

struct Tab {
  Window win;
  char name[256];
};

static struct Tab tabs[MAX_TABS];
static int ntabs = 0;
static int sel_tab = -1;

static Display *dpy;
static int screen;
static Window container;
static GC gc;
static Colormap cmap;
static XColor active_color, inactive_color, text_color, bg_color;
static XFontStruct *font;

static void die(const char *msg) {
  fputs(msg, stderr);
  exit(EXIT_FAILURE);
}

static void get_window_name(Window w, char *buf, size_t maxlen) {
  Atom net_wm_name = XInternAtom(dpy, "_NET_WM_NAME", False);
  Atom utf8 = XInternAtom(dpy, "UTF8_STRING", False);
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;

  buf[0] = '\0';

  if (XGetWindowProperty(dpy, w, net_wm_name, 0L, ~0L, False, utf8,
                         &actual_type, &actual_format, &nitems, &bytes_after,
                         &prop) == Success && prop) {
    if (actual_format == 8 && nitems > 0) {
      size_t len = (nitems < maxlen - 1) ? nitems : maxlen - 1;
      memcpy(buf, prop, len);
      buf[len] = '\0';
    }
    XFree(prop);
  }

  if (buf[0] == '\0') {
    XTextProperty prop_name;
    char **list = NULL;
    int count = 0;
    if (XGetWMName(dpy, w, &prop_name) && prop_name.nitems) {
      if (XmbTextPropertyToTextList(dpy, &prop_name, &list, &count) == Success &&
          list && count > 0) {
        strncpy(buf, list[0], maxlen);
        buf[maxlen - 1] = '\0';
        XFreeStringList(list);
      } else if (prop_name.value) {
        strncpy(buf, (const char *)prop_name.value, maxlen);
        buf[maxlen - 1] = '\0';
      }
      if (prop_name.value)
        XFree(prop_name.value);
    }
  }

  if (buf[0] == '\0') {
    snprintf(buf, maxlen, "Window 0x%lx", w);
  }
}

static void draw_tab_bar(int width) {
  /* Clear tab bar background */
  XSetForeground(dpy, gc, bg_color.pixel);
  XFillRectangle(dpy, container, gc, 0, 0, width, TAB_BAR_HEIGHT);

  if (ntabs == 0) return;

  int tab_width = width / ntabs;
  if (tab_width > 200) tab_width = 200;

  for (int i = 0; i < ntabs; i++) {
    /* Draw tab background */
    if (i == sel_tab) {
      XSetForeground(dpy, gc, active_color.pixel);
    } else {
      XSetForeground(dpy, gc, inactive_color.pixel);
    }
    XFillRectangle(dpy, container, gc, i * tab_width, 0, tab_width - 1, TAB_BAR_HEIGHT - 2);

    /* Draw border line */
    XSetForeground(dpy, gc, bg_color.pixel);
    XDrawLine(dpy, container, gc, (i + 1) * tab_width - 1, 0, (i + 1) * tab_width - 1, TAB_BAR_HEIGHT);

    /* Draw tab title */
    XSetForeground(dpy, gc, text_color.pixel);
    int text_y = (TAB_BAR_HEIGHT + font->ascent - font->descent) / 2;
    int text_width = XTextWidth(font, tabs[i].name, strlen(tabs[i].name));
    int text_x = i * tab_width + (tab_width - text_width) / 2;
    if (text_x < i * tab_width + 5) text_x = i * tab_width + 5;

    /* Truncate text if tab is too narrow */
    char draw_name[256];
    strncpy(draw_name, tabs[i].name, sizeof(draw_name));
    draw_name[sizeof(draw_name) - 1] = '\0';
    
    int draw_len = strlen(draw_name);
    while (draw_len > 0 && XTextWidth(font, draw_name, draw_len) > tab_width - 10) {
      draw_name[--draw_len] = '\0';
    }

    XDrawString(dpy, container, gc, text_x, text_y, draw_name, strlen(draw_name));
  }

  /* Draw bottom line of tab bar */
  XSetForeground(dpy, gc, active_color.pixel);
  XDrawLine(dpy, container, gc, 0, TAB_BAR_HEIGHT - 1, width, TAB_BAR_HEIGHT - 1);
  XFlush(dpy);
}

static void select_tab(int tab_idx, int width, int height) {
  if (tab_idx < 0 || tab_idx >= ntabs) return;

  sel_tab = tab_idx;

  /* Unmap all clients except the selected one */
  for (int i = 0; i < ntabs; i++) {
    if (i == sel_tab) {
      XMapWindow(dpy, tabs[i].win);
      XResizeWindow(dpy, tabs[i].win, width, height - TAB_BAR_HEIGHT);
      XSetInputFocus(dpy, tabs[i].win, RevertToParent, CurrentTime);
    } else {
      XUnmapWindow(dpy, tabs[i].win);
    }
  }

  draw_tab_bar(width);
}

static int (*xerrorxlib)(Display *, XErrorEvent *);

static int xerror(Display *d, XErrorEvent *ee) {
  if (ee->error_code == BadWindow ||
      (ee->request_code == 42 && ee->error_code == BadMatch) || /* X_SetInputFocus */
      (ee->request_code == 18 && ee->error_code == BadMatch) || /* X_ChangeWindowAttributes */
      (ee->request_code == 28 && ee->error_code == BadMatch)    /* X_GrabButton */) {
    return 0;
  }
  fprintf(stderr, "tabbed: error: request code=%d, error code=%d\n",
          ee->request_code, ee->error_code);
  return xerrorxlib(d, ee);
}

int main(int argc, char **argv) {
  if (argc > 1 && strcmp(argv[1], "-v") == 0) {
    printf("tabbed-%s, © 2026 Michael Garcia\n", VERSION);
    return EXIT_SUCCESS;
  }

  if (argc < 2) {
    die("usage: tabbed <window-id-1> [window-id-2] ...\n");
  }

  dpy = XOpenDisplay(NULL);
  if (!dpy) {
    die("tabbed: cannot open display\n");
  }

  xerrorxlib = XSetErrorHandler(xerror);

  screen = DefaultScreen(dpy);
  Window root = RootWindow(dpy, screen);

  /* Initialize colors */
  cmap = DefaultColormap(dpy, screen);
  XAllocNamedColor(dpy, cmap, "#2d2d2d", &bg_color, &bg_color);         /* Dark gray */
  XAllocNamedColor(dpy, cmap, "#007acc", &active_color, &active_color); /* Blue */
  XAllocNamedColor(dpy, cmap, "#3c3c3c", &inactive_color, &inactive_color); /* Gray */
  XAllocNamedColor(dpy, cmap, "#ffffff", &text_color, &text_color);     /* White */

  /* Load font */
  font = XLoadQueryFont(dpy, "fixed");
  if (!font) {
    font = XLoadQueryFont(dpy, "9x15");
  }
  if (!font) {
    die("tabbed: cannot load font\n");
  }

  unsigned int width = 800;
  unsigned int height = 600;

  container = XCreateSimpleWindow(dpy, root, 100, 100, width, height, 1,
                                  active_color.pixel, bg_color.pixel);

  XStoreName(dpy, container, "tabbed");

  /* Select events on container */
  XSelectInput(dpy, container, StructureNotifyMask | ExposureMask | KeyPressMask);

  /* Set graphics context */
  gc = XCreateGC(dpy, container, 0, NULL);
  XSetFont(dpy, gc, font->fid);

  /* Reparent client windows */
  for (int i = 1; i < argc && ntabs < MAX_TABS; i++) {
    char *endp = NULL;
    unsigned long id = strtoul(argv[i], &endp, 0);
    if (!id || (endp && *endp)) {
      fprintf(stderr, "tabbed: ignoring invalid window id '%s'\n", argv[i]);
      continue;
    }
    Window client = (Window)id;

    /* Get initial name */
    get_window_name(client, tabs[ntabs].name, sizeof(tabs[ntabs].name));
    tabs[ntabs].win = client;

    /* Select StructureNotify and PropertyChange on the client */
    XSelectInput(dpy, client, StructureNotifyMask | PropertyChangeMask);

    /* Reparent client to container */
    XReparentWindow(dpy, client, container, 0, TAB_BAR_HEIGHT);
    ntabs++;
  }

  if (ntabs == 0) {
    XDestroyWindow(dpy, container);
    XCloseDisplay(dpy);
    die("tabbed: no valid windows to manage\n");
  }

  XMapWindow(dpy, container);
  XSync(dpy, False);

  /* Select the first tab */
  select_tab(0, width, height);

  /* Event loop */
  XEvent ev;
  while (1) {
    XNextEvent(dpy, &ev);

    if (ev.type == Expose) {
      if (ev.xexpose.count == 0) {
        draw_tab_bar(width);
      }
    } else if (ev.type == ConfigureNotify && ev.xconfigure.window == container) {
      width = ev.xconfigure.width;
      height = ev.xconfigure.height;
      select_tab(sel_tab, width, height);
    } else if (ev.type == DestroyNotify) {
      /* One of the client windows was destroyed */
      int found_idx = -1;
      for (int i = 0; i < ntabs; i++) {
        if (tabs[i].win == ev.xdestroywindow.window) {
          found_idx = i;
          break;
        }
      }
      if (found_idx != -1) {
        /* Remove tab */
        for (int i = found_idx; i < ntabs - 1; i++) {
          tabs[i] = tabs[i + 1];
        }
        ntabs--;

        if (ntabs == 0) {
          /* No more windows to manage: exit */
          break;
        }

        /* Adjust selection index */
        if (sel_tab >= ntabs) {
          sel_tab = ntabs - 1;
        }
        select_tab(sel_tab, width, height);
      }
    } else if (ev.type == PropertyNotify) {
      /* Client window title changed */
      for (int i = 0; i < ntabs; i++) {
        if (tabs[i].win == ev.xproperty.window && 
            (ev.xproperty.atom == XA_WM_NAME || 
             ev.xproperty.atom == XInternAtom(dpy, "_NET_WM_NAME", False))) {
          get_window_name(tabs[i].win, tabs[i].name, sizeof(tabs[i].name));
          draw_tab_bar(width);
          break;
        }
      }
    } else if (ev.type == KeyPress) {
      KeySym ksym = XLookupKeysym(&ev.xkey, 0);
      /* Switch tabs using Alt + [1..9] */
      if (ev.xkey.state & Mod1Mask) {
        if (ksym >= XK_1 && ksym <= XK_9) {
          int tab_idx = ksym - XK_1;
          if (tab_idx < ntabs) {
            select_tab(tab_idx, width, height);
          }
        } else if (ksym == XK_Left) {
          int next_tab = (sel_tab - 1 + ntabs) % ntabs;
          select_tab(next_tab, width, height);
        } else if (ksym == XK_Right) {
          int next_tab = (sel_tab + 1) % ntabs;
          select_tab(next_tab, width, height);
        }
      }
    }
  }

  XFreeFont(dpy, font);
  XFreeGC(dpy, gc);
  XDestroyWindow(dpy, container);
  XCloseDisplay(dpy);

  return 0;
}
