# sel, lsw, wmname - miscellaneous X11 utilities
.PHONY: .c.o all clean dist install options uninstall

include config.mk

# allow overriding linker; default to CC
LD ?= ${CC}

PROGS = sel lsw wmname swallow
BINDIR = bin
BIN = $(addprefix $(BINDIR)/,$(PROGS))
SRC = sel.c lsw.c wmname.c swallow.c
OBJ = ${SRC:.c=.o}

# default target
all: options $(BIN)

options:
	@echo "build options:"
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"
	@echo "LD       = ${LD}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

$(BINDIR):
	@mkdir -p $(BINDIR)

${OBJ}: config.mk

$(BINDIR)/sel: sel.o | $(BINDIR)
	@echo CC -o $@
	@${CC} -o $@ $< ${LDFLAGS}

$(BINDIR)/lsw: lsw.o | $(BINDIR)
	@echo CC -o $@
	@${CC} -o $@ $< ${LDFLAGS}

$(BINDIR)/wmname: wmname.o | $(BINDIR)
	@echo CC -o $@
	@${CC} -o $@ $< ${LDFLAGS}
	@strip $@

$(BINDIR)/swallow: swallow.o | $(BINDIR)
	@echo CC -o $@
	@${CC} -o $@ $< ${LDFLAGS}
	@strip $@

clean:
	@echo cleaning
	@rm -f $(BIN) $(PROGS) ${OBJ} *.tar.gz
	@rm -rf $(BINDIR)

dist: clean
	@echo creating dist tarball
	@mkdir -p utils-${VERSION}
	@cp -R LICENSE README.md config.mk Makefile *.c *.1 utils-${VERSION}
	@tar -cf utils-${VERSION}.tar utils-${VERSION}
	@gzip utils-${VERSION}.tar
	@rm -rf utils-${VERSION}

install: all
	@echo installing executables to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@for p in $(PROGS); do \
		cp -f $(BINDIR)/$$p ${DESTDIR}${PREFIX}/bin && chmod 755 ${DESTDIR}${PREFIX}/bin/$$p; \
	done
	@echo installing manual pages to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@for m in $(PROGS); do \
		sed "s/VERSION/${VERSION}/g" < $$m.1 > ${DESTDIR}${MANPREFIX}/man1/$$m.1 && chmod 644 ${DESTDIR}${MANPREFIX}/man1/$$m.1; \
	done

uninstall:
	@echo removing executables from ${DESTDIR}${PREFIX}/bin
	@for p in $(PROGS); do rm -f ${DESTDIR}${PREFIX}/bin/$$p; done
	@echo removing manual pages from ${DESTDIR}${MANPREFIX}/man1
	@for m in $(PROGS); do rm -f ${DESTDIR}${MANPREFIX}/man1/$$m.1; done
