#include <EEPROM.h>

#define TdsSensorPin 27
#define VREF 3.3              // Tegangan referensi ESP32
#define SCOUNT 30             // Jumlah sampel analog

int analogBuffer[SCOUNT];
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
int copyIndex = 0;

float averageVoltage = 0;
float tdsValue = 0;
float temperature = 25; // Ganti dengan sensor suhu jika tersedia

float kalibrasi_tds = 1.0;  // Akan dimuat dari EEPROM

// EEPROM alamat penyimpanan kalibrasi
#define EEPROM_ADDR_K 0

// Median Filter
int getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];

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

// Simpan kalibrasi ke EEPROM
void simpanKalibrasi(float k) {
  EEPROM.put(EEPROM_ADDR_K, k);
  EEPROM.commit();
}

// Load kalibrasi dari EEPROM
float muatKalibrasi() {
  float k;
  EEPROM.get(EEPROM_ADDR_K, k);
  if (isnan(k) || k < 0.1 || k > 5.0) return 1.0; // default
  return k;
}

void setup() {
  Serial.begin(115200);
  pinMode(TdsSensorPin, INPUT);
  analogReadResolution(12);       // 12-bit untuk ESP32
  analogSetAttenuation(ADC_11db); // Untuk pembacaan tegangan 0–3.3V

  EEPROM.begin(8); // Inisialisasi EEPROM
  kalibrasi_tds = muatKalibrasi();
  Serial.println("Sistem TDS Siap. Ketik 'k=1.50' untuk kalibrasi faktor.");
}

void loop() {
  // --- Pembacaan Analog Sensor ---
  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40U) {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT) {
      analogBufferIndex = 0;
    }
  }

  // --- Hitung TDS dan Tampilkan ---
  static unsigned long printTimepoint = millis();
  if (millis() - printTimepoint > 1000U) {
    printTimepoint = millis();

    for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++) {
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
    }

    // Hitung rata-rata voltase
    averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 4096.0;

    // Kompensasi suhu
    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
    float compensationVoltage = averageVoltage / compensationCoefficient;

    // Rumus TDS
    tdsValue = (133.42 * pow(compensationVoltage, 3)
              - 255.86 * pow(compensationVoltage, 2)
              + 857.39 * compensationVoltage) * 0.5;

    // Kalibrasi akhir
    tdsValue *= kalibrasi_tds;

    // Output Serial
    Serial.print("TDS: ");
    Serial.print(tdsValue, 0);
    Serial.print(" ppm | k: ");
    Serial.println(kalibrasi_tds, 2);
  }

  // --- Cek Input Kalibrasi dari Serial ---
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.startsWith("k=")) {
      float newK = input.substring(2).toFloat();
      if (newK >= 0.1 && newK <= 5.0) {
        kalibrasi_tds = newK;
        simpanKalibrasi(newK);
        Serial.print("Faktor kalibrasi disimpan: ");
        Serial.println(newK, 2);
      } else {
        Serial.println("Nilai k tidak valid (0.1 - 5.0)");
      }
    }
  }
}
