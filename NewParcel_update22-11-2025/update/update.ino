#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <DNSServer.h>

// ======= GPIO =======
#define IR_SENSOR_PIN  D1   // GPIO5
#define LED_PIN        D4   // GPIO2
#define RESET_WIFI_PIN 16   // D0

// ======= Telegram =======
#define TELEGRAM_BOT_TOKEN "8171906066:AAEBn_3Ha3bns7zaqEbGJn508o6dFJcFd44"
#define TELEGRAM_CHAT_ID   "-1002591941089"

// ======= Flags =======
volatile bool flag_send_notification = false;

// ======= Debounce =======
volatile unsigned long last_ir_trigger = 0;
const unsigned long IR_DEBOUNCE_MS = 3000;

// ======= ISR =======
void ICACHE_RAM_ATTR IR_ISR() {
  unsigned long now = millis();
  if (now - last_ir_trigger > IR_DEBOUNCE_MS) {
    last_ir_trigger = now;
    flag_send_notification = true;
  }
}

// ======= Telegram Send Function =======
void sendTelegramMessage(const String &message) {
  WiFiClientSecure client;
  HTTPClient https;

  client.setInsecure(); // Skip SSL verification

  String url = "https://api.telegram.org/bot" + String(TELEGRAM_BOT_TOKEN) +
               "/sendMessage?chat_id=" + String(TELEGRAM_CHAT_ID) +
               "&text=" + message;

  Serial.println("Sending Telegram Message: " + message);

  https.begin(client, url);
  int httpCode = https.GET();

  if (httpCode > 0) {
    Serial.println("Telegram Message Sent Successfully");
  } else {
    Serial.println("Telegram Message Failed: " + String(httpCode));
  }

  https.end();
}

// ======= Setup =======
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Parcel Box System Starting ===");

  pinMode(IR_SENSOR_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RESET_WIFI_PIN, INPUT_PULLUP);

  // WiFi Manager
  WiFiManager wifiManager;
  wifiManager.autoConnect("Home Parcel Box");

  // Attach ISR for IR sensor
  attachInterrupt(digitalPinToInterrupt(IR_SENSOR_PIN), IR_ISR, FALLING);

  // Welcome messages
  sendTelegramMessage("✅ WiFi Connected");
  sendTelegramMessage("📦 ระบบตู้รับพัสดุพร้อมทำงาน");
}

// ======= Loop =======
void loop() {
  // WiFi Reset
  static bool last_reset_state = HIGH;
  bool current_reset_state = digitalRead(RESET_WIFI_PIN);
  if (last_reset_state == HIGH && current_reset_state == LOW) {
    Serial.println("Resetting WiFi settings...");
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    delay(1000);
    ESP.restart();
  }
  last_reset_state = current_reset_state;

  // Check flag from ISR
  if (flag_send_notification) {
    flag_send_notification = false;
    Serial.println("Parcel detected! Sending Telegram...");
    sendTelegramMessage("📦 ตรวจพบพัสดุมาส่ง!");
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
  }

  delay(10); // small delay to prevent CPU hog
}
