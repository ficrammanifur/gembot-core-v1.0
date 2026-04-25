#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// === LED STATUS ===
#define LED_BUILTIN 2

// === PIN MOTOR (SUDAH AMAN, NO INPUT-ONLY) ===
const int in1R = 12;
const int in2R = 14;
const int enR = 13;
const int in1L = 5;
const int in2L = 18;
const int enL = 33;

// === BLE CONFIG ===
#define BLE_NAME "ROBOT-BLE"
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// === BLE VAR ===
BLEServer *pServer = NULL; 
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// === MOTOR FUNCTION ===
void stopMotors() {
  digitalWrite(in1R, LOW); digitalWrite(in2R, LOW);
  digitalWrite(in1L, LOW); digitalWrite(in2L, LOW);
}

void moveForward(int speed) {
  digitalWrite(in1R, HIGH); digitalWrite(in2R, LOW);
  digitalWrite(in1L, HIGH); digitalWrite(in2L, LOW);
  analogWrite(enR, speed);
  analogWrite(enL, speed);
}

void moveBackward(int speed) {
  digitalWrite(in1R, LOW); digitalWrite(in2R, HIGH);
  digitalWrite(in1L, LOW); digitalWrite(in2L, HIGH);
  analogWrite(enR, speed);
  analogWrite(enL, speed);
}

void turnRight(int speed) {
  digitalWrite(in1R, HIGH); digitalWrite(in2R, LOW);
  digitalWrite(in1L, LOW); digitalWrite(in2L, HIGH);
  analogWrite(enR, speed);
  analogWrite(enL, speed);
}

void turnLeft(int speed) {
  digitalWrite(in1R, LOW); digitalWrite(in2R, HIGH);
  digitalWrite(in1L, HIGH); digitalWrite(in2L, LOW);
  analogWrite(enR, speed);
  analogWrite(enL, speed);
}

// === CALLBACK BLE CONNECT ===
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    digitalWrite(LED_BUILTIN, HIGH);
  }
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    digitalWrite(LED_BUILTIN, LOW);
  }
};

// === CALLBACK DATA MASUK (DIPERBAIKI) ===
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String rxValue = pCharacteristic->getValue(); // Menggunakan String, bukan std::string
    
    if (rxValue.length() >= 7) {
      uint8_t buttons = (uint8_t)rxValue[6];
      
      if (buttons & 0x01) {
        moveForward(200);
        Serial.println("Forward");
      }
      else if (buttons & 0x02) {
        moveBackward(150);
        Serial.println("Backward");
      }
      else if (buttons & 0x04) {
        turnLeft(150);
        Serial.println("Left");
      }
      else if (buttons & 0x08) {
        turnRight(150);
        Serial.println("Right");
      }
      else {
        stopMotors();
        Serial.println("Stop");
      }
    }
  }
};

// === SETUP ===
void setup() {
  Serial.begin(115200);
  
  // MOTOR PIN
  pinMode(in1R, OUTPUT);
  pinMode(in2R, OUTPUT);
  pinMode(enR, OUTPUT);
  pinMode(in1L, OUTPUT);
  pinMode(in2L, OUTPUT);
  pinMode(enL, OUTPUT);
  
  // Set enable pins HIGH untuk mengaktifkan motor driver
  digitalWrite(enR, HIGH);
  digitalWrite(enL, HIGH);
  
  // LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  // BLE INIT
  BLEDevice::init(BLE_NAME);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // TX (Notify)
  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
  pTxCharacteristic->addDescriptor(new BLE2902());
  
  // RX (Write)
  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_RX,
                        BLECharacteristic::PROPERTY_WRITE
                      );
  pRxCharacteristic->setCallbacks(new MyCallbacks());
  
  pService->start();
  pServer->getAdvertising()->start();
  
  Serial.println("BLE Ready...");
  Serial.println("Waiting for connection...");
}

// === LOOP ===
void loop() {
  // Handle koneksi BLE
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = true;
    Serial.println("BLE Connected");
  }
  
  if (!deviceConnected && oldDeviceConnected) {
    oldDeviceConnected = false;
    delay(500);
    pServer->startAdvertising();
    stopMotors();
    Serial.println("BLE Disconnected");
    Serial.println("Waiting for connection...");
  }
  
  delay(10);
}
