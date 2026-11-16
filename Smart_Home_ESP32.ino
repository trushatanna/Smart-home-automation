#define BLYNK_TEMPLATE_ID "TMPL3QhNllkMA"
#define BLYNK_TEMPLATE_NAME "Smart home automation and safety"
#define BLYNK_AUTH_TOKEN "09nyOTwXGVeZzi7KzSZoT-i8Y3sSAf8q"

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// -------- WIFI DETAILS --------
char ssid[] = "narzo 50A";
char pass[] = "Narzo 50";

// -------- PIN DEFINITIONS --------
#define IR_PIN       14
#define GAS_PIN      34
#define LM35_PIN     36
#define VOLTAGE_PIN  35

#define LED_PIN      4
#define BUZZER_PIN   18

#define RELAY_IN1    25
#define RELAY_IN2    26

// -------- SETTINGS --------
#define IR_DETECTED  LOW
#define GAS_DETECTED LOW   // Change to HIGH if your gas module outputs HIGH on detection

#define RELAY_ON  LOW
#define RELAY_OFF HIGH

#define HOT_TEMP  30.0
#define COLD_TEMP 20.0

#define VOLTAGE_DIVIDER_RATIO 5.0

#define BUZZER_FREQUENCY 2000

// -------- TIMING --------
#define IR_CLEAR_DELAY 1000UL

// -------- BLYNK TIMER --------
BlynkTimer timer;

// -------- VARIABLES --------
bool lightState = false;
bool previousIRState = false;
unsigned long lastIRClearTime = 0;

bool gasAlert = false;
bool buzzerStatus = false;

float temperature = 0.0;
float batteryVoltage = 0.0;

bool fanStatus = false;
bool heaterStatus = false;

// -------- IR SENSOR --------
void updateIR() {
  bool irReading = (digitalRead(IR_PIN) == IR_DETECTED);

  if (irReading && !previousIRState) {
    lightState = !lightState;
    digitalWrite(LED_PIN, lightState ? HIGH : LOW);

    Blynk.virtualWrite(V6, 1);
    Blynk.virtualWrite(V7, lightState ? 1 : 0);

    lastIRClearTime = millis();
  }

  if (!irReading && previousIRState) {
    lastIRClearTime = millis();
  }

  if (!irReading && millis() - lastIRClearTime >= IR_CLEAR_DELAY) {
    Blynk.virtualWrite(V6, 0);
  }

  previousIRState = irReading;
}

// -------- DIGITAL GAS SENSOR --------
void updateGasSensor() {
  bool active = (digitalRead(GAS_PIN) == GAS_DETECTED);

  gasAlert = active;
  buzzerStatus = active;

  static bool previousState = false;

  if (active != previousState) {
    ledcWriteTone(BUZZER_PIN, active ? BUZZER_FREQUENCY : 0);
    previousState = active;
  }
}

// -------- TEMPERATURE SENSOR --------
float readTemperature() {
  int milliVolts = analogReadMilliVolts(LM35_PIN);
  return milliVolts / 10.0;
}

// -------- VOLTAGE SENSOR --------
float readBatteryVoltage() {
  unsigned long total = 0;

  for (int i = 0; i < 100; i++) {
    total += analogReadMilliVolts(VOLTAGE_PIN);
    delayMicroseconds(200);
  }

  float averageMilliVolts = total / 100.0;
  float sensorVoltage = averageMilliVolts / 1000.0;

  return sensorVoltage * VOLTAGE_DIVIDER_RATIO;
}

// -------- RELAY CONTROL --------
void updateRelays() {
  temperature = readTemperature();

  if (temperature >= HOT_TEMP) {
    fanStatus = true;
    heaterStatus = false;
  }
  else if (temperature < COLD_TEMP) {
    fanStatus = false;
    heaterStatus = true;
  }
  else {
    fanStatus = false;
    heaterStatus = false;
  }

  digitalWrite(RELAY_IN1, fanStatus ? RELAY_ON : RELAY_OFF);
  digitalWrite(RELAY_IN2, heaterStatus ? RELAY_ON : RELAY_OFF);
}

// -------- SEND DATA TO BLYNK --------
void sendSensorData() {
  updateRelays();

  batteryVoltage = readBatteryVoltage();

  Blynk.virtualWrite(V0, fanStatus);
  Blynk.virtualWrite(V1, heaterStatus);
  Blynk.virtualWrite(V2, temperature);
  Blynk.virtualWrite(V3, batteryVoltage);
  Blynk.virtualWrite(V4, gasAlert);
  Blynk.virtualWrite(V5, buzzerStatus);
  Blynk.virtualWrite(V7, lightState);

  Serial.println("-------------------------");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" C");

  Serial.print("Battery Voltage: ");
  Serial.print(batteryVoltage);
  Serial.println(" V");

  Serial.print("Fan: ");
  Serial.println(fanStatus ? "ON" : "OFF");

  Serial.print("Heater: ");
  Serial.println(heaterStatus ? "ON" : "OFF");

  Serial.print("Gas Alert: ");
  Serial.println(gasAlert ? "ON" : "OFF");

  Serial.print("Buzzer: ");
  Serial.println(buzzerStatus ? "ON" : "OFF");

  Serial.print("Light: ");
  Serial.println(lightState ? "ON" : "OFF");
}

// -------- SETUP --------
void setup() {
  Serial.begin(115200);

  pinMode(IR_PIN, INPUT);
  pinMode(GAS_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(RELAY_IN1, OUTPUT);
  pinMode(RELAY_IN2, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  digitalWrite(RELAY_IN1, RELAY_OFF);
  digitalWrite(RELAY_IN2, RELAY_OFF);

  // Configure buzzer using ESP32 LEDC
  ledcAttach(BUZZER_PIN, BUZZER_FREQUENCY, 8);
  ledcWriteTone(BUZZER_PIN, 0);

  // Connect to Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  timer.setInterval(1000L, sendSensorData);

  Serial.println("Smart Home Automation Started");
}

// -------- MAIN LOOP --------
void loop() {
  Blynk.run();
  timer.run();

  updateIR();
  updateGasSensor();
}
