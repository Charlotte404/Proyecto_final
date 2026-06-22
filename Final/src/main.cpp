#include <Arduino.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>

#define RCWL_ADDRESS 0x57

volatile float distancies[181];

volatile float angleActual = 0.0;
volatile bool direccioCW = true;
volatile bool netejarRadar = false;

unsigned long tempsMoviment = 0;

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

void IRAM_ATTR limitISR()
{
    origenTrobat = true;
    netejarRadar = true;
}

void actualitzarAngle()
{
    static unsigned long ultimTemps = millis();

    unsigned long ara = millis();
    float dt = (ara - ultimTemps) / 1000.0;

    ultimTemps = ara;

    float velocitat = 180.0 / 2.0; // 90 graus/s

    if(direccioCW)
    {
        angleActual += velocitat * dt;

        if(angleActual > 180)
            angleActual = 180;
    }
    else
    {
        angleActual -= velocitat * dt;

        if(angleActual < 0)
            angleActual = 0;
    }
}

// ================= WEB =================

void handleData()
{
    String json = "{";

    json += "\"angle\":" + String(angleActual) + ",";
    json += "\"dist\":[";

    for(int i=0;i<=180;i++)
    {
        json += String(distancies[i]);
        if(i<180) json += ",";
    }

    json += "]}";

    server.send(200, "application/json", json);
}

String paginaHTML()
{
String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>

<style>
body{
    background:black;
    margin:0;
}
canvas{
    display:block;
    margin:auto;
}
</style>
</head>
<body>

<canvas id="radar" width="900" height="500"></canvas>

<script>

const canvas = document.getElementById("radar");
const ctx = canvas.getContext("2d");

const cx = 450;
const cy = 450;
const R = 400;
const DIST_MAX = 1000;

async function update()
{
    const res = await fetch("/data");
    const data = await res.json();

    const dades = data.dist;
    const angleActual = data.angle;

    ctx.clearRect(0,0,900,500);

    // grid
    ctx.strokeStyle = "green";

    for(let r=100;r<=400;r+=100)
    {
        ctx.beginPath();
        ctx.arc(cx,cy,r,Math.PI,2*Math.PI);
        ctx.stroke();
    }

    for(let a=0;a<=180;a+=30)
    {
        let rad=(a-180)*Math.PI/180;

        ctx.beginPath();
        ctx.moveTo(cx,cy);
        ctx.lineTo(cx+R*Math.cos(rad), cy+R*Math.sin(rad));
        ctx.stroke();
    }

    //linia de scan
    let scanAngle = (angleActual || 0);
    let rad = (scanAngle - 180) * Math.PI / 180;

    ctx.strokeStyle = "rgba(0,255,0,0.8)";
    ctx.lineWidth = 2;

    ctx.beginPath();
    ctx.moveTo(cx, cy);
    ctx.lineTo(
        cx + R * Math.cos(rad),
        cy + R * Math.sin(rad)
    );
    ctx.stroke();

    // punts
    ctx.fillStyle = "lime";

    for(let a=0;a<=180;a++)
    {
        let d = dades[a];
        if(d <= 0) continue;

        let rr = Math.min(d, DIST_MAX)/DIST_MAX * R;

        let rad = (a-180)*Math.PI/180;

        let x = cx + rr*Math.cos(rad);
        let y = cy + rr*Math.sin(rad);

        ctx.beginPath();
        ctx.arc(x,y,4,0,2*Math.PI);
        ctx.fill();
    }
}

setInterval(update, 100);

</script>
</body>
</html>
)rawliteral";

    return html;
}

void handleRoot()
{
    server.send(200,"text/html",paginaHTML());
}


// ================= TASCA SERVO =================

void homeServo()
{
    Serial.println("Buscant origen...");

    origenTrobat = false;

    servo.write(CCW);

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
        direccioCW = true;
        servo.write(CW);

        vTaskDelay(pdMS_TO_TICKS(2000));

        direccioCW = false;
        servo.write(CCW);

        vTaskDelay(pdMS_TO_TICKS(1500));

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
                distanciaMM=d/500.0;
                portEXIT_CRITICAL(&mux);

                Serial.print("Distancia: ");
                Serial.print(distanciaMM);
                Serial.println(" mm");
            }
        }

        if(netejarRadar)
        {
            for(int i=0;i<=180;i++)
            {
                distancies[i] = -1;
            }
        angleActual = 0;
        netejarRadar = false;
        }

        actualitzarAngle();

        int a = (int)round(angleActual);

        if(a >= 0 && a <= 180)
        {
            distancies[a] = distanciaMM;
        }

        vTaskDelay(pdMS_TO_TICKS(30));
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
    server.on("/data", handleData);
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