CC=gcc
CFLAGS=-g -Wall -Wfatal-errors -std=c99 $(shell pkg-config --cflags --libs gtk+-3.0 appindicator3-0.1 libgtop-2.0)

all: indicator-netspeed

indicator-netspeed: indicator-netspeed.c
	$(CC) $< $(CFLAGS) -o $@

clean:
	rm -f *.o indicator-netspeed

install:
	install --mode=755 indicator-netspeed  /usr/bin/
	install indicator-netspeed.gschema.xml /usr/share/glib-2.0/schemas/
	glib-compile-schemas /usr/share/glib-2.0/schemas/

uninstall:
	rm /usr/bin/indicator-netspeed
	rm /usr/share/glib-2.0/schemas/indicator-netspeed.gschema.xml

