#define TURBIDITY_PIN 35

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);  // 0–4095 untuk ESP32
  Serial.println("Kalibrasi Sensor Turbidity Dimulai...");
}

void loop() {
  int adcValue = analogRead(TURBIDITY_PIN);
  float voltase = adcValue * (3.3 / 4095.0);  // Tegangan sensor

  // Konversi tegangan ke NTU (dibalik logika)
  float ntu = (3.3 - voltase) * 3000.0;

  if (ntu < 0) ntu = 0;

  // Interpretasi NTU


  // Tampilkan
  Serial.print("ADC: ");
  Serial.print(adcValue);
  Serial.print(" | Volt: ");
  Serial.print(voltase, 3);
  Serial.print(" V | NTU: ");
  Serial.println(ntu, 2);

  delay(1000);
}
