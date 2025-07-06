#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// --- WiFi & MQTT ---
const char* ssid = "Konsol";
const char* password = "20011116";
const char* mqtt_server = "5c2a2c9f7fc5480cbc20a18da313b797.s2.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "hivemq.webclient.1751739498481";
const char* mqtt_password = "80CwRKYB@d4f2Z$v,c<n";

WiFiClientSecure espClient;
PubSubClient client(espClient);

// --- MQTT Topics ---
const char* topic_temp = "water/temperature";
const char* topic_ph = "water/ph";
const char* topic_tds = "water/tds";
const char* topic_turbidity = "water/turbidity";

// --- Pins ---
#define ONE_WIRE_BUS 21
#define PH_SENSOR_PIN 35
#define TURBIDITY_PIN 33
#define TDS_SENSOR_PIN 32

// --- DS18B20 ---
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

// --- TDS Variables ---
#define VREF 3.3
#define SCOUNT 30
int analogBuffer[SCOUNT];
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
int copyIndex = 0;
float averageVoltage = 0;
float tdsValue = 0;

// --- Timer untuk TDS ---
unsigned long analogSampleTimepoint = 0;
unsigned long tdsProcessTimepoint = 0;

// --- PH ---
int buffer_arr[10];
unsigned long int avgval;

// --- Fungsi Kalibrasi Linear dari data pengujian ---
float kalibrasiLinear(float sensorTDS) {
  return 1.037 * sensorTDS;
}

// --- Fungsi WiFi ---
void setup_wifi() {
  Serial.print("Menghubungkan WiFi: ");
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
      delay(3000);
    }
  }
}

// --- Fungsi Sensor ---
float readTemperature() {
  ds18b20.requestTemperatures();
  return ds18b20.getTempCByIndex(0);
}

float readPH() {
  for (int i = 0; i < 10; i++) {
    buffer_arr[i] = analogRead(PH_SENSOR_PIN);
    delay(30);
  }

  // Sorting array
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
  for (int i = 2; i < 8; i++) avgval += buffer_arr[i];
  float volt = (float)avgval * 3.3 / 4095.0 / 6.0;
  return -6.19 * volt + 25.39;
}

int getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++) bTab[i] = bArray[i];
  int bTemp;
  for (int j = 0; j < iFilterLen - 1; j++) {
    for (int i = 0; i < iFilterLen - j - 1; i++) {
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
  if (millis() - analogSampleTimepoint > 40U) {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TDS_SENSOR_PIN);
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT) analogBufferIndex = 0;
  }

  if (millis() - tdsProcessTimepoint > 800U) {
    tdsProcessTimepoint = millis();
    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++) {
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
    }

    int medianADC = getMedianNum(analogBufferTemp, SCOUNT);
    averageVoltage = medianADC * (float)VREF / 4095.0;

    // --- Tambahan log dan deteksi sensor tidak terendam air
    Serial.print("ADC Median: ");
    Serial.print(medianADC);
    Serial.print(", Tegangan: ");
    Serial.print(averageVoltage, 3);
    Serial.print(" V, ");

    if (averageVoltage < 0.3) {
      tdsValue = 0.0;
      return tdsValue;
    }

    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
    float compensationVoltage = averageVoltage / compensationCoefficient;

    float rawTDS = (133.42 * pow(compensationVoltage, 3)
                  - 255.86 * pow(compensationVoltage, 2)
                  + 857.39 * compensationVoltage) * 0.5;

    tdsValue = kalibrasiLinear(rawTDS);
    if (tdsValue < 0) tdsValue = 0;
  }

  return tdsValue;
}

float readTurbidity() {
  int adcValue = analogRead(TURBIDITY_PIN);
  float voltage = adcValue * (3.3 / 4095.0);
  float ntu = 0.0;

  // Kalibrasi linear berdasarkan uji empiris untuk 0–10 NTU
  if (voltage >= 2.5 && voltage <= 3.0) {
    // Linear mapping: jernih → NTU 0
    ntu = 0.0;
  } else if (voltage >= 1.5 && voltage < 2.5) {
    // Semakin keruh, NTU naik
    ntu = (2.5 - voltage) * (10.0 / (2.5 - 1.5));  // naik dari 0 ke 10
  } else if (voltage < 1.5) {
    ntu = 10.0; // sangat keruh
  }

  return ntu;
}



void setup() {
  Serial.begin(115200);
  setup_wifi();
  analogReadResolution(12);
  pinMode(TDS_SENSOR_PIN, INPUT);
  client.setServer(mqtt_server, mqtt_port);
  ds18b20.begin();
}

void loop() {
  float temp = readTemperature();
  float tds = readTDS(temp);
  float ph = readPH();
  float turbidity = readTurbidity();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  client.publish(topic_temp, String(temp, 2).c_str());
  client.publish(topic_tds, String(tds, 2).c_str());
  client.publish(topic_ph, String(ph, 2).c_str());
  client.publish(topic_turbidity, String(turbidity, 2).c_str());

  Serial.print("Temp: "); Serial.print(temp); Serial.print(" °C, ");
  Serial.print("TDS: "); Serial.print(tds); Serial.print(" ppm, ");
  Serial.print("pH: "); Serial.print(ph); Serial.print(", ");
  Serial.print("NTU: "); Serial.println(turbidity);

  delay(1000);
}
