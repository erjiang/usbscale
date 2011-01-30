#include <stdio.h>
#include <sys/types.h>

#include <libusb-1.0/libusb.h>
#include "scales.h"

#define DEBUG 1

static void print_devs(libusb_device **devs)
{
	
	if(DEBUG)
		libusb_set_debug(null, 3);

	libusb_device *dev;
	int i = 0;

	while ((dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		int r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			fprintf(stderr, "failed to get device descriptor");
			return;
		}
        int i;
        for (i = 0; i < scalesc; i++) {
            if(desc.idVendor  == scales[i][0] && 
               desc.idProduct == scales[i][1]) {
				if(DEBUG) {
					printf("Found scale %04x:%04x (bus %d, device %d)\n",
                    desc.idVendor, desc.idProduct,
                    libusb_get_bus_number(dev), libusb_get_device_address(dev));

					printf("It has descriptors:\n\tmanufc: %d\n\tprodct: %d\n\tserial: %d\n",
							desc.iManufacturer,
							desc.iProduct,
							desc.iSerialNumber);
				}
                unsigned char string[256];

                libusb_device_handle* hand;

                libusb_open(dev, &hand);

                r = libusb_get_string_descriptor_ascii(hand, desc.iManufacturer,
                        string, 256);
                printf("Manufacturer: %s\n", string);
                break;
            }
        }
	}
}

int main(void)
{
	libusb_device **devs;
	int r;
	ssize_t cnt;

	r = libusb_init(NULL);
	if (r < 0)
		return r;

	cnt = libusb_get_device_list(NULL, &devs);
	if (cnt < 0)
		return (int) cnt;

	print_devs(devs);
	libusb_free_device_list(devs, 1);

	libusb_exit(NULL);
	return 0;
}

