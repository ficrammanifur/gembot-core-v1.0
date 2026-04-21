// === PIN MOTOR (Sesuai konfigurasi kamu) ===
const int in1R = 12;
const int in2R = 14;
const int enR  = 13;

const int in1L = 18;
const int in2L = 5;
const int enL  = 33;

void setupMotors() {
  pinMode(in1R, OUTPUT);
  pinMode(in2R, OUTPUT);
  pinMode(enR, OUTPUT);
  pinMode(in1L, OUTPUT);
  pinMode(in2L, OUTPUT);
  pinMode(enL, OUTPUT);
  
  // Pastikan motor berhenti saat awal nyala
  stopMotors();
}

void stopMotors() {
  digitalWrite(in1R, LOW); digitalWrite(in2R, LOW);
  digitalWrite(in1L, LOW); digitalWrite(in2L, LOW);
  analogWrite(enR, 0);
  analogWrite(enL, 0);
}

void moveForward(int speed) {
  digitalWrite(in1R, LOW); digitalWrite(in2R, HIGH);
  digitalWrite(in1L, LOW); digitalWrite(in2L, HIGH);
  analogWrite(enR, speed);
  analogWrite(enL, speed);
}

void moveBackward(int speed) {
  digitalWrite(in1R, HIGH); digitalWrite(in2R, LOW);
  digitalWrite(in1L, HIGH); digitalWrite(in2L, LOW);
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
