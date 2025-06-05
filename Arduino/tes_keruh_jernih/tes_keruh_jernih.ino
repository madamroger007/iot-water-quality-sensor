#define BLYNK_TEMPLATE_ID "TMPL6lKGSEhEV"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN "XfnCt9mT84T0KWsnZVw_B1hk6dhjWmSi"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>


char ssid[] = "Konsol";
char pass[] = "20011116..";

const int tdsPin = 33;       // Pin untuk sensor TDS
const int turbidityPin = 25; // Pin untuk sensor turbidity

void setup() {
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass);
  pinMode(tdsPin, INPUT);
  pinMode(turbidityPin, INPUT);
}

void loop() {
  Blynk.run();

  int tdsValue = analogRead(tdsPin);
  int turbidityValue = analogRead(turbidityPin);

  // Kirim data ke aplikasi Blynk
  Blynk.virtualWrite(V1, tdsValue);
  Blynk.virtualWrite(V2, turbidityValue);

  // Tampilkan data di Serial Monitor
  Serial.print("TDS: ");
  Serial.print(tdsValue);
  Serial.print(" | Turbidity: ");
  Serial.println(turbidityValue);

  delay(1000);
}
