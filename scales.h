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
// **NSCALES** should be kept updated with the length of the list.
//
#define NSCALES 9

//
// What is the number of the weighing result to show, as the first result may be incorrect (from the previous weighing)
//
#define WEIGH_COUNT 2

//
// Scales
// ------
//
uint16_t scales[NSCALES][2] = {\
    // Stamps.com Model 510 5LB Scale
    {0x1446, 0x6a73},
    // USPS (Elane) PS311 "XM Elane Elane UParcel 30lb"
    {0x7b7c, 0x0100},
    // Stamps.com Stainless Steel 5 lb. Digital Scale
    {0x2474, 0x0550},
    // Stamps.com Stainless Steel 35 lb. Digital Scale
    {0x2474, 0x3550},
    // Mettler Toledo
    {0x0eb8, 0xf000},
    // SANFORD Dymo 10 lb USB Postal Scale
    {0x6096, 0x0158},
    // Fairbanks Scales SCB-R9000
    {0x0b67, 0x555e},
    // Dymo-CoStar Corp. M25 Digital Postal Scale
    {0x0922, 0x8004},
    // DYMO 1772057 Digital Postal Scale
    {0x0922, 0x8003}
};
