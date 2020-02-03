#include <WS2812FX.h>
#include <NeoPixelBus.h>

#define LED_PIN    5  // digital pin used to drive the LED strip, (for ESP8266 DMA, must use GPIO3/RX/D9)
#define LED_COUNT 1265  // number of LEDs on the strip

int TIMER_MS 60000

WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// create a NeoPixelBus instance
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(LED_COUNT, LED_PIN);

unsigned long last_change = 0;
unsigned long now = 0;

void BridgeLEDSetup() {
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
const uint8_t myModes[] = {3,7,8,11,12,17,18,32,33,36,38,39,42};

void updateTimerSeconds(int seconds){

    TIMER_MS = seconds * 1000;

}

void grabRandomEffectCheckTimeAndShowLights() {
  now = millis();

  ws2812fx.service();
  
  if(now - last_change > TIMER_MS) {
    int myModeCount = myModes [random(0,13)];
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
