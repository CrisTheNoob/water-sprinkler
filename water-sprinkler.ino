#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <WiFi.h>
#include <WebServer.h>

// 0.96 Oled Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// DHT11
#define DHTPIN1 19
#define DHTPIN2 18
#define DHTPIN3 5
#define DHTTYPE DHT11

DHT dht1(DHTPIN1, DHTTYPE);
DHT dht2(DHTPIN2, DHTTYPE);
DHT dht3(DHTPIN3, DHTTYPE);

// Water pump relay (Active LOW relay)
#define WATER_PUMP 27  

// Solenoid valves (Active LOW relays)
#define VALVE1 25   
#define VALVE2 26   
#define VALVE3 32   

// Timer Variables
unsigned long pumpStartTime = 0;
bool pumpRunning = false;
const unsigned long pumpDuration = 120000;  // 120 seconds (2 mins)

// Wi-Fi credentials
const char* ssid = "Your_SSID";
const char* password = "Your_PASSWORD";

// Web server instance
WebServer server(80);

// Manual Control Flags
bool manualControl = false;
bool manualPumpState = false;

void handleRoot() {
    server.send(200, "text/html", "<h1>ESP32 Sprinkler Control</h1><p><a href='/on'>Turn ON</a></p><p><a href='/off'>Turn OFF</a></p><p><a href='/auto'>Automatic Mode</a></p>");
}

void handleOn() {
    manualControl = true;
    manualPumpState = true;
    server.send(200, "text/html", "<h1>Pump Turned ON</h1><p><a href='/'>Back</a></p>");
}

void handleOff() {
    manualControl = true;
    manualPumpState = false;
    server.send(200, "text/html", "<h1>Pump Turned OFF</h1><p><a href='/'>Back</a></p>");
}

void handleAuto() {
    manualControl = false;
    server.send(200, "text/html", "<h1>Automatic Mode Enabled</h1><p><a href='/'>Back</a></p>");
}

void setup() {
    Serial.begin(9600);

    // Initialize OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        while (1);
    }

    // Initialize DHT sensors
    dht1.begin();
    dht2.begin();
    dht3.begin();

    // Initialize water pump and solenoids as OUTPUT
    pinMode(WATER_PUMP, OUTPUT);
    pinMode(VALVE1, OUTPUT);
    pinMode(VALVE2, OUTPUT);
    pinMode(VALVE3, OUTPUT);

    // Ensure all relays are OFF initially (Active LOW setup)
    digitalWrite(WATER_PUMP, HIGH);
    digitalWrite(VALVE1, HIGH);
    digitalWrite(VALVE2, HIGH);
    digitalWrite(VALVE3, HIGH);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("Connected!");
    Serial.println(WiFi.localIP());

    // Setup web server routes
    server.on("/", handleRoot);
    server.on("/on", handleOn);
    server.on("/off", handleOff);
    server.on("/auto", handleAuto);
    server.begin();
}

void loop() {
    server.handleClient();
    float temp1 = dht1.readTemperature();
    float temp2 = dht2.readTemperature();
    float temp3 = dht3.readTemperature();

    if (isnan(temp1) || isnan(temp2) || isnan(temp3)) {
        Serial.println("Failed to read from one or more DHT sensors!");
        return;
    }

    Serial.print("Temp1: "); Serial.print(temp1);
    Serial.print(" °C, Temp2: "); Serial.print(temp2);
    Serial.print(" °C, Temp3: "); Serial.print(temp3);
    Serial.println(" °C");

    // Automatic Control
    if (!manualControl) {
        bool tempHigh = (temp1 > 30 || temp2 > 30 || temp3 > 30);
        if (tempHigh && !pumpRunning) {
            pumpRunning = true;
            pumpStartTime = millis();
        }
        if (!tempHigh || (millis() - pumpStartTime >= pumpDuration)) {
            pumpRunning = false;
        }
    } else {
        pumpRunning = manualPumpState;
    }

    // Control the pump and valves
    digitalWrite(WATER_PUMP, pumpRunning ? LOW : HIGH);
    digitalWrite(VALVE1, temp1 > 30 ? LOW : HIGH);
    digitalWrite(VALVE2, temp2 > 30 ? LOW : HIGH);
    digitalWrite(VALVE3, temp3 > 30 ? LOW : HIGH);

    Serial.println(pumpRunning ? "Water Pump: ON" : "Water Pump: OFF");

    // Display Information
    display.clearDisplay();
    display.setCursor(10, 10);
    display.print("T1: "); display.print(temp1); display.print(" C");
    display.setCursor(10, 30);
    display.print("T2: "); display.print(temp2); display.print(" C");
    display.setCursor(10, 50);
    display.print("T3: "); display.print(temp3); display.print(" C");

    if (pumpRunning) {
        unsigned long remainingTime = (pumpStartTime + pumpDuration - millis()) / 1000;
        display.setCursor(80, 10);
        display.print("Time: ");
        display.print(remainingTime);
        display.print("s");
    }

    display.display();
    delay(3000);
}
