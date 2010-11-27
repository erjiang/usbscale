LD=-lusb-1.0
CC=gcc
CFLAGS=-Os

lsusb: lsusb.c
	$(CC) lsusb.c -o lsusb $(CFLAGS) $(LD)
