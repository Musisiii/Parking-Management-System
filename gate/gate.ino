#include <Servo.h>

// Pin Definitions
#define TRIGGER_PIN 2
#define ECHO_PIN 3
#define RED_LED_PIN 4
#define BLUE_LED_PIN 5
#define SERVO_PIN 6
#define GND_PIN_1 7
#define GND_PIN_2 8
#define BUZZER_PIN 11

// System State
bool gateOpen = false;
bool alertActive = false; // Alert state flag
unsigned long alertStartTime = 0; // Timer for alert duration
const unsigned long alertDuration = 10000; // Alert duration in milliseconds (10 seconds)
unsigned long gateOpenTime = 0; // Timer for gate open duration
const unsigned long gateOpenDuration = 10000; // Gate open duration in milliseconds (10 seconds)

Servo barrierServo;

// ================== INITIALIZATION ==================
void initializeSerial() {
  Serial.begin(9600);
}

void initializeUltrasonic() {
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void initializeLEDs() {
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);
}

void initializeBuzzer() {
  pinMode(BUZZER_PIN, OUTPUT);
}

void initializeHardcodedGrounds() {
  pinMode(GND_PIN_1, OUTPUT);
  pinMode(GND_PIN_2, OUTPUT);
  digitalWrite(GND_PIN_1, LOW);
  digitalWrite(GND_PIN_2, LOW);
}

void initializeServo() {
  barrierServo.attach(SERVO_PIN);
  setGatePosition(6); // Start with closed gate
}

// ================== CORE FUNCTIONS ==================
float measureDistance() {
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  return (duration * 0.0343) / 2.0;
}

void setGatePosition(int angle) {
  barrierServo.write(angle);
}

// ================== GATE CONTROL ==================
void openGate() {
  if (alertActive) return; // Prevent opening during alerts
  setGatePosition(90);
  gateOpen = true;
  gateOpenTime = millis(); // Start the timer for gate open duration
  digitalWrite(BLUE_LED_PIN, HIGH);
  digitalWrite(RED_LED_PIN, LOW);
  Serial.println("[GATE] Opened");
}

void closeGate() {
  setGatePosition(6);
  gateOpen = false;
  digitalWrite(BLUE_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, HIGH);
  Serial.println("[GATE] Closed");
}

void handleGateAutoClose() {
  if (gateOpen && millis() - gateOpenTime >= gateOpenDuration) {
    closeGate();
  }
}

// ================== ALERT SYSTEM ==================
void triggerAlert() {
  alertActive = true;
  alertStartTime = millis(); // Start the timer
  digitalWrite(RED_LED_PIN, HIGH);
  tone(BUZZER_PIN, 1000); // Play a 1kHz tone
  Serial.println("[ALERT] Unpaid vehicle detected! Buzzer activated.");
}

void handleAlert() {
  if (alertActive && millis() - alertStartTime >= alertDuration) {
    alertActive = false;
    digitalWrite(RED_LED_PIN, LOW);
    noTone(BUZZER_PIN); // Stop the tone
    Serial.println("[ALERT] Automatically stopped after 10 seconds");
  }
}

// ================== COMMAND HANDLER ==================
void handleSerialCommands() {
  if (Serial.available()) {
    char cmd = Serial.read();
    switch (cmd) {
      case '1': // Open gate
        openGate();
        break;
      case '0': // Close gate
        closeGate();
        break;
      case '2': // Trigger alert
        triggerAlert();
        break;
      default:
        Serial.println("[ERROR] Unknown command");
    }
  }
}

// ================== MAIN LOOP ==================
void setup() {
  initializeSerial();
  initializeUltrasonic();
  initializeLEDs();
  initializeBuzzer();
  initializeHardcodedGrounds();
  initializeServo();
  // Initial state
  digitalWrite(RED_LED_PIN, HIGH); // Closed gate indicator
  Serial.println("[SYSTEM] Ready");
}

void loop() {
  // 1. Sensor monitoring
  float distance = measureDistance();
  Serial.print("[DISTANCE] ");
  Serial.println(distance);

  // 2. Command handling
  handleSerialCommands();

  // 3. Alert management
  handleAlert();

  // 4. Gate auto-close management
  handleGateAutoClose();

  delay(50); // Main loop delay
}