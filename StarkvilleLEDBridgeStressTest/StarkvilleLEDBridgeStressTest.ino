/*
  This sketch introduces the use of a custom show() function.
  It borrows the DMA class from the NeoPixelBus library to use the ESP8266's DMA
  channel to drive LED updates instead of the default Adafruit_NeoPixel "bit bang"
  method.
  
  Keith Lord - 2018

  LICENSE

  The MIT License (MIT)

  Copyright (c) 2018  Keith Lord 

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
  
  CHANGELOG
  2018-05-30 initial version
*/

#include <WS2812FX.h>
#include <NeoPixelBus.h>

#define LED_PIN    5  // digital pin used to drive the LED strip, (for ESP8266 DMA, must use GPIO3/RX/D9)
#define LED_COUNT 1265  // number of LEDs on the strip

#define TIMER_MS 60000

WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// create a NeoPixelBus instance
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(LED_COUNT, LED_PIN);

unsigned long last_change = 0;
unsigned long now = 0;

void setup() {
  Serial.begin(115200);

  ws2812fx.init();

  // MUST run strip.Begin() after ws2812fx.init(), so GPIO3 is initalized properly
  strip.Begin();
  strip.Show();

  // set the custom show function
  ws2812fx.setCustomShow(myCustomShow);
  ws2812fx.setBrightness(255);
  ws2812fx.setMode(FX_MODE_STATIC);
  ws2812fx.setSpeed(5000);  //smaller numbers are faster
  ws2812fx.start();
  ws2812fx.setOptions(0, 0x0);
}
// might be "bridge friendly" effects
const uint8_t myModes[] = {3,7,8,11,12,17,18,32,33,36,38,39,42,44,54,10,2,15,20};

void loop() {
  now = millis();

  ws2812fx.service();
  
  if(now - last_change > TIMER_MS) {
    int myModeCount = myModes [random(0,18)];
    ws2812fx.setMode(myModeCount);  
    last_change = now;
  }
}

void myCustomShow(void) {
  if(strip.CanShow()) {
    // copy the WS2812FX pixel data to the NeoPixelBus instance
    memcpy(strip.Pixels(), ws2812fx.getPixels(), strip.PixelsSize());
    strip.Dirty();
    strip.Show();
  }
}
