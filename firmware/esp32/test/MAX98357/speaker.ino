#include "AudioTools.h"
#include <math.h>

#define I2S_BCLK 26
#define I2S_LRC  25
#define I2S_DOUT 27

I2SStream i2s;

void setup() {
  Serial.begin(115200);

  auto cfg = i2s.defaultConfig(TX_MODE);
  cfg.sample_rate = 16000;      // 🔥 PALING STABIL
  cfg.bits_per_sample = 16;
  cfg.channels = 2;
  cfg.buffer_size = 512;        // 🔥 penting
  cfg.buffer_count = 8;

  cfg.pin_bck = I2S_BCLK;
  cfg.pin_ws  = I2S_LRC;
  cfg.pin_data = I2S_DOUT;

  i2s.begin(cfg);
  Serial.println("🔊 CLEAN STEREO TONE");
}

void loop() {
  static float phase = 0;
  int16_t s = sin(phase) * 6000;   // 🔥 LEBIH KECIL
  phase += 2.0 * PI * 440 / 16000; // 440Hz

  int16_t stereo[2] = { s, s };
  i2s.write((uint8_t*)stereo, sizeof(stereo));

  delayMicroseconds(62); // 🔥 pacing (1/16000 detik)
}
