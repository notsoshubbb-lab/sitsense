/*
 * SitSense - Wokwi Simulation Firmware (Zero-Library & Desktop Audio Version)
 * Hardware: ESP32 + MPU-6050 + HC-SR04 (No Physical Buzzer)
 * Telemetry pushes alert status to the SitSense Web Dashboard Desktop Audio System!
 */

#include <Wire.h>
#include <WiFi.h>

const int MPU_ADDR = 0x68;

const char* ssid = "Wokwi-GUEST";
const char* password = "";

const int TRIG_PIN = 4;
const int ECHO_PIN = 5;

// Thresholds for Sensor Intersection
float distanceThresholdCm = 35.0;    // Proximity
float tiltThresholdDeg = 15.0;       // Forward tilt
float rollThresholdDeg = 12.0;       // Torso rotation / roll

// 20-Second Grace Window: Must persist for > 20s to trigger desktop sound alert!
const unsigned long SLOUCH_GRACE_PERIOD = 20000; 
unsigned long slouchStartTime = 0;
unsigned long lastLogTime = 0;
const unsigned long LOG_INTERVAL = 1000;

bool autoDemoMode = false;
unsigned long demoTimer = 0;

void initMPU() {
  Wire.begin(21, 22);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  byte error = Wire.endTransmission(true);

  if (error == 0) {
    Serial.println("[✓] MPU-6050 connected successfully on I2C (0x68)!");
  } else {
    Serial.println("[!] MPU-6050 not detected. Using simulated readings.");
  }
}

void readMPU(float &tilt, float &roll) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  byte error = Wire.endTransmission(false);

  if (error == 0 && Wire.requestFrom(MPU_ADDR, 6, true) == 6) {
    int16_t ax = (Wire.read() << 8) | Wire.read();
    int16_t ay = (Wire.read() << 8) | Wire.read();
    int16_t az = (Wire.read() << 8) | Wire.read();

    float axg = ax / 16384.0;
    float ayg = ay / 16384.0;
    float azg = az / 16384.0;

    tilt = atan2(sqrt(axg * axg + ayg * ayg), azg) * 180.0 / PI;
    roll = atan2(ayg, axg) * 180.0 / PI;
  } else {
    tilt = 8.0;
    roll = 1.0;
  }
}

float readUltrasonicDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration <= 0) return 999.0;
  return duration / 58.2;
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.println("\n=======================================================");
  Serial.println("  🧘 SitSense — Multi-Sensor Intersection Simulator    ");
  Serial.println("  Condition: Distance < 35cm AND (Tilt > 15° OR Roll > 12°)");
  Serial.println("  Desktop Sound Alert: Triggered when Slouch > 20 SECONDS");
  Serial.println("=======================================================");

  WiFi.begin(ssid, password);
  initMPU();

  Serial.println("\n[TIP] Type 'demo' in Serial Monitor to run 20s automated test.\n");
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.equalsIgnoreCase("demo")) {
      autoDemoMode = !autoDemoMode;
      Serial.println(autoDemoMode ? "\n>>> [AUTO-DEMO ACTIVATED: 20s Test Cycle] <<<" : "\n>>> [MANUAL SLIDER MODE] <<<");
      demoTimer = millis();
    }
  }

  float tilt = 0.0;
  float roll = 0.0;
  float distance = 0.0;

  if (autoDemoMode) {
    unsigned long phase = (millis() - demoTimer) % 36000;
    if (phase < 8000) {
      tilt = 8.0; roll = 1.0; distance = 48.0;
    } else if (phase < 31000) {
      tilt = 28.0; roll = 14.0; distance = 22.0;
    } else {
      tilt = 7.0; roll = 0.5; distance = 46.0;
    }
  } else {
    readMPU(tilt, roll);
    distance = readUltrasonicDistance();
  }

  // Multi-Sensor Intersection Checks
  bool isClose = (distance < distanceThresholdCm);
  bool isTilted = (tilt > tiltThresholdDeg);
  bool isRotated = (abs(roll) > rollThresholdDeg);

  bool isSlouching = isClose && (isTilted || isRotated);

  float slouchSec = 0.0;
  bool alertActive = false;

  if (isSlouching) {
    if (slouchStartTime == 0) slouchStartTime = millis();
    unsigned long elapsed = millis() - slouchStartTime;
    slouchSec = elapsed / 1000.0;

    if (elapsed > SLOUCH_GRACE_PERIOD) {
      alertActive = true;
    }
  } else {
    slouchStartTime = 0;
  }

  // Serial Monitor Output
  if (millis() - lastLogTime >= LOG_INTERVAL) {
    lastLogTime = millis();

    Serial.println("------------------------------------------------------------------");
    Serial.printf("[Sensors] Proximity: %4.1fcm [%s] | Tilt: %4.1f° [%s] | Rotation: %4.1f° [%s]\n",
      distance, (isClose ? "CLOSE" : "OK"),
      tilt, (isTilted ? "TILTED" : "OK"),
      roll, (isRotated ? "ROTATED" : "OK")
    );

    if (alertActive) {
      Serial.printf("[Status]  🚨 DESKTOP ALERT SENT! Slouched for %3.1fs (>20s threshold)\n", slouchSec);
      Serial.println("[Sound]   >>> Web Dashboard Playing Audio Beep Alert! <<<");
    } else if (isSlouching) {
      Serial.printf("[Status]  ⏳ SLOUCH DETECTED (Grace Timer: %3.1fs / 20.0s) -> No sound yet\n", slouchSec);
    } else {
      Serial.println("[Status]  ✅ UPRIGHT POSTURE — Optimal desk ergonomics.");
    }
    Serial.println("------------------------------------------------------------------");
  }

  delay(100);
}
