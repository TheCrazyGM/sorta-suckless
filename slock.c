#define _DEFAULT_SOURCE
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef VERSION
#define VERSION "not-specified"
#endif

static void die(const char *msg) {
  fputs(msg, stderr);
  exit(EXIT_FAILURE);
}

static char *get_password() {
  /* First try SLOCK_PASSWORD environment variable */
  char *env_pass = getenv("SLOCK_PASSWORD");
  if (env_pass && env_pass[0] != '\0') {
    return strdup(env_pass);
  }

  /* Fall back to ~/.slock_password */
  char *home = getenv("HOME");
  if (home) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/.slock_password", home);
    FILE *f = fopen(path, "r");
    if (f) {
      char buf[256];
      if (fgets(buf, sizeof(buf), f)) {
        /* Strip newline */
        buf[strcspn(buf, "\r\n")] = '\0';
        fclose(f);
        return strdup(buf);
      }
      fclose(f);
    }
  }

  return NULL;
}

int main(int argc, char **argv) {
  if (argc > 1 && strcmp(argv[1], "-v") == 0) {
    printf("slock-%s, © 2026 Michael Garcia\n", VERSION);
    return EXIT_SUCCESS;
  }

  char *password = get_password();
  if (!password) {
    die("slock: No password set. Please set the SLOCK_PASSWORD environment variable or write your password to ~/.slock_password.\n");
  }

  Display *dpy = XOpenDisplay(NULL);
  if (!dpy) {
    free(password);
    die("slock: cannot open display\n");
  }

  int screen = DefaultScreen(dpy);
  Window root = RootWindow(dpy, screen);
  unsigned int width = DisplayWidth(dpy, screen);
  unsigned int height = DisplayHeight(dpy, screen);

  /* Set window attributes for a fullscreen unmanaged window */
  XSetWindowAttributes wa;
  wa.override_redirect = True;
  wa.background_pixel = BlackPixel(dpy, screen);
  wa.event_mask = KeyPressMask;

  Window win = XCreateWindow(dpy, root, 0, 0, width, height, 0,
                              DefaultDepth(dpy, screen), CopyFromParent,
                              DefaultVisual(dpy, screen),
                              CWOverrideRedirect | CWBackPixel | CWEventMask, &wa);

  XMapRaised(dpy, win);

  /* Grab keyboard and mouse pointer */
  int grab_keyboard = GrabSuccess;
  int grab_pointer = GrabSuccess;
  
  /* Try to grab keyboard and pointer (loop in case other window manager has them grabbed briefly) */
  for (int i = 0; i < 1000; i++) {
    grab_keyboard = XGrabKeyboard(dpy, root, True, GrabModeAsync, GrabModeAsync, CurrentTime);
    if (grab_keyboard == GrabSuccess) break;
    usleep(1000);
  }

  for (int i = 0; i < 1000; i++) {
    grab_pointer = XGrabPointer(dpy, root, True,
                                 ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                                 GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
    if (grab_pointer == GrabSuccess) break;
    usleep(1000);
  }

  if (grab_keyboard != GrabSuccess || grab_pointer != GrabSuccess) {
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
    free(password);
    die("slock: unable to grab keyboard or pointer\n");
  }

  /* Define colors */
  Colormap cmap = DefaultColormap(dpy, screen);
  XColor black_color, red_color, blue_color;
  XAllocNamedColor(dpy, cmap, "black", &black_color, &black_color);
  XAllocNamedColor(dpy, cmap, "red", &red_color, &red_color);
  XAllocNamedColor(dpy, cmap, "blue", &blue_color, &blue_color);

  char input[256] = {0};
  int len = 0;

  XEvent ev;
  while (1) {
    XNextEvent(dpy, &ev);
    if (ev.type == KeyPress) {
      char buf[32];
      KeySym ksym;
      int num = XLookupString(&ev.xkey, buf, sizeof(buf), &ksym, NULL);

      if (ksym == XK_Return || ksym == XK_KP_Enter) {
        if (strcmp(input, password) == 0) {
          /* Correct password: exit */
          break;
        } else {
          /* Incorrect: flash red, reset input */
          XSetWindowBackground(dpy, win, red_color.pixel);
          XClearWindow(dpy, win);
          XFlush(dpy);
          sleep(1);
          XSetWindowBackground(dpy, win, black_color.pixel);
          XClearWindow(dpy, win);
          XFlush(dpy);
          memset(input, 0, sizeof(input));
          len = 0;
        }
      } else if (ksym == XK_Escape) {
        memset(input, 0, sizeof(input));
        len = 0;
        XSetWindowBackground(dpy, win, black_color.pixel);
        XClearWindow(dpy, win);
        XFlush(dpy);
      } else if (ksym == XK_BackSpace) {
        if (len > 0) {
          input[--len] = '\0';
          if (len == 0) {
            XSetWindowBackground(dpy, win, black_color.pixel);
          } else {
            XSetWindowBackground(dpy, win, blue_color.pixel);
          }
          XClearWindow(dpy, win);
          XFlush(dpy);
        }
      } else if (num > 0) {
        for (int i = 0; i < num; i++) {
          if (len < (int)sizeof(input) - 1 && !iscntrl((unsigned char)buf[i])) {
            input[len++] = buf[i];
            input[len] = '\0';
            XSetWindowBackground(dpy, win, blue_color.pixel);
            XClearWindow(dpy, win);
            XFlush(dpy);
          }
        }
      }
    }
  }

  /* Cleanup */
  XUngrabKeyboard(dpy, CurrentTime);
  XUngrabPointer(dpy, CurrentTime);
  XDestroyWindow(dpy, win);
  XCloseDisplay(dpy);
  free(password);

  return 0;
}
