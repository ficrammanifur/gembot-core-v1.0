#include <driver/i2s.h>

// ================= MOTOR =================
#define in1R 12
#define in2R 14
#define enR  13
#define in1L 5
#define in2L 18
#define enL  33

// ================= SPEAKER =================
#define BCLK 26
#define LRC  25
#define DOUT 27
#define I2S_PORT I2S_NUM_0

uint8_t buffer[512];

// Kecepatan motor (0-255)
#define MOTOR_SPEED 200

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  // MOTOR PIN SETUP
  pinMode(in1R, OUTPUT);
  pinMode(in2R, OUTPUT);
  pinMode(in1L, OUTPUT);
  pinMode(in2L, OUTPUT);
  
  // PENTING: Set enable pins HIGH untuk mengaktifkan motor driver
  pinMode(enR, OUTPUT);
  pinMode(enL, OUTPUT);
  digitalWrite(enR, HIGH);  // Aktifkan driver motor kanan
  digitalWrite(enL, HIGH);  // Aktifkan driver motor kiri

  // PWM SETUP (ESP32 Arduino Core 3.x)
  ledcAttach(enR, 20000, 8);
  ledcAttach(enL, 20000, 8);

  stopMotor();

  // I2S SPEAKER SETUP
  i2s_config_t config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .dma_buf_count = 4,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = BCLK,
    .ws_io_num = LRC,
    .data_out_num = DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_PORT, &config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);

  Serial.println("GEMBOT READY");
  Serial.println("Commands: FORWARD, BACKWARD, LEFT, RIGHT, STOP");
}

// ================= MOTOR FUNCTIONS =================
void stopMotor() {
  digitalWrite(in1R, LOW);
  digitalWrite(in2R, LOW);
  digitalWrite(in1L, LOW);
  digitalWrite(in2L, LOW);
  ledcWrite(enR, 0);
  ledcWrite(enL, 0);
}

void forward() {
  digitalWrite(in1R, HIGH);
  digitalWrite(in2R, LOW);
  digitalWrite(in1L, HIGH);
  digitalWrite(in2L, LOW);
  ledcWrite(enR, MOTOR_SPEED);
  ledcWrite(enL, MOTOR_SPEED);
  Serial.println("→ FORWARD");
}

void backward() {
  digitalWrite(in1R, LOW);
  digitalWrite(in2R, HIGH);
  digitalWrite(in1L, LOW);
  digitalWrite(in2L, HIGH);
  ledcWrite(enR, MOTOR_SPEED);
  ledcWrite(enL, MOTOR_SPEED);
  Serial.println("→ BACKWARD");
}

void left() {
  // BELOK KIRI (sama seperti BLE)
  digitalWrite(in1R, LOW);   // Motor kanan mundur
  digitalWrite(in2R, HIGH);
  digitalWrite(in1L, HIGH);  // Motor kiri maju
  digitalWrite(in2L, LOW);
  ledcWrite(enR, MOTOR_SPEED);
  ledcWrite(enL, MOTOR_SPEED);
  Serial.println("→ LEFT");
}

void right() {
  // BELOK KANAN (sama seperti BLE)
  digitalWrite(in1R, HIGH);  // Motor kanan maju
  digitalWrite(in2R, LOW);
  digitalWrite(in1L, LOW);   // Motor kiri mundur
  digitalWrite(in2L, HIGH);
  ledcWrite(enR, MOTOR_SPEED);
  ledcWrite(enL, MOTOR_SPEED);
  Serial.println("→ RIGHT");
}

// ================= LOOP =================
void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();  // Biar case insensitive

    // ===== COMMAND MOTOR =====
    if (cmd == "FORWARD" || cmd == "F") {
      forward();
    }
    else if (cmd == "BACKWARD" || cmd == "B") {
      backward();
    }
    else if (cmd == "LEFT" || cmd == "L") {
      left();
    }
    else if (cmd == "RIGHT" || cmd == "R") {
      right();
    }
    else if (cmd == "STOP" || cmd == "S") {
      stopMotor();
      Serial.println("→ STOP");
    }
    else if (cmd.length() > 0 && cmd != "") {
      // ===== AUDIO STREAM (binary data) =====
      // Balikin data ke buffer untuk audio
      int len = Serial.readBytes(buffer, sizeof(buffer));
      if (len > 0) {
        size_t written;
        i2s_write(I2S_PORT, buffer, len, &written, portMAX_DELAY);
      }
    }
  }
  
  // Tidak ada delay berlebihan - loop berjalan cepat
  delay(1);  // Small delay untuk stabil, tidak mengganggu response
}
