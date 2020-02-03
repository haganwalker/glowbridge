int ultrasonicCheckInterval = 10;

void SetupUltrasonicPin(int pinName){

    pinMode(36, INPUT);
}

int ReadUltrasonic(){

    return analogRead(36);

}

void ChangeUltrasonicCheckInterval(int updatedInterval){

    ultrasonicCheckInterval = updatedInterval;

}
