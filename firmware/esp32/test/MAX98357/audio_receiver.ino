#include <WiFi.h>
#include <WebServer.h>
#include "driver/i2s.h"

// ===== WIFI =====
#define WIFI_SSID "NAMAWIFI"
#define WIFI_PASS "PASSWORD"

// ===== I2S =====
#define I2S_PORT I2S_NUM_0
#define I2S_BCLK 26
#define I2S_LRC  25
#define I2S_DOUT 27

WebServer server(80);

// ===== I2S INIT =====
void setupI2S() {
  i2s_config_t config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .dma_buf_count = 8,
    .dma_buf_len = 512,
    .use_apll = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pins = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = -1
  };

  i2s_driver_install(I2S_PORT, &config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pins);
  i2s_zero_dma_buffer(I2S_PORT);
}

// ===== HTTP HANDLER =====
void handleAudio() {
  WiFiClient client = server.client();
  size_t total = server.arg("plain").length();
  uint8_t buffer[512];
  size_t written;

  while (client.available()) {
    size_t len = client.read(buffer, sizeof(buffer));
    i2s_write(I2S_PORT, buffer, len, &written, portMAX_DELAY);
  }

  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");
  Serial.println(WiFi.localIP());

  setupI2S();

  server.on("/play_audio", HTTP_POST, handleAudio);
  server.begin();

  Serial.println("Audio server ready");
}

void loop() {
  server.handleClient();
}
