#include <Arduino.h>

const int ADC_PIN = 34;   // GPIO34 = ADC input only, good for analog readings

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("RV Monitoring Gateway");
    Serial.println("001 - Voltage Monitoring Raw ADC Test");

    analogReadResolution(12); // ESP32 ADC range: 0...4095
}

void loop()
{
    int rawValue = analogRead(ADC_PIN);

    Serial.print("ADC raw value on GPIO");
    Serial.print(ADC_PIN);
    Serial.print(": ");
    Serial.println(rawValue);

    delay(1000);
}