import paho.mqtt.client as paho
from paho import mqtt
import os
import time
from datetime import datetime, timedelta
import ssl
import certifi
from dotenv import load_dotenv
from app import socketio  # Untuk emit ke client
from app.src.services.notification_service import notify_sensor_data_Service
from flask import current_app
from app.src.services.fuzzy_service import cek_kelayakan_air
# Load ENV
load_dotenv()

# Konfigurasi MQTT
BROKER = os.environ.get('MQTT_BROKER', '')
PORT = 8883
USERNAME = os.environ.get('MQTT_USERNAME')
PASSWORD = os.environ.get('MQTT_PASSWORD')
FLASK_URL = os.environ.get('FLASK_URL')

# Topik MQTT
TOPICS = [
    ("water/temperature", 1),
    ("water/tds", 1),
    ("water/ph", 1),
    ("water/turbidity", 1),
]

# Data terbaru
latest_sensor_data = {
    "PH": None,
    "TDS": None,
    "Suhu": None,
    "Turbidity": None
}

sensor_last_seen = {k: None for k in latest_sensor_data}
sensor_last_value = {k: None for k in latest_sensor_data}
sensor_last_change = {k: None for k in latest_sensor_data}
sensor_alert_sent = {k: False for k in latest_sensor_data}

# Parameter
check_times = [1, 5, 15, 24]
SENSOR_RESET_HOURS = 24
SENSOR_STALE_SECONDS = 60

app_context = None
client = None

# MQTT CONNECT
def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("✅ Terhubung ke MQTT Broker")
        for topic, qos in TOPICS:
            client.subscribe(topic, qos)
            print(f"📡 Subscribed: {topic}")
    else:
        print(f"❌ Gagal koneksi MQTT, kode: {rc}")

# MQTT MESSAGE HANDLER
def handle_sensor_data(client, userdata, msg):
    topic = msg.topic
    value = msg.payload.decode('utf-8').strip()

    mapping = {
        "water/ph": "PH",
        "water/tds": "TDS",
        "water/temperature": "Suhu",
        "water/turbidity": "Turbidity"
    }

    label = mapping.get(topic)
    if not label:
        return

    try:
        value_float = float(value)
    except ValueError:
        print(f"❌ Nilai tidak valid dari {topic}: {value}")
        return

    now = datetime.now()

    # Simpan data
    if sensor_last_value[label] != value_float:
        sensor_last_change[label] = now
        sensor_last_value[label] = value_float

    latest_sensor_data[label] = value_float
    sensor_last_seen[label] = now

    # Emit semua data sensor
    socketio.emit("sensor_update", latest_sensor_data)

    # Emit kelayakan jika semua sensor terisi (None bukan 0.0!)
    if all(val is not None for val in latest_sensor_data.values()):
        hasil = cek_kelayakan_air(
            latest_sensor_data["PH"],
            latest_sensor_data["TDS"],
            latest_sensor_data["Turbidity"],
            latest_sensor_data["Suhu"]
        )
        socketio.emit("air_status", {"kelayakan": hasil})
        print(f"💧 Status Kelayakan Air Realtime: {hasil}")

# CEK SENSOR STALE
def check_sensor_status():
    now = datetime.now()
    updated = False

    for label, last_seen in sensor_last_seen.items():
        last_change = sensor_last_change.get(label)
        val = latest_sensor_data[label]

        # Reset data jika tidak berubah selama 24 jam
        if last_change and (now - last_change).total_seconds() > SENSOR_RESET_HOURS * 3600:
            if val is not None:
                latest_sensor_data[label] = None
                updated = True
                print(f"⚠️ Data {label} tidak berubah 24 jam, direset.")

        # Kirim peringatan jika 1 menit tidak berubah
        if last_seen and (now - last_seen).total_seconds() > SENSOR_STALE_SECONDS:
            if not sensor_alert_sent[label]:
                notify_sensor_data_Service(
                    f"🚨 Peringatan: Sensor {label} tidak berubah lebih dari 1 menit.\n"
                    f"Cek sinyal/perangkat.\nDashboard: {FLASK_URL}",
                    app_context
                )
                sensor_alert_sent[label] = True
                print(f"🚨 WARNING: {label} stagnan >1 menit")

    if updated:
        socketio.emit("sensor_update", latest_sensor_data)

# Waktu jadwal berikutnya
def init_next_check_times():
    now = datetime.now()
    next_times = {}
    for jam in check_times:
        t = now.replace(hour=jam % 24, minute=0, second=0, microsecond=0)
        if t <= now:
            t += timedelta(days=1)
        next_times[jam] = t
    return next_times

next_check_time = init_next_check_times()

# Jalankan service MQTT
def run_mqtt_service(app_instance):
    global app_context, client
    app_context = app_instance

    print(f"🔌 Menghubungkan ke MQTT {BROKER}:{PORT}")
    client = paho.Client(client_id="iot_water_quality", protocol=paho.MQTTv5)
    client.username_pw_set(USERNAME, PASSWORD)
    client.tls_set(ca_certs=certifi.where(), tls_version=ssl.PROTOCOL_TLS_CLIENT)

    client.on_connect = on_connect
    for topic, _ in TOPICS:
        client.message_callback_add(topic, handle_sensor_data)

    try:
        client.connect(BROKER, PORT)
        client.loop_start()
    except Exception as e:
        print(f"❌ Gagal koneksi: {e}")
        socketio.emit("mqtt_error", {"error": str(e)})
        return

    try:
        while True:
            now = datetime.now()
            for jam in check_times:
                if now >= next_check_time[jam]:
                    print(f"⏱️ Cek status sensor jam {jam}")
                    check_sensor_status()
                    next_check_time[jam] += timedelta(days=1)
            time.sleep(60)

    except KeyboardInterrupt:
        print("🛑 MQTT dimatikan")
        client.disconnect()
        client.loop_stop()