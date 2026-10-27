// SitSense - ESP32 Firmware (Direct Wire.h - Zero External Libraries Required!)
// Multi-Sensor Intersection Posture Detection (HC-SR04 Proximity + MPU-6050 Tilt & Rotation)

#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>

const int MPU_ADDR = 0x68;

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASS";
const char* serverURL = "http://YOUR_LOCAL_IP:5000/api/posture";

const int TRIG_PIN = 4;
const int ECHO_PIN = 5;
const int BUZZER_PIN = 18;

// Configurable Baseline Thresholds
float distanceThresholdCm = 35.0;    // Proximity: chest closer than 35 cm indicates leaning in
float tiltThresholdDeg = 15.0;       // Forward tilt: pitch greater than 15 degrees
float rollThresholdDeg = 12.0;       // Torso rotation: roll / shoulder twist greater than 12 degrees

// 20-Second Grace Period: Only beep if bad posture persists continuously for >20 seconds
const unsigned long SLOUCH_GRACE_PERIOD = 20000; 
unsigned long slouchStartTime = 0;

unsigned long lastSend = 0;
const unsigned long SEND_INTERVAL = 1500; // Telemetry push every 1.5 seconds

void initMPU() {
  Wire.begin(21, 22);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0);    // Wake up MPU-6050
  byte error = Wire.endTransmission(true);

  if (error == 0) {
    Serial.println("[✓] MPU-6050 connected successfully on I2C (0x68)!");
  } else {
    Serial.println("[!] MPU-6050 connection error. Check SDA(21)/SCL(22) wiring.");
  }
}

void readMPU(float &tilt, float &roll) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // Starting at register 0x3B (ACCEL_XOUT_H)
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
    tilt = 0.0;
    roll = 0.0;
  }
}

float getDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  return duration > 0 ? (duration / 58.2) : 999.0;
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected to SitSense network!");

  initMPU();
}

void loop() {
  float tilt = 0.0;
  float roll = 0.0;
  readMPU(tilt, roll);
  float distance = getDistanceCm();

  // Multi-Sensor Intersection Conditions:
  // 1. Proximity: User's chest is too close to the desk / monitor
  bool isClose = (distance < distanceThresholdCm);

  // 2. Torso Deformation: Forward spinal tilt OR asymmetrical rotation / shoulder twist
  bool isTilted = (tilt > tiltThresholdDeg);
  bool isRotated = (abs(roll) > rollThresholdDeg);

  // True slouching requires INTERSECTION: User is close to desk AND exhibits torso deformation (tilt / rotation)
  bool isSlouching = isClose && (isTilted || isRotated);

  float currentDurationSec = 0.0;
  bool buzzerActive = false;

  // Hysteresis alert logic with 20-second grace window
  if (isSlouching) {
    if (slouchStartTime == 0) slouchStartTime = millis();
    unsigned long elapsed = millis() - slouchStartTime;
    currentDurationSec = elapsed / 1000.0;

    // Only beep if the slouched posture persists for MORE THAN 20 seconds!
    if (elapsed > SLOUCH_GRACE_PERIOD) {
      buzzerActive = true;
      tone(BUZZER_PIN, 2000, 200); // 2kHz alert beep
    }
  } else {
    slouchStartTime = 0; // Reset timer immediately once posture returns to normal
    noTone(BUZZER_PIN);
  }

  // Send comprehensive telemetry to Flask backend
  if (millis() - lastSend > SEND_INTERVAL && WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");

    String payload = String("{") +
      "\"tilt\":" + tilt + "," +
      "\"roll\":" + roll + "," +
      "\"distance\":" + distance + "," +
      "\"is_close\":" + (isClose ? "true" : "false") + "," +
      "\"is_tilted\":" + (isTilted ? "true" : "false") + "," +
      "\"is_rotated\":" + (isRotated ? "true" : "false") + "," +
      "\"slouching\":" + (isSlouching ? "true" : "false") + "," +
      "\"duration_sec\":" + currentDurationSec + "," +
      "\"buzzer\":" + (buzzerActive ? "true" : "false") +
    "}";

    http.POST(payload);
    http.end();
    lastSend = millis();
  }
  delay(100);
}
