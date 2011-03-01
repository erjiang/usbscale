LD=-lusb-1.0
CC=gcc
CFLAGS=-Os
LINKS=-lm
DOCCO=docco

usbscale: usbscale.c scales.h
	$(CC) usbscale.c -o usbscale $(LINKS) $(CFLAGS) $(LD)

lsscale: lsscale.c scales.h
	$(CC) lsscale.c -o lsscale $(CFLAGS) $(LD)

docs: usbscale.c
	docco usbscale.c

clean:
	rm -f lsscale
	rm -f usbscale
