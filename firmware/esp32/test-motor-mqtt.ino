#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

// Motor Pins
const int in1R=12, in2R=14, enR=13;
const int in1L=18, in2L=5, enL=33;

// MQTT Config
const char* mqtt_server = "192.168.1.19"; // Ganti dengan IP Raspberry Pi
const int mqtt_port = 1883;
const char* topic_cmd = "gembot/cmd";
const char* topic_status = "gembot/status";
const char* topic_tts = "gembot/tts";

WiFiClient espClient;
PubSubClient mqtt(espClient);

void setupMotors() {
  pinMode(in1R, OUTPUT); pinMode(in2R, OUTPUT); pinMode(enR, OUTPUT);
  pinMode(in1L, OUTPUT); pinMode(in2L, OUTPUT); pinMode(enL, OUTPUT);
  stopMotors();
}

void stopMotors() {
  digitalWrite(in1R, LOW); digitalWrite(in2R, LOW);
  digitalWrite(in1L, LOW); digitalWrite(in2L, LOW);
  analogWrite(enR, 0); analogWrite(enL, 0);
}

void moveForward(int s) {
  digitalWrite(in1R, LOW); digitalWrite(in2R, HIGH);
  digitalWrite(in1L, LOW); digitalWrite(in2L, HIGH);
  analogWrite(enR, s); analogWrite(enL, s);
}

void moveBackward(int s) {
  digitalWrite(in1R, HIGH); digitalWrite(in2R, LOW);
  digitalWrite(in1L, HIGH); digitalWrite(in2L, LOW);
  analogWrite(enR, s); analogWrite(enL, s);
}

void turnRight(int s) {
  digitalWrite(in1R, HIGH); digitalWrite(in2R, LOW);
  digitalWrite(in1L, LOW); digitalWrite(in2L, HIGH);
  analogWrite(enR, s); analogWrite(enL, s);
}

void turnLeft(int s) {
  digitalWrite(in1R, LOW); digitalWrite(in2R, HIGH);
  digitalWrite(in1L, HIGH); digitalWrite(in2L, LOW);
  analogWrite(enR, s); analogWrite(enL, s);
}

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  String msg;
  for(int i=0;i<len;i++) msg+=(char)payload[i];
  
  DynamicJsonDocument doc(200);
  deserializeJson(doc, msg);
  
  String cmd = doc["command"];
  int speed = doc["speed"] | 200;
  
  if(cmd == "forward") moveForward(speed);
  else if(cmd == "backward") moveBackward(speed);
  else if(cmd == "left") turnLeft(speed);
  else if(cmd == "right") turnRight(speed);
  else stopMotors();
  
  // Publish status
  DynamicJsonDocument status(200);
  status["status"] = cmd;
  status["command"] = cmd;
  status["speed"] = speed;
  String output;
  serializeJson(status, output);
  mqtt.publish(topic_status, output.c_str());
}

void reconnectMQTT() {
  while(!mqtt.connected()) {
    if(mqtt.connect("ESP32_Gembot")) {
      mqtt.subscribe(topic_cmd);
      mqtt.subscribe(topic_tts);
    } else delay(5000);
  }
}

void setup() {
  Serial.begin(115200);
  setupMotors();
  
  WiFiManager wm;
  wm.autoConnect("GembotRobot");
  
  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(mqttCallback);
  reconnectMQTT();
}

void loop() {
  if(!mqtt.connected()) reconnectMQTT();
  mqtt.loop();
  delay(10);
}
