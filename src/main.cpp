#ifdef TEN

#include <Arduino.h>
#include "max6675.h"
#include <WiFi.h>
#include "HX711.h"

const int thermoDO  = 19;
const int thermoCLK = 18;

// CS pins (must be OUTPUT-capable pins)
const int thermo1 = 23;
const int thermo2 = 22;
const int thermo3 = 14;
const int thermo4 = 27;
const int thermo5 = 32;
const int thermo6 = 33;

// ===== Pins =====
#define DT_PIN   26
#define SCK_PIN  25
#define FIRE_PIN 2

// ===== HX711 Setup =====
HX711 scale;

// Create 6 separate objects
MAX6675 tc1(thermoCLK, thermo1, thermoDO);
MAX6675 tc2(thermoCLK, thermo2, thermoDO);
MAX6675 tc3(thermoCLK, thermo3, thermoDO);
MAX6675 tc4(thermoCLK, thermo4, thermoDO);
MAX6675 tc5(thermoCLK, thermo5, thermoDO);
MAX6675 tc6(thermoCLK, thermo6, thermoDO);

int runstart;

unsigned long lastRead = 0;
const unsigned long READ_INTERVAL = 250; // ms

// ===== WiFi AP Setup =====
const char* ssid = "ESP32_Loadcell_AP";
const char* password = "12345678";

WiFiServer server(5050);
WiFiClient client;

// ===== State Variables =====
bool connected = false;
bool running = false;
bool aborted = false;
bool countdown = false;

unsigned long countdownStart = 0;
unsigned long t0_time = 0;
const int countdownDuration = 40000;  // 40 s countdown

double t1=0.0, t2=0.0, t3=0.0, t4=0.0, t5=0.0, t6=0.0;

inline void sendLine(const String &msg) {
  if (client.connected()) client.print(msg);
  else if (countdown && !running) {
    aborted = true;
    countdown = false;
    digitalWrite(FIRE_PIN, LOW);
  }
}

void setup() {
  Serial.begin(115200);
  scale.begin(DT_PIN, SCK_PIN);
  pinMode(FIRE_PIN, OUTPUT);
  digitalWrite(FIRE_PIN, LOW);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  server.begin();

  Serial.println("ESP32 Loadcell WiFi Server Ready.");
  Serial.print("SSID: "); Serial.println(ssid);
}

void loop() {
  // Accept a client
  if (!client || !client.connected()) {
    client = server.available();
    if (client) {
      connected = true;
      aborted = false;
      countdown = false;
      running = false;
      Serial.println("Client connected.");
      client.println("CONNECTED");
    }
  }

  // Handle commands
  if (client.connected() && client.available()) {
    String cmd = client.readStringUntil('\n');
    cmd.trim();

    if (cmd == "GO" && !running) {
      countdownStart = millis();
      countdown = true;
      aborted = false;
      sendLine("COUNTDOWN_START\n");
      Serial.println("Countdown started (40 s)...");
    }

    if (cmd == "ABORT" && countdown && !running) {
      aborted = true;
      countdown = false;
      digitalWrite(FIRE_PIN, LOW);
      sendLine("ABORTED\n");
      Serial.println("Aborted by user.");
    }
  }

  // Countdown phase
  if (countdown && !aborted && !running) {
    unsigned long now = millis();
    long elapsed = now - countdownStart;
    long remaining = countdownDuration - elapsed;

    // Time relative to T0 (negative before launch)
    float rel_time = -((float)remaining / 1000.0);

    // Read + send data
    while (!scale.is_ready()){}
    
    long reading = scale.read();
    if (millis() - lastRead > READ_INTERVAL){
      t1 = tc1.readCelsius();
      t2 = tc2.readCelsius();
      t3 = tc3.readCelsius();
      t4 = tc4.readCelsius();
      t5 = tc5.readCelsius();
      t6 = tc6.readCelsius();
      lastRead = millis();
    }
    String packet = String(rel_time, 3) + "," +
                    String(reading) + "," +
                    String(t1, 2) + "," + 
                    String(t2, 2) + "," + 
                    String(t3, 2) + "," + 
                    String(t4, 2) + "," + 
                    String(t5, 2) + "," + 
                    String(t6, 2) + "\n";
    sendLine(packet);

    // Check if T0 reached
    if (elapsed >= countdownDuration) {
      running = true;
      countdown = false;
      t0_time = millis();
      digitalWrite(FIRE_PIN, HIGH);
      sendLine("T0_REACHED\n");
      Serial.println("T0 reached — firing transistor HIGH");
      runstart = millis();
    }
  }

  // Post-T0 fast streaming
  if (running) {
    if (millis() - runstart >= 4000){
      digitalWrite(FIRE_PIN, LOW);
    }
    unsigned long now = millis();
    float rel_time = (float)(now - t0_time) / 1000.0;
    while (!scale.is_ready()){}
    long reading = scale.read();
    

    if (millis() - lastRead > READ_INTERVAL){
      t1 = tc1.readCelsius();
      t2 = tc2.readCelsius();
      t3 = tc3.readCelsius();
      t4 = tc4.readCelsius();
      t5 = tc5.readCelsius();
      t6 = tc6.readCelsius();
      lastRead = millis();
    }

    // if (last)
    String packet = String(rel_time, 3) + "," +
                    String(reading) + "," +
                    String(t1, 2) + "," + 
                    String(t2, 2) + "," + 
                    String(t3, 2) + "," + 
                    String(t4, 2) + "," + 
                    String(t5, 2) + "," + 
                    String(t6, 2) + "\n";
    // String packet = String(rel_time, 3) + "," + String(reading) + "," + String(thermo, 2) "\n";
    client.print(packet); // no checks for max speed
  }
}

#endif
// #include <Arduino.h>
// #include "HX711.h"

// // Your current pins
// #define DT_PIN   26
// #define SCK_PIN  25

// HX711 scale;

// void setup() {
//   Serial.begin(9600);
//   delay(1000);
  
//   Serial.println("--- HX711 RAW DIAGNOSTIC TEST ---");
  
//   scale.begin(DT_PIN, SCK_PIN);

//   // Check if the chip is connected
//   if (scale.is_ready()) {
//     Serial.println("HX711 found and ready.");
//   } else {
//     Serial.println("HX711 NOT FOUND. Check VCC, GND, DT, and SCK wiring.");
//     while(1); // Stop here if not found
//   }
// }

// void loop() {
//   if (scale.is_ready()) {
//     // Read raw value 10 times and average for stability
//     long reading = scale.read();
    
//     Serial.print("Raw Value: ");
//     Serial.print(reading);

//     // If the value is near 8388607 or 0, it's a wiring error
//     if (reading == 8388607 || reading == -8388608 || reading == 0) {
//       Serial.println(" -> [ERROR: Signal Saturation / Floating]");
//     } else {
//       Serial.println(" -> [OK: Signal detected]");
//     }
//   } else {
//     Serial.println("HX711 not ready...");
//   }
  
//   delay(500); // Slow down for readability
// }