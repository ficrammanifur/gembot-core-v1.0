#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include "EyeAnimation.h"

// ==== OLED CONFIG ====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 8
#define OLED_SCL 9

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ==== EYE ANIMATION ====
EyeAnimation eyeAnim;

// ==== TIMING ====
unsigned long lastDisplayUpdate = 0;
const unsigned long displayInterval = 50;

// ==== WIFI ====
WiFiManager wm;

void setup() {
  Serial.begin(115200);

  // ==== WIFI CONNECT ====
  Serial.println("Connecting WiFi...");
  if (!wm.autoConnect("EyeDevice-Setup")) {
    Serial.println("WiFi gagal, restart...");
    delay(2000);
    ESP.restart();
  }

  Serial.println("WiFi Connected!");

  // ==== OLED INIT ====
  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED gagal!");
    while (1);
  }

  // Splash screen
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(20, 28);
  display.print("Connected!");
  display.display();
  delay(1500);

  // ==== INIT EYE ====
  eyeAnim.begin();
}

void drawEyeScreen() {
  display.clearDisplay();

  // Border
  display.drawRoundRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 4, SSD1306_WHITE);

  // Animasi mata
  eyeAnim.update();
  eyeAnim.draw(display);

  // Status WiFi
  display.setTextSize(1);
  display.setCursor(30, 52);

  if (WiFi.status() == WL_CONNECTED) {
    display.print("Online");
  } else {
    display.print("Offline");
  }

  display.display();
}

void loop() {
  unsigned long now = millis();

  if (now - lastDisplayUpdate >= displayInterval) {
    lastDisplayUpdate = now;
    drawEyeScreen();
  }
}
