.POSIX:

VERSION = 1.0
TARGET = rd
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

CFLAGS += -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE

SRC != find src -name "*.c"
OBJS = $(SRC:.c=.o)
INCLUDE = include

.c.o:
	$(CC) -o $@ $(CFLAGS) -I$(INCLUDE) -c $<

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS)

dist:
	mkdir -p $(TARGET)-$(VERSION)
	cp -R README.md $(TARGET) $(TARGET)-$(VERSION)
	tar -czf $(TARGET)-$(VERSION).tar.gz $(TARGET)-$(VERSION)
	$(RM) -r $(TARGET)-$(VERSION)

install: $(TARGET)
	mkdir -p $(DESTDIR)$(BINDIR)
	cp -p $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	chmod 755 $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	$(RM) $(DESTDIR)$(BINDIR)/$(TARGET)

clean:
	$(RM) $(TARGET) *.o

all: $(TARGET)

.PHONY: all dist install uninstall clean
