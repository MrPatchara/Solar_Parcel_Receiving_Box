#include <EEPROM.h>
#include <WiFiManager.h>
#include <DNSServer.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#define TELEGRAM_BOT_TOKEN  ""  // ใส่ Token ของ Telegram Bot
#define TELEGRAM_CHAT_ID    ""  // ใส่ Chat ID ที่จะส่งข้อความหา

int Reset_wifi = 16; //D0 

void ICACHE_RAM_ATTR MailInt();
void ICACHE_RAM_ATTR DoorOpenInt();

unsigned int mail_counter;
bool flag_send_notification;
bool flag_send_door_open;

// -------------------- READ PARCEL COUNTER ----------------------
void read_mail_counter() {
  unsigned char data1, data2;
  data1 = EEPROM.read(0);
  data2 = EEPROM.read(1);
  mail_counter = (data1 << 8) | data2;
  Serial.println("Mail counter: " + String(mail_counter));
}

// -------------------- WRITE PARCEL COUNTER -----------------------
void write_mail_counter() {
  EEPROM.write(0, (unsigned char)(mail_counter >> 8));
  EEPROM.write(1, (unsigned char)(mail_counter & 0x0F));
  if (EEPROM.commit()) {
    Serial.println("EEPROM successfully committed");
  } else {
    Serial.println("ERROR! EEPROM commit failed");
  }
}

// -------------------- SEND TELEGRAM MESSAGE ----------------------
void sendTelegramMessage(String message) {
  WiFiClientSecure client;
  HTTPClient https;

  client.setInsecure();  // Skip SSL verification

  String url = "https://api.telegram.org/bot" + String(TELEGRAM_BOT_TOKEN) + "/sendMessage";
  url += "?chat_id=" + String(TELEGRAM_CHAT_ID);
  url += "&text=" + message;

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

// -------------------- GET PARCEL INTERRUPT  ----------------------
unsigned long ms_buf;
void MailInt() {
  unsigned long time_ms = millis();
  unsigned long ms_dif = time_ms - ms_buf;
  ms_buf = time_ms;

  if (ms_dif >= 3000 && digitalRead(D2)) { // 3 seconds
    mail_counter++;
    write_mail_counter();
    Serial.println("Get mail: " + String(mail_counter));
    flag_send_notification = 1;
  }
}

// -------------------- DOOR OPEN INTERRUPT  ----------------------
void DoorOpenInt() {
  if (mail_counter != 0) {
    mail_counter = 0;
    write_mail_counter();
    flag_send_door_open = 1;
  }
  Serial.println("Door Open");
}

// -------------------- INITIALIZATION  ----------------------
void setup() {
  Serial.begin(115200);
  Serial.println();

  pinMode(D1, INPUT_PULLUP);
  pinMode(D2, INPUT_PULLUP);
  pinMode(D4, OUTPUT);
  pinMode(Reset_wifi, INPUT_PULLUP); // กําหนด pin reset wifi button

  WiFiManager wifiManager;
  for (int i = 5; i > 0; i--) {
    delay(1000);
  }
  wifiManager.autoConnect("Inw Parcel Receiving Box");

  attachInterrupt(digitalPinToInterrupt(D1), MailInt, FALLING);
  attachInterrupt(digitalPinToInterrupt(D2), DoorOpenInt, FALLING);

  EEPROM.begin(512);
  read_mail_counter();

  // ส่งข้อความเมื่อเชื่อมต่อ WiFi สำเร็จแล้ว
  sendTelegramMessage("✅ เชื่อมต่อ WiFi แล้ว");
  sendTelegramMessage("📦 ยินดีต้อนรับสู่ตู้รับพัสดุขั้นเทพ v1.0 โดย นายพัชระ อัลอุมารี");
  sendTelegramMessage("📦 สถานะ -> พร้อมรับพัสดุ");
}

// -------------------- LOOP  ----------------------
void loop() {
  WiFiManager wifiManager;
  if (digitalRead(Reset_wifi) == LOW) {
    Serial.println("Reset WiFi");
    wifiManager.resetSettings();
  }

  // Send notification -> getting mail
  if (flag_send_notification) {
    flag_send_notification = 0;
    sendTelegramMessage("📦 มีพัสดุมาส่ง " + String(mail_counter) + " ชิ้น");
  }

  // Send notification -> door open
  if (flag_send_door_open) {
    flag_send_door_open = 0;
    sendTelegramMessage("🚪 ถูกเปิด ไม่มีพัสดุค้างในตู้แล้ว");
  }

  delay(1);
}
