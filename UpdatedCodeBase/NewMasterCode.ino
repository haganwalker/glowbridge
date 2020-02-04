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

int UltrasonicPinNumber = 36;

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
  ws2812fx.setBrightness(255);
  ws2812fx.setMode(FX_MODE_STATIC);
  ws2812fx.setSpeed(5000);  //smaller numbers are faster
  ws2812fx.start();
  ws2812fx.setOptions(0, 0x0);

  pinmode(2, INPUT); //Light Sensor Pin
  pinMode(UltrasonicPinNumber, INPUT); //ultrasonic pin
}
// might be "bridge friendly" effects
const uint8_t myModes[] = {3,7,8,11,12,17,18,32,33,36,38,39,42};

void loop() {
  
    ws2812fx.service();
  
    M5.update();
    if(M5.BtnA.wasReleased()){ //If we want to ender ultrasonic debug mode
  
        delay(1000);
        UltrasonicDebugMode();
  
    }else{
    
        if(checkForDaytime()){ //if it is daytime according to the sensor

            //TURN OFF LED's

        }else{
            now = millis();
            if(now - last_change > TIMER_MS) { //Have we passed the given time
                
                int packetSize = LoRa.parsePacket(); //Try and Parse for a packet from receiver

                if (packetSize) { //if the sender sent a packet (ultrasonic tripped)

                    int myModeCount = myModes [random(0,13)];
                    ws2812fx.setMode(myModeCount);  
                    last_change = now;

                }else{ //did not receive a packet so we'll check the ultrasonic Sensor

                    if(checkUltrasonicSensor(analogRead(UltrasonicPinNumber))){
                        
                        int myModeCount = myModes [random(0,13)];
                        ws2812fx.setMode(myModeCount);  
                        last_change = now;

                    }else{ //ultrasonic value is not tripped

                        //TURN OFF THE LIGHTS

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
            return(1);
        }
    
    }else if(((ultrasonicReading >= (BeginningOfStreet_UValue + sensitivityTreshhold)) || (ultrasonicReading <= (EndOfStreet_UValue + sensitivityTreshhold))) || 
            ((ultrasonicReading > (BeginningOfSideWalk_Uvalue - sensitivityTreshhold)) || (ultrasonicReading <= (EndOfSideWalk_UValue - sensitivityTreshhold)))){ 
        
        if(carFunctionality){ //Is Car Functionality On? (Change at Top)
            return(1);
        }

    }else{ //Don't do anything because neither threshold has been broken
        return 0;
    }


}

void UltrasonicDebugMode(){

    while(1){
        M5.update();
        M5.Lcd.clear(BLACK);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.setTextSize(5);
        M5.Lcd.println(analogRead(36));

        if(M5.BtnA.wasReleased()){
            
            M5.Lcd.clear(BLACK);
            break;

        }
    delay(100);
    }
}

void checkForDaytime(){

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


















void setup() {
  
  M5.begin();

  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(); // default set CS, reset, IRQ pin
  Serial.println("LoRa Receiver");
  M5.Lcd.println("LoRa Receiver");

  // frequency in Hz (433E6, 866E6, 915E6)
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    M5.Lcd.println("Starting LoRa failed!");
    while (1);
  }

  // LoRa.setSyncWord(0x69);
  Serial.println("LoRa init succeeded.");
  M5.Lcd.println("LoRa init succeeded.");
}

void loop() {
  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    Serial.print("Received packet: \"");
    M5.Lcd.print("Received packet: \"");

    // read packet
    while (LoRa.available()) {
      char ch = (char)LoRa.read();
      Serial.print(ch);
      M5.Lcd.print(ch);
    }

    // print RSSI of packet
    Serial.print("\" with RSSI ");
    Serial.println(LoRa.packetRssi());
    M5.Lcd.print("\" with RSSI ");
    M5.Lcd.println(LoRa.packetRssi());
  }
}