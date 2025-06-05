#define BLYNK_TEMPLATE_ID "TMPL6lKGSEhEV"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN "XfnCt9mT84T0KWsnZVw_B1hk6dhjWmSi"

char ssid[] = "Konsol";
char pass[] = "20011116..";

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Pin sensor
#define PIN_TURBIDITY 34
#define PIN_PH        35
#define PIN_TDS       2
#define ONE_WIRE_BUS  4

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Fungsi fuzzy sederhana
String fuzzy_pH(float pH) {
  if (pH < 6.0) return "Asam";
  else if (pH <= 8.0) return "Netral";
  else return "Basa";
}

String fuzzy_TDS(float tds) {
  if (tds < 500) return "Baik";
  else if (tds <= 1000) return "Sedang";
  else return "Buruk";
}

String fuzzy_Kekeruhan(int value) {
  if (value < 300) return "Jernih";
  else if (value <= 700) return "Sedang";
  else return "Keruh";
}

String fuzzy_Suhu(float suhu) {
  if (suhu < 20) return "Dingin";
  else if (suhu <= 30) return "Normal";
  else return "Panas";
}

unsigned long lastSendTime = 0;
const unsigned long interval = 5000; // setiap 5 detik

void setup() {
  Serial.begin(115200);

  Serial.print("Menghubungkan ke WiFi...");
  WiFi.begin(ssid, pass);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi TERSAMBUNG: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi GAGAL TERHUBUNG!");
  }

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  sensors.begin();
}

void loop() {
  Blynk.run();

  if (millis() - lastSendTime >= interval) {
    lastSendTime = millis();

    // Baca suhu dari DS18B20
    sensors.requestTemperatures();
    float suhu = sensors.getTempCByIndex(0);
    bool suhu_ok = suhu != -127.0;

    // Baca pH dari ADC
    int ph_adc = analogRead(PIN_PH);
    float volt_ph = ph_adc * (3.3 / 4095.0);
    float ph_value = 3.5 * volt_ph;  // kalibrasi
    bool ph_ok = ph_adc > 50;

    // Baca TDS
    int tds_adc = analogRead(PIN_TDS);
    float volt_tds = tds_adc * (3.3 / 4095.0);
    float tds_value = (133.42 * pow(volt_tds, 3) - 255.86 * pow(volt_tds, 2) + 857.39 * volt_tds) * 0.5;
    bool tds_ok = tds_adc > 50;

    // Baca turbidity
    int turbidity = analogRead(PIN_TURBIDITY);
    bool turbidity_ok = turbidity > 50;

    // Kirim ke Blynk
    Blynk.virtualWrite(V0, PIN_PH);
    Blynk.virtualWrite(V1, tds_value);
    Blynk.virtualWrite(V2, turbidity);
    Blynk.virtualWrite(V3, suhu);

    // Serial output
    Serial.println("==== STATUS SENSOR ====");
    Serial.print("WiFi: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "TERHUBUNG" : "TIDAK TERHUBUNG");

    Serial.print("pH: "); Serial.print(ph_value); 
    Serial.print(" (" + fuzzy_pH(ph_value) + ")");
    Serial.println(ph_ok ? " [BERHASIL]" : " [GAGAL]");

    Serial.print("TDS: "); Serial.print(tds_value); 
    Serial.print(" ppm (" + fuzzy_TDS(tds_value) + ")");
    Serial.println(tds_ok ? " [BERHASIL]" : " [GAGAL]");

    Serial.print("Kekeruhan: "); Serial.print(turbidity); 
    Serial.print(" (" + fuzzy_Kekeruhan(turbidity) + ")");
    Serial.println(turbidity_ok ? " [BERHASIL]" : " [GAGAL]");

    Serial.print("Suhu: "); Serial.print(suhu); 
    Serial.print(" °C (" + fuzzy_Suhu(suhu) + ")");
    Serial.println(suhu_ok ? " [BERHASIL]" : " [GAGAL]");
    Serial.println("========================\n");
  }
}
