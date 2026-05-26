#include <Arduino.h>
#include <ESP32Servo.h>

Servo servo;
const int servoPin = 5;

void setup() {
    servo.attach(servoPin);
    servo.write(93);
    delay(2000);
}

void loop() {

    // Gir lent en sentit horari
    servo.write(91);
    delay(2690);

    servo.write(93);
    delay(1000);

    // Gir lent en sentit antihorari
    servo.write(97);
    delay(2650);

    servo.write(93);
    delay(1000);
}