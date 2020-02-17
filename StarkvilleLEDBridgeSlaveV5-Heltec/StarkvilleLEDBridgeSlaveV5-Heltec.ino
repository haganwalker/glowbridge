/* This is the code that runs on the slave or satelite side of the bridge. When ultrasonic 
 * values are in the threshold via the ADC on pin 36 and the timer hasn't been triggered, 
 * this slave device will send data to the master ESP32 on the other side of the bridge
 * to trigger the LEDs. Pretty much 95% of the code is ran on the master controller. The
 * code below is based heavily on the Heltec example but has a few changes to limit
 * too many transmissions. For one, a message will ONLY be sent to the master device via
 * LoRa if the ultrasonic values are within threshold. However, to further prevent repeat
 * and overloading sending, there is also a timeout. After a message has been sent to the 
 * master device, this device will not send another message again for at least 10 seconds, 
 * even if someone or something is blocking the sensor. This should keep up well within the
 * FCC rules for 915Mhz transmissions. 
  This is a simple example to show the Heltec.LoRa sending data and displaying on the OLED.

  The onboard OLED display is SSD1306 driver and I2C interface. In order to make the
  OLED correctly operation, you should output a high-low-high(1-0-1) signal by soft-
  ware to OLED's reset pin, the low-level signal at least 5ms.

  OLED pins to ESP32 GPIOs via this connection:
  OLED_SDA -- GPIO4
  OLED_SCL -- GPIO15
  OLED_RST -- GPIO16
  
  by Aaron.Lee from HelTec AutoMation, ChengDu, China
  成都惠利特自动化科技有限公司
  www.heltec.cn
  
  this project also realess in GitHub:
  https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
*/
#include "heltec.h"
#include <Smoothed.h>


#define BAND    915E6  //you can set band here directly,e.g. 868E6,915E6

unsigned int counter = 0;
unsigned int peopleSeen = 0;
String rssi = "RSSI --";
String packSize = "--";
String packet ;
float smoothed = 0;

Smoothed <int> mySensor;

int canBroadcast = false;

void setup()
{ 

  mySensor.begin(SMOOTHED_AVERAGE, 3);
  //WIFI Kit series V1 not support Vext control
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.Heltec.Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
 
  Heltec.display->init();
  Heltec.display->flipScreenVertically();  
  Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->clear();
  Heltec.display->drawString(0, 0, "Wait 5 seconds..");
  Heltec.display->display();
  Heltec.LoRa.setSyncWord(0xF3); //to make sure no interference to receiver.
  delay(5000);
  Heltec.display->clear();
  Heltec.display->drawString(0, 0, "Setup Done!");
  Heltec.display->display();
  delay(3000);
}

#define LORA_CHECK_INTERVAL (10000)

void loop() {
  
  static uint32_t next_LoRa_read = 0;                  // don't know if you can still read valuable things...

  uint32_t now = millis();


  if(now > next_LoRa_read)                            // now we check quite less. Otherwise the code could also flip / bounce between on and off
  {                                     
                                                      // Idea for improvement:
                                                      // another idea is to have a "counter" which increments on darkness and decrements when there is light
                                                      // this could deboucne and the check intervall could be shorter... it would saturate at 0 (during the day)
                                                      // and a certain value (which is the used below to perform the tasks) like 40....
                                                      // if the increment is e.g. 2 and the decrement is e.g. 1 then you could also react quicker on darkness while
                                                      // it will be slower to got to OFF mode.
    canBroadcast = true;
    Heltec.display->drawString(0, 20, "Can Broadcast");
    Serial.println("canBroadcast");
    Heltec.display->display();
  }
  
  Heltec.display->clear();
  float currentSensorValue = analogRead(36);
  mySensor.add(currentSensorValue);
  smoothed = mySensor.get();
  Serial.println(smoothed);
  if((smoothed > 0) && (smoothed < 600) && (millis()>next_LoRa_read)){       // if in the range of 0.1 ft to 6 ft
      Serial.println("Triggered");
      Heltec.display->clear();
      Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
      Heltec.display->setFont(ArialMT_Plain_16);
      
      Heltec.display->drawString(0, 0, "Packet: ");
      Heltec.display->drawString(90, 0, String(counter));
      Heltec.display->drawString(0, 20, "Motion ");
      Heltec.display->drawString(60, 20, String(smoothed));
      Serial.println("Sending Packet...");
      Heltec.display->display();
      Heltec.LoRa.beginPacket();
      Heltec.LoRa.print("motion");
      Heltec.LoRa.print(counter);
      Heltec.LoRa.endPacket(); 
      Serial.println("Packet Sent");
      canBroadcast = false;
      counter++;  
      peopleSeen ++;
      next_LoRa_read = millis() + LORA_CHECK_INTERVAL;
   }
}
