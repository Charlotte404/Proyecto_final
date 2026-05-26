#include <Arduino.h>
#include <ESP32Servo.h>
#include <Wire.h>

#define RCWL_ADDRESS 0x57

Servo servo;
const int servoPin = 5;

// -----------------------------
// TASCA SERVO
// -----------------------------
void TaskServo(void *pvParameters)
{
    servo.attach(servoPin);

    servo.write(93); // aturat
    vTaskDelay(pdMS_TO_TICKS(2000));

    while(true)
    {
        // Gir horari
        servo.write(91);
        vTaskDelay(pdMS_TO_TICKS(2690));

        servo.write(93);
        vTaskDelay(pdMS_TO_TICKS(1000));

        // Gir antihorari
        servo.write(97);
        vTaskDelay(pdMS_TO_TICKS(2650));

        servo.write(93);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// -----------------------------
// TASCA SENSOR
// -----------------------------
void TaskRCWL(void *pvParameters)
{
    Wire.begin(9,8);
    Wire.setClock(100000);

    Serial.println("RCWL-1601 OK");

    while(true)
    {
        Wire.beginTransmission(RCWL_ADDRESS);
        Wire.write(0x01);
        Wire.endTransmission();

        vTaskDelay(pdMS_TO_TICKS(70));

        uint8_t n = Wire.requestFrom(RCWL_ADDRESS,3);

        if(n==3)
        {
            uint32_t d =
            ((uint32_t)Wire.read()<<16) |
            ((uint32_t)Wire.read()<<8)  |
             Wire.read();

            Serial.print("Distancia: ");
            Serial.print(d/1000.0);
            Serial.println(" mm");
        }
        else
        {
            Serial.println("Error lectura");
        }

        vTaskDelay(pdMS_TO_TICKS(300));
    }
}


void setup()
{
    Serial.begin(115200);
    xTaskCreatePinnedToCore(
        TaskServo,
        "Servo",
        4096,
        NULL,
        1,
        NULL,
        0
    );
    xTaskCreatePinnedToCore(
        TaskRCWL,
        "Sensor",
        4096,
        NULL,
        1,
        NULL,
        1
    );
}

void loop()
{
}