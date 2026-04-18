#include "Wire.h"
#include "I2Cdev.h"
#include "MPU6050.h"

#define SDA_PIN 21
#define SCL_PIN 22

MPU6050 accelgyro(0x68); // default address

int16_t ax, ay, az, gx, gy, gz;
long mean_ax, mean_ay, mean_az, mean_gx, mean_gy, mean_gz;
int ax_offset=0, ay_offset=0, az_offset=0;
int gx_offset=0, gy_offset=0, gz_offset=0;

int buffersize = 1000;
int acel_deadzone = 8;
int gyro_deadzone = 1;

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.println("Scanning I2C devices...");
  byte count = 0;
  for(byte i=1; i<127; i++){
    Wire.beginTransmission(i);
    if(Wire.endTransmission() == 0){
      Serial.print("I2C device found at 0x");
      Serial.println(i, HEX);
      count++;
    }
  }
  if(count == 0){
    Serial.println("No I2C device found. Check wiring!");
    while(1);
  }

  accelgyro.initialize();
  Serial.println("MPU-6050 initialized.");
  Serial.println("Place the sensor flat and do not move it...");
  delay(2000);

  accelgyro.setXAccelOffset(0);
  accelgyro.setYAccelOffset(0);
  accelgyro.setZAccelOffset(0);
  accelgyro.setXGyroOffset(0);
  accelgyro.setYGyroOffset(0);
  accelgyro.setZGyroOffset(0);

  Serial.println("Starting calibration...");
  calibrate();
}

void loop() {
  // Nothing here, calibration done in setup
}

void calibrate() {
  long buff_ax=0, buff_ay=0, buff_az=0;
  long buff_gx=0, buff_gy=0, buff_gz=0;

  Serial.println("Reading sensors to calculate mean values...");
  for(long i=0; i<(buffersize+100); i++){
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    if(i>100){
      buff_ax += ax;
      buff_ay += ay;
      buff_az += az;
      buff_gx += gx;
      buff_gy += gy;
      buff_gz += gz;
    }
    delay(2);
  }

  mean_ax = buff_ax/buffersize;
  mean_ay = buff_ay/buffersize;
  mean_az = buff_az/buffersize;
  mean_gx = buff_gx/buffersize;
  mean_gy = buff_gy/buffersize;
  mean_gz = buff_gz/buffersize;

  ax_offset = -mean_ax/8;
  ay_offset = -mean_ay/8;
  az_offset = (16384 - mean_az)/8;
  gx_offset = -mean_gx/4;
  gy_offset = -mean_gy/4;
  gz_offset = -mean_gz/4;

  int ready = 0;
  Serial.println("Applying offsets and fine-tuning...");
  while(ready < 6){
    accelgyro.setXAccelOffset(ax_offset);
    accelgyro.setYAccelOffset(ay_offset);
    accelgyro.setZAccelOffset(az_offset);
    accelgyro.setXGyroOffset(gx_offset);
    accelgyro.setYGyroOffset(gy_offset);
    accelgyro.setZGyroOffset(gz_offset);

    buff_ax = buff_ay = buff_az = 0;
    buff_gx = buff_gy = buff_gz = 0;
    ready = 0;

    for(long i=0; i<buffersize; i++){
      accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
      buff_ax += ax; buff_ay += ay; buff_az += az;
      buff_gx += gx; buff_gy += gy; buff_gz += gz;
      delay(1);
    }

    mean_ax = buff_ax/buffersize;
    mean_ay = buff_ay/buffersize;
    mean_az = buff_az/buffersize;
    mean_gx = buff_gx/buffersize;
    mean_gy = buff_gy/buffersize;
    mean_gz = buff_gz/buffersize;

    if(abs(mean_ax) <= acel_deadzone) ready++; else ax_offset -= mean_ax/acel_deadzone;
    if(abs(mean_ay) <= acel_deadzone) ready++; else ay_offset -= mean_ay/acel_deadzone;
    if(abs(16384 - mean_az) <= acel_deadzone) ready++; else az_offset += (16384 - mean_az)/acel_deadzone;
    if(abs(mean_gx) <= gyro_deadzone) ready++; else gx_offset -= mean_gx/(gyro_deadzone+1);
    if(abs(mean_gy) <= gyro_deadzone) ready++; else gy_offset -= mean_gy/(gyro_deadzone+1);
    if(abs(mean_gz) <= gyro_deadzone) ready++; else gz_offset -= mean_gz/(gyro_deadzone+1);

    Serial.print(".");
  }

  Serial.println("\nCalibration FINISHED!");
  Serial.println("Your offsets:");
  Serial.print("Accel X: "); Serial.println(ax_offset);
  Serial.print("Accel Y: "); Serial.println(ay_offset);
  Serial.print("Accel Z: "); Serial.println(az_offset);
  Serial.print("Gyro X: "); Serial.println(gx_offset);
  Serial.print("Gyro Y: "); Serial.println(gy_offset);
  Serial.print("Gyro Z: "); Serial.println(gz_offset);

  Serial.println("You can copy these offsets into your main sketch.");
}
