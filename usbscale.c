#include <stdio.h>
#include <sys/types.h>

#include <libusb-1.0/libusb.h>
#include "scales.h"

#define DEBUG 1
/*
 * These constants are from libhid/include/constants.h
 * and usage based on libhid/src/hid_exchange.c
 * also vvvoutput.txt
 */
#define HID_REPORT_GET 0x01
#define HID_RT_INPUT 0x01
#define WEIGH_REPORT_ID 0x06 // Scale Data Report ID
#define WEIGH_REPORT_SIZE 0x07

static libusb_device* find_scale(libusb_device**);

int main(void)
{
    libusb_device **devs;
    int r;
    ssize_t cnt;
    libusb_device* dev;
    libusb_device_handle* handle;

    r = libusb_init(NULL);
    if (r < 0)
        return r;

    /*
     * Get a list of USB devices
     */
    cnt = libusb_get_device_list(NULL, &devs);
    if (cnt < 0)
        return (int) cnt;

    /*
     * Look for a scale amongst the USB devices
     */
    dev = find_scale(devs);
    
    /*
     * Open a handle to this found scale
     */
    libusb_open(dev, &handle);
#ifdef __linux__
    libusb_detach_kernel_driver(handle, 0);
#endif
    libusb_claim_interface(handle, 0);

    /*
     * Try to transfer data about status
     *
     * http://www.beyondlogic.org/usbnutshell/usb6.shtml 
     * test_libhid.c
     */
    unsigned char data[WEIGH_REPORT_SIZE];
    unsigned int len = libusb_control_transfer(
        handle,
        //bmRequestType => direction: in, type: class,
                //    recipient: interface
        LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS |
            LIBUSB_RECIPIENT_INTERFACE,
        //bRequest => HID_REPORT_GET
        // see HID1_11.pdf 60/97 (pg 51)
        HID_REPORT_GET,
        //wValue => hid report, no report ID
        0x0100,
        0x00,   //windex => interface 0
        data,
        WEIGH_REPORT_SIZE,    //wLength
        10000 //timeout => 1 sec
        );
    int i;
    printf("Got %d bytes from control transfer:\n", len);
    for(i = 0; i < WEIGH_REPORT_SIZE; i++) {
        printf("%x\n", data[i]);
    }
    

    /*
     * Free the device handle, device list and other cleanup
     * tasks
     */
#ifdef __linux__
    libusb_attach_kernel_driver(handle, 0);
#endif
    libusb_close(handle);
    libusb_free_device_list(devs, 1);
    libusb_exit(NULL);
    return 0;
}

static libusb_device* find_scale(libusb_device **devs)
{
    if(DEBUG)
        libusb_set_debug(NULL, 3);

    int i = 0;
    libusb_device* dev;

    /*
     * Loop through each usb device, and for each device, loop through the
     * scales list to see if it's one of our listed scales
     */
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
                /*
                 * Debugging data about found scale
                 */
#ifdef DEBUG

                printf("Found scale %04x:%04x (bus %d, device %d)\n",
                        desc.idVendor,
                        desc.idProduct,
                        libusb_get_bus_number(dev),
                        libusb_get_device_address(dev));

                    printf("It has descriptors:\n\tmanufc: %d\n\tprodct: %d\n\tserial: %d\n\tclass: %d\n\tsubclass: %d\n",
                            desc.iManufacturer,
                            desc.iProduct,
                            desc.iSerialNumber,
                            desc.bDeviceClass,
                            desc.bDeviceSubClass);

                    /*
                     * A char buffer to pull string descriptors in from the device
                     */
                    unsigned char string[256];
                    libusb_device_handle* hand;
                    libusb_open(dev, &hand);

                    r = libusb_get_string_descriptor_ascii(hand, desc.iManufacturer,
                            string, 256);
                    printf("Manufacturer: %s\n", string);
                    libusb_close(hand);
#endif
                    return dev;
                    
                break;
            }
        }
    }
}

