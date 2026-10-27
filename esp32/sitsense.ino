// SitSense - ESP32 Firmware
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

float tiltThresholdDeg = 15.0;
float rollThresholdDeg = 15.0;
float distanceThresholdCm = 30.0;

unsigned long lastSend = 0;
const unsigned long SEND_INTERVAL = 2000;
unsigned long slouchStartTime = 0;
const unsigned long SLOUCH_GRACE_PERIOD = 3000; // 3 sec grace for reading/bending

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
  Serial.println("\nWiFi Connected!");

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

  float tilt = atan2(sqrt(axg * axg + ayg * ayg), azg) * 180.0 / PI;
  float roll = atan2(ayg, axg) * 180.0 / PI;
  float distance = getDistanceCm();

  bool isSlouching = (tilt > tiltThresholdDeg) || (abs(roll) > rollThresholdDeg) || (distance < distanceThresholdCm);

  if (isSlouching) {
    if (slouchStartTime == 0) slouchStartTime = millis();
    if (millis() - slouchStartTime > SLOUCH_GRACE_PERIOD) {
      tone(BUZZER_PIN, 1800, 250);
    }
  } else {
    slouchStartTime = 0;
  }

  if (millis() - lastSend > SEND_INTERVAL && WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");
    String payload = String("{\"tilt\":") + tilt + ",\"roll\":" + roll + ",\"distance\":" + distance + ",\"slouching\":" + (isSlouching ? "true" : "false") + "}";
    http.POST(payload);
    http.end();
    lastSend = millis();
  }
  delay(100);
}
