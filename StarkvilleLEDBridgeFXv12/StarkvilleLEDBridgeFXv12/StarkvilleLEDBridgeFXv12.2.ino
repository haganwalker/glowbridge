/* This version is for the MB7062 with code for PWM. It's a less than ideal
 * solution because PulseIn is a blocking function...
 * 
 * Welcome! Chances are, if you found this - it means I did something wrong.
 * Who knew that a light up bridge could be so complicated? On a surface level,
 * this code takes signals from a Maxbotix XL 7070 ultrasonic sensor either on
 * ESP32 master (the device this specific code is running on) or from the ESP32 
 * slave device, via LoRa. Once a significant change has been "seen", we cycle
 * through several stages, from fading into a random effect, then fading to 
 * solid white, and finally back to off. The code also has a provision to only
 * run from dusk to dawn, via a photocell. If the photocell does NOT detect
 * light, it turns ON a 3.3V circuit, which pulls pin 23 HIGH. Otherwise, pin
 * 23 is LOW, which sends a command to turn off the LEDs and ignores any 
 * sensor input. This code was written by Hagan Walker (haganwalker@gmail.com)
 * with support from users over at the WS2812FX github repository. A big thanks
 * to them, along with the creators and contributors of the NeoPixelBus library.
 * 
 * Now - here's the sloppy code for the Glo(R) Bridge.
 *
 *
 * Toby: I tried to change things to the better
 * But I do not have any ot the hardware so I did all of this in a text editor without even compiling or checking
 * I suspect it to fail in the fisrt place but it should give you an idea.
 * Let me know if it works or why not... :-)
 *
 */

#include "Arduino.h"
#include <M5Stack.h>
#include <M5LoRa.h>
#include <WS2812FX.h>
#include <NeoPixelBus.h>
#include <Smoothed.h>


/*Use This To Turn On/Off Pedestrian/Car detection (1 = ON | 0 = OFF)*/
int pedestrianFunctionality = 1;
int carFunctionality = 1;
/********************************/

/*Ultrasonic Street Variables*****************************************/

int sensitivityTreshhold = 0; //Used to update the "Slop" in the sensor readings

int EndOfStreet_UValue = 0; //End of ultrasonic threshold on street
int BeginningOfStreet_UValue=0; //Beginning of the ultrasonic threshold on street

int EndOfSideWalk_UValue=0; //End of ultrasonic threshold on sidewalk
int BeginningOfSideWalk_Uvalue=0; //Beginning of the ultrasonic threshold on sidewalk

/*********************************************************************/



unsigned int counter = 0;
unsigned int peopleSeen = 0;
float smoothed = 0;

Smoothed <int> ultrasonicSensor;

/********************NEOPIXEL VARIABLES******************************/

#define LED_COUNT 1260
#define LED_PIN 3  //DO NOT CHANGE THIS

/********************************************************************/

const int photoResistor = 23;
int nightTime = 0;

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
NeoEsp32I2s0800KbpsMethod dma = NeoEsp32I2s0800KbpsMethod(17, LED_COUNT, 3);

void setup() {

  M5.begin();

  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(); // default set CS, reset, IRQ pin
  Serial.println("LoRa Receiver");
  M5.Lcd.println("LoRa Receiver");

  // frequency in Hz (433E6, 866E6, 915E6)
  if (!LoRa.begin(433E6)) { //Try to Start the Lora Module
    Serial.println("Starting LoRa failed!");
    M5.Lcd.println("Starting LoRa failed!");
    while (1);
  }

  // LoRa.setSyncWord(0x69);
  Serial.println("LoRa init succeeded.");
  M5.Lcd.println("LoRa init succeeded.");

  Serial.begin(115200);
  Serial.println("Starting...");
  Serial.println(ESP.getFreeSketchSpace());

  pinMode(photoResistor, INPUT); //Initialize Photoresitor

/*******NEOPIXEL VARIABLES****************************************/
  ws2812fx.init();
  dma.Initialize();
  ws2812fx.setCustomShow(myCustomShow);
  ws2812fx.setBrightness(225);
  ws2812fx.setMode(FX_MODE_STATIC);
  ws2812fx.setSpeed(500);  //smaller numbers are faster
  ws2812fx.start();
  ws2812fx.setOptions(0, 0x0);
/****************************************************************/

  ultrasonicSensor.begin(SMOOTHED_AVERAGE, 10); //Star the Ultrasonic Sensor
  
  M5.Lcd.print("Wait 5 seconds..");
  delay(5000); //to allow power to come on before signal is sent.
  Serial.println("Setup Done...");
  M5.Lcd.print("Setup Done!");
  delay(3000);
}

// states we can have
typedef enum stages{
    OFF,                                                                        // default, everything is off
    FADE_IN,                                                                    // we fade into the random effect
    ANIM,                                                                       // we keep the animation for a deinfed time
    FADE_WHITE,                                                                 // we fade to white from that
    ON,                                                                         // we keep the lights on
    FADE_OUT                                                                    // and fade out again
};

// might be "bridge friendly" effects
const uint8_t myModes[] = {3,7,8,11,12,17,18,32,33,36,38,39,42};

/*
 * To change time of the events, edit the numbers below that AREN'T 255. This will change the timeline in MS.
 * To adjust max brightness, scroll down to the FADE_IN and FADE_WHITE stages and look for BRIGHTNESS comments.
 * Keep in mind, that if you change the max brightness from 255, the time scaling here will be slightly faster,
 * as the LED's will reach max brightness sooner, and move on to the next line of code. To compensate, you can
 * simply adjust the time in MS here to be longer.
 */
#define FADE_IN_TIMESTEP    (6000/255)                                          // replace the first number to adjust fade time in milliseconds.
                                                                                // no matter if we fade colors or brightness. 
                                                                                // There are 255 values to be done in the milliseconds given
#define FADE_WHITE_TIMESTEP (15000/255)  
#define FADE_OUT_TIMESTEP   (4000/255)
#define ANIM_TIME           (7500)                                              // how long to have the effect running
#define LIGHT_ON_TIME       (15000)                                              // how long to have white light on for (15 sec)


#define NIGHT_TIME_CHECK_INT (10000)   											// one minute is probably sufficient - daylight does usually not change quickly
																				// .... You might miss the one pointing a flashlight to the sensor....
																				// ... set it to a low value for DEBUG....
																				
#define ULTRASONIC_CHECK_INTERVAL (10)											// 100 ms checking makes 10 checks per second
																				// one would need to pass within 100 to 200 ms to get uncaught
																				// and apparently the sensor updates with 10Hz anyway...
																				
																				
																				
void loop() {
    
    // static ensures that they are not deleted when going out of "loop" and 
    // still keeps them local. Another option would be a global variable 
    // (as you did with PIR_SENSOR)
    static bool new_motion_detected = false;                                    // to detect motion on the sensor and to debounce and detect new motion
    static bool anim_running = false;                                           // whether in a stage with animation running or not
    static stages stage = OFF;                                                  // our little basic state machine
    static uint8_t fade_amount = 0;                                             // used for fading to white
    static uint32_t fade_step = 0;                                              // time base used for fades
    static uint32_t anim_end = 0;                                               // time for the animation stage to last / end
    static uint32_t on_end   = 0;                                               // end of the on time of the white on stage
	

    M5.update();
    if(M5.BtnA.wasReleased()){
  
    delay(1000);
    UltrasonicDebugMode();
    }
    
  	// there are probably simpler ways than using many uint32_t timing variables. 
  	// anyway, I hope memory does not become a problem on these devices
  	static uint32_t next_night_check = 0;										// checking if it is night time....
  	static uint32_t next_ultrasonic_read = 0;									// don't know if you can still read valuable things...

    uint32_t now = millis();                                                    // gets the current time at every call

    uint8_t brightness = ws2812fx.getBrightness();                              // get the current brightness - for fade in and fade out

    if(anim_running) ws2812fx.service();                                        // during animation we use the libraries service routine for the modes
    else ws2812fx.show();                                                       // for the fading we just use show to draw the pixels
    
	if(now > next_night_check)													// now we check quite less. Otherwise the code could also flip / bounce between on and off
	{																			
																				// Idea for improvement:
																				// another idea is to have a "counter" which increments on darkness and decrements when there is light
	//Turning This in to a function												// this could deboucne and the check intervall could be shorter... it would saturate at 0 (during the day)
																				// and a certain value (which is the used below to perform the tasks) like 40....
																				// if the increment is e.g. 2 and the decrement is e.g. 1 then you could also react quicker on darkness while
																				// it will be slower to got to OFF mode.
		next_night_check = now + NIGHT_TIME_CHECK_INT;
        checkForNight(); //This function changes the lightCounter variable. Decrements when light levels are low, increases when light levels are high
		nightTime = digitalRead(photoResistor);
	}
    int lightThreshold = 10; //Threshold lightCounter needs to be under to show that it is night
 /*  
  *  MAIN CODE: This starts the main code. First, we check if it is night time. If it is (state is HIGH or 1), 
  *  then we check the sensor. If the sensor is high, and the motion is new, we start the animation.
  */
  if(now > next_ultrasonic_read) {
	next_ultrasonic_read = now + ULTRASONIC_CHECK_INTERVAL;   // doing this here instead of the end provides a more stable interval...
    
	if(lightCounter < lightThreshold) {  //If the lightCounter is below the threshold needed to signal light, then we do led stuff.
      
      // If the motion is detected by the Master Device (Left Side of the Bridge)
      float currentSensorValue = pulseIn(38, HIGH) / 58.0;
      float inInches = currentSensorValue / 2.54;
      Serial.println(inInches);
      ultrasonicSensor.add(inInches);
      //smoothed = ultrasonicSensor.get();
      //Serial.println(smoothed);
	  
		  if(determineAndSendObject(ultrasonicSensor.get())){                     //Return True or false falue based on 
			if(new_motion_detected == false) {                                    // when this is a new motion
				new_motion_detected = true;                                       // we have motion detected
				int myModeCount = myModes [random(0,13)];
				ws2812fx.setMode(myModeCount);                                    // set a random mode from the ones above 
				ws2812fx.setBrightness(0);                                        // start at zero brigthness
				//Serial.println("Motion LEFT");  
				M5.Lcd.print("Motion LEFT");
				//Serial.print("mode is "); Serial.println(ws2812fx.getModeName(ws2812fx.getMode()));

				// now we switch to the first stage:
				stage = FADE_IN;
				fade_step = now + FADE_IN_TIMESTEP;
				anim_running = true;
			}
			else {                                                                 // We have motion, but not new motion. Keep going.
				on_end = now + LIGHT_ON_TIME;                                      // Sensor still triggered --> extend to further ... seconds
				//Serial.println("Retriggered LEFT");
				M5.Lcd.print("Retriggered LEFT");
				if(stage == FADE_OUT) {                                              // hurry, we were fading out already.... lets return to fade white
					stage = FADE_WHITE;
					fade_step = now + FADE_WHITE_TIMESTEP;
					fade_amount = 0;
				}
			 }
		  }


	  
	  // don't know how often LoRa sends.... in Europe in most commercial devices using the 868 MHz this is limited (by law) with a duty cycle of 1% per hour.
	  // so a device is allowed to send 36 seconds per hour.
	  
	  // so it may be worth thinking about limiting the transmissions from the slave....
	  
	  int packetSize = LoRa.parsePacket();

      // If the motion is detected by the Slave Device and sent by LoRa (Right Side of the Bridge)
      
      if (packetSize) {
        while (LoRa.available()){
          char ch = (char)LoRa.read();
          if(new_motion_detected == false) {                                    // when this is a new motion
              new_motion_detected = true;                                       // we have motion detected
              int myModeCount = myModes [random(0,13)];
              ws2812fx.setOptions(0, REVERSE);                                  // reverse the direction if coming from LoRa.
              ws2812fx.setMode(myModeCount);                                    // set a random mode, 
              ws2812fx.setBrightness(0);
              //Serial.println("Motion LoRa");  
              M5.Lcd.print("Motion LoRa ");
              M5.Lcd.println(ch);
              M5.Lcd.print("\" with RSSI ");
              M5.Lcd.println(LoRa.packetRssi());
              //Serial.print("mode is "); Serial.println(ws2812fx.getModeName(ws2812fx.getMode()));
  
              // now we switch to the first stage:
              stage = FADE_IN;
              fade_step = now + FADE_IN_TIMESTEP;
              anim_running = true;
            }
          }
        }
          else {                                                                 // We have motion, but not new motion. Keep going.
          
              on_end = now + LIGHT_ON_TIME;                                     // Sensor still triggered --> extend to further ... seconds
              //Serial.println("Retriggered, LoRa");
              M5.Lcd.print("Retrig, LoRa ");
              M5.Lcd.print("\" with RSSI ");
              M5.Lcd.println(LoRa.packetRssi());
              if(stage == FADE_OUT) {                                             // hurry, we were fading out already.... lets return to fade white
                  stage = FADE_WHITE;
                  fade_step = now + FADE_WHITE_TIMESTEP;
                  fade_amount = 0;
              }
           }
        }
    }

    // It's daytime outside. We're not going to do anything but say so on the OLED.
    // this is going to happen too often.... the if - else calls either the one or the other.
	// here you do nothing...
	
    else {
	  // I would not do anything...
	  // let the last animation run to the end when the last one crosed during dawn...
      // ws2812fx.setBrightness(0);
      // stage == OFF;
      //Serial.println("It's Daytime!");
        M5.Lcd.println("It's Daytime!");
        M5.Lcd.print("Light Counter:");
        M5.Lcd.println(lightCounter);
    }
  
/*  
 *  MAIN CODE: End. This is the end of the main loop. If it is daytime, the else statement above sends
 *  the stage to OFF and the LEDs stay off while it's light outside.
 */

 /*  
  *  SWITCHES: This is the start of the switches defined. They are OFF (no lights, if lights - turn them off),
  *  FADE_IN (fades into the animation), ANIM (Animation - shows a random animation from the ones we selected 
  *  to show further up in code), FADE_WHITE (Fade from the Animation into solid white), ON (stay in solid 
  *  white for a predetermined time), FADE_OUT (Fade from white to OFF), and DEFAULT (just to catch errors, 
  *  if needed - sends stage to OFF).
  */
    switch (stage) {
        case OFF :                                                                  // default stage, not much to do.
            //Serial.println("case is OFF");
            M5.Lcd.print("Case is OFF");
            if(brightness != 0)                                                     // sanity.... but in case its not 0, we set it to 0
            {
                ws2812fx.setBrightness(0);
            }
            new_motion_detected = false;                                            // if everything is off, we can surly wait for a new motion detection
        break;
        case FADE_IN :                                                              // fade into the random effect
            //Serial.println("case is FADE IN");
            if(now > fade_step)
            {
                fade_step = now + FADE_IN_TIMESTEP;                                 // next step
                if(brightness < 170)                                                // BRIGHTNESS: change the value here to change max brightness.
                {                                                                   // MAX IS 255. be sure to do this for WHITE case below too. 170 is 2/3rds
                    ws2812fx.setBrightness(brightness + 1);                         // as long as the brightness is not at max, increase it
                }
                else                                                                // once we reached maximum brightness, we switch to the next mode with initial time
                {
                    stage = ANIM;
                    anim_end = now + ANIM_TIME; // end time of the next stage
                }
            }
        break;
        case ANIM :                                                                 // keep the animation running for a while
            //Serial.println("Case is Animation");
            M5.Lcd.print("Animation");
            if(now > anim_end)
            {
                anim_running = false;                                               // stop the animation at this frame;
                stage = FADE_WHITE;                                                 // switch to fade white stage
                fade_step = now + FADE_WHITE_TIMESTEP;
                fade_amount = 0;                                                    // how "white" we are already - zero when starting
            }
        break;
        case FADE_WHITE :                                                           // fade to white stage
            //Serial.println("Case is Fade to White");
            M5.Lcd.print("Fade to White");
            if(now > fade_step)
            {
                fade_step = now + FADE_WHITE_TIMESTEP;                              // next fade step
                for(uint16_t i=0; i<LED_COUNT; i++)                                 // we fade the color of every LED
                {
                    uint32_t color = ws2812fx.getPixelColor(i);
                    color = ws2812fx.color_blend(color, WHITE, fade_amount);
                    ws2812fx.setPixelColor(i, color);       
                }
                if(fade_amount < 170) {                                             // BRIGHTNESS: change this value to change max brightness. MAX IS 255. 170 is 2/3rds.
                    fade_amount ++;                                                 // fade towards full wide
                } 
                else
                {
                    // at full white, switch to the on stage with
                    // the calculated on_end time
                    stage = ON;
                    on_end = now + LIGHT_ON_TIME;
                }
            }
        break;
        case ON :                                                                   // stage where plain white is shown for the defined time
            //Serial.println("Case is SOLID White");
            M5.Lcd.print("Solid White");
            if(now > on_end)                                                        // not much to do. At the end, switch to FADE OUT
            {
                stage = FADE_OUT;
                fade_step = now + FADE_OUT_TIMESTEP;
            }
        break;
        case FADE_OUT :  // fade from White to OFF
            //Serial.println("Case is Fade to OFF");
            M5.Lcd.print("Fade OFF");
            if(now > fade_step)
            {
                fade_step = now + FADE_OUT_TIMESTEP;                             // next step
                if(brightness > 0)
                {
                    ws2812fx.setBrightness(brightness - 1);                      // we dim, as long as > 0
                }
                else
                {
                    // when fading is done, we only need to switch to stage off
                    stage = OFF;
                }
            }    
        break;
        default :                                                               // every switch - case should have a default
            //Serial.println("We're in the DEFAULT case");
            stage = OFF;
        break;
    }

 // Let's clear the display - ehhh. Not sure how to clear the M5Stack yet. Include clear code here if needed.
  }

/*
 * This is very important code. It takes the WS2812FX library and "pipes" it to the
 * NeoPixelBus library. NeoPixelBus implements hardware-level code for the WS2812B
 * LED driver, meaning this can output any lighting effect as quickly as possible.
 * Without this code, nothing else will work.
 */
void myCustomShow(void) {
  if(dma.IsReadyToUpdate()) {
    // copy the ws2812fx pixel data to the dma pixel data
    memcpy(dma.getPixels(), ws2812fx.getPixels(), dma.getPixelsSize());
    dma.Update();
  }
}

void UltrasonicDebugMode(){

    while(1){
        M5.update();
        M5.Lcd.clear(BLACK);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.setTextSize(5);
        M5.Lcd.println(ultrasonicSensor.get());

        if(M5.BtnA.wasReleased()){
            
            M5.Lcd.clear(BLACK);
            break;

        }
    delay(100);
    }
}

void determineAndSendObject(float ultrasonicReading){ //takes in reading from the ultrasonic sensor and uses it to return true or false values for detection

    int PedestrianCode = 111; //Code used to determine a pedestrian
    int CarCode = 222; //Code used to determine a car

    if(((ultrasonicReading > (BeginningOfSideWalk_Uvalue + sensitivityTreshhold)) || (ultrasonicReading <= (EndOfSideWalk_UValue+ sensitivityTreshhold))) || 
       ((ultrasonicReading > (BeginningOfSideWalk_Uvalue - sensitivityTreshhold)) || (ultrasonicReading <= (EndOfSideWawlk_UValue - sensitivityTreshhold)))){ 
        
        if(pedestrianFunctionality){ //Is Pedestrian Functionality On? (Change at Top)
            return true;
        }
    
    }else if(((ultrasonicReading >= (BeginningOfStreet_UValue + sensitivityTreshhold)) || (ultrasonicReading <= (EndOfStreet_UValue + sensitivityTreshhold))) || 
            ((ultrasonicReading > (BeginningOfSideWalk_Uvalue - sensitivityTreshhold)) || (ultrasonicReading <= (EndOfSideWalk_UValue - sensitivityTreshhold)))){ 
        
        if(carFunctionality){ //Is Car Functionality On? (Change at Top)
            return true;
        }

    }else{ //Don't do anything because neither threshold has been broken
        return false;
    }


}

int lightCounter = 50;
bool checkForNight(){
if(nightTime = digitalRead(photoResistor)){ //High = Night
    if((lightCounter - 1) < 0){
        lightCounter  = 0;
    }else{
        lightCounter--;
    }
}else{
    if((lightCounter + 1) > 100){
        lightCounter = 100;
    }else{
        lightCounter++;
    }
}

}