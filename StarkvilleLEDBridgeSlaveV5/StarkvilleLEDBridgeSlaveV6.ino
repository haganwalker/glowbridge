#include <String.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>



/*Use This To Turn On/Off Pedestrian/Car detection (1 = ON | 0 = OFF)*/
int pedestrianFunctionality = 1;
int carFunctionality = 1;
/********************************/

/***      Set this radio as radio number 0 or 1         ***/
bool radioNumber = 0;

RF24 radio(5, 4); // CE, CSN Pins

byte addresses[][6] = {"1Node","2Node"}; 

typedef enum { role_ping_out = 1, role_pong_back } role_e;                 // The various roles supported by this sketch
const char* role_friendly_name[] = { "invalid", "Ping out", "Pong back"};  // The debug-friendly names of those roles
role_e role = role_ping_out;


void setup() {
    Serial.begin(9600);
    
    radio.enableAckPayload();                     // Allow optional ack payloads
    radio.enableDynamicPayloads();                // Ack payloads are dynamic payloads

    radio.begin();

    radio.enableAckPayload();                     // Allow optional ack payloads
    radio.enableDynamicPayloads();                // Ack payloads are dynamic payloads
    radio.writeAckPayload(1,1,1); 
    
    if(radioNumber){
    
        radio.openWritingPipe(addresses[1]);        // Both radios listen on the same pipes by default, but opposite addresses
        radio.openReadingPipe(1,addresses[0]);      // Open a reading pipe on address 0, pipe 1
    
    }else{
    
        radio.openWritingPipe(addresses[0]);
        radio.openReadingPipe(1,addresses[1]);
    
    }
  
    //radio.startListening(); //Ask the radio to start listenting

}

void loop(){

}


void determineAndSendObject(int ultrasonicReading){ //takes in reading from the ultrasonic sensor and uses it to create a pedestrian or car code

    int sensitivityTreshhold = 0; //Used to update the "Slop" in the sensor readings

    int EndOfStreet_UValue = 0; //End of ultrasonic threshold on street
    int BeginningOfStreet_UValue=0; //Beginning of the ultrasonic threshold on street

    int EndOfSideWalk_UValue=0; //End of ultrasonic threshold on sidewalk
    int BeginningOfSideWalk_Uvalue=0; //Beginning of the ultrasonic threshold on sidewalk

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
    
    if (role == role_ping_out){
        
        byte gotByte;                          // Initialize a variable for the incoming ACK response
        byte informationPacket = determinationOfObject; // Send the information of the passed in var to the info packet
        radio.stopListening();                 // First, if we are listening, stop listening so we can talk.         
                                                        
        if ( radio.write(&informationPacket,1) ){ // Send the Distance Packet to the other radio 
            
            serial.println("Successfully Sent Distance Data!");
            
            if(radio.available()){             //If we hear back from the radio after we send date we got an ack             
                
                while(radio.available() ){  // If an ack with payload was received
                    serial.println("Ack Received!")
                    radio.read( &gotByte, 1 );  //read the ack payload  
                    return;                     //leave the function
                
                }
            
            }else{      //No Ack Received
                
                serial.println("No Ack received... That's odd...");
                
            }
        
        }else{
            
            serial.println("Failed To Send Distance Data!");
        
        } 
   } 
}