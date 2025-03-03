#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <AccelStepper.h>

const char* ssid = "RotaryTableAP";
const char* password = "password123";

// EEPROM offsets
const int EEPROM_SPEED_OFFSET = 0;
const int EEPROM_MICROSTEPS_OFFSET = 32;
const int EEPROM_GEAR_RATIO_OFFSET = 64;
const int EEPROM_ACCELERATION_OFFSET = 96;
const int EEPROM_JOG_STEPS_OFFSET = 128;

// Function declarations
void handleRoot();
void handleSetSpeed();
void handleSetMicrosteps();
void handleSetGearRatio();
void handleSetAcceleration();
void handleSetJogSteps();
void handleStart();
void handlePause();
void handleResume();
void handleRestart();
void handleJogLeft();
void handleJogRight();

// Stepper motor configuration
const int stepPin = D1;
const int dirPin = D2;
const int enablePin = D3;

// Create AccelStepper object (using DRIVER mode for external driver)
AccelStepper stepper(AccelStepper::DRIVER, stepPin, dirPin);

// Variables for storing settings
int speed = 1000; // Default speed in steps per second
int microsteps = 4; // Default microsteps (adjustable)
float gearRatio = 30; // Default gear ratio
int acceleration = 1000; // Default acceleration in steps per second^2
int jogSteps = 1000; // Default jog steps

ESP8266WebServer server(80);

void handleRoot() {
    String html = "<html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; }";
    html += "h1 { font-size: 24px; }";
    html += "p { margin: 10px 0; }";
    html += "input, select, button { width: 100%; padding: 10px; margin: 5px 0; font-size: 18px; }";
    html += "button { background-color: #4CAF50; color: white; border: none; cursor: pointer; }";
    html += "button:hover { background-color: #45a049; }";
    html += "</style>";
    html += "</head><body>";
    html += "<h1>Rotary Table Control</h1>";
    html += "<p>Speed (degrees/s): <input type='number' id='speed' value='" + String(speed / (200.0 * microsteps * gearRatio / 360.0)) + "' onchange='setSpeed()'></p>";
    html += "<p>Microsteps: <select id='microsteps' onchange='setMicrosteps()'>";
    html += "<option value='1' " + String(microsteps == 1 ? "selected" : "") + ">1</option>";
    html += "<option value='2' " + String(microsteps == 2 ? "selected" : "") + ">2</option>";
    html += "<option value='4' " + String(microsteps == 4 ? "selected" : "") + ">4</option>";
    html += "<option value='8' " + String(microsteps == 8 ? "selected" : "") + ">8</option>";
    html += "<option value='16' " + String(microsteps == 16 ? "selected" : "") + ">16</option>";
    html += "<option value='32' " + String(microsteps == 32 ? "selected" : "") + ">32</option>";
    html += "</select></p>";
    html += "<p>Gear Ratio: <input type='number' id='gear_ratio' value='" + String(gearRatio) + "' step='0.01' onchange='setGearRatio()'></p>";
    html += "<p>Acceleration (degrees/s^2): <input type='number' id='acceleration' value='" + String(acceleration / (200.0 * microsteps * gearRatio / 360.0)) + "' onchange='setAcceleration()'></p>";
    html += "<p>Jog Steps (degrees): <input type='number' id='jog_steps' value='" + String(jogSteps / (200.0 * microsteps * gearRatio / 360.0)) + "' onchange='setJogSteps()'></p>";
    html += "<button onclick='startRotation()'>Start Full Rotation</button>";
    html += "<button onclick='pauseRotation()'>Pause</button>";
    html += "<button onclick='resumeRotation()'>Resume</button>";
    html += "<button onclick='restartRotation()'>Restart</button>";
    html += "<button onclick='jogLeft()'>Left</button>";
    html += "<button onclick='jogRight()'>Right</button>";
    html += "<script>";
    html += "function setSpeed() { fetch('/set_speed?value=' + document.getElementById('speed').value); }";
    html += "function setMicrosteps() { fetch('/set_microsteps?value=' + document.getElementById('microsteps').value); }";
    html += "function setGearRatio() { fetch('/set_gear_ratio?value=' + document.getElementById('gear_ratio').value); }";
    html += "function setAcceleration() { fetch('/set_acceleration?value=' + document.getElementById('acceleration').value); }";
    html += "function setJogSteps() { fetch('/set_jog_steps?value=' + document.getElementById('jog_steps').value); }";
    html += "function startRotation() { fetch('/start').then(response => response.text()); }";
    html += "function pauseRotation() { fetch('/pause').then(response => response.text()); }";
    html += "function resumeRotation() { fetch('/resume').then(response => response.text()); }";
    html += "function restartRotation() { fetch('/restart').then(response => response.text()); }";
    html += "function jogLeft() { fetch('/jog_left').then(response => response.text()); }";
    html += "function jogRight() { fetch('/jog_right').then(response => response.text()); }";
    html += "</script>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void handleSetSpeed() {
    if (server.hasArg("value")) {
        float speedDegrees = server.arg("value").toFloat();
        speed = speedDegrees * (200.0 * microsteps * gearRatio / 360.0);
        EEPROM.put(EEPROM_SPEED_OFFSET, speed);
        EEPROM.commit();
        stepper.setMaxSpeed(speed);
        server.send(200, "text/plain", "Speed set to " + String(speedDegrees) + " degrees/s");
    } else {
        server.send(400, "text/plain", "Bad Request");
    }
}

void handleSetMicrosteps() {
    if (server.hasArg("value")) {
        microsteps = server.arg("value").toInt();
        EEPROM.put(EEPROM_MICROSTEPS_OFFSET, microsteps);
        EEPROM.commit();
        server.send(200, "text/plain", "Microsteps set to " + String(microsteps));
    } else {
        server.send(400, "text/plain", "Bad Request");
    }
}

void handleSetGearRatio() {
    if (server.hasArg("value")) {
        gearRatio = server.arg("value").toFloat();
        EEPROM.put(EEPROM_GEAR_RATIO_OFFSET, gearRatio);
        EEPROM.commit();
        server.send(200, "text/plain", "Gear ratio set to " + String(gearRatio));
    } else {
        server.send(400, "text/plain", "Bad Request");
    }
}

void handleSetAcceleration() {
    if (server.hasArg("value")) {
        float accelerationDegrees = server.arg("value").toFloat();
        acceleration = accelerationDegrees * (200.0 * microsteps * gearRatio / 360.0);
        EEPROM.put(EEPROM_ACCELERATION_OFFSET, acceleration);
        EEPROM.commit();
        stepper.setAcceleration(acceleration);
        server.send(200, "text/plain", "Acceleration set to " + String(accelerationDegrees) + " degrees/s^2");
    } else {
        server.send(400, "text/plain", "Bad Request");
    }
}

void handleSetJogSteps() {
    if (server.hasArg("value")) {
        float jogStepsDegrees = server.arg("value").toFloat();
        jogSteps = jogStepsDegrees * (200.0 * microsteps * gearRatio / 360.0);
        EEPROM.put(EEPROM_JOG_STEPS_OFFSET, jogSteps);
        EEPROM.commit();
        server.send(200, "text/plain", "Jog steps set to " + String(jogStepsDegrees) + " degrees");
    } else {
        server.send(400, "text/plain", "Bad Request");
    }
}

void handlePause() {
    stepper.stop();
    server.send(200, "text/plain", "Rotation paused");
}

void handleResume() {
    stepper.run();
    server.send(200, "text/plain", "Rotation resumed");
}

void handleRestart() {
    stepper.setCurrentPosition(0);
    server.send(200, "text/plain", "Rotation restarted");
}

void handleJogLeft() {
    stepper.move(-jogSteps); // Jog left the specified number of steps
    server.send(200, "text/plain", "Jogging left");
}

void handleJogRight() {
    stepper.move(jogSteps); // Jog right the specified number of steps
    server.send(200, "text/plain", "Jogging right");
}

void handleStart() {
    // Calculate the total number of steps for a full rotation
    long steps = 200 * microsteps * gearRatio; // NEMA 24 has 200 steps per revolution
    
    // Start the rotation (non-blocking)
    stepper.moveTo(steps); // Move the motor the calculated number of steps

    server.send(200, "text/plain", "Rotation started");
}

void setup() {
    Serial.begin(115200);

    // Set up the ESP8266 as an access point
    WiFi.softAP(ssid, password);
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);

    EEPROM.begin(512); // Initialize EEPROM
    
    // Read settings from EEPROM if available
    EEPROM.get(EEPROM_SPEED_OFFSET, speed);
    EEPROM.get(EEPROM_MICROSTEPS_OFFSET, microsteps);
    EEPROM.get(EEPROM_GEAR_RATIO_OFFSET, gearRatio);
    EEPROM.get(EEPROM_ACCELERATION_OFFSET, acceleration);
    EEPROM.get(EEPROM_JOG_STEPS_OFFSET, jogSteps);

    // Set up the stepper motor
    pinMode(enablePin, OUTPUT);
    digitalWrite(enablePin, LOW); // Enable driver
    stepper.setMaxSpeed(speed);
    stepper.setAcceleration(acceleration); // Set acceleration to a reasonable value

    // Define server routes
    server.on("/", HTTP_GET, handleRoot);
    server.on("/set_speed", HTTP_GET, handleSetSpeed);
    server.on("/set_microsteps", HTTP_GET, handleSetMicrosteps);
    server.on("/set_gear_ratio", HTTP_GET, handleSetGearRatio);
    server.on("/set_acceleration", HTTP_GET, handleSetAcceleration);
    server.on("/set_jog_steps", HTTP_GET, handleSetJogSteps);
    server.on("/start", HTTP_GET, handleStart);
    server.on("/pause", HTTP_GET, handlePause);
    server.on("/resume", HTTP_GET, handleResume);
    server.on("/restart", HTTP_GET, handleRestart);
    server.on("/jog_left", HTTP_GET, handleJogLeft);
    server.on("/jog_right", HTTP_GET, handleJogRight);

    // Start the server
    server.begin();
}

void loop() {
    server.handleClient();
    stepper.run(); // Non-blocking motor control
}