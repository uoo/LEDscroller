 /*
  * displays animations stored in flash memory
  *
  * Wiring is on the default Teensy 3.1 SPI pins, and chip select can be on any GPIO,
  * set by defining SD_CS in the code below
  * Function     | Pin
  * DOUT         |  11
  * DIN          |  12
  * CLK          |  13
  * CS (default) |  15
  */

#include <math.h>
#include <stdlib.h>
#include <SPI.h>
#include <ADC.h>
#include <SmartMatrix3.h>
#include "lcgb3.h"

//#define   FRAMEDELAY  8
#define   NRGB        3

#define ASPEED  A3 // 17/SDA
#define ABRIGHT A2 // 16/SCL

static ADC *  adc = new ADC();

#define COLOR_DEPTH 24                  // known working: 24, 48 - If the sketch uses type `rgb24` directly, COLOR_DEPTH must be 24

static const uint8_t kMatrixWidth = 64;        // known working: 32, 64, 96, 128
static const uint8_t kMatrixHeight = 32;       // known working: 16, 32, 48, 64
static const uint8_t kRefreshDepth = 36;       // known working: 24, 36, 48
static const uint8_t kDmaBufferRows = 4;       // known working: 2-4, use 2 to save memory, more to keep from dropping frames and automatically lowering refresh rate
static const uint8_t kPanelType = SMARTMATRIX_HUB75_32ROW_MOD16SCAN;   // use SMARTMATRIX_HUB75_16ROW_MOD8SCAN for common 16x32 panels
static const uint8_t kMatrixOptions = (SMARTMATRIX_OPTIONS_NONE);      // see http://docs.pixelmatix.com/SmartMatrix for options
static const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);
static const uint8_t kScrollingLayerOptions = (SM_SCROLLING_OPTIONS_NONE);
static const uint8_t kIndexedLayerOptions = (SM_INDEXED_OPTIONS_NONE);

SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);
SMARTMATRIX_ALLOCATE_SCROLLING_LAYER(scrollingLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kScrollingLayerOptions);

static const int defaultBrightness = 100*(255/100);    // full brightness
//const int defaultBrightness = 15*(255/100);    // dim: 15% brightness
static const int defaultScrollOffset = 6;
static const rgb24 defaultBackgroundColor = {0x40, 0, 0};

// Teensy 3.0 has the LED on pin 13
static const int ledPin = LED_BUILTIN;

// Chip select for SD card on the SmartMatrix Shield
#define SD_CS 15

static int           pos;
static int           olddelay;
static boolean       ledstate;
static unsigned long next;

void
scrollText(
  const char *  msg)
{
  scrollingLayer.setColor({0xff, 0xff, 0xff});
  scrollingLayer.setMode(wrapForward);
  scrollingLayer.setSpeed(40);
  scrollingLayer.setFont(font6x10);
  scrollingLayer.start(msg, 1);
  Serial.println(msg);
}

void
setup()
{
  pinMode(ledPin, OUTPUT);  // does LED conflict with an analog pin?
  pinMode(ASPEED, INPUT);
  pinMode(ABRIGHT, INPUT);

  Serial.begin(115200);

  matrix.addLayer(&backgroundLayer);
  matrix.addLayer(&scrollingLayer); 
  matrix.begin();
  matrix.setBrightness(defaultBrightness);

  backgroundLayer.enableColorCorrection(true);

  backgroundLayer.fillScreen(defaultBackgroundColor);
  backgroundLayer.swapBuffers();

  scrollingLayer.setOffsetFromTop(defaultScrollOffset);

  pos      = 0;
  next     = 0;
  ledstate = false;

  adc->setReference(ADC_REF_3V3);
  adc->setAveraging(1);
  adc->setResolution(12);
  adc->setConversionSpeed(ADC_HIGH_SPEED);
  adc->setSamplingSpeed(ADC_HIGH_SPEED);

  adc->startContinuous(ASPEED, ADC_0);
  adc->startContinuous(ABRIGHT, ADC_1);

  //scrollText("finished initialization");
  //delay(3000);
  olddelay = 0;
}

void
loop()
{
    int                   x;
    int                   y;
    int                   brightVal;
    int                   speedVal;
    int                   delayAmt;
    unsigned long         now;
    const unsigned char * iptr;
    int off;
    //char posstr[6];

    now = millis();

    if (now > next) {
        speedVal  = adc->analogReadContinuous(ADC_0);
        brightVal = adc->analogReadContinuous(ADC_1);

        digitalWrite(ledPin, ledstate ? HIGH : LOW);
        ledstate = !ledstate;

        for (y = 0; y < kMatrixHeight; ++y) {
            off = y * xsize + pos;
            //itoa(off, posstr, 16);
            //scrollText(posstr);
            //delay(1000);
            iptr = data + off;
            for (x = 0; x < kMatrixWidth; ++x) {
                iptr = data + (off + x) * NRGB;
                rgb24 pixel = {iptr[0], iptr[1], iptr[2]};
                backgroundLayer.drawPixel(x, y, pixel);
            }
        }

        matrix.setBrightness(brightVal / 4);

        backgroundLayer.swapBuffers();
        ++pos;

        if (pos >= (xsize - kMatrixWidth)) {
          //scrollText("reached edge, recirculating");
          pos = 0;
        }

        delayAmt = speedVal / 0x100;

        if (delayAmt != olddelay) {
          Serial.println(delayAmt);
          olddelay = delayAmt;
        }

        next = now + delayAmt;
    }
}
