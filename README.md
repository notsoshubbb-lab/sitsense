# SitSense 🧘‍♂️

Real-time posture monitoring and ergonomic guidance system using **ESP32**, **MPU-6050** (tilt & rotation), **HC-SR04** (distance), and a modern React web dashboard.

## System Architecture
- **Hardware**: ESP32 + MPU6050 (upper torso tilt/roll) + HC-SR04 (chest distance from desk) + buzzer alert.
- **Backend**: Python Flask REST API collecting sensor telemetry and session posture patterns.
- **Frontend**: React + Vite dashboard displaying real-time tilt, distance, slouch durations, and habit metrics.
