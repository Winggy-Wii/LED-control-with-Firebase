#include <FastLED.h>

#define LEFT_LED_PIN 13
#define RIGHT_LED_PIN 12
#define NUM_LEDS 15

// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100
#define COOLING 55

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 120
#define FRAMES_PER_SECOND 20

CRGB left_leds[NUM_LEDS];
CRGB right_leds[NUM_LEDS];

bool gReverseDirection = false;
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void ControlLED(int r, int g, int b, int br)
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    left_leds[i] = CRGB(r, g, b);
    right_leds[i] = CRGB(r, g, b);
    FastLED.setBrightness(br);
    FastLED.show();
  }
}

void fadeall(int num)
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    right_leds[i].nscale8(num);
    left_leds[i].nscale8(num);
  }
}

void Cylon(int br)
{
  static uint8_t hue = 0;
  Serial.print("x");
  // First slide the led in one direction
  for (int i = 0; i < NUM_LEDS; i++)
  {
    // Set the i'th led to red
    right_leds[i] = CHSV(hue++, 255, 255);
    left_leds[i] = CHSV(hue++, 255, 255);
    FastLED.setBrightness(br);
    // Show the leds
    FastLED.show();
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    fadeall(250);
    // Wait a little bit before we loop around and do it again
    FastLED.delay(1000 / 20);
  }
  Serial.print("x");

  // Now go in the other direction.
  for (int i = (NUM_LEDS)-1; i >= 0; i--)
  {
    // Set the i'th led to red
    right_leds[i] = CHSV(hue++, 255, 255);
    left_leds[i] = CHSV(hue++, 255, 255);
    FastLED.setBrightness(br);
    // Show the leds
    FastLED.show();
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    fadeall(250);
    // Wait a little bit before we loop around and do it again
    FastLED.delay(1000 / 20);
  }
}

void Wave(int r, int g, int b, int br)
{
  fadeall(0);
  FastLED.setBrightness(0);
  for (int i = 0; i < NUM_LEDS; i++)
  {
    // Set the i'th led to red
    right_leds[i] = CRGB(r, g, b);
    left_leds[i] = CRGB(r, g, b);
    FastLED.setBrightness(br);
    // Show the leds
    FastLED.show();
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    // Wait a little bit before we loop around and do it again
    FastLED.delay(1000 / 20);
  }
  for (int i = 0; i < NUM_LEDS; i++)
  {
    // Set the i'th led to red
    right_leds[i] = CRGB(0, 0, 0);
    left_leds[i] = CRGB(0, 0, 0);
    FastLED.setBrightness(br);
    // Show the leds
    FastLED.show();
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    // Wait a little bit before we loop around and do it again
    FastLED.delay(1000 / 20);
  }
}

void fade(int r, int g, int b, int br)
{

  for (int i = 0; i < NUM_LEDS; i++)
  {
    // Set the i'th led to red
    right_leds[i] = CRGB(r, g, b);
    left_leds[i] = CRGB(r, g, b);
    FastLED.setBrightness(br);
    // Show the leds
    FastLED.show();
    fadeall(200);
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    // Wait a little bit before we loop around and do it again
    FastLED.delay(1000 / 20);
  }
  for (int i = NUM_LEDS; i >= 0; i--)
  {
    // Set the i'th led to red
    right_leds[i] = CRGB(r, g, b);
    left_leds[i] = CRGB(r, g, b);
    FastLED.setBrightness(br);
    // Show the leds
    FastLED.show();
    fadeall(200);
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    // Wait a little bit before we loop around and do it again
    FastLED.delay(1000 / 20);
  }
}
