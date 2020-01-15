#include <String.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

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

void SendInformation(int determinationOfObject){
    
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