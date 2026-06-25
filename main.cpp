#include <Arduino.h>
#include <esp_sleep.h>
#include <Adafruit_NeoPixel.h>

RTC_DATA_ATTR uint32_t bootCount = 0;

constexpr uint8_t PIN_RGB_LED = 48;
Adafruit_NeoPixel rgbLed(1, PIN_RGB_LED, NEO_GRB + NEO_KHZ800);

void setRgb(uint8_t r, uint8_t g, uint8_t b)
{
    rgbLed.setPixelColor(0, rgbLed.Color(r, g, b));
    rgbLed.show();
}

void setup()
{
    rgbLed.begin();
    rgbLed.clear();
    rgbLed.show();
    Serial.begin(115200);

    delay(1000);

    bootCount++;

    Serial.println();
    Serial.println("================================");
    Serial.printf("Boot Count: %lu\n", bootCount);

    esp_sleep_wakeup_cause_t cause =
        esp_sleep_get_wakeup_cause();

    switch (cause)
    {
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("Wakeup by TIMER");
            setRgb(255, 0, 0);   // RED
            break;

        default:
            Serial.println("Normal startup");
            setRgb(0,0,225);    //BLUE
            break;
    }

    Serial.println("Going to sleep in 3 seconds...");
    Serial.flush();

    delay(3000);

    esp_sleep_enable_timer_wakeup(5ULL * 1000000ULL);

    Serial.println("Entering deep sleep");
    Serial.flush();

    delay(100);

    esp_deep_sleep_start();
}

void loop()
{
}