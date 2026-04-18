#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

// ================= PIN =================
#define I2S_BCLK 26
#define I2S_LRC  25
#define I2S_DOUT 27

// ================= OBJECT =================
I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Starting Bluetooth Speaker...");

  // ===== I2S CONFIG =====
  auto cfg = i2s.defaultConfig(TX_MODE);
  cfg.sample_rate = 44100;
  cfg.bits_per_sample = 16;
  cfg.channels = 2;

  cfg.pin_bck = I2S_BCLK;
  cfg.pin_ws  = I2S_LRC;
  cfg.pin_data = I2S_DOUT;

  i2s.begin(cfg);

  // ===== BLUETOOTH =====
  a2dp_sink.set_volume(80); // 0–100
  a2dp_sink.start("MyMusic");

  Serial.println("Bluetooth ready, pair now!");
}

void loop() {
  // kosong
}
