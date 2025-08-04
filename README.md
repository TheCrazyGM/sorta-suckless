# sel – Simple X11 Selection Printer

`sel` is a tiny utility that copies the current **PRIMARY** selection from the X11 server and prints it to `stdout` as UTF-8 text.  It is designed to be minimal, dependency-free, and script-friendly.

---

## Features

* Reads arbitrarily large selections in 4 KiB chunks (see `CHUNK_SIZE` in `sel.c`).
* UTF-8 aware – requests the `UTF8_STRING` target first and falls back gracefully.
* Zero configuration: just build the single C source file.
* MIT-licensed, ~100 lines of readable C.

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

## Files

* `sel.c` – main program
* `Makefile` – build rules (uses `config.mk` for local overrides)
* `LICENSE` – MIT license

---

## License

This project is released under the terms of the MIT license.  See the `LICENSE` file for the full text.

---

## Author

Michael Garcia <thecrazygm@gmail.com>

Contributions and pull requests are welcome.
