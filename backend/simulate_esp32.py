"""
SitSense - ESP32 Telemetry Simulator (Desktop Audio & SQLite Persistence)
Simulates:
  - Proximity (HC-SR04): Distance < 35cm
  - Torso Deformation (MPU-6050): Tilt > 15° AND/OR Rotation > 12°
  - Hysteresis: Alerts ONLY when sustained continuously for > 20 seconds!
  - Alerts are routed to the Web Dashboard Desktop Audio System (No physical buzzer)
"""

import argparse
import json
import random
import sys
import time
import urllib.request
import urllib.error

SERVER_URL = "http://localhost:5000/api/posture"
INTERVAL_SECONDS = 1.5

SCENARIOS = [
    {
        "name": "Phase 1: Healthy Upright Sitting",
        "duration_cycles": 8,
        "tilt_range": (5.0, 10.0),
        "roll_range": (-2.0, 2.0),
        "dist_range": (45.0, 52.0),
        "simulate_slouch": False,
        "description": "User is sitting straight. Chest is ~48cm, upright alignment."
    },
    {
        "name": "Phase 2: Brief Close Reading (< 20s Grace Period)",
        "duration_cycles": 5, # 7.5 seconds (< 20s)
        "tilt_range": (18.0, 22.0),
        "roll_range": (13.0, 16.0),
        "dist_range": (26.0, 31.0),
        "simulate_slouch": True,
        "description": "User leans forward to inspect a document for ~7.5 seconds. Grace period prevents false alarm."
    },
    {
        "name": "Phase 3: Resumed Upright Posture",
        "duration_cycles": 6,
        "tilt_range": (6.0, 9.0),
        "roll_range": (-1.0, 2.0),
        "dist_range": (46.0, 50.0),
        "simulate_slouch": False,
        "description": "User sat back upright. Grace period timer was safely reset."
    },
    {
        "name": "Phase 4: Prolonged Multi-Sensor Slouch (> 20s Threshold)",
        "duration_cycles": 16, # 24 seconds (> 20s)
        "tilt_range": (26.0, 34.0),
        "roll_range": (14.0, 19.0),
        "dist_range": (18.0, 24.0),
        "simulate_slouch": True,
        "description": "User hunches and twists towards the screen for > 20 seconds -> Desktop Web Sound Beeps!"
    },
    {
        "name": "Phase 5: Posture Correction After Sound Alert",
        "duration_cycles": 8,
        "tilt_range": (5.0, 8.0),
        "roll_range": (0.0, 2.0),
        "dist_range": (47.0, 51.0),
        "simulate_slouch": False,
        "description": "User responds to desktop sound reminder, rolls shoulders back to upright posture."
    }
]

def send_telemetry(payload):
    data_bytes = json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(
        SERVER_URL,
        data=data_bytes,
        headers={"Content-Type": "application/json"}
    )
    try:
        with urllib.request.urlopen(req, timeout=3) as resp:
            return resp.status == 200, payload
    except urllib.error.URLError as e:
        return False, str(e)

def seed_history(count=40):
    print(f"\n[+] Pre-populating {count} historical records in SQLite database...")
    now_ms = int(time.time() * 1000)
    for i in range(count):
        offset_ms = now_ms - ((count - i) * 2000)
        is_slouch = (i % 14 >= 9)
        tilt = random.uniform(25.0, 32.0) if is_slouch else random.uniform(6.0, 11.0)
        dist = random.uniform(20.0, 25.0) if is_slouch else random.uniform(45.0, 50.0)
        roll = random.uniform(13.0, 18.0) if is_slouch else random.uniform(-2.0, 2.0)
        payload = {
            "tilt": round(tilt, 1),
            "roll": round(roll, 1),
            "distance": round(dist, 1),
            "is_close": dist < 35.0,
            "is_tilted": tilt > 15.0,
            "is_rotated": abs(roll) > 12.0,
            "slouching": is_slouch,
            "duration_sec": 22.0 if is_slouch else 0.0,
            "alert_active": is_slouch,
            "timestamp": offset_ms
        }
        send_telemetry(payload)
    print(f"[✓] Seeding complete! Database populated.\n")

def main():
    parser = argparse.ArgumentParser(description="SitSense ESP32 Telemetry Simulator")
    parser.add_argument("--seed", action="store_true", help="Pre-populate history with 40 records in SQLite first")
    args = parser.parse_args()

    print("=" * 70)
    print("   🧘 SitSense — Multi-Sensor Intersection & Desktop Audio Simulator")
    print("   Intersection Condition: Distance < 35cm AND (Tilt > 15° OR Roll > 12°)")
    print("   Alarm Condition: Must persist continuously for > 20 SECONDS")
    print("   Alert Destination: Desktop Webapp Audio Chime (No physical buzzer)")
    print(f"   Target Backend: {SERVER_URL}")
    print("=" * 70)

    if args.seed:
        seed_history(40)

    cycle_count = 0
    slouch_start_time = None

    try:
        while True:
            for scenario in SCENARIOS:
                print(f"\n▶ {scenario['name']}")
                print(f"  Info: {scenario['description']}")
                print("-" * 70)

                for _ in range(scenario["duration_cycles"]):
                    cycle_count += 1
                    tilt = round(random.uniform(*scenario["tilt_range"]), 1)
                    roll = round(random.uniform(*scenario["roll_range"]), 1)
                    dist = round(random.uniform(*scenario["dist_range"]), 1)

                    is_close = (dist < 35.0)
                    is_tilted = (tilt > 15.0)
                    is_rotated = (abs(roll) > 12.0)
                    is_slouching = is_close and (is_tilted or is_rotated)

                    duration_sec = 0.0
                    alert_active = False

                    if is_slouching:
                        if slouch_start_time is None:
                            slouch_start_time = time.time()
                        elapsed = time.time() - slouch_start_time
                        duration_sec = round(elapsed, 1)
                        if elapsed >= 20.0:
                            alert_active = True
                    else:
                        slouch_start_time = None

                    payload = {
                        "tilt": tilt,
                        "roll": roll,
                        "distance": dist,
                        "is_close": is_close,
                        "is_tilted": is_tilted,
                        "is_rotated": is_rotated,
                        "slouching": is_slouching,
                        "duration_sec": duration_sec,
                        "alert_active": alert_active
                    }

                    success, res = send_telemetry(payload)

                    if success:
                        if alert_active:
                            status_tag = f"🚨 DESKTOP ALERT (>20s: {duration_sec}s)"
                        elif is_slouching:
                            status_tag = f"⏳ Grace ({duration_sec}s / 20.0s)"
                        else:
                            status_tag = "✅ Upright"
                        print(f"[{cycle_count:03d}] {status_tag:<28} | Dist: {dist:4.1f}cm | Tilt: {tilt:4.1f}° | Roll: {roll:4.1f}°")
                    else:
                        print(f"[{cycle_count:03d}] ❌ Error: Flask backend offline. Run 'python app.py'")

                    time.sleep(INTERVAL_SECONDS)

    except KeyboardInterrupt:
        print("\n[✓] Simulator stopped.")

if __name__ == "__main__":
    main()
