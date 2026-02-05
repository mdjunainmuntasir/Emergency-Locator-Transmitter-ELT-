/*
  ELT Receiver / Gateway (ESP32 + LoRa + Wi-Fi)
  - Receives binary LoRa payload (lat/lon)
  - Uploads to ThingSpeak and Adafruit IO via HTTP REST
*/

#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>

// ---- LoRa pins (adjust for your board/module wiring) ----
static const int LORA_SS   = 5;
static const int LORA_RST  = 14;
static const int LORA_DIO0 = 26;

// ---- Wi-Fi credentials (fill yours) ----
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

// ---- ThingSpeak (fill yours) ----
const char* TS_SERVER   = "api.thingspeak.com";
const char* TS_WRITEKEY = "YOUR_THINGSPEAK_WRITE_KEY";
static const unsigned long TS_MIN_INTERVAL_MS = 16000; // >= 15s

// ---- Adafruit IO (fill yours) ----
const char* AIO_SERVER   = "io.adafruit.com";
const char* AIO_USERNAME = "YOUR_ADAFRUIT_USERNAME";
const char* AIO_KEY      = "YOUR_ADAFRUIT_IO_KEY";
const char* AIO_FEED_KEY = "location"; // location feed key
static const unsigned long AIO_MIN_INTERVAL_MS = 2000;

// ---- LoRa payload: MUST match transmitter ----
struct Payload {
  float lat;
  float lon;
};

bool wifiOK = false;
unsigned long lastTSUpload  = 0;
unsigned long lastAIOUpload = 0;

static void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting to Wi-Fi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    wifiOK = true;
    Serial.print("Wi-Fi connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    wifiOK = false;
    Serial.println("Wi-Fi timeout. Continuing without cloud uploads.");
  }
}

static void sendToThingSpeak(float lat, float lon, int rssi) {
  if (!wifiOK || WiFi.status() != WL_CONNECTED) return;

  WiFiClient client;

  String url = "/update?api_key=" + String(TS_WRITEKEY);
  url += "&field1=" + String(lat, 6);
  url += "&field2=" + String(lon, 6);
  url += "&field3=" + String(rssi);

  if (!client.connect(TS_SERVER, 80)) {
    Serial.println("ThingSpeak connect failed.");
    return;
  }

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + TS_SERVER + "\r\n" +
               "Connection: close\r\n\r\n");

  client.stop();
  Serial.println("ThingSpeak upload sent.");
}

static void sendToAdafruitLocation(float lat, float lon) {
  if (!wifiOK || WiFi.status() != WL_CONNECTED) return;

  WiFiClient client;

  // JSON for location feed
  String json = "{";
  json += "\"value\":0,";
  json += "\"lat\":" + String(lat, 6) + ",";
  json += "\"lon\":" + String(lon, 6);
  json += "}";

  String url = "/api/v2/";
  url += AIO_USERNAME;
  url += "/feeds/";
  url += AIO_FEED_KEY;
  url += "/data";

  if (!client.connect(AIO_SERVER, 80)) {
    Serial.println("Adafruit IO connect failed.");
    return;
  }

  client.print("POST " + url + " HTTP/1.1\r\n");
  client.print("Host: " + String(AIO_SERVER) + "\r\n");
  client.print("Content-Type: application/json\r\n");
  client.print("X-AIO-Key: " + String(AIO_KEY) + "\r\n");
  client.print("Content-Length: " + String(json.length()) + "\r\n");
  client.print("Connection: close\r\n\r\n");
  client.print(json);

  client.stop();
  Serial.println("Adafruit IO location upload sent.");
}

void setup() {
  Serial.begin(9600);
  delay(500);

  connectWiFi();

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(915E6)) {
    Serial.println("LoRa init failed!");
    while (true) { delay(1000); }
  }

  Serial.println("LoRa RX ready.");
}

void loop() {
  int packetSize = LoRa.parsePacket();

  if (packetSize == (int)sizeof(Payload)) {
    Payload pkt{};
    LoRa.readBytes((uint8_t*)&pkt, sizeof(pkt));
    int rssi = LoRa.packetRssi();

    Serial.print("RX: ");
    Serial.print(pkt.lat, 6);
    Serial.print(", ");
    Serial.print(pkt.lon, 6);
    Serial.print("  RSSI: ");
    Serial.println(rssi);

    unsigned long now = millis();

    if (now - lastAIOUpload >= AIO_MIN_INTERVAL_MS) {
      sendToAdafruitLocation(pkt.lat, pkt.lon);
      lastAIOUpload = now;
    }

    if (now - lastTSUpload >= TS_MIN_INTERVAL_MS) {
      sendToThingSpeak(pkt.lat, pkt.lon, rssi);
      lastTSUpload = now;
    }
  }
}
