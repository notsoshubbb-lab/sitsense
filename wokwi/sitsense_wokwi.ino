/*
 * SitSense - Wokwi Simulation Firmware
 * Simulates ESP32 + MPU-6050 (Torso) + HC-SR04 (Chest Distance) + Buzzer
 *
 * How to interact in Wokwi:
 *  - Click on the HC-SR04 sensor during simulation to drag the distance slider.
 *  - Click on the MPU-6050 sensor to modify acceleration / orientation values.
 *  - Type 'demo' in Serial Monitor to toggle automated sample posture cycles!
 */

#include <Wire.h>
#include <MPU6050.h>
#include <WiFi.h>

// Wokwi Virtual WiFi Credentials
const char* ssid = "Wokwi-GUEST";
const char* password = "";

MPU6050 mpu;
const int TRIG_PIN = 4;
const int ECHO_PIN = 5;
const int BUZZER_PIN = 18;

// Thresholds
float tiltThresholdDeg = 15.0;       // Max allowable forward tilt
float rollThresholdDeg = 15.0;       // Max allowable shoulder roll
float distanceThresholdCm = 35.0;    // Minimum chest-to-desk distance

unsigned long slouchStartTime = 0;
const unsigned long SLOUCH_GRACE_PERIOD = 3000; // 3 seconds grace for reading/bending
unsigned long lastLogTime = 0;
const unsigned long LOG_INTERVAL = 1000;       // 1 second log interval

// Automated demo pattern toggle
bool autoDemoMode = false;
unsigned long demoTimer = 0;

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("\n=======================================================");
  Serial.println("         🧘 SitSense — Wokwi ESP32 Simulator          ");
  Serial.println("=======================================================");

  // Connect to virtual WiFi
  Serial.print("Connecting to Wokwi-GUEST WiFi");
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(300);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" Connected! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println(" (WiFi skipped for local simulation)");
  }

  // Initialize MPU6050
  Wire.begin(21, 22);
  mpu.initialize();
  if (mpu.testConnection()) {
    Serial.println("[✓] MPU-6050 connection successful!");
  } else {
    Serial.println("[!] MPU-6050 not detected. Using simulated readings.");
  }

  Serial.println("\n[TIP] Click on HC-SR04 to adjust distance slider.");
  Serial.println("[TIP] Type 'demo' in Serial Monitor to run automated scenarios.\n");
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
  // Check if user entered commands in Serial Monitor
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.equalsIgnoreCase("demo")) {
      autoDemoMode = !autoDemoMode;
      Serial.println(autoDemoMode ? "\n>>> [AUTO-DEMO ACTIVATED] <<<" : "\n>>> [MANUAL SLIDER MODE ACTIVATED] <<<");
      demoTimer = millis();
    }
  }

  float tilt = 0.0;
  float roll = 0.0;
  float distance = 0.0;

  if (autoDemoMode) {
    // Automated cyclic scenarios (every 16 seconds total cycle)
    unsigned long phase = (millis() - demoTimer) % 18000;
    if (phase < 6000) {
      // Phase 1: Good Posture
      tilt = 8.5; roll = 1.0; distance = 46.0;
    } else if (phase < 9000) {
      // Phase 2: Close Reading (Grace period, <3s)
      tilt = 19.0; roll = 2.0; distance = 30.0;
    } else if (phase < 14000) {
      // Phase 3: Sustained Slouching (>3s) -> Buzzer will sound!
      tilt = 30.0; roll = 9.0; distance = 21.0;
    } else {
      // Phase 4: Recovering posture
      tilt = 7.0; roll = 0.5; distance = 48.0;
    }
  } else {
    // Read from Wokwi virtual sensors
    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    float axg = ax / 16384.0;
    float ayg = ay / 16384.0;
    float azg = az / 16384.0;

    tilt = atan2(sqrt(axg * axg + ayg * ayg), azg) * 180.0 / PI;
    roll = atan2(ayg, axg) * 180.0 / PI;
    distance = readUltrasonicDistance();
  }

  // Slouch condition: excessive tilt, rotation hunch, or too close to desk
  bool isSlouching = (tilt > tiltThresholdDeg) || (abs(roll) > rollThresholdDeg) || (distance < distanceThresholdCm);

  // Hysteresis & Buzzer Alert Logic
  float slouchDurationSec = 0.0;
  bool buzzerActive = false;

  if (isSlouching) {
    if (slouchStartTime == 0) slouchStartTime = millis();
    unsigned long elapsed = millis() - slouchStartTime;
    slouchDurationSec = elapsed / 1000.0;

    if (elapsed > SLOUCH_GRACE_PERIOD) {
      buzzerActive = true;
      tone(BUZZER_PIN, 2000, 200); // 2 kHz alert tone
    }
  } else {
    slouchStartTime = 0;
    noTone(BUZZER_PIN);
  }

  // Periodic Serial Monitor dashboard printout
  if (millis() - lastLogTime >= LOG_INTERVAL) {
    lastLogTime = millis();

    Serial.println("-------------------------------------------------------");
    Serial.printf("[SitSense] Torso Tilt: %4.1f° | Roll: %4.1f° | Distance: %4.1f cm\n", tilt, roll, distance);

    if (buzzerActive) {
      Serial.printf("[Status]   🚨 SUSTAINED SLOUCH (Active: %3.1fs) -> BUZZER BEEPING!\n", slouchDurationSec);
      Serial.println("[Reminder] Please straighten your spine and roll shoulders back.");
    } else if (isSlouching) {
      Serial.printf("[Status]   ⏳ SLOUCH DETECTED (Grace timer: %3.1fs / 3.0s) -> No beep yet\n", slouchDurationSec);
    } else {
      Serial.println("[Status]   ✅ UPRIGHT POSTURE — Great ergonomics!");
    }
    Serial.println("-------------------------------------------------------");
  }

  delay(100);
}
