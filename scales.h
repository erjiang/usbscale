//
// scales.h
// ========
//
// This file is a list of all currently-recognized scales' vendor and product
// IDs.
//
// For example, the USB product 1446:6173 becomes {0x1446, 0x6173}
//
#include <stdint.h>

//
// **scalesc** should be kept updated with the length of the list.
//
int scalesc = 2;

//
// Scales
// ------
//
uint16_t scales[2][2] = {\
    // Stamps.com 10-lb USB scale
    {0x1446, 0x6a73},
    // USPS (Elane) PS311 "XM Elane Elane UParcel 30lb"
    {0x7b7c, 0x0100}
};
