/*
 * swallow.c – swallow the currently-focused X11 window while running a command
 * ---------------------------------------------------------------------------
 * If a window has the input focus, it is temporarily unmapped (hidden), the
 * specified command is executed, and the window is mapped again afterwards.
 * This is handy for launching transient programs (like password prompts)
 * without the parent window flashing underneath.
 *
 * Build & usage instructions live in README and swallow.1.
 */

#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNSAFE_CHARS "`\"'()[]&$;|<> 	"
#define CMD_BUF_SIZE 1024

#ifndef VERSION
#define VERSION "not-specified"
#endif

/*
 * run_command() - Build a command string from the argv parameters.
 *
 * This version escapes any character found in UNSAFE_CHARS and checks
 * for buffer overflow before appending each character.
 */
static void run_command(char **argv) {
  char *arg;
  char cmd[CMD_BUF_SIZE];
  size_t len = 0; /* Current length in the buffer */
  int i;

  cmd[0] = '\0';

  /* Skip the current program's name */
  argv++;

  /* Process each argument */
  while ((arg = *argv++)) {
    for (i = 0; arg[i] != '\0'; i++) {
      /* If the character is unsafe, add a backslash escape */
      if (strchr(UNSAFE_CHARS, arg[i])) {
        if (len + 1 >= CMD_BUF_SIZE) {
          fprintf(stderr, "Command buffer overflow in escape\n");
          exit(EXIT_FAILURE);
        }
        cmd[len++] = '\\';
      }
      if (len + 1 >= CMD_BUF_SIZE) {
        fprintf(stderr, "Command buffer overflow in copy\n");
        exit(EXIT_FAILURE);
      }
      cmd[len++] = arg[i];
    }
    /* Add an extra space between arguments */
    if (len + 1 >= CMD_BUF_SIZE) {
      fprintf(stderr, "Command buffer overflow in spacing\n");
      exit(EXIT_FAILURE);
    }
    cmd[len++] = ' ';
  }
  cmd[len] = '\0';

  /* Run the command using the system call */
  system(cmd);
}

int main(int argc, char **argv) {
  int revert;
  Window win;
  Display *dis = XOpenDisplay(NULL);

  if (dis == NULL) {
    fprintf(stderr, "Error: Unable to open X display.\n");
    return EXIT_FAILURE;
  }

  /* Version flag */
  if (argc > 1 && !strncmp(argv[1], "-v", 3)) {
    printf("swallow-%s, © 2025 Michael Garcia\n", VERSION);
    return EXIT_SUCCESS;
  }

  /* Get the current input focus */
  XGetInputFocus(dis, &win, &revert);

  /* If a window is in focus, unmap/hide it */
  if (win != None) {
    XUnmapWindow(dis, win);
    XFlush(dis);
  }

  /* Execute the provided command */
  run_command(argv);

  /* Restore (map) the previously hidden window */
  if (win != None) {
    XMapWindow(dis, win);
    XFlush(dis);
  }

  /* Clean up */
  XCloseDisplay(dis);

  return EXIT_SUCCESS;
}
