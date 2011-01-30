LD=-lusb-1.0
CC=gcc
CFLAGS=-Os

usbscale: usbscale.c scales.h
	$(CC) usbscale.c -o usbscale $(CFLAGS) $(LD)

lsscale: lsscale.c scales.h
	$(CC) lsscale.c -o lsscale $(CFLAGS) $(LD)

lsusb: lsusb.c
	$(CC) lsusb.c -o lsusb $(CFLAGS) $(LD)
