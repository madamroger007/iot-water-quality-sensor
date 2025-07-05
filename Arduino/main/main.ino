#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// --- WiFi & MQTT Konfigurasi ---
const char* ssid = "Konsol";
const char* password = "20011116";
const char* mqtt_server = "5c2a2c9f7fc5480cbc20a18da313b797.s2.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "hivemq.webclient.1751739498481";
const char* mqtt_password = "80CwRKYB@d4f2Z$v,c<n";

WiFiClientSecure espClient;
PubSubClient client(espClient);

// --- Topic MQTT ---
const char* topic_temp = "water/temperature";
const char* topic_ph = "water/ph";
const char* topic_tds = "water/tds";
const char* topic_turbidity = "water/turbidity";

// --- Pin Sensor ---
#define TDS_PIN 27
#define PH_SENSOR_PIN 34
#define TURBIDITY_PIN 35
#define ONE_WIRE_BUS 4

// --- DS18B20 Setup ---
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

// --- TDS ---
#define VREF 3.3
#define SCOUNT 30
int analogBuffer[SCOUNT];
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
int copyIndex = 0;
float averageVoltage = 0;
float tdsValue = 0;
float kalibrasi_tds = 1.50;

// --- PH ---
int buffer_arr[10];
unsigned long int avgval;

// --- Fungsi WiFi ---
void setup_wifi() {
  Serial.print("Menghubungkan WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi terhubung");
  espClient.setInsecure();
}

// --- Fungsi MQTT ---
void reconnect() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke MQTT...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_password)) {
      Serial.println("Terhubung");
    } else {
      Serial.print("Gagal, rc=");
      Serial.print(client.state());
      Serial.println(". Coba lagi dalam 3 detik...");
      delay(3000);
    }
  }
}

float readTemperature() {
  ds18b20.requestTemperatures();
  return ds18b20.getTempCByIndex(0);
}

float readPH() {
  for (int i = 0; i < 10; i++) {
    buffer_arr[i] = analogRead(PH_SENSOR_PIN);
    delay(30);
  }
  for (int i = 0; i < 9; i++) {
    for (int j = i + 1; j < 10; j++) {
      if (buffer_arr[i] > buffer_arr[j]) {
        int temp = buffer_arr[i];
        buffer_arr[i] = buffer_arr[j];
        buffer_arr[j] = temp;
      }
    }
  }
  avgval = 0;
  for (int i = 2; i < 8; i++) {
    avgval += buffer_arr[i];
  }
  float volt = (float)avgval * 3.3 / 4095.0 / 6.0;
  float phValue = -6.19 * volt + 25.39;
  return phValue;
}

int getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++) bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
   if ((iFilterLen & 1) > 0)
    return bTab[(iFilterLen - 1) / 2];
  else
    return (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
}

float readTDS(float temperature) {
  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40U) {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TDS_PIN);
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT) {
      analogBufferIndex = 0;
    }
  }

  static unsigned long printTimepoint = millis();
  if (millis() - printTimepoint > 800U) {
    printTimepoint = millis();

    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++) {
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
    }

    // Median voltage
    averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 4096.0;

    // Kompensasi suhu
    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
    float compensationVoltage = averageVoltage / compensationCoefficient;

    // Rumus TDS dasar
    tdsValue = (133.42 * pow(compensationVoltage, 3)
                - 255.86 * pow(compensationVoltage, 2)
                + 857.39 * compensationVoltage) * 0.5;
  tdsValue = tdsValue * kalibrasi_tds;
  return tdsValue;
}
}

float readTurbidity() {
  int adcValue = analogRead(TURBIDITY_PIN);
  float voltage = adcValue * (3.3 / 4095.0);
  float ntu = (3.3 - voltage) * 3000.0;
  if (ntu < 0) ntu = 0;
  return ntu;
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  pinMode(TDS_PIN, INPUT);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  ds18b20.begin();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float temp = readTemperature();
  float ph = readPH();
  float tds = readTDS(temp);
  float turbidity = readTurbidity();

  client.publish(topic_temp, String(temp, 2).c_str());
  client.publish(topic_ph, String(ph, 2).c_str());
  client.publish(topic_tds, String(tds, 2).c_str());
  client.publish(topic_turbidity, String(turbidity, 2).c_str());

  Serial.print("Temp: "); Serial.print(temp); Serial.print(" °C, ");
  Serial.print("pH: "); Serial.print(ph); Serial.print(", ");
  Serial.print("TDS: "); Serial.print(tds); Serial.print(" ppm, ");
  Serial.print("NTU: "); Serial.println(turbidity);

  delay(3000);
}