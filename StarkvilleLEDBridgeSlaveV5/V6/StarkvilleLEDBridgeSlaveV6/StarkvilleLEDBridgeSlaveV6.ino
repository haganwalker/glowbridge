#include <M5Stack.h>
#include <M5LoRa.h>

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

int UltrasonicPinNumber = 36;

/*********************************************************************/
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

void setup() {

    Serial.begin(9600);
    M5.begin();
    
    pinMode(UltrasonicPinNumber, INPUT);


  // override the default CS, reset, and IRQ pins (optional)
    LoRa.setPins(); // default set CS, reset, IRQ pin
    Serial.println("LoRa Sender");
    M5.Lcd.println("LoRa Sender");

    if (!LoRa.begin(433E6)) {
        Serial.println("Starting LoRa failed!");
        M5.Lcd.println("Starting LoRa failed!");
        while (1);
    }

    Serial.println("LoRa init succeeded.");
    M5.Lcd.println("LoRa init succeeded.");

}

void loop(){
  M5.update();
  if(M5.BtnA.wasReleased()){
  
    delay(1000);
    UltrasonicDebugMode();
  
  }else{
    
    determineAndSendObject(analogRead(UltrasonicPinNumber));
  
  }
}

void determineAndSendObject(int ultrasonicReading){ //takes in reading from the ultrasonic sensor and uses it to create a pedestrian or car code

    int PedestrianCode = 111; //Code used to determine a pedestrian
    int CarCode = 222; //Code used to determine a car

    if(((ultrasonicReading > (BeginningOfSideWalk_Uvalue + sensitivityTreshhold)) || (ultrasonicReading <= (EndOfSideWalk_UValue+ sensitivityTreshhold))) || 
       ((ultrasonicReading > (BeginningOfSideWalk_Uvalue - sensitivityTreshhold)) || (ultrasonicReading <= (EndOfSideWalk_UValue - sensitivityTreshhold)))){ 
        
        if(pedestrianFunctionality){ //Is Pedestrian Functionality On? (Change at Top)
            SendInformation(PedestrianCode);
        }
    
    }else if(((ultrasonicReading >= (BeginningOfStreet_UValue + sensitivityTreshhold)) || (ultrasonicReading <= (EndOfStreet_UValue + sensitivityTreshhold))) || 
            ((ultrasonicReading > (BeginningOfSideWalk_Uvalue - sensitivityTreshhold)) || (ultrasonicReading <= (EndOfSideWalk_UValue - sensitivityTreshhold)))){ 
        
        if(carFunctionality){ //Is Car Functionality On? (Change at Top)
            SendInformation(CarCode);
        }

    }else{ //Don't do anything because neither threshold has been broken

    }


}


void SendInformation(int determinationOfObject){ //send int code from slave to master and looks for an ack
    
    M5.Lcd.clear(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextSize(3);
    
    if(determinationOfObject == 111){
        M5.Lcd.println("Pedestrian Detected");
    }else{
        M5.Lcd.println("Vehicle Detected");
    }
    
    M5.Lcd.println("Sending Packet!");
    LoRa.beginPacket();
    LoRa.print(determinationOfObject);
    LoRa.endPacket();
    delay(100);
} 
