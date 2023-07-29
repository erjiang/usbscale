// usbscale
// =========
//
// C utility to read weight from a USB scale.
//
// Usage: usbscale
//
// There are no options. **usbscale** will try to read data from the first
// scale it finds when enumerating your USB devices.
//
/*
usbscale
Copyright (C) 2011--2023 Eric Jiang

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <argp.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

//
// This program uses libusb-1.0 (not the older libusb-0.1) for USB
// functionality.
//
#include <libusb-1.0/libusb.h>
//
// To enable a bunch of extra debugging data, simply define `#define DEBUG`
// here and recompile.
//
//     #define DEBUG
//
// The "scales.h" file contains a listing of all the Vendor and Product codes
// that this program will try to read. To try your scale, add your scale's
// vendor and product codes to scales.h and recompile.
//
#include "scales.h"

// Define the number of bytes long that each type of report is
#define WEIGH_REPORT_SIZE 6
#define CONTROL_REPORT_SIZE 2

// `le16toh` is not available on MacOS (as of 12.6) but we can replace it with
// a similar function.
#if defined(__APPLE__)
  #include <libkern/OSByteOrder.h>
  #define le16toh(x) OSSwapLittleToHostInt16(x)
#endif

//
// **find_scale** takes a libusb device list and finds the first USB device
// that matches a device listed in scales.h.
//
static libusb_device* find_nth_scale(libusb_device**, int);
//
// **print_scale_data** takes the 6-byte output from the scale and interprets
// it, printing out the result to the screen. It also returns a 1 if the
// program should read again (i.e. continue looping).
//
static int print_scale_data(unsigned char*);

//
// take device and fetch bEndpointAddress for the first endpoint
//
uint8_t get_first_endpoint_address(libusb_device* dev);

// **is_scale** checks if the given USB vendor and product IDs are a known scale
// (as defined in scales.h)
bool is_scale(uint16_t idVendor, uint16_t idProduct);

//
// **UNITS** is an array of all the unit abbreviations as set forth by *HID
// Point of Sale Usage Tables*, version 1.02, by the USB Implementers' Forum.
// The list is laid out so that the unit code returned by the scale is the
// index of its corresponding string.
//
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

// Setup argument parsing
const char *argp_program_version = "usbscale 0.2";
const char *argp_program_bug_address = "<https://www.github.com/erjiang/usbscale/issues>";
static char doc[] = "Read weight from a USB scale\n"
"The `tare' command will request the scale to reset to zero (not supported by all scales).\n";
static char args_doc[] = "[zero]";
static struct argp_option options[] = {
    { "index", 'i', "INDEX", 0, "Index of scale to read (default: 1)" },
    { 0 }
};

// setup argp to get index
struct arguments {
    int index;
    bool tare;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;
    switch (key) {
        case 'i':
            arguments->index = atoi(arg);
            // Make sure index is >0 since the first scale has index 1.
            if (arguments->index < 1) {
                argp_usage(state);
            }
            break;
        case ARGP_KEY_ARG:
            if (strcmp(arg, "zero") == 0) {
                arguments->tare = true;
            } else {
                argp_usage(state);
            }
            break;
        case ARGP_KEY_END:
            return 0;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

//
// main
// ----
//
int main(int argc, char **argv)
{

    // Parse arguments
    struct arguments arguments;
    // By default, get the first scale's weight
    arguments.index = 1;
    arguments.tare = false;
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    libusb_device **devs;
    int r; // holds return codes
    ssize_t cnt;
    libusb_device* dev;
    libusb_device_handle* handle;

    int weigh_count = WEIGH_COUNT - 1;

    //
    // We first try to init libusb.
    //
    r = libusb_init(NULL);
    //
    // If `libusb_init` errored, then we quit immediately.
    //
    if (r < 0)
        return r;

#ifdef DEBUG
        libusb_set_debug(NULL, 3);
#endif

    //
    // Next, we try to get a list of USB devices on this computer.
    cnt = libusb_get_device_list(NULL, &devs);
    if (cnt < 0)
        return (int) cnt;

    //
    // Once we have the list, we use **find_scale** to loop through and match
    // every device against the scales.h list. **find_scale** will return the
    // first device that matches, or 0 if none of them matched.
    //
    dev = find_nth_scale(devs, arguments.index);
    if(dev == 0) {
        if (arguments.index > 1)
            fprintf(stderr, "No scale with index %d found on this computer.\n", arguments.index);
        else
            fprintf(stderr, "No USB scale found on this computer.\n");
        return -1;
    }

    //
    // Once we have a pointer to the USB scale in question, we open it.
    //
    r = libusb_open(dev, &handle);
    //
    // Note that this requires that we have permission to access this device.
    // If you get the "permission denied" error, check your udev rules.
    //
    if(r < 0) {
        if(r == LIBUSB_ERROR_ACCESS) {
            fprintf(stderr, "Permission denied to scale.\n");
        }
        else if(r == LIBUSB_ERROR_NO_DEVICE) {
            fprintf(stderr, "Scale has been disconnected.\n");
        }
        return -1;
    }
    //
    // On Linux, we typically need to detach the kernel driver so that we can
    // handle this USB device. We are a userspace tool, after all.
    //
#ifdef __linux__
    libusb_detach_kernel_driver(handle, 0);
#endif
    //
    // Finally, we can claim the interface to this device and begin I/O.
    //
    libusb_claim_interface(handle, 0);



    /*
     * Try to transfer data about status
     *
     * http://rowsandcolumns.blogspot.com/2011/02/read-from-magtek-card-swipe-reader-in.html
     */
    unsigned char data[WEIGH_REPORT_SIZE];
    int len;
    len = 0;
    int scale_result = -1;

    // lowest bit is Enforced Zero Return, second bit is Zero Scale
    unsigned char tare_report[] = {0x02, 0x02};

    if (arguments.tare) {
        r = libusb_interrupt_transfer(
            handle,
            LIBUSB_ENDPOINT_OUT + 2, // direction=host to device, type=standard, recipient=device
            tare_report,
            CONTROL_REPORT_SIZE,
            &len,
            10000
            );

        if (r != 0) {
            fprintf(stderr, "errno=%s r=%d (%s) transferred %d bytes\n", strerror(errno), r, libusb_error_name(r), len);
        } else {
            fprintf(stderr, "tared\n");
        }
    }

    //
    // For some reason, we get old data the first time, so let's just get that
    // out of the way now. It can't hurt to grab another packet from the scale.
    //
    r = libusb_interrupt_transfer(
        handle,
        //bmRequestType => direction: in, type: class,
                //    recipient: interface
        LIBUSB_ENDPOINT_IN + 1,
        data,
        WEIGH_REPORT_SIZE, // length of data
        &len,
        10000 //timeout => 10 sec
        );
    //
    // We read data from the scale in an infinite loop, stopping when
    // **print_scale_data** tells us that it's successfully gotten the weight
    // from the scale, or if the scale or transmissions indicates an error.
    //
    for(;;) {
        //
        // A `libusb_interrupt_transfer` of 6 bytes from the scale is the
        // typical scale data packet, and the usage is laid out in *HID Point
        // of Sale Usage Tables*, version 1.02.
        //
        r = libusb_interrupt_transfer(
            handle,
            //bmRequestType => direction: in, type: class,
                    //    recipient: interface
            get_first_endpoint_address(dev),
            data,
            WEIGH_REPORT_SIZE, // length of data
            &len,
            10000 //timeout => 10 sec
            );
        //
        // If the data transfer succeeded, then we pass along the data we
        // received to **print_scale_data**.
        //
        if(r == 0) {
#ifdef DEBUG
            int i;
            for(i = 0; i < WEIGH_REPORT_SIZE; i++) {
                printf("%x\n", data[i]);
            }
#endif
            if (weigh_count < 1) {
                scale_result = print_scale_data(data);
                if(scale_result != 1)
                   break;
            }
            weigh_count--;
        }
        else {
            fprintf(stderr, "Error in USB transfer\n");
            scale_result = -1;
            break;
        }
    }
    
    // 
    // At the end, we make sure that we reattach the kernel driver that we
    // detached earlier, close the handle to the device, free the device list
    // that we retrieved, and exit libusb.
    //
#ifdef __linux__
    libusb_attach_kernel_driver(handle, 0);
#endif
    libusb_close(handle);
    libusb_free_device_list(devs, 1);
    libusb_exit(NULL);

    // 
    // The return code will be 0 for success or -1 for errors (see
    // `libusb_init` above if it's neither 0 nor -1).
    // 
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
static int print_scale_data(unsigned char* dat) {

    // 
    // We keep around `lastStatus` so that we're not constantly printing the
    // same status message while waiting for a weighing. If the status hasn't
    // changed from last time, **print_scale_data** prints nothing.
    // 
    static uint8_t lastStatus = 0;

    //
    // Gently rip apart the scale's data packet according to *HID Point of Sale
    // Usage Tables*.
    //
    uint8_t report = dat[0];
    uint8_t status = dat[1];
    uint8_t unit   = dat[2];
    // Accoring to the docs, scaling applied to the data as a base ten exponent
    int8_t  expt   = dat[3];
    // convert to machine order at all times
    double weight = (double) le16toh(dat[5] << 8 | dat[4]);
    // since the expt is signed, we do not need no trickery
    weight = weight * pow(10, expt);

    //
    // The scale's first byte, its "report", is always 3.
    //
    if(report != 0x03 && report != 0x04) {
        fprintf(stderr, "Error reading scale data\n");
        return -1;
    }

    //
    // Switch on the status byte given by the scale. Note that we make a
    // distinction between statuses that we simply wait on, and statuses that
    // cause us to stop (`return -1`).
    //
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
        //
        // 0x04 is the only final, successful status, and it indicates that we
        // have a finalized weight ready to print. Here is where we make use of
        // the `UNITS` lookup table for unit names.
        //
        case 0x04:
            printf("%g %s\n", weight, UNITS[unit]);
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

//
// find_scale
// ----------
//
// **find_scale** takes a `libusb_device\*\*` list and loop through it,
// matching each device's vendor and product IDs to the scales.h list. It
// return the first matching `libusb_device\*` or 0 if no matching device is
// found.
//
static libusb_device* find_nth_scale(libusb_device **devs, int index)
{

    int i = 0;
    // **curr_index** counts the index of each scale, in order to find the nth
    // scale as specified by **index**. Counting is 1-based, so the first scale
    // has index 1.
    int curr_index = 0;
    libusb_device* dev = 0;
    // Since each device shows up multiple times in **devs**, skip the device if
    // the address is the same as the previous entry.
    uint16_t last_device_address = 0;

    //
    // Loop through each USB device, and for each device, loop through the
    // scales list to see if it's one of our listed scales.
    //
    while ((dev = devs[i++]) != NULL) {

        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            fprintf(stderr, "failed to get device descriptor");
            return NULL;
        }
        int i;
        for (i = 0; i < NSCALES; i++) {
            if (is_scale(desc.idVendor, desc.idProduct)) {

                // Skip this device if it's the same as the last one.
                uint16_t this_device_address = (libusb_get_bus_number(dev) << 8) + libusb_get_device_address(dev);
                if(this_device_address == last_device_address) {
                    continue;
                }
                last_device_address = this_device_address;
#ifdef DEBUG
                /*
                 * Debugging data about found scale
                 */
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
                curr_index++;
                if (curr_index == index) {
                    return dev;
                }
            }
        }
    }
    return NULL;
}

// **is_scale** loops through the scales list to see if the given vendor and
// product ID match any of the known USB scales.
bool is_scale(uint16_t idVendor, uint16_t idProduct) {
    int i;
    for (i = 0; i < NSCALES; i++) {
        if(idVendor  == scales[i][0] && idProduct == scales[i][1]) {
            return true;
        }
    }
    return false;
}

uint8_t get_first_endpoint_address(libusb_device* dev)
{
    // default value
    uint8_t endpoint_address = LIBUSB_ENDPOINT_IN | LIBUSB_RECIPIENT_INTERFACE; //| LIBUSB_RECIPIENT_ENDPOINT;

    struct libusb_config_descriptor *config;
    int r = libusb_get_config_descriptor(dev, 0, &config);
    if (r == 0) {
        // assuming we have only one endpoint
        endpoint_address = config->interface[0].altsetting[0].endpoint[0].bEndpointAddress;
        libusb_free_config_descriptor(config);
    }

    #ifdef DEBUG
    printf("bEndpointAddress 0x%02x\n", endpoint_address);
    #endif

    return endpoint_address;
}
