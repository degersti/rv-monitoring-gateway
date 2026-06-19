#include <Arduino.h>

#define PIN_WATER 13
#define PIN_SMOKE 14

void setup()
{
    Serial.begin(115200);

    pinMode(PIN_WATER, INPUT_PULLUP);
    pinMode(PIN_SMOKE, INPUT_PULLUP);
}

void loop()
{
    Serial.print("Water: ");
    Serial.print(digitalRead(PIN_WATER));

    Serial.print("  Smoke: ");
    Serial.println(digitalRead(PIN_SMOKE));

    delay(500);
}