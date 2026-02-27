#include <WiFi.h> 
#include <PubSubClient.h>

/* ===== WiFi ===== */
const char* ssid = "OPPO Reno10";
const char* password = "bank8458";

/* ===== MQTT ===== */
const char* mqtt_server = "119.59.99.233";
const int   mqtt_port = 1883;
const char* mqtt_user = "aiotchlorella";
const char* mqtt_pass = "P@$$w0rd2025";
const char* topic = "esp32/ESP32_001/message/SoilSensor"; 

/* ===== ขาเชื่อมต่อ Sensor ===== */
const int soilPin = 35;

// กำหนดให้สามารถส่งข้อมูลผ่าน API เข้า MQTT Server
WiFiClient espClient;
// กำหนดให้สามารถส่งข้อมูล API แบบสาธารณะได้
PubSubClient client(espClient);


void setup() {
  // กำหนดความเร็วในการส่งข้อมูลเป็น 9600 สำหรับส่งข้อมูลของ board ESP32
  Serial.begin(9600); 

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

void loop() {
  // ถ้าหากไม่ได้เชื่อมต่อ [บังคับให้เชื่อมต่อทันที่]
  if (!client.connected()) {
    reconnect();
  }
  // ถ้าหากเชื่อมต่อแล้ว ก็จะให้ทำงานแบบปกติ
  client.loop();


  // --- ส่วนอ่านค่าจาก Sensor และส่งข้อมูล ---
  // 1. อ่านค่า Analog
  int rawValue = analogRead(soilPin);

  // 2. แปลงเป็นเปอร์เซ็นต์
  int moisturePercent = map(rawValue, 3500, 1500, 0, 100);
  moisturePercent = constrain(moisturePercent, 0, 100);

  int dryVal = 0; // กำหนดค่าความแห่งของดินเป็น ค่าเริ่มต้น
  int moistVal = 0; // กำหนดค่าความชื้นของดินเป็น ค่าเริ่มต้น
  int wetVal = 0; // กำหนดค่าความเปี๊ยกของดินเป็น ค่าเริ่มต้น

    // ถ้าค่าของดินมันต่ำกว่า 30 แสดงว่าดินแห้ง (ต้องลดน้ำเพิ่ม)
  if (moisturePercent < 30) {
    dryVal = moisturePercent;
    // ถ้าค่าดินอยู่ในช่วง 30-70 แสดงว่าดินชื้น
  } else if (moisturePercent >= 30 && moisturePercent <= 70) {
    moistVal = moisturePercent;
    // ถ้าค่ามากกว่า 70 ขึ้นไป แสดงว่าดินเปี๊ยก
  } else {
    wetVal = moisturePercent;
  }

  // ส่งค่าดินทั้งหมดในรูปแบบ JSON กลับไปให้ MQTT Server เก็บ
  String payload = "{";
  payload += "\"Soil Dry\":" + String(dryVal) + ", ";
  payload += "\"Soil Moisture\":" + String(moistVal) + ", ";
  payload += "\"Soil Wet\":" + String(wetVal);
  payload += "}";
  
  // ส่งค่าไปที่ MQTT Topic เพื่อให้สามารถตอบกลับออกไปอีกฝั่ง API
  Serial.print("Publish message: ");
  Serial.println(payload); // โดยให้ส่งข้อมูล payload ที่เป็นค่าดินกลับออกไป
  client.publish(topic, payload.c_str()); 

  delay(5000); // ให้ทำงานทุก 5 วินาที
}