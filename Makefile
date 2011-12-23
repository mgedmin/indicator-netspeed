CC=gcc
CFLAGS=-Wall $(shell pkg-config --cflags --libs gtk+-3.0 appindicator3-0.1 libgtop-2.0)

all: indicator-netspeed

indicator-netspeed: indicator-netspeed.c
	$(CC) $< $(CFLAGS) -o $@

clean:
	rm -f *.o indicator-netspeed
