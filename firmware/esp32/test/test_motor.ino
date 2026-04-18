#define SPEED 200

// === Motor Driver Pins ===
#define MOTOR_ENA 11  // Motor kanan (PWM)
#define MOTOR_IN1 12  // Motor kanan
#define MOTOR_IN2 13

#define MOTOR_ENB 5   // Motor kiri (PWM)
#define MOTOR_IN3 6   // Motor kiri
#define MOTOR_IN4 7

void setup() {
  Serial.begin(9600);

  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  pinMode(MOTOR_IN3, OUTPUT);
  pinMode(MOTOR_IN4, OUTPUT);
  pinMode(MOTOR_ENA, OUTPUT);
  pinMode(MOTOR_ENB, OUTPUT);

  stopMotors();

  Serial.println("Kontrol Robot via Serial Monitor");
  Serial.println("Perintah:");
  Serial.println("w = Maju\ta = Kiri");
  Serial.println("s = Mundur\td = Kanan");
  Serial.println("x = Stop");
  Serial.println("-----------------------------");
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();

    // Abaikan karakter newline dan carriage return
    if (cmd == '\n' || cmd == '\r') return;

    switch (cmd) {
      case 'w': maju(); break;
      case 's': mundur(); break;
      case 'a': belokKiri(); break;
      case 'd': belokKanan(); break;
      case 'x': stopMotors(); break;
      default: Serial.println("Perintah tidak valid! Gunakan w,a,s,d,x");
    }
  }
}

// === Fungsi Motor ===
void maju() {
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, HIGH);
  digitalWrite(MOTOR_IN3, LOW);
  digitalWrite(MOTOR_IN4, HIGH);
  analogWrite(MOTOR_ENA, SPEED + 3); // Koreksi motor kanan
  analogWrite(MOTOR_ENB, SPEED);
  Serial.println("MAJU");
}

void mundur() {
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, HIGH);
  digitalWrite(MOTOR_IN4, LOW);
  analogWrite(MOTOR_ENA, SPEED);
  analogWrite(MOTOR_ENB, SPEED);
  Serial.println("MUNDUR");
}

void belokKiri() {
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, LOW);
  digitalWrite(MOTOR_IN4, HIGH);
  analogWrite(MOTOR_ENA, SPEED);
  analogWrite(MOTOR_ENB, SPEED);
  Serial.println("BELOK KIRI");
}

void belokKanan() {
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, HIGH);
  digitalWrite(MOTOR_IN3, HIGH);
  digitalWrite(MOTOR_IN4, LOW);
  analogWrite(MOTOR_ENA, SPEED);
  analogWrite(MOTOR_ENB, SPEED);
  Serial.println("BELOK KANAN");
}

void stopMotors() {
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, LOW);
  digitalWrite(MOTOR_IN3, LOW);
  digitalWrite(MOTOR_IN4, LOW);
  analogWrite(MOTOR_ENA, 0);
  analogWrite(MOTOR_ENB, 0);
  Serial.println("STOP");
}
