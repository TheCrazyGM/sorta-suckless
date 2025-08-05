# X11 Utility Suite – `sel`, `lsw`, `wmname`

This repository contains three small, dependency-free X11 helpers written in C, inspired by the [suckless](https://suckless.org) suite:

- **`sel`** – prints the current **PRIMARY** selection to `stdout`.
- **`lsw`** – lists the mapped child windows of a given window (defaults to the root window) together with their XID and title.
- **`wmname`** – prints or sets the `_NET_WM_NAME` (EWMH) property of the root window.
- **`swallow`** – temporarily hides the focused window while running a command.

They share a common build system and install into the same `$PREFIX/bin` directory.

---

## Features

- Reads arbitrarily large selections in 4 KiB chunks (see `CHUNK_SIZE` in `sel.c`).
- UTF-8 aware – requests the `UTF8_STRING` target first and falls back gracefully.
- Zero configuration: just build the single C source file.
- MIT-licensed, ~100 lines of readable C.

---

## Building

You only need the Xlib development headers (package `libx11-dev` on most distros) and a C99-capable compiler.

```sh
# optional: adjust PREFIX, CFLAGS, … in config.mk
make            # build
sudo make install  # install to $PREFIX/bin (default: ~/.local)
```

To clean intermediate files:

```sh
make clean
```

---

## Usage

```sh
sel                # dump PRIMARY selection to stdout
sel -v             # print version and exit
```

Typical examples:

```sh
# Copy PRIMARY selection to the clipboard (requires xclip/xsel)
sel | xclip -selection clipboard

# Save selected text to a file
sel > selection.txt
```

`sel` writes a trailing newline if any data was read, making it easy to pipe into POSIX tools.

---

## Other Utilities

Below are quick examples of the additional tools:

```sh
lsw                    # list top-level windows on the root window
lsw 0x3600007          # list children of a specific XID

wmname                  # print current WM name
wmname LG3D             # work-around for old Java toolkits

swallow passmenu         # hide parent window while passmenu runs
```

---

## Files

- `sel.c`, `lsw.c`, `wmname.c`, `swallow.c` – source files for each utility
- `sel.1`, `lsw.1`, `wmname.1`, `swallow.1` – manual pages (installed to `$MANPREFIX/man1`)
- `Makefile`, `config.mk` – build rules and system-specific overrides
- `LICENSE` – MIT license

---

## License

This project is released under the terms of the MIT license. See the `LICENSE` file for the full text.

---

## Authors

Original code by Anselm R. Garbe.
Current maintenance and additional utilities by Michael Garcia <thecrazygm@gmail.com>.

---

Contributions and pull requests are welcome.
