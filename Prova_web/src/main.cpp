#include <Arduino.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>

#define RCWL_ADDRESS 0x57

// ================= WIFI =================
const char* ssid = "Nautilus";
const char* password = "20000Leguas";

WebServer server(80);

// ================= SERVO =================
Servo servo;
const int servoPin = 5;
const int limitPin = 4;

const int STOP = 93;
const int CW    = 91;
const int CCW   = 97;

volatile bool origenTrobat = false;
// Valor compartit entre tasques
volatile float distanciaMM = -1;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;


// ================= WEB =================
String paginaHTML()
{
    float d;

    portENTER_CRITICAL(&mux);
    d = distanciaMM;
    portEXIT_CRITICAL(&mux);

    String html =
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<meta http-equiv='refresh' content='1'>"
    "<title>Sensor distancia</title>"
    "<style>"
    "body{font-family:Arial;text-align:center;margin-top:60px;}"
    "h1{font-size:28px;}"
    ".valor{font-size:55px;font-weight:bold;}"
    "</style>"
    "</head>"
    "<body>"
    "<h1>Distancia RCWL-1601</h1>";

    if(d >= 0)
    {
        html += "<div class='valor'>";
        html += String(d,1);
        html += " mm</div>";
    }
    else
    {
        html += "<div class='valor'>Esperant dades...</div>";
    }

    html += "</body></html>";

    return html;
}

void handleRoot()
{
    server.send(200,"text/html",paginaHTML());
}


// ================= TASCA SERVO =================

void IRAM_ATTR limitISR()
{
    origenTrobat = true;
}

void homeServo()
{
    Serial.println("Buscant origen...");

    origenTrobat = false;

    servo.write(CW);

    while(!origenTrobat)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    servo.write(STOP);

    Serial.println("Origen trobat");
}

void TaskServo(void *pvParameters)
{
    servo.attach(servoPin);

    vTaskDelay(pdMS_TO_TICKS(2000));

    homeServo();

    while(true)
    {
        servo.write(CCW);
        vTaskDelay(pdMS_TO_TICKS(2650));

        servo.write(STOP);
        vTaskDelay(pdMS_TO_TICKS(1000));

        servo.write(CW);
        vTaskDelay(pdMS_TO_TICKS(2650));

        servo.write(STOP);
        vTaskDelay(pdMS_TO_TICKS(1000));

        // recalibració
        homeServo();
    }
}


// ================= TASCA SENSOR =================
void TaskRCWL(void *pvParameters)
{
    Wire.begin(9,8);
    Wire.setClock(100000);

    while(true)
    {
        Wire.beginTransmission(RCWL_ADDRESS);
        Wire.write(0x01);

        // Si falla, ignorem
        if(Wire.endTransmission()!=0)
        {
            vTaskDelay(pdMS_TO_TICKS(300));
            continue;
        }

        vTaskDelay(pdMS_TO_TICKS(70));

        uint8_t n=Wire.requestFrom(RCWL_ADDRESS,3);

        // Ignorar errors I2C
        if(n==3)
        {
            uint32_t d=
            ((uint32_t)Wire.read()<<16)|
            ((uint32_t)Wire.read()<<8)|
             Wire.read();

            if(d>0 && d<1000000)
            {
                portENTER_CRITICAL(&mux);
                distanciaMM=d/1000.0;
                portEXIT_CRITICAL(&mux);

                Serial.print("Distancia: ");
                Serial.print(distanciaMM);
                Serial.println(" mm");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(300));
    }
}


// ================= TASCA WEB =================
void TaskWeb(void *pvParameters)
{
    while(true)
    {
        server.handleClient();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}



void setup()
{
    Serial.begin(115200);

    WiFi.begin(ssid,password);

    Serial.print("Connectant");

    while(WiFi.status()!=WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }

    Serial.println();
    Serial.println("WiFi connectat");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    server.on("/",handleRoot);
    server.begin();

    pinMode(limitPin, INPUT_PULLUP);

    attachInterrupt(
        digitalPinToInterrupt(limitPin),
        limitISR,
        FALLING
    );

    xTaskCreatePinnedToCore(
    TaskServo,
    "Servo",
    4096,
    NULL,
    1,
    NULL,
    0);

    xTaskCreatePinnedToCore(
    TaskRCWL,
    "Sensor",
    4096,
    NULL,
    1,
    NULL,
    1);

    xTaskCreatePinnedToCore(
    TaskWeb,
    "Web",
    4096,
    NULL,
    1,
    NULL,
    1);
}

void loop(){}