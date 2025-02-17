#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AccelStepper.h>

// WiFi Credentials
const char* ssid = "DESKTOP";
const char* password = "naotedigo";

// Define stepper motor pins (NEMA 17 + A4988 example)
#define STEP_PIN D1
#define DIR_PIN D2
#define ENABLE_PIN D3

AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Stepper Control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; text-align: center; }
        button { font-size: 20px; padding: 10px; margin: 10px; }
    </style>
</head>
<body>
    <h2>Stepper Motor Control</h2>
    <button onclick="fetch('/forward')">Forward</button>
    <button onclick="fetch('/backward')">Backward</button>
    <button onclick="fetch('/stop')">Stop</button>
</body>
</html>
)rawliteral";

void setup() {
    Serial.begin(115200);
    
    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi");
    Serial.println(WiFi.localIP());
    
    // Configure stepper
    pinMode(ENABLE_PIN, OUTPUT);
    digitalWrite(ENABLE_PIN, LOW); // Enable driver
    stepper.setMaxSpeed(1000);
    stepper.setAcceleration(500);
    
    // Web server handlers
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
    });

    server.on("/forward", HTTP_GET, [](AsyncWebServerRequest *request){
        stepper.move(1000); // Move forward 200 steps
        request->send(200, "text/plain", "Moving forward");
    });
    
    server.on("/backward", HTTP_GET, [](AsyncWebServerRequest *request){
        stepper.move(-1000); // Move backward 200 steps
        request->send(200, "text/plain", "Moving backward");
    });
    
    server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request){
        stepper.stop();
        request->send(200, "text/plain", "Stopped");
    });
    
    server.begin();
}

void loop() {
    stepper.run();
}
