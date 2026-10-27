# SitSense 🧘‍♂️

**SitSense** is an end-to-end intelligent posture correction and habit-tracking system developed for desk workers.

It combines an **ESP32 microcontroller**, a wearable **MPU-6050 6-axis IMU** (placed on the upper torso to track spinal tilt and rotational slouch), a desk-mounted **HC-SR04 ultrasonic distance sensor** (measuring chest-to-desk proximity), a piezo buzzer for feedback, a **Python Flask telemetry backend**, and a **React + Vite dashboard**.

---

## 🛠️ Hardware Wiring Guide

| Component | Component Pin | ESP32 GPIO | Description / Notes |
| :--- | :--- | :--- | :--- |
| **MPU-6050** | VCC | 3.3V | Power supply |
| **MPU-6050** | GND | GND | Ground |
| **MPU-6050** | SDA | GPIO 21 | I2C Data line |
| **MPU-6050** | SCL | GPIO 22 | I2C Clock line |
| **HC-SR04** | VCC | 5V (or VIN) | Power supply |
| **HC-SR04** | GND | GND | Ground |
| **HC-SR04** | TRIG | GPIO 4 | Trigger pulse pin |
| **HC-SR04** | ECHO | GPIO 5 | Echo pulse pin |
| **Buzzer** | Positive (+) | GPIO 18 | Piezo alert buzzer |
| **Buzzer** | Negative (-) | GND | Ground |

---

## 🧠 Smart Posture Detection & Hysteresis Logic

Unlike naive sensors that beep the moment you look down to take notes or read closely, SitSense implements **graceful hysteresis**:
1. **Torso Tilt**: Measures forward pitch angle from vertical baseline (default threshold: `> 15°`).
2. **Torso Roll**: Measures shoulder hunch / rotation (default threshold: `> 15°`).
3. **Chest Proximity**: Monitors distance between the chest and desk/screen (default threshold: `< 35 cm`).
4. **Grace Period**: When slouching is detected, the timer starts. If the user temporarily leans in to read for less than **3 seconds**, no alarm sounds. If slouching persists beyond 3 seconds, the buzzer alerts the user to sit straight.

---

## 📁 Repository Structure

```text
sitsense/
├── esp32/
│   └── sitsense.ino        # ESP32 Arduino firmware
├── backend/
│   ├── app.py              # Flask REST API server
│   └── requirements.txt    # Python dependencies
├── frontend/
│   ├── src/
│   │   ├── App.jsx         # Live telemetry charts & posture status
│   │   └── main.jsx
│   ├── index.html
│   ├── package.json
│   └── vite.config.js
└── .github/
    └── workflows/
        └── ci.yml          # Continuous integration build test
```

---

## 🚀 Getting Started

### 1. Flash the ESP32
1. Open [`esp32/sitsense.ino`](file:///c:/Users/shubh/OneDrive/synapse/sitsense/esp32/sitsense.ino) in the Arduino IDE.
2. Install the **MPU6050** library by Electronic Cats (Library Manager).
3. Update `YOUR_WIFI_SSID`, `YOUR_WIFI_PASS`, and set `serverURL` to your local machine's IP address (e.g. `http://192.168.1.X:5000/api/posture`).
4. Select your ESP32 board and COM port, then click **Upload**.

### 2. Run the Backend API
```bash
cd backend
pip install -r requirements.txt
python app.py
```
*Backend runs on `http://localhost:5000`.*

### 3. Launch the Web Dashboard
```bash
cd frontend
npm install
npm run dev
```
*Dashboard opens at `http://localhost:5173`.*
