# LEDscroller
Code for scrolling text or other graphics on an RGB LED array with a HUB75 connector

The Arduino code runs on a Teensy 3.2 on a SmartMatrix board.
To configure the image you want to use, save it in PPM format,
use ppm2inc.c to convert it from that to a C header file, and
include that from the arduino code.  There's plenty of flash
storage in a Teensy 3.2.

The ledmatrix.ps file prints out a template for mounting a
IFH6-32x32-16S-V1.0 LED matrix, along with cutouts for the
signal and power connectors.

Teensy 3.2:
[PJRC](http://pjrc.com/store/teensy32.html)

SmartMatrix:
[Pixelmatix](http://docs.pixelmatix.com/SmartMatrix/shieldref.html)
