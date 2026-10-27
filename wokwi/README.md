# SitSense — Wokwi Simulation Guide

This folder contains everything needed to simulate the **SitSense ESP32** hardware in [Wokwi.com](https://wokwi.com).

## 🚀 How to Run in Wokwi (Step-by-Step):

1. **Open Wokwi**: Go to [https://wokwi.com/projects/new/esp32](https://wokwi.com/projects/new/esp32).
2. **Load the Code**:
   - Copy the contents of [`sitsense_wokwi.ino`](file:///c:/Users/shubh/OneDrive/synapse/sitsense/wokwi/sitsense_wokwi.ino) and paste into the **`sketch.ino`** tab.
3. **Load the Diagram**:
   - In Wokwi, switch to the **`diagram.json`** tab.
   - Replace its contents with [`diagram.json`](file:///c:/Users/shubh/OneDrive/synapse/sitsense/wokwi/diagram.json).
4. **Add Library**:
   - In the **Library Manager** tab in Wokwi, search and add **`MPU6050`** (or paste `MPU6050` into `libraries.txt`).
5. **Click Play (▶ Green Button)**!

---

## 🎮 How to Test in the Simulation:

- **Manual Testing**:
  - Click on the **HC-SR04** distance sensor: A slider pops up. Drag it under `35 cm` to simulate leaning in too close!
  - Click on the **MPU-6050** sensor: Tweak the angles to simulate torso tilting!
  - If you stay in a slouched position for >3 seconds, you will hear the **Wokwi buzzer beep**!
- **Auto-Demo Mode**:
  - Type `demo` in the Wokwi Serial Monitor and press Enter.
  - The simulation will automatically run through sample scenarios (Good Posture -> Quick Close Reading -> Prolonged Slouch -> Recovery).

