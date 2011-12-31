LDLIBS=-lm -lusb-1.0
CC=gcc
CFLAGS=-Os
DOCCO=docco

usbscale: usbscale.c scales.h

lsscale: lsscale.c scales.h

docs: usbscale.c
	docco usbscale.c

clean:
	rm -f lsscale
	rm -f usbscale
