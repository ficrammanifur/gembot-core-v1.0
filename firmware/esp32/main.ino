/*
GEMBOT ESP32 Main Firmware
Handles TTS, Motor Control, and I2S Audio Output
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <driver/i2s.h>

// ==================== CONFIGURATION ====================

// WiFi Configuration
const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";
const char* DEVICE_NAME = "GEMBOT";
const uint16_t SERVER_PORT = 80;

// I2S Configuration (MAX98357A Audio Output)
#define I2S_BCLK_PIN 26
#define I2S_LRC_PIN 25
#define I2S_DOUT_PIN 27
#define I2S_NUM 0

// Motor Control Pins
#define LEFT_MOTOR_PIN_1 12
#define LEFT_MOTOR_PIN_2 14
#define LEFT_MOTOR_PWM_PIN 5

#define RIGHT_MOTOR_PIN_1 19
#define RIGHT_MOTOR_PIN_2 21
#define RIGHT_MOTOR_PWM_PIN 18

// PWM Configuration
#define PWM_FREQUENCY 1000
#define PWM_RESOLUTION 8
#define LEFT_PWM_CHANNEL 0
#define RIGHT_PWM_CHANNEL 1

// System Configuration
const int MAX_SPEECH_LENGTH = 500;
volatile bool is_speaking = false;
unsigned long last_activity = 0;

// ==================== GLOBAL OBJECTS ====================

WebServer server(SERVER_PORT);
DynamicJsonDocument doc(256);

// ==================== SETUP ====================

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n\n");
    Serial.println("=====================================");
    Serial.println("GEMBOT ESP32 Firmware v1.0");
    Serial.println("=====================================");
    
    // Initialize pins
    init_pins();
    
    // Initialize I2S for audio
    init_i2s();
    
    // Connect to WiFi
    connect_wifi();
    
    // Setup web server routes
    setup_web_server();
    
    // Start web server
    server.begin();
    Serial.println("Web server started on port " + String(SERVER_PORT));
    Serial.println("Ready for connections!");
    
    // Test audio
    play_startup_sound();
}

// ==================== MAIN LOOP ====================

void loop() {
    server.handleClient();
    
    // Check for timeout
    if (millis() - last_activity > 30000) {
        last_activity = millis();
        send_status_to_raspi();
    }
    
    delay(1);
}

// ==================== INITIALIZATION ====================

void init_pins() {
    // Motor pins as outputs
    pinMode(LEFT_MOTOR_PIN_1, OUTPUT);
    pinMode(LEFT_MOTOR_PIN_2, OUTPUT);
    pinMode(RIGHT_MOTOR_PIN_1, OUTPUT);
    pinMode(RIGHT_MOTOR_PIN_2, OUTPUT);
    
    // Configure PWM
    ledcSetup(LEFT_PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcSetup(RIGHT_PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcAttachPin(LEFT_MOTOR_PWM_PIN, LEFT_PWM_CHANNEL);
    ledcAttachPin(RIGHT_MOTOR_PWM_PIN, RIGHT_PWM_CHANNEL);
    
    // Stop motors initially
    stop_motors();
    
    Serial.println("Pins initialized");
}

void init_i2s() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false
    };
    
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK_PIN,
        .ws_io_num = I2S_LRC_PIN,
        .data_out_num = I2S_DOUT_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    
    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM, &pin_config);
    
    Serial.println("I2S initialized for audio output");
}

void connect_wifi() {
    Serial.println("Connecting to WiFi: " + String(WIFI_SSID));
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✓ Connected to WiFi");
        Serial.println("IP Address: " + WiFi.localIP().toString());
    } else {
        Serial.println("\n✗ WiFi connection failed");
    }
}

void setup_web_server() {
    // Speak endpoint
    server.on("/speak", HTTP_POST, handle_speak);
    
    // Motor control endpoint
    server.on("/motor", HTTP_POST, handle_motor_control);
    
    // Status endpoint
    server.on("/status", HTTP_GET, handle_status);
    
    // Root endpoint
    server.on("/", HTTP_GET, handle_root);
    
    // 404 handler
    server.onNotFound(handle_not_found);
}

// ==================== WEB HANDLERS ====================

void handle_root() {
    String response = R"(
    {
        "name": "GEMBOT ESP32",
        "version": "1.0.0",
        "status": "online",
        "endpoints": {
            "/speak": "Text-to-speech (POST)",
            "/motor": "Motor control (POST)",
            "/status": "Get status (GET)"
        }
    }
    )";
    
    server.sendHeader("Content-Type", "application/json");
    server.send(200, "application/json", response);
}

void handle_speak() {
    last_activity = millis();
    
    if (server.hasArg("plain")) {
        String body = server.arg("plain");
        
        // Parse JSON
        StaticJsonDocument<256> request;
        DeserializationError error = deserializeJson(request, body);
        
        if (error) {
            server.send(400, "application/json", "{\"error\": \"Invalid JSON\"}");
            return;
        }
        
        String text = request["text"] | "";
        
        if (text.length() == 0) {
            server.send(400, "application/json", "{\"error\": \"No text provided\"}");
            return;
        }
        
        Serial.println("TTS Request: " + text);
        
        // Simulate text-to-speech
        // In production, use TTS library like ESPeak
        simulate_speech(text);
        
        server.send(200, "application/json", "{\"success\": true}");
    } else {
        server.send(400, "application/json", "{\"error\": \"No JSON body\"}");
    }
}

void handle_motor_control() {
    last_activity = millis();
    
    if (server.hasArg("plain")) {
        String body = server.arg("plain");
        
        // Parse JSON
        StaticJsonDocument<256> request;
        DeserializationError error = deserializeJson(request, body);
        
        if (error) {
            server.send(400, "application/json", "{\"error\": \"Invalid JSON\"}");
            return;
        }
        
        int left_speed = request["left_speed"] | 0;
        int right_speed = request["right_speed"] | 0;
        
        // Clamp speeds
        left_speed = constrain(left_speed, -255, 255);
        right_speed = constrain(right_speed, -255, 255);
        
        Serial.println("Motor Command - Left: " + String(left_speed) + 
                      ", Right: " + String(right_speed));
        
        set_motor_speeds(left_speed, right_speed);
        
        server.send(200, "application/json", "{\"success\": true}");
    } else {
        server.send(400, "application/json", "{\"error\": \"No JSON body\"}");
    }
}

void handle_status() {
    StaticJsonDocument<256> response;
    response["status"] = "online";
    response["uptime"] = millis() / 1000;
    response["is_speaking"] = is_speaking;
    
    String json;
    serializeJson(response, json);
    
    server.sendHeader("Content-Type", "application/json");
    server.send(200, "application/json", json);
}

void handle_not_found() {
    server.send(404, "application/json", "{\"error\": \"Not found\"}");
}

// ==================== MOTOR CONTROL ====================

void set_motor_speeds(int left_speed, int right_speed) {
    // Left motor
    if (left_speed > 0) {
        digitalWrite(LEFT_MOTOR_PIN_1, HIGH);
        digitalWrite(LEFT_MOTOR_PIN_2, LOW);
    } else if (left_speed < 0) {
        digitalWrite(LEFT_MOTOR_PIN_1, LOW);
        digitalWrite(LEFT_MOTOR_PIN_2, HIGH);
    } else {
        digitalWrite(LEFT_MOTOR_PIN_1, LOW);
        digitalWrite(LEFT_MOTOR_PIN_2, LOW);
    }
    ledcWrite(LEFT_PWM_CHANNEL, abs(left_speed));
    
    // Right motor
    if (right_speed > 0) {
        digitalWrite(RIGHT_MOTOR_PIN_1, HIGH);
        digitalWrite(RIGHT_MOTOR_PIN_2, LOW);
    } else if (right_speed < 0) {
        digitalWrite(RIGHT_MOTOR_PIN_1, LOW);
        digitalWrite(RIGHT_MOTOR_PIN_2, HIGH);
    } else {
        digitalWrite(RIGHT_MOTOR_PIN_1, LOW);
        digitalWrite(RIGHT_MOTOR_PIN_2, LOW);
    }
    ledcWrite(RIGHT_PWM_CHANNEL, abs(right_speed));
}

void stop_motors() {
    set_motor_speeds(0, 0);
}

// ==================== AUDIO FUNCTIONS ====================

void play_startup_sound() {
    Serial.println("Playing startup sound...");
    
    // Generate simple beep sound
    int sample_rate = 16000;
    int frequency = 1000; // Hz
    int duration = 200; // ms
    
    int samples = (sample_rate * duration) / 1000;
    int16_t *audio_data = (int16_t *)malloc(samples * sizeof(int16_t));
    
    // Generate sine wave
    for (int i = 0; i < samples; i++) {
        float angle = 2 * 3.14159 * frequency * i / sample_rate;
        audio_data[i] = (int16_t)(sin(angle) * 32767);
    }
    
    // Send to I2S
    size_t bytes_written = 0;
    i2s_write(I2S_NUM, audio_data, samples * 2, &bytes_written, portMAX_DELAY);
    
    free(audio_data);
}

void simulate_speech(String text) {
    is_speaking = true;
    Serial.println("Simulating speech: " + text);
    
    // Play beep sequence to simulate speech
    for (int i = 0; i < 3; i++) {
        play_tone(1000 + i * 100, 100);
        delay(50);
    }
    
    is_speaking = false;
}

void play_tone(int frequency, int duration) {
    int sample_rate = 16000;
    int samples = (sample_rate * duration) / 1000;
    
    int16_t *audio_data = (int16_t *)malloc(samples * sizeof(int16_t));
    
    for (int i = 0; i < samples; i++) {
        float angle = 2 * 3.14159 * frequency * i / sample_rate;
        audio_data[i] = (int16_t)(sin(angle) * 32767);
    }
    
    size_t bytes_written = 0;
    i2s_write(I2S_NUM, audio_data, samples * 2, &bytes_written, portMAX_DELAY);
    
    free(audio_data);
}

// ==================== STATUS REPORTING ====================

void send_status_to_raspi() {
    Serial.println("Status: Online, Uptime: " + String(millis() / 1000) + "s");
}
