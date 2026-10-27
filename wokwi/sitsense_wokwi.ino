/*
 * SitSense - Wokwi Simulation Firmware
 * Multi-Sensor Intersection Posture Logic:
 *  - HC-SR04 Proximity (< 35cm)
 *  - MPU-6050 Forward Tilt (> 15°)
 *  - MPU-6050 Torso Rotation / Roll (> 12°)
 *  - Grace Period: Beeps ONLY after sustained slouching for > 20 SECONDS!
 */

#include <Wire.h>
#include <MPU6050.h>
#include <WiFi.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";

MPU6050 mpu;
const int TRIG_PIN = 4;
const int ECHO_PIN = 5;
const int BUZZER_PIN = 18;

// Thresholds for Sensor Intersection
float distanceThresholdCm = 35.0;    // Proximity
float tiltThresholdDeg = 15.0;       // Forward tilt
float rollThresholdDeg = 12.0;       // Torso rotation / roll

// 20-Second Grace Window: Must persist for > 20s to beep!
const unsigned long SLOUCH_GRACE_PERIOD = 20000; 
unsigned long slouchStartTime = 0;
unsigned long lastLogTime = 0;
const unsigned long LOG_INTERVAL = 1000;

bool autoDemoMode = false;
unsigned long demoTimer = 0;

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("\n=======================================================");
  Serial.println("  🧘 SitSense — Multi-Sensor Intersection Simulator    ");
  Serial.println("  Threshold: Distance < 35cm AND (Tilt > 15° OR Roll > 12°)");
  Serial.println("  Alarm Condition: Sustained Slouch for > 20 SECONDS   ");
  Serial.println("=======================================================");

  WiFi.begin(ssid, password);

  Wire.begin(21, 22);
  mpu.initialize();
  if (mpu.testConnection()) {
    Serial.println("[✓] MPU-6050 connection successful!");
  } else {
    Serial.println("[!] MPU-6050 simulated.");
  }

  Serial.println("\n[TIP] Type 'demo' in Serial Monitor to run 20s automated scenario.\n");
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
    // 35-second demo cycle to clearly demonstrate the 20s grace period and trigger
    unsigned long phase = (millis() - demoTimer) % 36000;
    if (phase < 8000) {
      // Phase 1: Good Posture (8 seconds)
      tilt = 8.0; roll = 1.0; distance = 48.0;
    } else if (phase < 31000) {
      // Phase 2: Sustained Slouch (23 seconds total)
      // Seconds 8..28 (0s to 20s): Grace period, countdown displays, NO beep!
      // Seconds 28..31 (> 20s): Buzzer ACTIVATES!
      tilt = 28.0; roll = 14.0; distance = 22.0;
    } else {
      // Phase 3: Posture Correction
      tilt = 7.0; roll = 0.5; distance = 46.0;
    }
  } else {
    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    float axg = ax / 16384.0;
    float ayg = ay / 16384.0;
    float azg = az / 16384.0;

    tilt = atan2(sqrt(axg * axg + ayg * ayg), azg) * 180.0 / PI;
    roll = atan2(ayg, axg) * 180.0 / PI;
    distance = readUltrasonicDistance();
  }

  // Multi-Sensor Intersection
  bool isClose = (distance < distanceThresholdCm);
  bool isTilted = (tilt > tiltThresholdDeg);
  bool isRotated = (abs(roll) > rollThresholdDeg);

  // Intersection: Must be close to desk AND deformed (tilted or rotated)
  bool isSlouching = isClose && (isTilted || isRotated);

  float slouchSec = 0.0;
  bool buzzerActive = false;

  if (isSlouching) {
    if (slouchStartTime == 0) slouchStartTime = millis();
    unsigned long elapsed = millis() - slouchStartTime;
    slouchSec = elapsed / 1000.0;

    // Beep only when slouching persists for MORE THAN 20 SECONDS!
    if (elapsed > SLOUCH_GRACE_PERIOD) {
      buzzerActive = true;
      tone(BUZZER_PIN, 2000, 200);
    }
  } else {
    slouchStartTime = 0;
    noTone(BUZZER_PIN);
  }

  // Serial Monitor Dashboard Output
  if (millis() - lastLogTime >= LOG_INTERVAL) {
    lastLogTime = millis();

    Serial.println("------------------------------------------------------------------");
    Serial.printf("[Sensors] Proximity: %4.1fcm [%s] | Tilt: %4.1f° [%s] | Rotation: %4.1f° [%s]\n",
      distance, (isClose ? "CLOSE" : "OK"),
      tilt, (isTilted ? "TILTED" : "OK"),
      roll, (isRotated ? "ROTATED" : "OK")
    );

    if (buzzerActive) {
      Serial.printf("[Status]  🚨 ALARM TRIGGERED! Slouched for %3.1fs (>20s threshold)\n", slouchSec);
      Serial.println("[ALERT]   >>> BUZZER BEEPING! Please sit upright! <<<");
    } else if (isSlouching) {
      Serial.printf("[Status]  ⏳ SLOUCH DETECTED (Intersection Met) — Timer: %3.1fs / 20.0s (Grace Period)\n", slouchSec);
    } else {
      Serial.println("[Status]  ✅ UPRIGHT POSTURE — Optimal desk ergonomics.");
    }
    Serial.println("------------------------------------------------------------------");
  }

  delay(100);
}
