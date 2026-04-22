#include <WiFiManager.h>
#include <Wire.h>
#include <MPU6050.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include "driver/i2s.h"
#include <PubSubClient.h>

// ==================== PIN DEFINITIONS ====================
// Motor Driver Pins
#define MOTOR_ENA 13
#define MOTOR_IN1 12
#define MOTOR_IN2 14
#define MOTOR_ENB 33
#define MOTOR_IN3 5
#define MOTOR_IN4 18
#define SPEED 200

// I2S Audio Pins
#define I2S_BCK 26
#define I2S_WS 25
#define I2S_DOUT 27
#define SAMPLE_RATE 16000
#define I2S_PORT I2S_NUM_0

// GPS Pins
#define RXD2 16
#define TXD2 17

// I2C for MPU6050
#define SDA_PIN 21
#define SCL_PIN 22

// MQTT Configuration
const char* mqtt_server = "192.168.1.16"; // Ganti dengan IP broker MQTT Anda
const int mqtt_port = 1883;
const char* mqtt_topic_cmd = "gembot/command";
const char* mqtt_topic_status = "gembot/status";
const char* mqtt_topic_sensor = "gembot/sensor";

// ==================== GLOBAL OBJECTS ====================
MPU6050 accelgyro;
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// MPU6050 Offsets (kalibrasi)
int ax_offset = -7011, ay_offset = -6660, az_offset = 8628;
int gx_offset = -71, gy_offset = -64, gz_offset = -55;
int16_t ax, ay, az, gx, gy, gz;

// Audio buffer
uint8_t audioBuffer[4096];
size_t audioBufferLen = 0;

// Status flags
bool motorActive = false;
char lastCommand[10] = "STOP";
unsigned long lastSensorPublish = 0;
unsigned long lastMqttReconnect = 0;

// ==================== MOTOR FUNCTIONS ====================
void maju() {
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, HIGH);
  digitalWrite(MOTOR_IN3, LOW);
  digitalWrite(MOTOR_IN4, HIGH);
  analogWrite(MOTOR_ENA, SPEED + 3);
  analogWrite(MOTOR_ENB, SPEED);
  strcpy(lastCommand, "MAJU");
  motorActive = true;
  Serial.println("✅ MOTOR: MAJU");
  publishStatus("MAJU");
}

void mundur() {
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, HIGH);
  digitalWrite(MOTOR_IN4, LOW);
  analogWrite(MOTOR_ENA, SPEED);
  analogWrite(MOTOR_ENB, SPEED);
  strcpy(lastCommand, "MUNDUR");
  motorActive = true;
  Serial.println("✅ MOTOR: MUNDUR");
  publishStatus("MUNDUR");
}

void belokKiri() {
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, LOW);
  digitalWrite(MOTOR_IN4, HIGH);
  analogWrite(MOTOR_ENA, SPEED);
  analogWrite(MOTOR_ENB, SPEED);
  strcpy(lastCommand, "KIRI");
  motorActive = true;
  Serial.println("✅ MOTOR: BELOK KIRI");
  publishStatus("KIRI");
}

void belokKanan() {
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, HIGH);
  digitalWrite(MOTOR_IN3, HIGH);
  digitalWrite(MOTOR_IN4, LOW);
  analogWrite(MOTOR_ENA, SPEED);
  analogWrite(MOTOR_ENB, SPEED);
  strcpy(lastCommand, "KANAN");
  motorActive = true;
  Serial.println("✅ MOTOR: BELOK KANAN");
  publishStatus("KANAN");
}

void stopMotors() {
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, LOW);
  digitalWrite(MOTOR_IN4, LOW);
  analogWrite(MOTOR_ENA, 0);
  analogWrite(MOTOR_ENB, 0);
  strcpy(lastCommand, "STOP");
  motorActive = false;
  Serial.println("✅ MOTOR: STOP");
  publishStatus("STOP");
}

// ==================== I2S AUDIO SETUP ====================
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
  Serial.println("🎧 I2S Audio Ready");
}

void playAudio(const uint8_t* data, size_t len) {
  size_t written;
  i2s_write(I2S_PORT, data, len, &written, portMAX_DELAY);
}

void playBeep() {
  int16_t tone[320];
  for (int i = 0; i < 320; i++) {
    tone[i] = 16000 * sin(2 * PI * 880 * i / SAMPLE_RATE);
  }
  size_t written;
  i2s_write(I2S_PORT, tone, sizeof(tone), &written, portMAX_DELAY);
  delay(100);
  i2s_zero_dma_buffer(I2S_PORT);
}

// ==================== MPU6050 ====================
void setupMPU6050() {
  Wire.begin(SDA_PIN, SCL_PIN);
  accelgyro.initialize();
  
  accelgyro.setXAccelOffset(ax_offset);
  accelgyro.setYAccelOffset(ay_offset);
  accelgyro.setZAccelOffset(az_offset);
  accelgyro.setXGyroOffset(gx_offset);
  accelgyro.setYGyroOffset(gy_offset);
  accelgyro.setZGyroOffset(gz_offset);
  
  if(accelgyro.testConnection()) {
    Serial.println("✅ MPU6050 Connected");
  } else {
    Serial.println("❌ MPU6050 Failed!");
  }
}

void readMPU6050() {
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
}

// ==================== GPS ====================
void setupGPS() {
  gpsSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial.println("📍 GPS Started");
}

void readGPS() {
  while (gpsSerial.available() > 0) {
    char c = gpsSerial.read();
    gps.encode(c);
  }
}

// ==================== MQTT ====================
void publishStatus(const char* command) {
  if (mqttClient.connected()) {
    char msg[128];
    snprintf(msg, sizeof(msg), "{\"command\":\"%s\",\"time\":%lu}", command, millis());
    mqttClient.publish(mqtt_topic_status, msg);
  }
}

void publishSensorData() {
  if (!mqttClient.connected()) return;
  
  char msg[512];
  snprintf(msg, sizeof(msg), 
    "{\"accel\":[%d,%d,%d],\"gyro\":[%d,%d,%d],\"lat\":%.6f,\"lng\":%.6f,\"speed\":%.2f,\"sat\":%d,\"cmd\":\"%s\"}",
    ax, ay, az, gx, gy, gz,
    gps.location.lat(), gps.location.lng(),
    gps.speed.kmph(),
    gps.satellites.value(),
    lastCommand);
  
  mqttClient.publish(mqtt_topic_sensor, msg);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char message[256];
  memcpy(message, payload, length);
  message[length] = '\0';
  
  Serial.printf("📨 MQTT: %s -> %s\n", topic, message);
  
  // Parse command (format: "CMD:value" atau langsung perintah)
  String cmd = String(message);
  cmd.toLowerCase();
  
  if (cmd == "maju" || cmd == "forward" || cmd == "w") {
    maju();
    playBeep();
  }
  else if (cmd == "mundur" || cmd == "backward" || cmd == "s") {
    mundur();
    playBeep();
  }
  else if (cmd == "kiri" || cmd == "left" || cmd == "a") {
    belokKiri();
    playBeep();
  }
  else if (cmd == "kanan" || cmd == "right" || cmd == "d") {
    belokKanan();
    playBeep();
  }
  else if (cmd == "stop" || cmd == "x") {
    stopMotors();
  }
  else if (cmd.startsWith("speak:")) {
    // Format: "speak:Hello World"
    // Audio akan dikirim via TCP terpisah, ini hanya notifikasi
    Serial.printf("🔊 AI Response: %s\n", cmd.substring(6).c_str());
  }
  else if (cmd == "status") {
    publishSensorData();
  }
}

void reconnectMQTT() {
  if (mqttClient.connected()) return;
  
  unsigned long now = millis();
  if (now - lastMqttReconnect < 5000) return;
  lastMqttReconnect = now;
  
  Serial.print("📡 Connecting MQTT...");
  if (mqttClient.connect("ESP32_Gembot")) {
    Serial.println(" ✅ Connected!");
    mqttClient.subscribe(mqtt_topic_cmd);
    publishStatus("BOOT");
  } else {
    Serial.printf(" ❌ Failed (rc=%d)\n", mqttClient.state());
  }
}

// ==================== TCP AUDIO SERVER ====================
WiFiServer audioServer(3333);

void setupAudioServer() {
  audioServer.begin();
  Serial.println("🔊 Audio TCP Server on port 3333");
}

void handleAudioClient() {
  WiFiClient client = audioServer.available();
  if (!client) return;
  
  Serial.println("🔊 Audio client connected");
  playBeep(); // Indikasi koneksi
  
  while (client.connected()) {
    int len = client.read(audioBuffer, sizeof(audioBuffer));
    if (len > 0) {
      playAudio(audioBuffer, len);
    }
    delay(1);
  }
  
  client.stop();
  Serial.println("🔊 Audio client disconnected");
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  Serial.println("\n\n========================================");
  Serial.println("🤖 GEMBOT - Smart Package Robot");
  Serial.println("========================================\n");
  
  // Setup Motor Pins
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  pinMode(MOTOR_IN3, OUTPUT);
  pinMode(MOTOR_IN4, OUTPUT);
  pinMode(MOTOR_ENA, OUTPUT);
  pinMode(MOTOR_ENB, OUTPUT);
  stopMotors();
  
  // Setup WiFi dengan WiFiManager
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  wm.setTitle("GEMBOT WiFi Setup");
  
  bool res;
  res = wm.autoConnect("GEMBOT-Setup", "password123");
  
  if (!res) {
    Serial.println("❌ WiFi Gagal, restarting...");
    ESP.restart();
  } else {
    Serial.println("✅ WiFi Connected!");
    Serial.print("📡 IP Address: ");
    Serial.println(WiFi.localIP());
  }
  
  // Setup MQTT
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  
  // Setup Hardware
  setupI2S();
  setupMPU6050();
  setupGPS();
  setupAudioServer();
  
  // Play startup sound
  playBeep();
  delay(200);
  playBeep();
  
  Serial.println("\n✅ GEMBOT READY!");
  Serial.println("========================================");
  Serial.println("📌 MQTT Topic Subscribe: " + String(mqtt_topic_cmd));
  Serial.println("📌 MQTT Topic Publish: " + String(mqtt_topic_status));
  Serial.println("🔊 Audio TCP Port: 3333");
  Serial.println("========================================\n");
}

// ==================== LOOP ====================
void loop() {
  static unsigned long lastSensorRead = 0;
  static unsigned long lastGpsPublish = 0;
  
  // Handle MQTT
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();
  
  // Handle Audio Client
  handleAudioClient();
  
  // Read sensors periodically
  if (millis() - lastSensorRead > 100) {
    readMPU6050();
    readGPS();
    lastSensorRead = millis();
  }
  
  // Publish sensor data every 2 seconds
  if (millis() - lastSensorPublish > 2000 && mqttClient.connected()) {
    publishSensorData();
    lastSensorPublish = millis();
  }
  
  // Print GPS every 5 seconds if valid
  if (millis() - lastGpsPublish > 5000 && gps.location.isUpdated()) {
    Serial.printf("📍 LAT:%.6f LNG:%.6f SPEED:%.1fkm/h SAT:%d\n",
      gps.location.lat(), gps.location.lng(),
      gps.speed.kmph(), gps.satellites.value());
    lastGpsPublish = millis();
  }
  
  // Serial command for debugging
  if (Serial.available()) {
    char cmd = Serial.read();
    switch (cmd) {
      case 'w': maju(); break;
      case 's': mundur(); break;
      case 'a': belokKiri(); break;
      case 'd': belokKanan(); break;
      case 'x': stopMotors(); break;
      default: break;
    }
  }
  
  delay(10);
}
