#include <Wire.h>


float calibration_value = 21.34 + 1.5;
unsigned long int avgval;
int buffer_arr[10], temp;
float ph_act;

// Tentukan pin analog untuk sensor pH
#define PH_SENSOR_PIN 34   // GPIO34 contoh, bisa ganti ke pin analog lain

void setup() {
  Wire.begin();
  Serial.begin(115200);   // Baudrate umum ESP32

  Serial.print("pH Sensor Ready");
  delay(2000);
}

void loop() {
  // Baca data analog
  for (int i = 0; i < 10; i++) {
    buffer_arr[i] = analogRead(PH_SENSOR_PIN);
    delay(30);
  }

  // Urutkan data (bubble sort)
  for (int i = 0; i < 9; i++) {
    for (int j = i + 1; j < 10; j++) {
      if (buffer_arr[i] > buffer_arr[j]) {
        temp = buffer_arr[i];
        buffer_arr[i] = buffer_arr[j];
        buffer_arr[j] = temp;
      }
    }
  }

  // Ambil rata-rata dari data tengah
  avgval = 0;
  for (int i = 2; i < 8; i++)
    avgval += buffer_arr[i];

  // Konversi ke voltase ESP32
  float volt = (float)avgval * 3.3 / 4095.0 / 6;
  ph_act = -5.70 * volt + calibration_value;

  // Tampilkan di Serial Monitor
  Serial.print("pH Value: ");
  Serial.println(ph_act);

  // Tampilkan di LCD

  Serial.print("pH Value:");
  Serial.println(ph_act, 2);    // 2 angka di belakang koma

  delay(1000); // Delay untuk kestabilan pembacaan
}
