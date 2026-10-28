// SitSense - ESP32 Firmware (No Physical Buzzer — Telemetry Sent to Desktop Sound System)
// Hardware: ESP32 + MPU-6050 (Torso) + HC-SR04 (Chest Distance)

#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>

const int MPU_ADDR = 0x68;

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASS";
const char* serverURL = "http://YOUR_LOCAL_IP:5000/api/posture";

const int TRIG_PIN = 4;
const int ECHO_PIN = 5;

// Baseline thresholds
float distanceThresholdCm = 35.0;    // Proximity: chest closer than 35 cm indicates leaning in
float tiltThresholdDeg = 15.0;       // Forward tilt: pitch greater than 15 degrees
float rollThresholdDeg = 12.0;       // Torso rotation: shoulder twist greater than 12 degrees

// 20-Second Grace Period: Desktop alert triggers if sustained slouch persists >20 seconds
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
  Wire.write(0x3B); // Register 0x3B (ACCEL_XOUT_H)
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
  bool isClose = (distance < distanceThresholdCm);
  bool isTilted = (tilt > tiltThresholdDeg);
  bool isRotated = (abs(roll) > rollThresholdDeg);

  // True slouching requires INTERSECTION: Close to desk AND torso deformed (tilt / rotation)
  bool isSlouching = isClose && (isTilted || isRotated);

  float currentDurationSec = 0.0;
  bool alertActive = false;

  // Hysteresis alert logic: 20-second countdown
  if (isSlouching) {
    if (slouchStartTime == 0) slouchStartTime = millis();
    unsigned long elapsed = millis() - slouchStartTime;
    currentDurationSec = elapsed / 1000.0;

    // Trigger desktop alert if sustained for MORE THAN 20 seconds
    if (elapsed > SLOUCH_GRACE_PERIOD) {
      alertActive = true;
    }
  } else {
    slouchStartTime = 0; // Reset timer immediately once posture returns to normal
  }

  // Push telemetry to Flask backend for desktop webapp sound alerts
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
      "\"alert_active\":" + (alertActive ? "true" : "false") +
    "}";

    http.POST(payload);
    http.end();
    lastSend = millis();
  }
  delay(100);
}
