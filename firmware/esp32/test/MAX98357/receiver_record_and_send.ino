#include <WiFi.h>
#include "driver/i2s.h"

// ===== WIFI =====
const char* ssid = "FRISS";
const char* pass = "mamahfris";

// ===== TCP =====
WiFiServer server(3333);

// ===== I2S PIN =====
#define I2S_BCK  26
#define I2S_WS   25
#define I2S_DOUT 27

#define SAMPLE_RATE 16000
#define I2S_PORT I2S_NUM_0

// ================= I2S SETUP =================
void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  i2s_zero_dma_buffer(I2S_PORT);

  Serial.println("🎧 I2S ready (16kHz / 16bit / mono)");
}

// ================= TEST TONE =================
void testTone() {
  int16_t tone[160];
  for (int i = 0; i < 160; i++) {
    tone[i] = 8000 * sin(2 * PI * 440 * i / SAMPLE_RATE);
  }

  size_t written;
  for (int i = 0; i < 200; i++) {
    i2s_write(I2S_PORT, tone, sizeof(tone), &written, portMAX_DELAY);
  }

  Serial.println("🔊 Test tone played");
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, pass);
  Serial.print("📡 Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ WiFi connected");
  Serial.println(WiFi.localIP());

  setupI2S();
  delay(500);
  testTone();  // HARUS ADA BUNYI NORMAL

  server.begin();
  Serial.println("🚀 ESP32 Audio Server Ready (PORT 3333)");
}

// ================= LOOP =================
void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("🔊 Client connected");

    uint8_t buffer[1024];
    while (client.connected()) {
      int len = client.read(buffer, sizeof(buffer));
      if (len > 0) {
        size_t written;
        i2s_write(I2S_PORT, buffer, len, &written, portMAX_DELAY);
      }
    }

    client.stop();
    Serial.println("🔇 Client disconnected");
  }
}
