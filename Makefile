CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra
LDFLAGS ?= -lX11 -lXtst

.PHONY: all clean

all: dist/cliptyper

dist/cliptyper: src/cliptyper_linux.c
	mkdir -p dist
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)
	chmod +x $@

clean:
	rm -rf dist
