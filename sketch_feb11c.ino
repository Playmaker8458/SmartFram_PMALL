#include <WiFi.h> 
#include <PubSubClient.h>

/* ===== WiFi ===== */
const char* ssid = "OPPO Reno10";
const char* password = "bank8458";

/* ===== MQTT ===== */
const char* mqtt_server = "119.59.99.233";
const int   mqtt_port   = 1883;
const char* mqtt_user   = "aiotchlorella";
const char* mqtt_pass   = "P@$$w0rd2025";
const char* topic       = "esp32/ESP32_001/message";

// กำหนดตัวแปลเก็บค่าฝุ่น โดยจะมี pm1.0, pm2.5 และ pm10.0
struct PMSData {
  uint16_t pm1_0;
  uint16_t pm2_5;
  uint16_t pm10_0;
};
// ดึงข้อมูลจากตัวแปรทั้งหมดมาเก็บลงใน pms
PMSData pms;

// กำหนดให้สามารถส่งข้อมูลผ่าน API เข้า MQTT Server
WiFiClient espClient;
// กำหนดให้สามารถส่งข้อมูล API แบบสาธารณะได้
PubSubClient client(espClient);

/* ===== Timer ===== */
unsigned long lastMsg = 0;
const long interval = 10000; // ส่งทุก 8 วินาที


void setup() {
  // กำหนดความเร็วในการส่งข้อมูลเป็น 9600 สำหรับส่งข้อมูลของ board ESP32
  Serial.begin(9600); 
  
  // กำหนดความเร็วในการรับข้อมูลเป็น 9600
  // กำหนดตัวกลางในการรับข้อมูลของ ตัวแปลงของ sensor เป็น SERIAL_8N1
  // กำหนดขาในการรับข้อมูลเป็น RX และ TX หรือ ขา 16 และ 17
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  // เชื่อมต่อ WIFI ผ่าน ESP32
  WiFi.mode(WIFI_STA);
  // กำหนดชื่อ WIFI และ รหัสผ่าน เพื่อให้ ESP32 สามารถเข้าถึงได้
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");

  // ถ้าต่อ Wifi ไม่สำเร็จจะแสดงข้อความ .
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(5000); // รันทุก 5 วินาที
  }
  // ถ้าต่อ Wifi สำเร็จจะแสดงข้อความ WiFi connected พร้อมโชว์ IP ของ WIFI ที่เราต่อออกมา
  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());

  /* MQTT setup */
  client.setServer(mqtt_server, mqtt_port);
}

// เชื่อมต่อ MQTT Server
void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting MQTT...");
    // ถ้าเชื่อมต่อ MQTT Server สำเร็จจะแสดงข้อความ connected
    if (client.connect("ESP32_001", mqtt_user, mqtt_pass)) {
      Serial.println("connected");
    } else {
      // ถ้าเชื่อมต่อ MQTT Server ไม่สำเร็จจะแสดงข้อความ failed พร้อมกับแสดงสถานะออกมา
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(5000); // รันทุกๆ 5 วินาที
    }
  }
}

// ใช้สำหร้บอ่านค่า Sensor ฝุ่น [byte ให้กลายเป็นฐาน 10]
boolean readPMSdata(Stream *s) {
  // 
  if (!s->available()) return false;
  if (s->peek() != 0x42) {
    s->read();
    return false;
  }
  if (s->available() < 32) return false;
  
  uint8_t buffer[32];
  s->readBytes(buffer, 32);

  if (buffer[1] != 0x4D) return false;

  // ดึงค่า PM และทำการแปลงเพื่อให้ดึงค่าจริงกลับออกมา [ค่าจริงคือ ฐาน 10]
  pms.pm1_0 = (buffer[10] << 8) | buffer[11];
  pms.pm2_5 = (buffer[12] << 8) | buffer[13];
  pms.pm10_0 = (buffer[14] << 8) | buffer[15];
  return true;
}

void loop() {
  // ถ้าหากไม่ได้เชื่อมต่อ [บังคับให้เชื่อมต่อทันที่]
  if (!client.connected()) {
    reconnect();
  }
  // ถ้าหากเชื่อมต่อแล้ว ก็จะให้ทำงานแบบปกติ
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > interval) {
    lastMsg = now;
    // เช็คข้อมูลของเซนเซอร์วัดค่าฝุ่น
    if (readPMSdata(&Serial2)) {

      int Sum_PMALL = pms.pm1_0 + pms.pm2_5 + pms.pm10_0;
      float Average_PM = Sum_PMALL / 3.0;
      
      // [ถ้ามี : ก็จะส่งข้อมูลใน PM1.0, PM2.5 และ PM10 ในรูปแบบ JSON กลับไปให้ MQTT Server]
      String payload = "{";
      payload += "\"pm1_0\":" + String(pms.pm1_0) + ",";
      payload += "\"pm2_5\":" + String(pms.pm2_5) + ",";
      payload += "\"pm10_0\":" + String(pms.pm10_0) + ",";
      payload += "\"Average PM ALL\":" + String(Average_PM);
      payload += "}";

      // ส่งค่าไปที่ MQTT Topic เพื่อให้สามารถตอบกลับออกไปอีกฝั่งนึงได้
      Serial.print("Publish message: ");
      Serial.println(payload); // โดยให้ส่งข้อมูล payload ที่เป็นค่าฝุ่นกลับออกไป
      client.publish(topic, payload.c_str());
    } else {
      Serial.println("Sensor not ready or data error");
    }
    
    // ทุกครั้งที่มีการส่งข้อมูลกลับออกไปก็จะให้ ล้างข้อมูลเดิมเพื่อไม่ให้ข้อมูลค้างอยู่ใน Sensor
    while(Serial2.available()) Serial2.read();
  }
}