// SitSense - ESP32 Firmware
// Multi-Sensor Intersection Posture Detection (HC-SR04 Proximity + MPU6050 Tilt & Rotation)

#include <Wire.h>
#include <MPU6050.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASS";
const char* serverURL = "http://YOUR_LOCAL_IP:5000/api/posture";

MPU6050 mpu;
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

  Wire.begin();
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed!");
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

void loop() {
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  float axg = ax / 16384.0;
  float ayg = ay / 16384.0;
  float azg = az / 16384.0;

  // Calculate forward tilt (pitch) and torso rotation (roll)
  float tilt = atan2(sqrt(axg * axg + ayg * ayg), azg) * 180.0 / PI;
  float roll = atan2(ayg, axg) * 180.0 / PI;
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
