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

#define LED_PIN   17  // digital pin used to drive the LED strip, (for ESP8266 DMA, must use GPIO3/RX/D9)
#define LED_COUNT 20  // number of LEDs on the strip

#define TIMER_MS 5000

WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// create a NeoPixelBus instance
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(LED_COUNT, LED_PIN);

unsigned long last_change = 0;
unsigned long now = 0;

void setup() {
  Serial.begin(115200);
//HERE WE INITIALIZE THE LEDS VIA THE WS2812FX LIBRARY.
  ws2812fx.init();
  strip.Begin();
  strip.Show();
  ws2812fx.setCustomShow(myCustomShow);
  ws2812fx.setBrightness(255);
  ws2812fx.setMode(FX_MODE_STATIC);
  ws2812fx.setSpeed(500);  //smaller numbers are faster
  ws2812fx.start();
  ws2812fx.setOptions(0, 0x0);
}

// might be "bridge friendly" effects
const uint8_t myModes[] = {
//    FX_MODE_STATIC                  ,
//    FX_MODE_BLINK                   ,
//    FX_MODE_BREATH                  ,
    FX_MODE_COLOR_WIPE              ,
//    FX_MODE_COLOR_WIPE_INV          ,
//    FX_MODE_COLOR_WIPE_REV          ,
//    FX_MODE_COLOR_WIPE_REV_INV      ,
//    FX_MODE_COLOR_WIPE_RANDOM       ,
    FX_MODE_RANDOM_COLOR            ,
    FX_MODE_SINGLE_DYNAMIC          ,
    FX_MODE_MULTI_DYNAMIC           ,
    FX_MODE_RAINBOW                 ,
    FX_MODE_RAINBOW_CYCLE           ,
//    FX_MODE_SCAN                    ,
//    FX_MODE_DUAL_SCAN               ,
//    FX_MODE_FADE                    ,
    FX_MODE_THEATER_CHASE           ,
    FX_MODE_THEATER_CHASE_RAINBOW   ,
    FX_MODE_RUNNING_LIGHTS          ,
//    FX_MODE_TWINKLE                 ,
//    FX_MODE_TWINKLE_RANDOM          ,
//    FX_MODE_TWINKLE_FADE            ,
//    FX_MODE_TWINKLE_FADE_RANDOM     ,
//    FX_MODE_SPARKLE                 ,
//    FX_MODE_FLASH_SPARKLE           ,
//    FX_MODE_HYPER_SPARKLE           ,
//    FX_MODE_STROBE                  ,
//    FX_MODE_STROBE_RAINBOW          ,
//    FX_MODE_MULTI_STROBE            ,
//    FX_MODE_BLINK_RAINBOW           ,
//    FX_MODE_CHASE_WHITE             ,
    FX_MODE_CHASE_COLOR             ,
    FX_MODE_CHASE_RANDOM            ,
    FX_MODE_CHASE_RAINBOW           ,
//   FX_MODE_CHASE_FLASH             ,
//    FX_MODE_CHASE_FLASH_RANDOM      ,
//    FX_MODE_CHASE_RAINBOW_WHITE     ,
    FX_MODE_CHASE_BLACKOUT          ,
    FX_MODE_CHASE_BLACKOUT_RAINBOW  ,
//    FX_MODE_COLOR_SWEEP_RANDOM      ,
    FX_MODE_RUNNING_COLOR           ,
//    FX_MODE_RUNNING_RED_BLUE        ,
    FX_MODE_RUNNING_RANDOM          ,
    FX_MODE_LARSON_SCANNER          ,
//    FX_MODE_COMET                   ,
//    FX_MODE_FIREWORKS               ,
//    FX_MODE_FIREWORKS_RANDOM        ,
    FX_MODE_MERRY_CHRISTMAS         ,
//    FX_MODE_FIRE_FLICKER            ,
    FX_MODE_FIRE_FLICKER_SOFT       ,
//    FX_MODE_FIRE_FLICKER_INTENSE    ,
//    FX_MODE_CIRCUS_COMBUSTUS        ,
//    FX_MODE_HALLOWEEN               ,
    FX_MODE_BICOLOR_CHASE           ,
    FX_MODE_TRICOLOR_CHASE          ,
//    FX_MODE_ICU                     
};
const uint8_t myModeCount = (sizeof(myModes)/sizeof(myModes[0]));

// Let's do a small function to choose a random color
uint32_t color[] = {RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA, PURPLE, ORANGE, PINK};

void loop() {
  now = millis();

  ws2812fx.service();

  if(now - last_change > TIMER_MS) {
    ws2812fx.setColor(color[random(0,8)]);                                    // choose a random color
    delay(1000);
    ws2812fx.setBrightness(255);
    ws2812fx.setMode(random(myModeCount));
    last_change = now;
  }

  delay(10);
}

void myCustomShow(void) {
  if(strip.CanShow()) {
    // copy the WS2812FX pixel data to the NeoPixelBus instance
    memcpy(strip.Pixels(), ws2812fx.getPixels(), strip.PixelsSize());
    strip.Dirty();
    strip.Show();
  }
}
