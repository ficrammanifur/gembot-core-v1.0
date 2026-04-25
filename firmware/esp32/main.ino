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

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  // MOTOR PIN SETUP
  pinMode(in1R, OUTPUT);
  pinMode(in2R, OUTPUT);
  pinMode(in1L, OUTPUT);
  pinMode(in2L, OUTPUT);

  // PWM SETUP - NEW API untuk ESP32 Arduino Core 3.x
  ledcAttach(enR, 20000, 8);   // pin, frequency, resolution
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
  ledcWrite(enR, 180);
  ledcWrite(enL, 180);
}

void backward() {
  digitalWrite(in1R, LOW);
  digitalWrite(in2R, HIGH);
  digitalWrite(in1L, LOW);
  digitalWrite(in2L, HIGH);
  ledcWrite(enR, 180);
  ledcWrite(enL, 180);
}

void left() {
  digitalWrite(in1R, HIGH);
  digitalWrite(in2R, LOW);
  digitalWrite(in1L, LOW);
  digitalWrite(in2L, HIGH);
  ledcWrite(enR, 180);
  ledcWrite(enL, 180);
}

void right() {
  digitalWrite(in1R, LOW);
  digitalWrite(in2R, HIGH);
  digitalWrite(in1L, HIGH);
  digitalWrite(in2L, LOW);
  ledcWrite(enR, 180);
  ledcWrite(enL, 180);
}

// ================= LOOP =================
void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    // ===== COMMAND MOTOR =====
    if (cmd == "FORWARD") {
      forward();
      Serial.println("MAJU");
    }
    else if (cmd == "BACKWARD") {
      backward();
      Serial.println("MUNDUR");
    }
    else if (cmd == "LEFT") {
      left();
      Serial.println("KIRI");
    }
    else if (cmd == "RIGHT") {
      right();
      Serial.println("KANAN");
    }
    else if (cmd == "STOP") {
      stopMotor();
      Serial.println("STOP");
    }
    else {
      // ===== AUDIO STREAM (jika bukan command) =====
      int len = Serial.readBytes(buffer, sizeof(buffer));
      if (len > 0) {
        size_t written;
        i2s_write(I2S_PORT, buffer, len, &written, portMAX_DELAY);
      }
    }
  }
}
