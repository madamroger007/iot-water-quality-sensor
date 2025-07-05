#include <Wire.h>

#define PH_SENSOR_PIN 34   // GPIO34 untuk sensor pH
int buffer_arr[10], temp;
unsigned long int avgval;
float ph_act;

void setup() {
  Wire.begin();
  Serial.begin(115200);   // Baudrate umum ESP32

  Serial.println("pH Sensor Ready");
  delay(2000);
}

void loop() {
  // Ambil 10 sample pembacaan analog
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

  // Ambil nilai tengah dari pembacaan untuk menghindari noise ekstrem
  avgval = 0;
  for (int i = 2; i < 8; i++) {
    avgval += buffer_arr[i];
  }

  // Konversi nilai ADC ke tegangan (volt)
  float volt = (float)avgval * 3.3 / 4095.0 / 6.0;

  // Hitung nilai pH berdasarkan hasil kalibrasi regresi linier
  ph_act = -6.19 * volt + 25.39;

  // Tampilkan data
  Serial.print("ADC: "); Serial.print(avgval / 6);
  Serial.print(" | Tegangan: "); Serial.print(volt, 3); Serial.print(" V");
  Serial.print(" | pH Value: "); Serial.println(ph_act, 2);

  delay(1000);
}
