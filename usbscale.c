#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <math.h>

#include <libusb-1.0/libusb.h>
#include "scales.h"

//#define DEBUG
/*
 * These constants are from libhid/include/constants.h
 * and usage based on libhid/src/hid_exchange.c
 * also vvvoutput.txt
 */
#define WEIGH_REPORT_SIZE 0x06

static libusb_device* find_scale(libusb_device**);
static int print_scale_data(char*);
const char* UNITS[13] = {
    "units",        // unknown unit
    "mg",           // milligram
    "g",            // gram
    "kg",           // kilogram
    "cd",           // carat
    "taels",        // lian
    "gr",           // grain
    "dwt",          // pennyweight
    "tonnes",       // metric tons
    "tons",         // avoir ton
    "ozt",          // troy ounce
    "oz",           // ounce
    "lbs"           // pound
};

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
     * http://rowsandcolumns.blogspot.com/2011/02/read-from-magtek-card-swipe-reader-in.html
     */
    unsigned char data[WEIGH_REPORT_SIZE];
    unsigned int len;
    unsigned int res;
    int continue_reading = 0;
    int scale_result = -1;
    
    for(;;) {
        res= libusb_interrupt_transfer(
            handle,
            //bmRequestType => direction: in, type: class,
                    //    recipient: interface
            LIBUSB_ENDPOINT_IN | //LIBUSB_REQUEST_TYPE_CLASS |
                LIBUSB_RECIPIENT_INTERFACE,
            data,
            WEIGH_REPORT_SIZE, // length of data
            &len,
            10000 //timeout => 10 sec
            );

        if(res == 0) {
#ifdef DEBUG
            int i;
            for(i = 0; i < WEIGH_REPORT_SIZE; i++) {
                printf("%x\n", data[i]);
            }
#endif
            scale_result = print_scale_data(data);
            if(scale_result != 1)
                break;
        }
        else {
            fprintf(stderr, "Error in USB transfer\n");
            scale_result = -1;
            break;
        }
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

    return scale_result;
}

//
// print_scale_data
// ----------------
//
// **print_scale_data** takes the 6 bytes of binary data sent by the scale and
// interprets and prints it out.
//
// **Returns:** `0` if weight data was successfully read, `1` if the data
// indicates that more data needs to be read (i.e. keep looping), and `-1` if
// the scale data indicates that some error occurred and that the program
// should terminate.
//
static int print_scale_data(char* dat) {

    static uint8_t lastStatus = 0;

    uint8_t report = dat[0];
    uint8_t status = dat[1];
    uint8_t unit   = dat[2];
    uint8_t expt   = dat[3];
    long weight = dat[4] + (dat[5] << 8);

    if(expt != 255 && expt != 0) {
        weight = pow(weight, expt);
    }

    if(report != 0x03) {
        fprintf(stderr, "Error reading scale data\n");
        return -1;
    }

    switch(status) {
        case 0x01:
            fprintf(stderr, "Scale reports Fault\n");
            return -1;
        case 0x02:
            if(status != lastStatus)
                fprintf(stderr, "Scale is zero'd...\n");
            break;
        case 0x03:
            if(status != lastStatus)
                fprintf(stderr, "Weighing...\n");
            break;
        case 0x04:
            printf("%ld %s\n", weight, UNITS[unit]);
            return 0;
        case 0x05:
            if(status != lastStatus)
                fprintf(stderr, "Scale reports Under Zero\n");
            break;
        case 0x06:
            if(status != lastStatus)
                fprintf(stderr, "Scale reports Over Weight\n");
            break;
        case 0x07:
            if(status != lastStatus)
                fprintf(stderr, "Scale reports Calibration Needed\n");
            break;
        case 0x08:
            if(status != lastStatus)
                fprintf(stderr, "Scale reports Re-zeroing Needed!\n");
            break;
        default:
            if(status != lastStatus)
                fprintf(stderr, "Unknown status code: %d\n", status);
            return -1;
    }
    
    lastStatus = status;
    return 1;
}

static libusb_device* find_scale(libusb_device **devs)
{
#ifdef DEBUG
        libusb_set_debug(NULL, 3);
#endif

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

                fprintf(stderr,
                        "Found scale %04x:%04x (bus %d, device %d)\n",
                        desc.idVendor,
                        desc.idProduct,
                        libusb_get_bus_number(dev),
                        libusb_get_device_address(dev));

                    fprintf(stderr,
                            "It has descriptors:\n\tmanufc: %d\n\tprodct: %d\n\tserial: %d\n\tclass: %d\n\tsubclass: %d\n",
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
                    fprintf(stderr,
                            "Manufacturer: %s\n", string);
                    libusb_close(hand);
#endif
                    return dev;
                    
                break;
            }
        }
    }
}

