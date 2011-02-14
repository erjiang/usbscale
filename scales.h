/*
 * This file is a list of all currently-recognized scales,
 * by their vendor and product IDs.
 *
 * For example, the USB product 1446:6173 becomes {0x1446, 0x6173}
 */
#include <stdint.h>

// array length
int scalesc = 1;

uint16_t scales[1][2] = {\
    {0x1446, 0x6a73}    // Stamps.com USB scale
};
