/*
  ELT Transmitter (ESP32 + GPS + LoRa)
  - Sleeps in deep sleep (ultra-low power)
  - Wakes on emergency button press (EXT0)
  - Powers GPS via P-MOSFET gate pin
  - Reads GPS (TinyGPSPlus) and sends binary payload over LoRa @ 915 MHz
*/

#include <SPI.h>
#include <LoRa.h>
#include <TinyGPSPlus.h>
#include "esp_sleep.h"

TinyGPSPlus gps;

// ---- LoRa pins (adjust for your board/module wiring) ----
static const int LORA_SS   = 5;   // NSS / CS
static const int LORA_RST  = 14;  // RESET
static const int LORA_DIO0 = 26;  // DIO0

// ---- GPS UART pins on ESP32 ----
// ESP32 RX2 = GPIO16  (connect to GPS TX)
// ESP32 TX2 = GPIO17  (connect to GPS RX)

// ---- GPS power control (P-MOSFET gate) ----
static const int GPS_POWER_PIN = 25; // Gate of P-MOSFET (via resistor)

// ---- Emergency wake button (active LOW, to GND) ----
static const int BUTTON_PIN = 27;

// ---- Dummy coordinates (used if no GPS fix yet) ----
static const float DUMMY_LAT = 43.79560f;
static const float DUMMY_LON = -79.35051f;

// ---- Payload struct must match receiver ----
struct Payload {
  float lat;
  float lon;
};

static void gpsPowerOn() {
  // Gate LOW => P-MOSFET ON => GPS gets power
  digitalWrite(GPS_POWER_PIN, LOW);
  delay(150);
}

static void gpsPowerOff() {
  // Gate HIGH => P-MOSFET OFF => GPS unpowered
  digitalWrite(GPS_POWER_PIN, HIGH);
}

static void goToDeepSleep() {
  Serial.println("ELT: ARM mode (deep sleep). Waiting for emergency trigger...");
  delay(200);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(9600);
  delay(250);

  // Button wakeup on GPIO27 active LOW
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, 0);

  // GPS power pin: default OFF (HIGH)
  digitalWrite(GPS_POWER_PIN, HIGH);
  pinMode(GPS_POWER_PIN, OUTPUT);

  // Check why we woke up
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  if (cause != ESP_SLEEP_WAKEUP_EXT0) {
    // Power-up/reset -> go directly to deep sleep
    goToDeepSleep();
  }

  // Emergency triggered
  Serial.println("ELT: Emergency triggered! Starting GPS + LoRa TX...");

  // Power GPS and start UART2
  gpsPowerOn();
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  // Setup LoRa
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(915E6)) {
    Serial.println("LoRa init failed. Going back to sleep.");
    gpsPowerOff();
    goToDeepSleep();
  }

  Serial.println("LoRa TX ready. Transmitting every 2 seconds...");

  // Continuous transmit loop
  while (true) {
    // Read GPS for ~1 second (non-blocking style)
    unsigned long start = millis();
    while (millis() - start < 1000) {
      while (Serial2.available()) {
        gps.encode(Serial2.read());
      }
    }

    Payload pkt{};
    bool hasFix = gps.location.isValid();

    if (hasFix) {
      pkt.lat = (float)gps.location.lat();
      pkt.lon = (float)gps.location.lng();
    } else {
      pkt.lat = DUMMY_LAT;
      pkt.lon = DUMMY_LON;
    }

    // Send binary payload
    LoRa.beginPacket();
    LoRa.write((uint8_t*)&pkt, sizeof(pkt));
    LoRa.endPacket();

    Serial.print("TX: ");
    Serial.print(pkt.lat, 6);
    Serial.print(", ");
    Serial.print(pkt.lon, 6);
    Serial.print("  Status: ");
    Serial.println(hasFix ? "GPS" : "Dummy");

    delay(2000);
  }
}

void loop() {
  // Not used (continuous loop runs in setup after wake)
}
