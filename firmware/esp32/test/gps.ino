#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

// GPS object
TinyGPSPlus gps;

// Pakai UART2 ESP32
HardwareSerial gpsSerial(2);

// Pin GPS
#define RXD2 16
#define TXD2 17

void setup() {
  Serial.begin(115200);
  Serial.println("GPS Starting...");

  // Init GPS Serial
  gpsSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
}

void loop() {
  // Baca data GPS
  while (gpsSerial.available() > 0) {
    char c = gpsSerial.read();
    gps.encode(c);
  }

  // Jika lokasi valid
  if (gps.location.isUpdated()) {
    Serial.println("=== GPS DATA ===");
    Serial.print("Latitude  : ");
    Serial.println(gps.location.lat(), 6);

    Serial.print("Longitude : ");
    Serial.println(gps.location.lng(), 6);

    Serial.print("Speed (km/h): ");
    Serial.println(gps.speed.kmph());

    Serial.print("Satellites : ");
    Serial.println(gps.satellites.value());

    Serial.print("Altitude (m): ");
    Serial.println(gps.altitude.meters());

    Serial.println("================\n");
  }

  // Debug kalau belum dapet sinyal
  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println("GPS belum terbaca, cek wiring atau sinyal!");
  }
}
