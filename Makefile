LDLIBS=-lm -lusb-1.0
CFLAGS=-Os -Wall
DOCCO=docco

usbscale: usbscale.c scales.h
	$(CC) $(CFLAGS) $< $(LDLIBS) -o $@

lsusb: lsusb.c scales.h

docs: usbscale.c
	$(DOCCO) usbscale.c

clean:
	rm -f lsscale
	rm -f usbscale
