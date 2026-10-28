import os
import sqlite3
import time
from flask import Flask, request, jsonify
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

DB_PATH = os.path.join(os.path.dirname(__file__), "sitsense.db")

def get_db_connection():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

def init_db():
    conn = get_db_connection()
    cursor = conn.cursor()
    # Posture telemetry readings table
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS readings (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp INTEGER NOT NULL,
            tilt REAL NOT NULL,
            roll REAL NOT NULL,
            distance REAL NOT NULL,
            is_close INTEGER NOT NULL,
            is_tilted INTEGER NOT NULL,
            is_rotated INTEGER NOT NULL,
            slouching INTEGER NOT NULL,
            duration_sec REAL NOT NULL,
            alert_active INTEGER NOT NULL
        )
    """)
    # Calibration & threshold settings table
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS settings (
            id INTEGER PRIMARY KEY CHECK (id = 1),
            baseline_tilt REAL DEFAULT 8.0,
            baseline_roll REAL DEFAULT 0.0,
            baseline_distance REAL DEFAULT 48.0,
            tilt_threshold REAL DEFAULT 15.0,
            roll_threshold REAL DEFAULT 12.0,
            distance_threshold REAL DEFAULT 35.0,
            grace_period_ms INTEGER DEFAULT 20000,
            updated_at INTEGER
        )
    """)
    # Insert default settings if empty
    cursor.execute("""
        INSERT OR IGNORE INTO settings (id, baseline_tilt, baseline_roll, baseline_distance, tilt_threshold, roll_threshold, distance_threshold, grace_period_ms, updated_at)
        VALUES (1, 8.0, 0.0, 48.0, 15.0, 12.0, 35.0, 20000, ?)
    """, (int(time.time() * 1000),))
    conn.commit()
    conn.close()

init_db()

@app.route("/api/posture", methods=["POST"])
def add_reading():
    data = request.get_json(force=True)
    if not data:
        return jsonify({"error": "No data received"}), 400

    ts = data.get("timestamp", int(time.time() * 1000))
    tilt = float(data.get("tilt", 0.0))
    roll = float(data.get("roll", 0.0))
    distance = float(data.get("distance", 0.0))
    is_close = 1 if data.get("is_close") else 0
    is_tilted = 1 if data.get("is_tilted") else 0
    is_rotated = 1 if data.get("is_rotated") else 0
    slouching = 1 if data.get("slouching") else 0
    duration_sec = float(data.get("duration_sec", 0.0))
    alert_active = 1 if (data.get("alert_active") or data.get("buzzer") or duration_sec >= 20.0) else 0

    conn = get_db_connection()
    cursor = conn.cursor()
    cursor.execute("""
        INSERT INTO readings (timestamp, tilt, roll, distance, is_close, is_tilted, is_rotated, slouching, duration_sec, alert_active)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    """, (ts, tilt, roll, distance, is_close, is_tilted, is_rotated, slouching, duration_sec, alert_active))
    conn.commit()
    conn.close()

    return jsonify({"status": "recorded", "alert_active": bool(alert_active)}), 200

@app.route("/api/history", methods=["GET"])
def get_history():
    limit = int(request.args.get("limit", 40))
    since = int(request.args.get("since", 0))

    conn = get_db_connection()
    cursor = conn.cursor()
    if since > 0:
        cursor.execute("SELECT * FROM readings WHERE timestamp >= ? ORDER BY timestamp ASC LIMIT ?", (since, limit))
    else:
        cursor.execute("SELECT * FROM (SELECT * FROM readings ORDER BY timestamp DESC LIMIT ?) ORDER BY timestamp ASC", (limit,))
    rows = cursor.fetchall()
    conn.close()

    result = []
    for r in rows:
        result.append({
            "id": r["id"],
            "timestamp": r["timestamp"],
            "tilt": r["tilt"],
            "roll": r["roll"],
            "distance": r["distance"],
            "is_close": bool(r["is_close"]),
            "is_tilted": bool(r["is_tilted"]),
            "is_rotated": bool(r["is_rotated"]),
            "slouching": bool(r["slouching"]),
            "duration_sec": r["duration_sec"],
            "alert_active": bool(r["alert_active"])
        })
    return jsonify(result)

@app.route("/api/stats", methods=["GET"])
def get_stats():
    conn = get_db_connection()
    cursor = conn.cursor()
    cursor.execute("SELECT COUNT(*) as total FROM readings")
    total = cursor.fetchone()["total"]

    if total == 0:
        conn.close()
        return jsonify({
            "total_samples": 0,
            "slouch_percent": 0,
            "slouch_count": 0,
            "good_posture_count": 0,
            "alarms_triggered": 0
        })

    cursor.execute("SELECT COUNT(*) as slouches FROM readings WHERE slouching = 1")
    slouches = cursor.fetchone()["slouches"]

    cursor.execute("SELECT COUNT(*) as alarms FROM readings WHERE alert_active = 1")
    alarms = cursor.fetchone()["alarms"]

    conn.close()

    return jsonify({
        "total_samples": total,
        "slouch_count": slouches,
        "good_posture_count": total - slouches,
        "slouch_percent": round((slouches / total) * 100, 1),
        "alarms_triggered": alarms
    })

@app.route("/api/settings", methods=["GET", "POST"])
def manage_settings():
    conn = get_db_connection()
    cursor = conn.cursor()

    if request.method == "POST":
        data = request.get_json(force=True)
        if data:
            cursor.execute("""
                UPDATE settings SET
                    baseline_tilt = COALESCE(?, baseline_tilt),
                    baseline_roll = COALESCE(?, baseline_roll),
                    baseline_distance = COALESCE(?, baseline_distance),
                    tilt_threshold = COALESCE(?, tilt_threshold),
                    roll_threshold = COALESCE(?, roll_threshold),
                    distance_threshold = COALESCE(?, distance_threshold),
                    grace_period_ms = COALESCE(?, grace_period_ms),
                    updated_at = ?
                WHERE id = 1
            """, (
                data.get("baseline_tilt"),
                data.get("baseline_roll"),
                data.get("baseline_distance"),
                data.get("tilt_threshold"),
                data.get("roll_threshold"),
                data.get("distance_threshold"),
                data.get("grace_period_ms"),
                int(time.time() * 1000)
            ))
            conn.commit()

    cursor.execute("SELECT * FROM settings WHERE id = 1")
    row = cursor.fetchone()
    conn.close()

    return jsonify({
        "baseline_tilt": row["baseline_tilt"],
        "baseline_roll": row["baseline_roll"],
        "baseline_distance": row["baseline_distance"],
        "tilt_threshold": row["tilt_threshold"],
        "roll_threshold": row["roll_threshold"],
        "distance_threshold": row["distance_threshold"],
        "grace_period_ms": row["grace_period_ms"]
    }), 200

@app.route("/api/calibrate", methods=["POST"])
def calibrate_posture():
    """One-click calibration: sets the current upright sitting posture as baseline."""
    data = request.get_json(force=True) if request.data else {}
    conn = get_db_connection()
    cursor = conn.cursor()

    # If values not explicitly passed, snapshot from the most recent reading
    baseline_tilt = data.get("tilt")
    baseline_roll = data.get("roll")
    baseline_distance = data.get("distance")

    if baseline_tilt is None or baseline_distance is None:
        cursor.execute("SELECT tilt, roll, distance FROM readings ORDER BY timestamp DESC LIMIT 1")
        latest = cursor.fetchone()
        if latest:
            baseline_tilt = latest["tilt"]
            baseline_roll = latest["roll"]
            baseline_distance = latest["distance"]
        else:
            baseline_tilt = 8.0
            baseline_roll = 0.0
            baseline_distance = 48.0

    cursor.execute("""
        UPDATE settings SET
            baseline_tilt = ?,
            baseline_roll = ?,
            baseline_distance = ?,
            updated_at = ?
        WHERE id = 1
    """, (baseline_tilt, baseline_roll, baseline_distance, int(time.time() * 1000)))
    conn.commit()
    conn.close()

    return jsonify({
        "status": "calibrated",
        "baseline": {
            "tilt": baseline_tilt,
            "roll": baseline_roll,
            "distance": baseline_distance
        }
    }), 200

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
