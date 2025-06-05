#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL6lKGSEhEV"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN "XfnCt9mT84T0KWsnZVw_B1hk6dhjWmSi"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <OneWire.h>
#include <DallasTemperature.h>

char ssid[] = "Konsol";
char pass[] = "20011116..";

#define PH_PIN 35       // Pin ADC untuk sensor pH
#define TEMP_PIN 4     // Pin digital untuk sensor DS18B20

OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

BlynkTimer timer;

float readTemperature() {
  sensors.requestTemperatures();
  float temperatureC = sensors.getTempCByIndex(0);
  return temperatureC;
}

float readPH() {
  const int samples = 10;
  int adcValue = 0;
  for (int i = 0; i < samples; i++) {
    adcValue += analogRead(PH_PIN);
    delay(10);
  }
  float voltage = (adcValue / (float)samples) * (3.3 / 4095.0);
  float pHValue = 7 + ((2.50 - voltage) / 0.18); // Kalibrasi pH
  return pHValue;
}

void sendSensorData() {
  float temperature = readTemperature();
  float pH = readPH();

  Serial.print("Suhu: ");
  Serial.print(temperature);
  Serial.print(" °C | pH: ");
  Serial.println(pH);

  Blynk.virtualWrite(V0, pH);
  Blynk.virtualWrite(V1, temperature);
}

void setup() {
  Serial.begin(9600);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  timer.setInterval(1000L, sendSensorData);
    sensors.begin();
}

void loop() {
  Blynk.run();
  timer.run();
}
