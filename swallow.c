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
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef VERSION
#define VERSION "not-specified"
#endif

/*
 * run_command() - Execute the provided command directly.
 *
 * This version uses fork() and execvp() to completely bypass the shell,
 * eliminating any possibility of command injection.
 */
static void run_command(char **argv) {
  if (argv[1] == NULL) {
    return;
  }

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  if (pid == 0) {
    /* Child process: execute command */
    execvp(argv[1], &argv[1]);
    perror("execvp");
    _exit(EXIT_FAILURE);
  } else {
    /* Parent process: wait for child to complete */
    int status;
    if (waitpid(pid, &status, 0) < 0) {
      perror("waitpid");
    }
  }
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
    XCloseDisplay(dis);
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
