#define TURBIDITY_PIN 35  // GPIO yang dipakai untuk turbidity sensor

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);  // Untuk ESP32: 0 - 4095
  Serial.println("Cek Sensor Turbidity Siap...");
}

void loop() {
  int adcValue = analogRead(TURBIDITY_PIN);
  float voltase = adcValue * (3.3 / 4095.0);  // Konversi ke voltase

  Serial.print("ADC Value: ");
  Serial.print(adcValue);
  Serial.print(" | Tegangan: ");
  Serial.print(voltase, 3);
  Serial.println(" V");

  delay(1000);  // Delay 1 detik
}
