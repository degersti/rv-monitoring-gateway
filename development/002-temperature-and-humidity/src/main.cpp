#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"

// Definition der Wunsch-Pins
#define I2C_SDA 40
#define I2C_SCL 41

Adafruit_SHT31 sht31 = Adafruit_SHT31();

void setup() {
  Serial.begin(115200);
  delay(3000);

  Serial.println();
  Serial.println("SHT31 Test mit ESP32-S3");

  Wire.begin(I2C_SDA, I2C_SCL); 

  if (!sht31.begin(0x44)) { 
    Serial.println("Sensor nicht gefunden!");
    while (1) delay(1000);
  }

  Serial.println("Sensor gefunden!");
}

void loop() {
  float t = sht31.readTemperature();
  float h = sht31.readHumidity();
  
  if (!isnan(t)) {
    Serial.printf("Temp: %.2f °C | Feuchte: %.2f %%\n", t, h);
  }
  delay(2000);
}