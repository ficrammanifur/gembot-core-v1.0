#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// === LED untuk status koneksi ===
#define LED_BUILTIN 2 // Pin LED bawaan untuk menunjukkan status koneksi BLE

// === Pin Motor ===
const int in1R = 14; // Pin input 1 untuk motor kanan
const int in2R = 27; // Pin input 2 untuk motor kanan
const int enR = 12;  // Pin enable (PWM) untuk kecepatan motor kanan
const int in1L = 26; // Pin input 1 untuk motor kiri
const int in2L = 25; // Pin input 2 untuk motor kiri
const int enL = 33;  // Pin enable (PWM) untuk kecepatan motor kiri

// === BLE Variables ===
BLEServer *pServer = NULL; // Pointer ke server BLE
BLECharacteristic *pTxCharacteristic; // Karakteristik untuk mengirim data (notify)
bool deviceConnected = false; // Status koneksi BLE saat ini
bool oldDeviceConnected = false; // Status koneksi BLE sebelumnya untuk deteksi perubahan

#define BLE_NAME "ROBOT-BLE" // Nama perangkat BLE
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UUID untuk layanan BLE
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // UUID untuk karakteristik penerima data
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // UUID untuk karakteristik pengirim data

// === Deklarasi Fungsi Motor ===
void stopMotors(); // Fungsi untuk menghentikan motor
void moveForward(int speed); // Fungsi untuk menggerakkan robot maju
void moveBackward(int speed); // Fungsi untuk menggerakkan robot mundur
void turnRight(int speed); // Fungsi untuk memutar robot ke kanan
void turnLeft(int speed); // Fungsi untuk memutar robot ke kiri

// === Callback Saat Koneksi BLE Terhubung / Terputus ===
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true; // Set status terhubung
    digitalWrite(LED_BUILTIN, HIGH); // Nyalakan LED saat terhubung
  }

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false; // Set status terputus
    digitalWrite(LED_BUILTIN, LOW); // Matikan LED saat terputus
  }
};

// === Callback Saat Data Masuk dari Aplikasi ===
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String rxValue = pCharacteristic->getValue(); // Ambil data yang diterima dari klien

    if (rxValue.length() >= 7) { // Periksa panjang data minimal 7 byte
      uint8_t buttons = rxValue[6]; // Ambil byte ke-7 untuk data tombol

      if (buttons & 0x01) { // Jika bit 0 aktif (tombol maju)
        moveForward(200); // Gerakkan robot maju dengan kecepatan 200
        Serial.println("Forward"); // Cetak status ke Serial Monitor
      } else if (buttons & 0x02) { // Jika bit 1 aktif (tombol mundur)
        moveBackward(150); // Gerakkan robot mundur dengan kecepatan 150
        Serial.println("Backward"); // Cetak status
      } else if (buttons & 0x04) { // Jika bit 2 aktif (tombol kiri)
        turnLeft(150); // Putar robot ke kiri dengan kecepatan 150
        Serial.println("Left"); // Cetak status
      } else if (buttons & 0x08) { // Jika bit 3 aktif (tombol kanan)
        turnRight(150); // Putar robot ke kanan dengan kecepatan 150
        Serial.println("Right"); // Cetak status
      } else { // Jika tidak ada tombol aktif
        stopMotors(); // Hentikan motor
        Serial.println("Stop"); // Cetak status
      }
    }
  }
};

void setup() {
  Serial.begin(115200); // Inisialisasi Serial Monitor dengan baud rate 115200

  // === Setup Pin Motor ===
  pinMode(enR, OUTPUT); // Set pin enable motor kanan sebagai output
  pinMode(in1R, OUTPUT); // Set pin input 1 motor kanan sebagai output
  pinMode(in2R, OUTPUT); // Set pin input 2 motor kanan sebagai output
  pinMode(enL, OUTPUT); // Set pin enable motor kiri sebagai output
  pinMode(in1L, OUTPUT); // Set pin input 1 motor kiri sebagai output
  pinMode(in2L, OUTPUT); // Set pin input 2 motor kiri sebagai output

  digitalWrite(enR, HIGH); // Aktifkan enable motor kanan
  digitalWrite(enL, HIGH); // Aktifkan enable motor kiri

  // === Setup LED Status Koneksi ===
  pinMode(LED_BUILTIN, OUTPUT); // Set pin LED sebagai output
  digitalWrite(LED_BUILTIN, LOW); // Matikan LED saat inisialisasi

  // === BLE Setup ===
  BLEDevice::init(BLE_NAME); // Inisialisasi BLE dengan nama ROBOT-BLE
  pServer = BLEDevice::createServer(); // Buat server BLE
  pServer->setCallbacks(new MyServerCallbacks()); // Set callback untuk koneksi

  BLEService *pService = pServer->createService(SERVICE_UUID); // Buat layanan BLE dengan UUID

  // Buat karakteristik untuk notifikasi (mengirim data ke klien)
  pTxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pTxCharacteristic->addDescriptor(new BLE2902()); // Tambahkan deskriptor untuk notifikasi

  // Buat karakteristik untuk menerima data dari klien
  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX,
    BLECharacteristic::PROPERTY_WRITE
  );
  pRxCharacteristic->setCallbacks(new MyCallbacks()); // Set callback untuk data masuk

  pService->start(); // Mulai layanan BLE
  pServer->getAdvertising()->start(); // Mulai advertising agar perangkat ditemukan

  Serial.println("BLE Ready, waiting for connection..."); // Cetak status ke Serial Monitor
}

// === LOOP ===
void loop() {
  // Jika perangkat baru terhubung
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = true; // Perbarui status koneksi sebelumnya
    Serial.println("BLE Connected"); // Cetak status terhubung
  }

  // Jika perangkat terputus
  if (!deviceConnected && oldDeviceConnected) {
    oldDeviceConnected = false; // Perbarui status koneksi sebelumnya
    delay(500); // Tunggu 500ms sebelum mulai ulang advertising
    pServer->startAdvertising(); // Mulai ulang advertising
    stopMotors(); // Hentikan motor
    Serial.println("BLE Disconnected, advertising..."); // Cetak status terputus
  }

  delay(10); // Delay kecil untuk mencegah loop berjalan terlalu cepat
}

// === MOTOR CONTROL ===
void stopMotors() {
  digitalWrite(in1R, LOW); digitalWrite(in2R, LOW); // Matikan motor kanan
  digitalWrite(in1L, LOW); digitalWrite(in2L, LOW); // Matikan motor kiri
}

void moveForward(int speed) {
  digitalWrite(in1R, LOW); digitalWrite(in2R, HIGH); // Atur motor kanan maju
  digitalWrite(in1L, LOW); digitalWrite(in2L, HIGH); // Atur motor kiri maju
  analogWrite(enR, speed); // Set kecepatan motor kanan
  analogWrite(enL, speed); // Set kecepatan motor kiri
}

void moveBackward(int speed) {
  digitalWrite(in1R, HIGH); digitalWrite(in2R, LOW); // Atur motor kanan mundur
  digitalWrite(in1L, HIGH); digitalWrite(in2L, LOW); // Atur motor kiri mundur
  analogWrite(enR, speed); // Set kecepatan motor kanan
  analogWrite(enL, speed); // Set kecepatan motor kiri
}

void turnRight(int speed) {
  digitalWrite(in1R, HIGH); digitalWrite(in2R, LOW); // Motor kanan mundur
  digitalWrite(in1L, LOW); digitalWrite(in2L, HIGH); // Motor kiri maju
  analogWrite(enR, speed); // Set kecepatan motor kanan
  analogWrite(enL, speed); // Set kecepatan motor kiri
}

void turnLeft(int speed) {
  digitalWrite(in1R, LOW); digitalWrite(in2R, HIGH); // Motor kanan maju
  digitalWrite(in1L, HIGH); digitalWrite(in2L, LOW); // Motor kiri mundur
  analogWrite(enR, speed); // Set kecepatan motor kanan
  analogWrite(enL, speed); // Set kecepatan motor kiri
}
