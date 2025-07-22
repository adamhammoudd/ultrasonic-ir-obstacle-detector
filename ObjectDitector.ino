#include <LiquidCrystal_I2C.h>
#include <IRremote.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int trigPin = A1;
const int echoPin = A0;
const int RECV_PIN = 12;

IRrecv irrecv(RECV_PIN);
decode_results results;

float duration, distance;
unsigned long lastValidCode = 0;
int mode = 0;  // 0 = idle, 1 = <100cm, 2 = <50cm, 3 = <25cm
unsigned long lastUpdate = 0;
bool modeChanged = false;
unsigned long modeChangeTime = 0;

void setup() {
  lcd.init();
  lcd.backlight();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  Serial.begin(9600);
  irrecv.enableIRIn();
  irrecv.blink13(true);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Press 1, 2 or 3");
}

void measureDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration * 0.0343) / 2;
}

void displayModeChange() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Mode ");
  lcd.print(mode);
  lcd.print(": ");

  lcd.setCursor(0, 1);
  switch(mode) {
    case 1: lcd.print("No range"); break;
    case 2: lcd.print("100cm range"); break;
    case 3: lcd.print("50cm range"); break;
    default: lcd.print("Idle mode"); break;
  }
}

void displayScanning() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scanning...");
}

void displayObstacle() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Obstacle Found!");
  lcd.setCursor(0, 1);
  lcd.print("Dist: ");
  lcd.print(distance, 1);
  lcd.print(" cm");
}

void loop() {
  // Check IR input
  if (irrecv.decode(&results)) {
    unsigned long key = results.value;

    // Handle repeat
    if (key == 0xFFFFFFFF) {
      key = lastValidCode;
    } else {
      lastValidCode = key;
    }

    Serial.print("IR Code: 0x");
    Serial.println(key, HEX);

    // Update mode based on key (maintaining your original mode ranges)
    int newMode = mode;
    if (key == 0xFF6897 || key == 0xC101E57B) newMode = 1;     // Button "1" (<100cm)
    else if (key == 0xFF9867 || key == 0x9078D207 || key == 0x97483BFB || key == 0xFCABFFBF || key == 0x297C14F0 || key == 0xB91B3FB4 ||
             key == 0x9BE5ECC5 || key == 0xCE206818 || key == 0xFDAC0150) newMode = 2; // Button "2" (<50cm)
    else if (key == 0xFFB04F || key == 0xF0C41643 || key == 0x6819A1AB || key == 0xEAD7523A) newMode = 3; // Button "3" (<25cm)
    else newMode = 0;                     // Any other button goes back to idle

    if (newMode != mode) {
      mode = newMode;
      modeChanged = true;
      modeChangeTime = millis();
      displayModeChange();
    }

    irrecv.resume();
    delay(200); // Prevent bounce
  }

  // Handle mode change display timeout
  if (modeChanged && (millis() - modeChangeTime >= 1000)) {
    modeChanged = false;
    lastUpdate = millis() - 1000; // Force immediate update
  }

  // Handle normal operation
  if (!modeChanged) {
    if (mode == 0) {
      if (millis() - lastUpdate >= 2000) {
        lastUpdate = millis();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Press 1, 2 or 3");
      }
    } else {
      // Measure and display every 1 second
      if (millis() - lastUpdate >= 1000) {
        lastUpdate = millis();
        measureDistance();
        Serial.print("Distance: ");
        Serial.println(distance);

        // Original obstacle detection logic
        if ((mode == 1) ||
            (mode == 2 && distance <= 100) ||
            (mode == 3 && distance <= 50)) {
          displayObstacle();
        } else {
          displayScanning();
        }
      }
    }
  }
}