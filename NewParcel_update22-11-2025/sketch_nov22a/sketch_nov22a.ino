#include <WiFiManager.h>
#include <DNSServer.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>


#define D1 1   // GPIO5 (IR Sensor)
#define D2 2   // GPIO4 (Door switch - ไม่ใช้แล้ว)
#define D4 4   // GPIO2

#define TELEGRAM_BOT_TOKEN  "8171906066:AAEBn_3Ha3bns7zaqEbGJn508o6dFJcFd44"
#define TELEGRAM_CHAT_ID    "-1002591941089"

int Reset_wifi = 16; //D0 

void ICACHE_RAM_ATTR MailInt();

bool flag_send_notification = false;

// ------------- ส่งข้อความ Telegram -------------
void sendTelegramMessage(String message) {
  WiFiClientSecure client;
  HTTPClient https;

  client.setInsecure();  // Skip SSL

  String url = "https://api.telegram.org/bot" + String(TELEGRAM_BOT_TOKEN)
               + "/sendMessage?chat_id=" + String(TELEGRAM_CHAT_ID)
               + "&text=" + message;

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

// ------------- INTERRUPT เมื่อมีพัสดุมา -------------
unsigned long ms_buf = 0;

void MailInt() {
  unsigned long now = millis();
  unsigned long diff = now - ms_buf;
  ms_buf = now;

  // กันเด้ง 3 วินาที (ตามโค้ดเดิม)
  if (diff >= 3000) {
    Serial.println("Parcel Detected!");
    flag_send_notification = true;
  }
}

// ------------- SETUP -------------
void setup() {
  Serial.begin(115200);
  Serial.println();

  pinMode(D1, INPUT_PULLUP);
  pinMode(D2, INPUT_PULLUP);   // ไม่ใช้ แต่ปล่อยให้เป็น input ได้
  pinMode(D4, OUTPUT);
  pinMode(Reset_wifi, INPUT_PULLUP);

  WiFiManager wifiManager;

  for (int i = 5; i > 0; i--) {
    delay(1000);
  }
  wifiManager.autoConnect("Inw Parcel Receiving Box");

  attachInterrupt(digitalPinToInterrupt(D1), MailInt, FALLING);

  sendTelegramMessage("✅ เชื่อมต่อ WiFi แล้ว");
  sendTelegramMessage("📦 ยินดีต้อนรับสู่ตู้รับพัสดุขั้นเทพ v1.0 โดย นายพัชระ อัลอุมารี");
  sendTelegramMessage("📦 สถานะ -> พร้อมทำงาน (ตรวจจับพัสดุ)");
}

// ------------- LOOP -------------
void loop() {
  WiFiManager wifiManager;

  // Reset WiFi
  if (digitalRead(Reset_wifi) == LOW) {
    Serial.println("Reset WiFi");
    wifiManager.resetSettings();
  }

  // เมื่อมีพัสดุ
  if (flag_send_notification) {
    flag_send_notification = false;
    sendTelegramMessage("📦 ตรวจพบพัสดุมาส่งแล้วครับ!");
  }

  delay(1);
}
