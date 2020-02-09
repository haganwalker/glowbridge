#include <WS2812FX.h>
#include <NeoPixelBus.h>

#include <M5Stack.h>
#include <M5LoRa.h>

#define LED_PIN    5  // digital pin used to drive the LED strip, (for ESP8266 DMA, must use GPIO3/RX/D9)
#define LED_COUNT 1265  // number of LEDs on the strip

#define TIMER_MS 60000

WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// create a NeoPixelBus instance
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(LED_COUNT, LED_PIN);

unsigned long last_change = 0;
unsigned long now = 0;

/*Ultrasonic Street Variables*****************************************/

int sensitivityTreshhold = 0; //Used to update the "Slop" in the sensor readings

int EndOfStreet_UValue = 0; //End of ultrasonic threshold on street
int BeginningOfStreet_UValue=0; //Beginning of the ultrasonic threshold on street

int EndOfSideWalk_UValue=0; //End of ultrasonic threshold on sidewalk
int BeginningOfSideWalk_Uvalue=0; //Beginning of the ultrasonic threshold on sidewalk

int pedestrianFunctionality = 1;

int carFunctionality = 1;

int UltrasonicPinNumber = 36;

int ultrasonicFlag = 0;

/*********************************************************************/

void setup() {

  M5.begin();

  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(); // default set CS, reset, IRQ pin

  Serial.begin(115200);

  ws2812fx.init();

  // MUST run strip.Begin() after ws2812fx.init(), so GPIO3 is initalized properly
  strip.Begin();
  strip.Show();



  // set the custom show function
  ws2812fx.setCustomShow(myCustomShow);
  ws2812fx.setBrightness(0);
  ws2812fx.setMode(FX_MODE_STATIC);
  ws2812fx.setSpeed(5000);  //smaller numbers are faster
  ws2812fx.start();
  ws2812fx.setOptions(0, 0x0);

  M5.update();
  M5.Lcd.clear(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(5);
  M5.Lcd.println("Brightness Set To 0");
  
  delay(2000);
  
  pinMode(2, INPUT); //Light Sensor Pin
  pinMode(UltrasonicPinNumber, INPUT); //ultrasonic pin
}
// might be "bridge friendly" effects
const uint8_t myModes[] = {3,7,8,11,12,17,18,32,33,36,38,39,42,44,54,10,2,15};

void loop() {
  
    ws2812fx.service();
  
    M5.update();

    if(M5.BtnA.wasReleased()){ //If we want to ender ultrasonic debug mode
  
        delay(1000);
        UltrasonicDebugMode();
  
    }else{
    
        if(checkForDaytime()){ //if it is daytime according to the sensor

            ws2812fx.setBrightness(0);

        }else{
            now = millis();
            
            if(ultrasonicFlag == 0){                
                checkUltrasonicSensor(analogRead(UltrasonicPinNumber)); //constantly check the ultrasonic Variable to see if it has been tripped, if tripped, set flag to 1.
            }

            if(now - last_change > TIMER_MS) { //Have we passed the given time

                int packetSize = LoRa.parsePacket(); //Try and Parse for a packet from receiver

                if (packetSize) { //if the sender sent a packet (ultrasonic tripped on other end)

                    int myModeCount = myModes [random(0,17)];
                    ws2812fx.setBrightness(255);
                    ws2812fx.setOptions(0, REVERSE);
                    ws2812fx.setMode(myModeCount);  
                    last_change = now;
                    ultrasonicFlag = 0; //We dont care that this side ultrasonic tripped 

                }else{ //did not receive a packet so we'll check the ultrasonic Sensor Flag

                    if(ultrasonicFlag){ //flag value is tripped
                        
                        int myModeCount = myModes [random(0,17)];
                        ws2812fx.setBrightness(255);
                        ws2812fx.setMode(myModeCount);  
                        last_change = now;
                        ultrasonicFlag = 0;

                    }else{ //ultrasonic value is not tripped

                        ws2812fx.setBrightness(0);

                    }
                }
            }
        }
    }
}

void checkUltrasonicSensor(int ultrasonicReading){ //takes in reading from the ultrasonic sensor and uses it to create a pedestrian or car code

    int PedestrianCode = 111; //Code used to determine a pedestrian
    int CarCode = 222; //Code used to determine a car

    if(((ultrasonicReading > (BeginningOfSideWalk_Uvalue + sensitivityTreshhold)) || (ultrasonicReading <= (EndOfSideWalk_UValue+ sensitivityTreshhold))) || 
       ((ultrasonicReading > (BeginningOfSideWalk_Uvalue - sensitivityTreshhold)) || (ultrasonicReading <= (EndOfSideWalk_UValue - sensitivityTreshhold)))){ 
        
        if(pedestrianFunctionality){ //Is Pedestrian Functionality On? (Change at Top)
            ultrasonicFlag = 1;
        }
    
    }else if(((ultrasonicReading >= (BeginningOfStreet_UValue + sensitivityTreshhold)) || (ultrasonicReading <= (EndOfStreet_UValue + sensitivityTreshhold))) || 
            ((ultrasonicReading > (BeginningOfSideWalk_Uvalue - sensitivityTreshhold)) || (ultrasonicReading <= (EndOfSideWalk_UValue - sensitivityTreshhold)))){ 
        
        if(carFunctionality){ //Is Car Functionality On? (Change at Top)
           ultrasonicFlag = 1;
        }

    }else{ //Don't do anything because neither threshold has been broken
        
    }


}

void UltrasonicDebugMode(){

    delay(2000);

    while(1){
//        M5.update();
//        M5.Lcd.clear(BLACK);
//        M5.Lcd.setCursor(0, 0);
//        M5.Lcd.setTextSize(5);
//        M5.Lcd.println(analogRead(36));
        Serial.println(analogRead(36));
//        if(M5.BtnA.wasReleased()){
//            
//            M5.Lcd.clear(BLACK);
//            break;
//
//        }
    delay(10);
    }
}

int checkForDaytime(){

    if(digitalRead(2) == HIGH){
        return 0; //Nighttime
    }else{
        return 1; //DayTime
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
