#define PIN_KILL 42
#define PIN_INT  40

void setup() {
  Serial.begin(115200);
  pinMode(PIN_INT, INPUT_PULLUP);
}

uint8_t lastIntVal = 1;
uint8_t pushCount = 0;

void loop() {
  delay(50);
  uint8_t int_val = digitalRead(PIN_INT);
  if (lastIntVal != int_val) {
    if (0 == int_val) {
      pushCount++;
    }
    Serial.printf("int_val changed to=%d, count=%d\n", lastIntVal, pushCount);
    lastIntVal = int_val;
    if (pushCount >= 3) {
      Serial.printf("Count==3 reached, shutdown power NOW!\n");
      delay(200);
      pinMode(PIN_KILL, OUTPUT);
      digitalWrite(PIN_KILL, LOW);
    }
  }
}
