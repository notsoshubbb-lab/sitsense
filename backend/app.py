from flask import Flask, request, jsonify
from flask_cors import CORS
import time

app = Flask(__name__)
CORS(app)

records = []
settings = {
    "tilt_threshold": 15.0,
    "roll_threshold": 15.0,
    "distance_threshold": 35.0,
    "grace_period_ms": 3000
}

@app.route("/api/posture", methods=["POST"])
def add_reading():
    data = request.get_json(force=True)
    if not data:
        return jsonify({"error": "No data received"}), 400
    data["timestamp"] = int(time.time() * 1000)
    records.append(data)
    if len(records) > 2000:
        records.pop(0)
    return jsonify({"status": "recorded"}), 200

@app.route("/api/history", methods=["GET"])
def get_history():
    since = int(request.args.get("since", 0))
    filtered = [r for r in records if r.get("timestamp", 0) >= since]
    return jsonify(filtered)

@app.route("/api/stats", methods=["GET"])
def get_stats():
    if not records:
        return jsonify({"total_samples": 0, "slouch_percent": 0, "slouch_count": 0, "good_posture_count": 0})
    slouch_count = sum(1 for r in records if r.get("slouching"))
    return jsonify({
        "total_samples": len(records),
        "slouch_count": slouch_count,
        "good_posture_count": len(records) - slouch_count,
        "slouch_percent": round((slouch_count / len(records)) * 100, 1)
    })

@app.route("/api/settings", methods=["GET", "POST"])
def manage_settings():
    global settings
    if request.method == "POST":
        new_settings = request.get_json(force=True)
        if new_settings:
            settings.update(new_settings)
            return jsonify({"status": "updated", "settings": settings}), 200
    return jsonify(settings), 200

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
