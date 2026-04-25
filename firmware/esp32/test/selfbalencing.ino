/*
 * SELF-BALANCING 4WD v4.3 – Micro Correction + Always Active
 * Sekarang robot harus langsung bereaksi tanpa harus dimiringin dulu
 */

#include <Arduino.h>
#include <Wire.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <MPU6050.h>

// ====================== PIN 4WD ======================
#define PIN_SDA 21
#define PIN_SCL 22

#define PIN_IN1L 5
#define PIN_IN2L 18
#define PIN_ENL  33

#define PIN_IN1R 12
#define PIN_IN2R 14
#define PIN_ENR  13

#define LEDC_FREQ 20000
#define LEDC_RES  8

// ====================== TUNING (SUDAH DIOPTIMALKAN) ======================
float TARGET_ANGLE = 0.0f;

#define PID_KP         33.0f     // Naik sedikit
#define PID_KI         0.0f      // Masih dimatikan
#define GYRO_DAMP      1.6f
#define INTEGRAL_MAX   35.0f
#define MAX_PWM        195
#define MIN_PWM        6         // Diturunkan drastis
#define CORRECTION_R   4
#define FALL_ANGLE     50.0f

#define MAX_SPEED_LIMIT  40.0f
#define MICRO_BIAS       0.45f   // Bias kecil supaya selalu aktif balancing

// MPU6050 Offset
#define AX_OFFSET -7011
#define AY_OFFSET -6660
#define AZ_OFFSET 8628
#define GX_OFFSET -71
#define GY_OFFSET -64
#define GZ_OFFSET -55

// ====================== SHARED STATE ======================
struct RobotState {
  float pitch = 0.0f;
  float gyroY = 0.0f;
  float pidOutput = 0.0f;
  float pidError = 0.0f;
  bool fallen = false;
};

MPU6050 imu;
SemaphoreHandle_t xMutex;
RobotState gState;

float targetPitch = 0.0f;
bool slowingLock = false;

// ====================== MOTOR ======================
static inline int smoothDeadZone(int spd) {
  if (spd == 0) return 0;
  int a = abs(spd);
  return (spd > 0) ? max(a, MIN_PWM) : -max(a, MIN_PWM);
}

static void motorApply(int spdL, int spdR) {
  if (spdL > 0)      { digitalWrite(PIN_IN1L, HIGH); digitalWrite(PIN_IN2L, LOW); }
  else if (spdL < 0) { digitalWrite(PIN_IN1L, LOW);  digitalWrite(PIN_IN2L, HIGH); }
  else               { digitalWrite(PIN_IN1L, LOW);  digitalWrite(PIN_IN2L, LOW); }
  ledcWrite(PIN_ENL, abs(spdL));

  if (spdR > 0)      { digitalWrite(PIN_IN1R, HIGH); digitalWrite(PIN_IN2R, LOW); }
  else if (spdR < 0) { digitalWrite(PIN_IN1R, LOW);  digitalWrite(PIN_IN2R, HIGH); }
  else               { digitalWrite(PIN_IN1R, LOW);  digitalWrite(PIN_IN2R, LOW); }
  ledcWrite(PIN_ENR, abs(spdR));
}

static void motorStop() {
  motorApply(0, 0);
}

// ====================== TASK IMU ======================
void taskIMU(void* pv) {
  TickType_t wake = xTaskGetTickCount();
  float angle = 0.0f;
  uint32_t lastUs = micros();

  delay(2500);

  for (;;) {
    int16_t ax, ay, az, gx, gy, gz;
    imu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    float accAngle = atan2f((float)ay, (float)az) * RAD_TO_DEG;
    float gyroRate = (float)gy / 131.0f;

    uint32_t now = micros();
    float dt = (float)(now - lastUs) * 1e-6f;
    if (dt <= 0 || dt > 0.05f) dt = 0.002f;
    lastUs = now;

    angle = 0.96f * (angle + gyroRate * dt) + 0.04f * accAngle;

    if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(1)) == pdTRUE) {
      gState.pitch = angle;
      gState.gyroY = gyroRate;
      gState.fallen = (fabsf(angle) > FALL_ANGLE);
      xSemaphoreGive(xMutex);
    }
    vTaskDelayUntil(&wake, pdMS_TO_TICKS(2));
  }
}

// ====================== TASK CONTROL ======================
void taskControl(void* pv) {
  TickType_t wake = xTaskGetTickCount();
  float integral = 0.0f;
  float prevPitch = 0.0f;

  for (;;) {
    float pitch = 0.0f, gyroY = 0.0f;
    bool fallen = false;

    if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(1)) == pdTRUE) {
      pitch = gState.pitch;
      gyroY = gState.gyroY;
      fallen = gState.fallen;
      xSemaphoreGive(xMutex);
    }

    if (fallen) {
      motorStop();
      integral = 0;
      vTaskDelayUntil(&wake, pdMS_TO_TICKS(5));
      continue;
    }

    float newTarget = targetPitch + MICRO_BIAS;   // Bias kecil supaya selalu aktif

    float speedEstimate = (pitch - prevPitch) * 60.0f;

    if (abs(speedEstimate) > MAX_SPEED_LIMIT && !slowingLock) {
      slowingLock = true;
      newTarget += (speedEstimate > 0) ? -0.7f : 0.7f;
    } 
    else if (targetPitch == 0.0f && slowingLock) {
      slowingLock = false;
    }

    float error = newTarget - pitch;

    if (fabsf(error) < 25.0f) {
      integral += error * 0.005f;
      integral = constrain(integral, -INTEGRAL_MAX, INTEGRAL_MAX);
    }

    float derivative = -gyroY * GYRO_DAMP;
    float output = (PID_KP * error) + (PID_KI * integral) + derivative;
    output = constrain(output, -MAX_PWM, MAX_PWM);

    // === MICRO CORRECTION ===
    if (fabsf(error) < 2.0f && fabsf(output) > 0) {
      output *= 1.6f;                    // Boost untuk koreksi kecil
    }

    // Kick start jika output sangat kecil tapi tidak nol
    if (abs(output) > 0 && abs(output) < MIN_PWM) {
      output = (output > 0) ? MIN_PWM : -MIN_PWM;
    }

    int spdL = smoothDeadZone((int)output);
    int spdR = smoothDeadZone((int)output + (output >= 0 ? CORRECTION_R : -CORRECTION_R));

    motorApply(spdL, spdR);

    prevPitch = pitch;

    static uint32_t lastPrint = 0;
    if (millis() - lastPrint > 200) {
      Serial.printf("Tgt:%.2f  Pitch:%.2f  Err:%.2f  Out:%.1f  L:%d  R:%d\n", 
                    newTarget, pitch, error, output, spdL, spdR);
      lastPrint = millis();
    }

    vTaskDelayUntil(&wake, pdMS_TO_TICKS(5));
  }
}

// ====================== SETUP ======================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println(F("\n=== SELF-BALANCING 4WD v4.3 - Micro Active Mode ==="));

  xMutex = xSemaphoreCreateMutex();

  ledcAttach(PIN_ENL, LEDC_FREQ, LEDC_RES);
  ledcAttach(PIN_ENR, LEDC_FREQ, LEDC_RES);

  pinMode(PIN_IN1L, OUTPUT); pinMode(PIN_IN2L, OUTPUT);
  pinMode(PIN_IN1R, OUTPUT); pinMode(PIN_IN2R, OUTPUT);
  motorStop();

  // Motor Test
  Serial.println(F("[TEST] 4WD maju 3 detik..."));
  motorApply(170, 170);
  delay(3000);
  motorStop();

  // IMU
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);
  imu.initialize();
  delay(100);

  imu.setXAccelOffset(AX_OFFSET);
  imu.setYAccelOffset(AY_OFFSET);
  imu.setZAccelOffset(AZ_OFFSET);
  imu.setXGyroOffset(GX_OFFSET);
  imu.setYGyroOffset(GY_OFFSET);
  imu.setZGyroOffset(GZ_OFFSET);

  delay(2500);
  int16_t ax, ay, az, gx, gy, gz;
  imu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  TARGET_ANGLE = atan2f((float)ay, (float)az) * RAD_TO_DEG;
  targetPitch = TARGET_ANGLE;

  Serial.print(F("[SYS] Target Angle = "));
  Serial.print(TARGET_ANGLE, 2);
  Serial.println("°");

  xTaskCreatePinnedToCore(taskIMU,     "IMU",     4096, NULL, 4, NULL, 1);
  xTaskCreatePinnedToCore(taskControl, "Control", 4096, NULL, 5, NULL, 1);

  Serial.println(F("[SYS] Robot siap! Letakkan tegak → seharusnya langsung aktif balancing"));
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(10000));
}
