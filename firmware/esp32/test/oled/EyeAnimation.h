#ifndef EYE_ANIMATION_H
#define EYE_ANIMATION_H
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <stdlib.h> // for random
// ==== MOCHI EYES ANIMATION CLASS ====
// Simple Mochi-style eyes like the Python reference: solid white blobs with x-offset animation and blink
class EyeAnimation {
private:
  int currentOffsetIndex = 0;
  unsigned long lastOffsetChange = 0;
  bool blinking = false;
  unsigned long blinkStart = 0;
  unsigned long nextBlinkTime = 0;
  
  // Offsets for look around
  static const int NUM_OFFSETS = 8;
  static const int offsets[NUM_OFFSETS];
  
  // Timing constants
  static const unsigned long OFFSET_INTERVAL = 150; // ms between offset changes
  static const unsigned long BLINK_DURATION = 200;  // ms for blink
  static const unsigned long MIN_BLINK_INTERVAL = 2000;
  static const unsigned long MAX_BLINK_INTERVAL = 4000;
  
  // Eye parameters
  static const int LEFT_CENTER_X = 40;
  static const int RIGHT_CENTER_X = 88;
  static const int EYE_CENTER_Y = 22;
  static const int EYE_RADIUS = 12;
  static const int EYE_WIDTH = 24; // for blink line
  
public:
  EyeAnimation() {}
  
  void begin() {
    randomSeed(analogRead(0));
    nextBlinkTime = millis() + random(MIN_BLINK_INTERVAL, MAX_BLINK_INTERVAL);
    currentOffsetIndex = 2; // Start at offset 0
  }
  
  void update() {
    unsigned long now = millis();
    
    if (blinking) {
      if (now - blinkStart >= BLINK_DURATION) {
        blinking = false;
        nextBlinkTime = now + random(MIN_BLINK_INTERVAL, MAX_BLINK_INTERVAL);
        currentOffsetIndex = 0; // Reset cycle after blink
      }
    } else {
      if (now >= nextBlinkTime) {
        blinking = true;
        blinkStart = now;
      }
      
      if (now - lastOffsetChange >= OFFSET_INTERVAL) {
        lastOffsetChange = now;
        currentOffsetIndex = (currentOffsetIndex + 1) % NUM_OFFSETS;
      }
    }
  }
  
  void draw(Adafruit_SSD1306 &display) {
    int offset = offsets[currentOffsetIndex];
    int leftX = LEFT_CENTER_X + offset;
    int rightX = RIGHT_CENTER_X + offset;
    int lineStartLeft = LEFT_CENTER_X - EYE_RADIUS + offset;
    int lineStartRight = RIGHT_CENTER_X - EYE_RADIUS + offset;
    
    if (blinking) {
      // Draw thick horizontal lines for closed eyes
      for (int thick = 0; thick < 3; thick++) {
        int y = EYE_CENTER_Y - 1 + thick;
        display.drawFastHLine(lineStartLeft, y, EYE_WIDTH, SSD1306_WHITE);
        display.drawFastHLine(lineStartRight, y, EYE_WIDTH, SSD1306_WHITE);
      }
    } else {
      // Draw solid white circles for open eyes
      display.fillCircle(leftX, EYE_CENTER_Y, EYE_RADIUS, SSD1306_WHITE);
      display.fillCircle(rightX, EYE_CENTER_Y, EYE_RADIUS, SSD1306_WHITE);
    }
  }
};

// Static array definition
const int EyeAnimation::offsets[NUM_OFFSETS] = {-4, -2, 0, 2, 4, 2, 0, -2};
#endif
