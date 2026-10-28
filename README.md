# SitSense 🧘‍♂️

**SitSense** is an end-to-end intelligent posture correction and habit-tracking system developed for desk workers.

It combines an **ESP32 microcontroller**, a wearable **MPU-6050 6-axis IMU** (placed on the upper torso to track spinal tilt and rotational slouch), a desk-mounted **HC-SR04 ultrasonic distance sensor** (measuring chest-to-desk proximity), a **Python Flask telemetry backend with SQLite persistence**, and a **React + Vite dashboard with built-in desktop audio alerts and one-click calibration**.

---

## 🛠️ Simplified Hardware Wiring Guide (No Buzzer Needed!)

*SitSense routes audio alerts directly through your computer/laptop speakers via the web dashboard.*

| Component | Component Pin | ESP32 GPIO | Description / Notes |
| :--- | :--- | :--- | :--- |
| **MPU-6050** | VCC | 3.3V | Power supply |
| **MPU-6050** | GND | GND | Ground |
| **MPU-6050** | SDA | GPIO 21 | I2C Data line (Native Wire.h) |
| **MPU-6050** | SCL | GPIO 22 | I2C Clock line (Native Wire.h) |
| **HC-SR04** | VCC | 5V (or VIN) | Power supply |
| **HC-SR04** | GND | GND | Ground |
| **HC-SR04** | TRIG | GPIO 4 | Trigger pulse pin |
| **HC-SR04** | ECHO | GPIO 5 | Echo pulse pin |

---

## 🧠 Smart Posture Detection & Hysteresis Logic

Unlike naive sensors that beep the moment you look down to take notes or read closely, SitSense implements **multi-sensor intersection with a 20-second grace window**:
1. **Torso Tilt**: Measures forward pitch angle from vertical baseline (`> 15°`).
2. **Torso Roll / Rotation**: Measures shoulder hunch and rotational twist (`> 12°`).
3. **Chest Proximity**: Monitors distance between the chest and desk/screen (`< 35 cm`).
4. **Multi-Sensor Intersection**: Slouch is ONLY triggered when chest is closer than 35 cm **AND** torso is deformed (tilted or rotated).
5. **20-Second Grace Period**: If the user temporarily leans in for less than 20 seconds, no sound plays. If slouching persists beyond 20 seconds, the **desktop audio system chimes**.
6. **🎯 One-Click Calibration**: Click the *Calibrate My Posture* button on the dashboard to store your personalized upright seated position in the SQLite database.

---

## 📁 Repository Structure

```text
sitsense/
├── esp32/
│   └── sitsense.ino        # ESP32 Arduino firmware (Zero external libraries)
├── backend/
│   ├── app.py              # Flask REST API server with SQLite persistence
│   ├── simulate_esp32.py   # Full hardware telemetry simulator
│   ├── requirements.txt    # Python dependencies
│   └── sitsense.db         # Persistent SQLite posture database
├── frontend/
│   ├── src/
│   │   ├── App.jsx         # Live telemetry charts, audio alert & calibration
│   │   └── main.jsx
│   ├── index.html
│   ├── package.json
│   └── vite.config.js
└── wokwi/
    ├── diagram.json        # Pre-wired Wokwi simulation circuit
    ├── sitsense_wokwi.ino  # Wokwi simulation sketch
    └── README.md
```

---

## 🚀 Getting Started

### 1. Run the Backend API (SQLite Database)
```bash
cd backend
pip install -r requirements.txt
python app.py
```
*Backend runs on `http://localhost:5000`.*

### 2. (Optional) Run the ESP32 Hardware Simulator
Test the real-time pipeline, 20-second grace window, and audio alerts without hardware:
```bash
cd backend
python simulate_esp32.py --seed
```

### 3. Launch the Web Dashboard
```bash
cd frontend
npm.cmd run dev
```
*Dashboard opens at `http://localhost:5173`.*

### 4. Flash the ESP32 (When Hardware Arrives)
1. Open [`esp32/sitsense.ino`](file:///c:/Users/shubh/OneDrive/synapse/sitsense/esp32/sitsense.ino) in Arduino IDE.
2. Update `ssid` and `password` with your Wi-Fi details.
3. Set `serverURL` to your computer's local IP address (e.g. `http://192.168.1.X:5000/api/posture`).
4. Select your ESP32 board and COM port, then click **Upload**!
