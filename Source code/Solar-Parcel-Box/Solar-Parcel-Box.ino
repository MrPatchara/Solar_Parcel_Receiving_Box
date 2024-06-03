#include <EEPROM.h>
#include <TridentTD_LineNotify.h>
#include <WiFiManager.h>
#include <DNSServer.h>

#define LINE_TOKEN  "qeZLKwTAb4nN6rCvBtl7c4d5cBzbmnQjtEDy8rSL0a3" 

int Reset_wifi = 16; //D0 

void ICACHE_RAM_ATTR MailInt();
void ICACHE_RAM_ATTR DoorOpenInt();

unsigned int mail_counter;
bool flag_send_notification;
bool flag_send_door_open;

// -------------------- READ PARCEL COUNTER ----------------------
void read_mail_counter()
{
  unsigned char data1, data2;

  data1 = EEPROM.read(0);
  data2 = EEPROM.read(1);

  mail_counter = (data1 << 8) | data2;
  Serial.println("Mail counter: " + String(mail_counter));
}

// -------------------- WRITE PARCEL COUNTER -----------------------
void write_mail_counter()
{
  EEPROM.write(0, (unsigned char)(mail_counter >> 8));
  EEPROM.write(1, (unsigned char)(mail_counter & 0x0F));

  if (EEPROM.commit()) 
  {
    Serial.println("EEPROM successfully committed");
  } 
  else 
  {
    Serial.println("ERROR! EEPROM commit failed");
  }
}

// -------------------- GET PARCEL INTERRUPT  ----------------------
unsigned long ms_buf;
void MailInt()
{
  unsigned long time_ms = millis();
  unsigned long ms_dif = time_ms - ms_buf;

  ms_buf = time_ms;


  if ( ms_dif >= 3000 && digitalRead(D2) ) // 3 seconds
  { 
    mail_counter++;
    write_mail_counter();
    Serial.println("Get mail: " + String(mail_counter));

    flag_send_notification = 1;
  }
}

// -------------------- DOOR OPEN INTERRUPT  ----------------------
void DoorOpenInt()
{
  if (mail_counter != 0)
  {
    mail_counter = 0;
    write_mail_counter();
    flag_send_door_open = 1;
  }

  Serial.println("Door Open");
}

// -------------------- INITIALIZATION  ----------------------
void setup() 
{
  Serial.begin(115200); 
  Serial.println();
  Serial.println(LINE.getVersion());

  pinMode(D1, INPUT_PULLUP);
  pinMode(D2, INPUT_PULLUP);
  pinMode(D4, OUTPUT);

  pinMode(Reset_wifi, INPUT_PULLUP); // กําหนด pin reset wifi button
  WiFiManager wifiManager;
  for(int i = 5; i>0; i--) 
  {
  delay(1000);
  }
  wifiManager.autoConnect("Inw Parcel Receiving Box"); 

  attachInterrupt(digitalPinToInterrupt(D1), MailInt, FALLING);
  attachInterrupt(digitalPinToInterrupt(D2), DoorOpenInt, FALLING);

  EEPROM.begin(512);
  read_mail_counter();

  // กำหนด Line Token
  LINE.setToken(LINE_TOKEN);

  // ส่งข้อความเมื่อเชื่อมต่อ WiFi สำเร็จแล้ว
  LINE.notify("เชื่อมต่อ WiFi แล้ว");
  LINE.notify(" ยินดีต้อนรับสู่ตู้รับพัสดุขั้นเทพ v1.0 โดย นายพัชระ อัลอุมารี");
  LINE.notifyPicture("https://cdn-icons-png.flaticon.com/256/3595/3595843.png");
  LINE.notify(" สถานะ -> พร้อมรับพัสดุ");
  
}

// -------------------- LOOP  ----------------------
void loop() 
{
  WiFiManager wifiManager;
  if( digitalRead(Reset_wifi) == LOW) 
  {
    Serial.println("Reset wifi");
    wifiManager.resetSettings();
  }

  // Send notification -> getting mail
  if (flag_send_notification)
  {
    flag_send_notification = 0;
    LINE.notifyPicture("https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcTmuKmfaUqYCQoEBO0cEkdbUju8jXIZ3FARHEpqNvWIATAgYsA_Dvxm1VYtPoD7_47nFBA&usqp=CAU");
    LINE.notify(" มีพัสดุมาส่ง " + String(mail_counter) + " ชิ้น");
  }

  // Send notification -> door open
  if (flag_send_door_open)
  {
    flag_send_door_open = 0;
    LINE.notify("ถูกเปิดไม่มีพัสดุค้างในตู้แล้ว");
    LINE.notifySticker(8522, 16581289);
  }
  
  delay(1);
}
