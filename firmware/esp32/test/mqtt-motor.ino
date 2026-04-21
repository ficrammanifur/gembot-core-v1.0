#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <driver/i2s.h>

// === LED INDICATOR ===
#define LED_BUILTIN 2

// === PIN MOTOR (SAMA SEPERTI KODE BLE YANG BERHASIL) ===
// MOTOR KANAN
const int in1R = 12;
const int in2R = 14;
const int enR = 13;

// MOTOR KIRI
const int in1L = 18;
const int in2L = 5;
const int enL = 33;

// === SPEAKER MAX98357 I2S ===
#define SPK_BCLK 26
#define SPK_LRC  25
#define SPK_DOUT 27

// === MQTT CONFIG ===
const char* mqtt_server = "100.120.23.93";
const int mqtt_port = 1883;
const char* mqtt_topic_cmd = "gembot/cmd";
const char* mqtt_topic_status = "gembot/status";

// === TCP Server for Audio ===
WiFiServer audioServer(3333);
WiFiClient audioClient;

// === I2S Speaker ===
#define I2S_SPK I2S_NUM_0
#define SAMPLE_RATE 16000

// === Motor Variables ===
int currentSpeed = 200;
String lastCommand = "";

// === MQTT Client ===
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// === LED Status ===
unsigned long lastLedBlink = 0;
int ledState = LOW;
String wifiStatus = "DISCONNECTED";
String mqttStatus = "DISCONNECTED";

// === MOTOR FUNCTIONS (SAMA PERSIS DENGAN KODE BLE) ===
void setupMotors() {
  pinMode(in1R, OUTPUT);
  pinMode(in2R, OUTPUT);
  pinMode(enR, OUTPUT);
  pinMode(in1L, OUTPUT);
  pinMode(in2L, OUTPUT);
  pinMode(enL, OUTPUT);
  
  // Set enable pins HIGH untuk mengaktifkan motor driver (PENTING!)
  digitalWrite(enR, HIGH);
  digitalWrite(enL, HIGH);
  
  stopMotors();
  Serial.println("✓ Motor initialized");
}

void stopMotors() {
  digitalWrite(in1R, LOW);
  digitalWrite(in2R, LOW);
  digitalWrite(in1L, LOW);
  digitalWrite(in2L, LOW);
  lastCommand = "STOP";
  Serial.println("Motor STOP");
}

void moveForward(int speed) {
  digitalWrite(in1R, LOW);
  digitalWrite(in2R, HIGH);
  digitalWrite(in1L, LOW);
  digitalWrite(in2L, HIGH);
  analogWrite(enR, speed);
  analogWrite(enL, speed);
  lastCommand = "FORWARD";
  Serial.printf("Motor FORWARD speed: %d\n", speed);
}

void moveBackward(int speed) {
  digitalWrite(in1R, HIGH);
  digitalWrite(in2R, LOW);
  digitalWrite(in1L, HIGH);
  digitalWrite(in2L, LOW);
  analogWrite(enR, speed);
  analogWrite(enL, speed);
  lastCommand = "BACKWARD";
  Serial.printf("Motor BACKWARD speed: %d\n", speed);
}

void turnRight(int speed) {
  digitalWrite(in1R, HIGH);
  digitalWrite(in2R, LOW);
  digitalWrite(in1L, LOW);
  digitalWrite(in2L, HIGH);
  analogWrite(enR, speed);
  analogWrite(enL, speed);
  lastCommand = "RIGHT";
  Serial.printf("Motor RIGHT speed: %d\n", speed);
}

void turnLeft(int speed) {
  digitalWrite(in1R, LOW);
  digitalWrite(in2R, HIGH);
  digitalWrite(in1L, HIGH);
  digitalWrite(in2L, LOW);
  analogWrite(enR, speed);
  analogWrite(enL, speed);
  lastCommand = "LEFT";
  Serial.printf("Motor LEFT speed: %d\n", speed);
}

// === LED Functions ===
void setupLED() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
}

void updateLEDIndicator() {
  unsigned long currentMillis = millis();
  
  if (wifiStatus == "CONNECTING") {
    if (currentMillis - lastLedBlink > 100) {
      ledState = !ledState;
      digitalWrite(LED_BUILTIN, ledState);
      lastLedBlink = currentMillis;
    }
  }
  else if (wifiStatus == "CONNECTED" && mqttStatus == "CONNECTING") {
    if (currentMillis - lastLedBlink > 500) {
      ledState = !ledState;
      digitalWrite(LED_BUILTIN, ledState);
      lastLedBlink = currentMillis;
    }
  }
  else if (wifiStatus == "CONNECTED" && mqttStatus == "CONNECTED") {
    digitalWrite(LED_BUILTIN, HIGH);
  }
  else if (wifiStatus == "DISCONNECTED") {
    if (currentMillis - lastLedBlink > 1000) {
      ledState = !ledState;
      digitalWrite(LED_BUILTIN, ledState);
      lastLedBlink = currentMillis;
    }
  }
  else {
    digitalWrite(LED_BUILTIN, LOW);
  }
}

// === I2S Speaker Init ===
void initSpeaker() {
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pins = {
    .bck_io_num = SPK_BCLK,
    .ws_io_num = SPK_LRC,
    .data_out_num = SPK_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_SPK, &cfg, 0, NULL);
  i2s_set_pin(I2S_SPK, &pins);
  i2s_set_clk(I2S_SPK, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
  Serial.println("✓ Speaker initialized");
}

void playAudio(const uint8_t* data, size_t len) {
  size_t written;
  i2s_write(I2S_SPK, data, len, &written, portMAX_DELAY);
  Serial.printf("Played %d bytes\n", len);
}

// === MQTT Callback ===
void mqttCallback(char* topic, byte* payload, unsigned int len) {
  String msg;
  for(int i=0; i<len; i++) msg += (char)payload[i];
  
  Serial.print("MQTT received: ");
  Serial.println(msg);
  
  DynamicJsonDocument doc(200);
  DeserializationError error = deserializeJson(doc, msg);
  
  if (!error) {
    String cmd = doc["command"];
    int speed = doc["speed"] | 200;
    
    Serial.printf("Command: %s, Speed: %d\n", cmd.c_str(), speed);
    
    if(cmd == "forward") {
      moveForward(speed);
    }
    else if(cmd == "backward") {
      moveBackward(speed);
    }
    else if(cmd == "left") {
      turnLeft(speed);
    }
    else if(cmd == "right") {
      turnRight(speed);
    }
    else {
      stopMotors();
    }
    
    currentSpeed = speed;
    
    // Publish status
    DynamicJsonDocument status(200);
    status["command"] = lastCommand;
    status["speed"] = currentSpeed;
    String output;
    serializeJson(status, output);
    mqttClient.publish(mqtt_topic_status, output.c_str());
    Serial.printf("Status published: %s\n", output.c_str());
  } else {
    Serial.println("JSON parse error");
  }
}

void reconnectMQTT() {
  while(!mqttClient.connected()) {
    Serial.print("Connecting to MQTT...");
    mqttStatus = "CONNECTING";
    
    if(mqttClient.connect("ESP32_Gembot")) {
      mqttClient.subscribe(mqtt_topic_cmd);
      mqttStatus = "CONNECTED";
      Serial.println(" ✓ Connected");
      
      // Publish online status
      DynamicJsonDocument status(200);
      status["command"] = "ONLINE";
      status["speed"] = currentSpeed;
      String output;
      serializeJson(status, output);
      mqttClient.publish(mqtt_topic_status, output.c_str());
    } else {
      Serial.print(" Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retry in 5 sec");
      delay(5000);
    }
  }
}

// === Setup ===
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== GEMBOT ROBOT MQTT ===");
  
  // Setup LED
  setupLED();
  wifiStatus = "DISCONNECTED";
  
  // Blink 3x to indicate boot
  for(int i=0; i<3; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
  }
  
  // Setup motors (SAMA SEPERTI KODE BLE)
  setupMotors();
  
  // Test motor briefly to verify connection
  Serial.println("Testing motor...");
  moveForward(150);
  delay(500);
  stopMotors();
  Serial.println("Motor test complete");
  
  // Setup speaker
  initSpeaker();
  
  // WiFi Manager
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  wm.setConnectTimeout(30);
  
  Serial.println("Starting WiFi Manager...");
  wifiStatus = "CONNECTING";
  
  if(!wm.autoConnect("GembotRobot", "12345678")) {
    Serial.println("WiFi failed, restarting...");
    wifiStatus = "DISCONNECTED";
    delay(3000);
    ESP.restart();
  }
  
  wifiStatus = "CONNECTED";
  Serial.println("✓ WiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  // MQTT Setup
  mqttStatus = "CONNECTING";
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(2048);
  reconnectMQTT();
  
  // TCP Audio Server
  audioServer.begin();
  Serial.println("✓ TCP Audio Server on port 3333");
  
  Serial.println("\n=== SYSTEM READY ===");
  Serial.println("Access dashboard: http://100.120.23.93:5000");
  
  // Final LED indication
  delay(500);
  for(int i=0; i<2; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}

// === Loop ===
void loop() {
  // Update LED indicator
  updateLEDIndicator();
  
  // MQTT reconnect if needed
  if(!mqttClient.connected()) {
    mqttStatus = "CONNECTING";
    reconnectMQTT();
  } else {
    mqttStatus = "CONNECTED";
    mqttClient.loop();
  }
  
  // Handle audio connection
  if(audioServer.hasClient()) {
    if(audioClient.connected()) {
      audioClient.stop();
    }
    audioClient = audioServer.available();
    Serial.println("✓ Audio client connected");
  }
  
  // Receive audio data
  if(audioClient.connected() && audioClient.available()) {
    uint8_t buffer[1024];
    int len = audioClient.read(buffer, sizeof(buffer));
    if(len > 0) {
      playAudio(buffer, len);
    }
  }
  
  // Auto-stop after 1.5 seconds (opsional, bisa di-comment)
  static unsigned long lastMove = 0;
  if(lastCommand != "STOP") {
    if(millis() - lastMove > 1500) {
      stopMotors();
      Serial.println("Auto-stop triggered");
      lastMove = millis();
    }
  } else {
    lastMove = millis();
  }
  
  delay(10);
}
