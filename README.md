usbscale
========

**Usbscale** is a program that reads weight data from a USB scale. Compilation
is very simple and should work on any system with libusb-1.0 and gcc.

Installation
------------

You will need the development headers for **libusb-1.0**. Once you have that, you simply need to run `make` in the source directory.

Usage
-----

Simply run `usbscale` and it will automatically report on the first USB scale
that it finds.  The weight will be output to `stdout`, while any diagnostic or
error messages will be sent to `stderr`. An exit code of 0 means that a scale
was found and a weight was successfully read. Any other error code indicates
that a weight reading was unavailable.

Adding support for more scales
------------------------------

By default, **usbscale** only supports a very limited number of USB scales. It
does not search for just any USB scale, but only those listed in **scales.h**.
To add support for another USB scale, it should be enough to add its vendor and
product IDs to **scales.h**. In any case, you should contact me (see below) so
that I can add your new scale to the main source code to benefit all users who
may have the same scale.

License
-------

The license for **usbscale** is the GPLv3, whose full text can be found in the
file `COPYING`.  This means that you can reproduce and redistribute this
software, as long as you provide any and all modifications that you have made
to the software available under the same license. Notably, you may not
integrate this software into another product without making the whole product
open source. This paragraph is not definitive; please read the license
carefully.

Additional licensing terms may be negotiable.

Contact
-------

The best way to contact me is by sending mail to erjiang at indiana.edu.
