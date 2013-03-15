#!/usr/bin/make -f

CFLAGS 	+= -D__STDC_CONSTANT_MACROS $(shell pkg-config --cflags libavutil libavformat libavcodec libswscale)
LIBS 	+= $(shell pkg-config --libs libavutil libavformat libavcodec libswscale)

all:
	g++ $(CFLAGS) -Wall -g shifter.cc -o shifter $(LIBS)

clean:
	rm -f shifter

install: shifter
	mkdir -p $(DESTDIR)/usr/bin/
	cp shifter $(DESTDIR)/usr/bin/

uninstall:
	rm $(DESTDIR)/usr/bin/shifter
