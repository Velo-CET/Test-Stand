#ifdef EIGHTY

#include <Arduino.h>
#include "max6675.h"
#include <WiFi.h>
#include "HX711.h"

// ================= PINS =================
const int thermoDO  = 19;
const int thermoCLK = 18;

// CS pins (must be OUTPUT-capable pins)
const int thermo1 = 23;
const int thermo2 = 22;
const int thermo3 = 14;
const int thermo4 = 27;
const int thermo5 = 32;
const int thermo6 = 33;

// Load Cell
#define DT_PIN   26
#define SCK_PIN  25

// Ignition
#define FIRE_PIN 2

// ================= OBJECTS =================
HX711 scale;

MAX6675 tc1(thermoCLK, thermo1, thermoDO);
MAX6675 tc2(thermoCLK, thermo2, thermoDO);
MAX6675 tc3(thermoCLK, thermo3, thermoDO);
MAX6675 tc4(thermoCLK, thermo4, thermoDO);
MAX6675 tc5(thermoCLK, thermo5, thermoDO);
MAX6675 tc6(thermoCLK, thermo6, thermoDO);

// ================= TIMING =================
const unsigned long SAMPLE_INTERVAL_US = 12500; // 80Hz
const unsigned long TEMP_INTERVAL_MS = 250;    // 4Hz for MAX6675

unsigned long lastSampleTime = 0;
unsigned long lastTempTime = 0;

// ================= WIFI =================
const char* ssid = "ESP32_Loadcell_AP";
const char* password = "12345678";

WiFiServer server(5050);
WiFiClient client;

// ================= STATE =================
bool connected = false;
bool running = false;
bool aborted = false;
bool countdown = false;

unsigned long countdownStart = 0;
unsigned long t0_time = 0;
const unsigned long countdownDuration = 40000; // 40 seconds

unsigned long runstart = 0;

// ================= DATA =================
double t1 = 0, t2 = 0, t3 = 0, t4 = 0, t5 = 0, t6 = 0;

// ================= HELPERS =================
inline void sendLine(const String &msg) {
  if (client.connected()) {
    client.print(msg);
  } else if (countdown && !running) {
    aborted = true;
    countdown = false;
    digitalWrite(FIRE_PIN, LOW);
  }
}

// ================= STREAM ENGINE =================
void streamData(bool isCountdown) {
  if (!client.connected()) return;

  unsigned long now = micros();
  if (now - lastSampleTime < SAMPLE_INTERVAL_US) return;
  lastSampleTime = now;

  // ===== Time =====
  float rel_time;
  if (isCountdown) {
    unsigned long elapsed = millis() - countdownStart;
    long remaining = countdownDuration - elapsed;
    rel_time = -((float)remaining / 1000.0);
  } else {
    rel_time = (float)(millis() - t0_time) / 1000.0;
  }

  // ===== HX711 (non-blocking) =====
  long reading = 0;
  if (scale.is_ready()) {
    reading = scale.read();
  }

  // ===== Thermocouples (slow, update at 4Hz) =====
  if (millis() - lastTempTime >= TEMP_INTERVAL_MS) {
    t1 = tc1.readCelsius();
    t2 = tc2.readCelsius();
    t3 = tc3.readCelsius();
    t4 = tc4.readCelsius();
    t5 = tc5.readCelsius();
    t6 = tc6.readCelsius();
    lastTempTime = millis();
  }

  // ===== Packet =====
  String packet = String(rel_time, 3) + "," +
                  String(reading) + "," +
                  String(t1, 2) + "," +
                  String(t2, 2) + "," +
                  String(t3, 2) + "," +
                  String(t4, 2) + "," +
                  String(t5, 2) + "," +
                  String(t6, 2) + "\n";

  client.print(packet);
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  // Load Cell
  scale.begin(DT_PIN, SCK_PIN);

  // Ignition
  pinMode(FIRE_PIN, OUTPUT);
  digitalWrite(FIRE_PIN, LOW);

  // WiFi AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  server.begin();

  Serial.println("\n=== ESP32 TCP Telemetry Server Ready ===");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Port: 5050");
}

// ================= LOOP =================
void loop() {
  // ===== Accept Client =====
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

  // ===== Commands =====
  if (client.connected() && client.available()) {
    String cmd = client.readStringUntil('\n');
    cmd.trim();

    if (cmd == "GO" && !running) {
      countdownStart = millis();
      countdown = true;
      aborted = false;
      sendLine("COUNTDOWN_START\n");
      Serial.println("Countdown started (40s)...");
    }

    if (cmd == "ABORT" && countdown && !running) {
      aborted = true;
      countdown = false;
      digitalWrite(FIRE_PIN, LOW);
      sendLine("ABORTED\n");
      Serial.println("Aborted by user.");
    }
  }

  // ===== Countdown Phase =====
  if (countdown && !aborted && !running) {
    streamData(true);

    if (millis() - countdownStart >= countdownDuration) {
      running = true;
      countdown = false;
      t0_time = millis();
      digitalWrite(FIRE_PIN, HIGH);
      sendLine("T0_REACHED\n");
      Serial.println("T0 reached — FIRE HIGH");
      runstart = millis();
    }
  }

  // ===== Running Phase =====
  if (running) {
    // Auto shutoff after 4 seconds
    if (millis() - runstart >= 4000) {
      digitalWrite(FIRE_PIN, LOW);
    }

    streamData(false);
  }
}

#endif