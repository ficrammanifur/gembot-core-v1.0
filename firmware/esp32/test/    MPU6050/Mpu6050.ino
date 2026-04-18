#include "Wire.h"
#include "MPU6050.h"

// ESP32 I2C pins
#define SDA_PIN 21
#define SCL_PIN 22

MPU6050 accelgyro;

// Offsets hasil kalibrasi
int ax_offset = -7844;
int ay_offset = -5615;
int az_offset = 8636;
int gx_offset = -26;
int gy_offset = -8;
int gz_offset = -67;

int16_t ax, ay, az, gx, gy, gz;

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.println("Initializing MPU6050...");
  accelgyro.initialize();

  // Terapkan offsets kalibrasi
  accelgyro.setXAccelOffset(ax_offset);
  accelgyro.setYAccelOffset(ay_offset);
  accelgyro.setZAccelOffset(az_offset);
  accelgyro.setXGyroOffset(gx_offset);
  accelgyro.setYGyroOffset(gy_offset);
  accelgyro.setZGyroOffset(gz_offset);

  if(accelgyro.testConnection()){
    Serial.println("MPU6050 connected successfully!");
  } else {
    Serial.println("MPU6050 connection failed!");
    while(1);
  }

  Serial.println("Reading sensor data...");
}

void loop() {
  // Ambil data accelerometer dan gyroscope
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // Tampilkan data di Serial Monitor
  Serial.print("Accel [X Y Z]: ");
  Serial.print(ax); Serial.print(" ");
  Serial.print(ay); Serial.print(" ");
  Serial.println(az);

  Serial.print("Gyro  [X Y Z]: ");
  Serial.print(gx); Serial.print(" ");
  Serial.print(gy); Serial.print(" ");
  Serial.println(gz);

  Serial.println("-----------------------------");
  delay(200); // update tiap 200ms
}
