"""
SitSense - ESP32 Telemetry Simulator
Simulates live data streaming from the ESP32 (MPU-6050 + HC-SR04) to the Flask backend.
Uses standard Python library (urllib) - zero external pip dependencies!

Usage:
  python simulate_esp32.py          # Streams live simulated telemetry continuously
  python simulate_esp32.py --seed   # Pre-populates 40 historical readings immediately, then streams live
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

# Realistic ergonomic phases for desk work
SCENARIOS = [
    {
        "name": "Phase 1: Healthy Upright Sitting",
        "duration_cycles": 8,
        "tilt_range": (6.0, 11.0),
        "roll_range": (-2.0, 2.0),
        "dist_range": (45.0, 52.0),
        "force_slouch": False,
        "description": "User is sitting straight. Chest is ~48cm from monitor, spinal tilt is minimal."
    },
    {
        "name": "Phase 2: Leaning in for Close Reading (Grace Period)",
        "duration_cycles": 2,
        "tilt_range": (18.0, 22.0),
        "roll_range": (-3.0, 3.0),
        "dist_range": (28.0, 32.0),
        "force_slouch": False,
        "description": "User leans forward to inspect small text. Within 3s grace window -> no alarm."
    },
    {
        "name": "Phase 3: Prolonged Slouching & Hunching",
        "duration_cycles": 10,
        "tilt_range": (26.0, 36.0),
        "roll_range": (6.0, 14.0),
        "dist_range": (18.0, 25.0),
        "force_slouch": True,
        "description": "User fatigued, slumping forward. Sustained >3s -> Buzzer sounds & alert triggered!"
    },
    {
        "name": "Phase 4: Posture Correction",
        "duration_cycles": 8,
        "tilt_range": (5.0, 9.0),
        "roll_range": (-1.0, 2.0),
        "dist_range": (46.0, 50.0),
        "force_slouch": False,
        "description": "User responds to buzzer, rolls shoulders back and resumes ergonomic posture."
    }
]

def send_telemetry(tilt: float, roll: float, distance: float, slouching: bool, custom_ts=None):
    payload = {
        "tilt": round(tilt, 1),
        "roll": round(roll, 1),
        "distance": round(distance, 1),
        "slouching": slouching
    }
    if custom_ts:
        payload["timestamp"] = custom_ts
    
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
    print(f"\n[+] Seeding {count} historical posture records into backend...")
    now_ms = int(time.time() * 1000)
    for i in range(count):
        offset_ms = now_ms - ((count - i) * 2000)
        # Alternate between good posture and occasional slouch
        is_slouch = (i % 12 >= 8)
        tilt = random.uniform(25.0, 32.0) if is_slouch else random.uniform(6.0, 12.0)
        dist = random.uniform(20.0, 26.0) if is_slouch else random.uniform(44.0, 50.0)
        roll = random.uniform(4.0, 10.0) if is_slouch else random.uniform(-2.0, 2.0)
        send_telemetry(tilt, roll, dist, is_slouch, custom_ts=offset_ms)
    print(f"[✓] Seeding complete! Dashboard will now show a full history timeline.\n")

def main():
    parser = argparse.ArgumentParser(description="SitSense ESP32 Telemetry Simulator")
    parser.add_argument("--seed", action="store_true", help="Pre-populate history with 40 sample records first")
    args = parser.parse_args()

    print("=" * 65)
    print("   🧘 SitSense — ESP32 Hardware Telemetry Simulator")
    print(f"   Target Backend: {SERVER_URL}")
    print("   Press Ctrl+C to stop simulation at any time.")
    print("=" * 65)

    if args.seed:
        seed_history(40)

    cycle_count = 0
    try:
        while True:
            for scenario in SCENARIOS:
                print(f"\n▶ {scenario['name']}")
                print(f"  Note: {scenario['description']}")
                print("-" * 65)

                for _ in range(scenario["duration_cycles"]):
                    cycle_count += 1
                    tilt = random.uniform(*scenario["tilt_range"])
                    roll = random.uniform(*scenario["roll_range"])
                    dist = random.uniform(*scenario["dist_range"])
                    slouch = scenario["force_slouch"]

                    success, result = send_telemetry(tilt, roll, dist, slouch)

                    if success:
                        status_str = "⚠️ SLOUCHING" if slouch else "✅ GOOD"
                        print(f"[{cycle_count:03d}] {status_str} | Tilt: {tilt:4.1f}° | Roll: {roll:4.1f}° | Dist: {dist:4.1f}cm -> Sent")
                    else:
                        print(f"[{cycle_count:03d}] ❌ Could not reach Flask backend ({result}). Make sure 'python app.py' is running!")

                    time.sleep(INTERVAL_SECONDS)

    except KeyboardInterrupt:
        print("\n\n[✓] Simulation stopped by user.")

if __name__ == "__main__":
    main()
