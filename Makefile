# sel - simple print selection
.PHONY: .c.o all clean dist sel install options uninstall

include config.mk

SRC = sel.c
OBJ = ${SRC:.c=.o}

all: options sel

options:
	@echo "sel build options:"
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.mk

sel: ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f sel ${OBJ} sel-${VERSION}.tar.gz

dist: clean
	@echo creating dist tarball
	@mkdir -p sel-${VERSION}
	@cp -R LICENSE Makefile README config.mk ${SRC} sel-${VERSION}
	@tar -cf sel-${VERSION}.tar sel-${VERSION}
	@gzip sel-${VERSION}.tar
	@rm -rf sel-${VERSION}

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f sel ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/sel

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/sel
