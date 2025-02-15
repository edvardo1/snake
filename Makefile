CC=cc
PKGS=sdl3
LIBS=$(shell pkg-config --libs $(PKGS))
CFLAGS=-Wall -pedantic -std=c11 $(shell pkg-config --cflags $(PKGS))

snake: main.c
	$(CC) $(CFLAGS) $(LIBS) -o $@ $<
