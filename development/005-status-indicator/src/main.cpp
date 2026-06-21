#include <Adafruit_NeoPixel.h>

#define RGB_LED_PIN 48
#define NUM_PIXELS 1

Adafruit_NeoPixel pixel(NUM_PIXELS, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);

void setup()
{
    pixel.begin();
    pixel.clear();
    pixel.show();
}

void loop()
{
    // Rot
    pixel.setPixelColor(0, pixel.Color(255, 0, 0));
    pixel.show();
    delay(1000);

    // Grün
    pixel.setPixelColor(0, pixel.Color(0, 255, 0));
    pixel.show();
    delay(1000);

    // Blau
    pixel.setPixelColor(0, pixel.Color(0, 0, 255));
    pixel.show();
    delay(1000);

    // Aus
    pixel.clear();
    pixel.show();
    delay(1000);
}