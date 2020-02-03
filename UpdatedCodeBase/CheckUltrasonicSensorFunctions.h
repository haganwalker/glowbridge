#include <Smoothed.h>

int ultrasonicCheckInterval = 10;

Smoothed <int> mySensor;

void SetupUltrasonicPin(int pinName){

    
    mySensor.add(analogRead(SENSOR_PIN));
    mySensor.begin(SMOOTHED_AVERAGE, 10);
}

int ReadUltrasonic(){

    mySensor.get();

}

void ChangeUltrasonicCheckInterval(int updatedInterval){

    ultrasonicCheckInterval = updatedInterval;

}
