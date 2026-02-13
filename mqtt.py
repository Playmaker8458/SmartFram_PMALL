from dotenv import dotenv_values
import paho.mqtt.client as mqtt
import pymongo
import json

env_value = dotenv_values(".env")

# กำหนดตัวเข้าถึง MQTT Server
BROKER = env_value["BROKER_MQTT"]
PORT = int(env_value["PORT_MQTT"])
USERNAME = env_value["USERNAME_MQTT"]
PASSWORD = env_value["PASSWORD_MQTT"]

# กำหนดหัวข้อที่จะส่ง โดยเลือกหัวข้อเป็น ESP32 สำหรับบันทึกข้อมูลในนั้น 
TOPIC = "esp32/ESP32_001/message"

# เชื่อมต่อ MongoDB
myclient = pymongo.MongoClient(env_value["URL_MOGODB_CONNECTION"])
mydb = myclient["SmartFram"] # เข้าถึงฐานข้อมูล SmartFram
table = mydb["PM_ALL"] # เข้าถึงตาราง PM_ALL

# เชื่อมต่อ MQTT Server
def on_connect(client, userdata, flags, reason_code, properties):
    print("Connected with reason code:", reason_code)
    client.subscribe(TOPIC)

# ดึงข้อมูลจาก MQTT 
def on_message(client, userdata, msg):
    payload = msg.payload.decode()
    # แปลง dictionary ที่เป็น string ให้เป็น dictionary
    res_payload = json.loads(payload)
    table.insert_one(res_payload) # เพิ่มข้อมูลค่าฝุ่นไปเก็บ ในตาราง PM_ALL

    print(f"payload: {payload}")

client = mqtt.Client(
    mqtt.CallbackAPIVersion.VERSION2
)

client.username_pw_set(USERNAME, PASSWORD)
client.on_connect = on_connect
client.on_message = on_message

client.connect(BROKER, PORT, 60)
client.loop_forever()