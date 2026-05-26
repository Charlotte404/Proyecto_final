#include <Arduino.h>
#include <Wire.h>

#define RCWL_ADDRESS 0x57

void setup() {
  Serial.begin(115200);

  Wire.begin(9, 8);
  Wire.setClock(100000);

  Serial.println("RCWL-1601 OK");
}

void loop() {

  Wire.beginTransmission(RCWL_ADDRESS);
  Wire.write(0x01);
  Wire.endTransmission();

  delay(70);

  uint8_t n = Wire.requestFrom(RCWL_ADDRESS, 3);

  if (n == 3) {

    uint32_t d =
      ((uint32_t)Wire.read() << 16) |
      ((uint32_t)Wire.read() << 8)  |
       Wire.read();

    Serial.print("Distancia: ");
    Serial.print(d / 1000.0);
    Serial.println(" mm");

  } else {
    Serial.println("Error lectura");
  }

  delay(300);
}