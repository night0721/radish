.POSIX:
.SUFFIXES:

VERSION = 1.0
TARGET = rd
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

CFLAGS = -Os -march=native -mtune=native -pipe -s -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE

SRC != find src -name "*.c"

$(TARGET): $(SRC)
	$(CC) $(SRC) -Iinclude -o $@ $(CFLAGS)

dist:
	mkdir -p $(TARGET)-$(VERSION)
	cp -R README.md $(TARGET) $(TARGET)-$(VERSION)
	tar -cf $(TARGET)-$(VERSION).tar $(TARGET)-$(VERSION)
	gzip $(TARGET)-$(VERSION).tar
	rm -rf $(TARGET)-$(VERSION)

install: $(TARGET)
	mkdir -p $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(MANDIR)
	cp -p $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	chmod 755 $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	rm $(DESTDIR)$(BINDIR)/$(TARGET)

clean:
	rm $(TARGET)

all: $(TARGET)

.PHONY: all dist install uninstall clean
