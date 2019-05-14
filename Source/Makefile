#
# Makefile for syasokoban
# Copyright (c) David Joffe, 2001-2003
#


CPP=gcc
BIN=syasokoban
CFLAGS= -ansi -Wall -I/usr/local/include $(shell sdl-config --cflags)

OBJFILES=src/djFont.o src/Games.o src/GameRegistry.o src/djUtils.o src/Sokoban.o src/Game.o src/djMenu.o src/Main.o src/djButton.o src/djItem.o src/djDesktop.o src/djGraph.o src/djSettings.o

LIBS= -lstdc++ $(shell sdl-config --libs) -lm


default: syasokoban

syasokoban: $(OBJFILES)
	$(CPP) -o $(BIN) $(OBJFILES) $(LIBS)

clean:
	rm -f $(BIN) $(OBJFILES) *~ core #*

install:
	cp $(BIN) /usr/local/bin

%.o: %.cpp
	$(CPP) $(CFLAGS) -c $< -o $@

