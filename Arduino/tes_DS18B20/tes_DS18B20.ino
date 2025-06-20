#include <Arduino.h>

#include <OneWire.h>
#include <DallasTemperature.h>

const int oneWireBus_DS18B20 = 34;
OneWire oneWire_DS18B20(oneWireBus_DS18B20);
DallasTemperature DS18B20(&oneWire_DS18B20);


float baca_DS18B20() {
  DS18B20.requestTemperatures();
  delay(100);
  float temperatureC = DS18B20.getTempCByIndex(0);
  return temperatureC;
}


void setup()
{
Serial.begin(9600);
DS18B20.begin();

}
void loop(){
 float nilai_DS18B20 = baca_DS18B20();
  if ( nilai_DS18B20 != DEVICE_DISCONNECTED_C) {
    Serial.print("Temperature: ");
    Serial.print( nilai_DS18B20);
    Serial.println(" °C");
  } else {
    Serial.println("Error: Could not read temperature data");
  }


}