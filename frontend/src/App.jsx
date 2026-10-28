import { useEffect, useState, useRef } from "react";
import axios from "axios";
import { Line } from "react-chartjs-2";
import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend
} from "chart.js";

ChartJS.register(CategoryScale, LinearScale, PointElement, LineElement, Title, Tooltip, Legend);

const API_BASE = "http://localhost:5000/api";

export default function App() {
  const [history, setHistory] = useState([]);
  const [stats, setStats] = useState({ total_samples: 0, slouch_percent: 0, alarms_triggered: 0 });
  const [settings, setSettings] = useState({
    baseline_tilt: 8.0,
    baseline_roll: 0.0,
    baseline_distance: 48.0,
    tilt_threshold: 15.0,
    roll_threshold: 12.0,
    distance_threshold: 35.0,
    grace_period_ms: 20000
  });
  const [isConnected, setIsConnected] = useState(false);
  const [audioEnabled, setAudioEnabled] = useState(true);
  const [calibrateMsg, setCalibrateMsg] = useState("");

  const lastBeepTimeRef = useRef(0);

  // Synthesize desktop reminder chime via Web Audio API
  const playAlertChime = () => {
    try {
      const AudioCtx = window.AudioContext || window.webkitAudioContext;
      if (!AudioCtx) return;
      const ctx = new AudioCtx();

      // Dual-tone ergonomic notification chime
      const osc = ctx.createOscillator();
      const gain = ctx.createGain();

      osc.connect(gain);
      gain.connect(ctx.destination);

      osc.type = "sine";
      osc.frequency.setValueAtTime(880, ctx.currentTime); // High pitch (A5)
      osc.frequency.exponentialRampToValueAtTime(587, ctx.currentTime + 0.35); // Gentle drop to D5

      gain.gain.setValueAtTime(0.25, ctx.currentTime);
      gain.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.35);

      osc.start(ctx.currentTime);
      osc.stop(ctx.currentTime + 0.35);
    } catch (e) {
      console.warn("Desktop audio playback waiting for user interaction...", e);
    }
  };

  const fetchData = async () => {
    try {
      const [histRes, statsRes, settingsRes] = await Promise.all([
        axios.get(`${API_BASE}/history?limit=35`),
        axios.get(`${API_BASE}/stats`),
        axios.get(`${API_BASE}/settings`)
      ]);
      setHistory(histRes.data);
      setStats(statsRes.data);
      setSettings(settingsRes.data);
      setIsConnected(true);

      // Check if 20-second threshold exceeded -> Trigger Desktop Audio Chime
      const latestReading = histRes.data[histRes.data.length - 1];
      if (latestReading && latestReading.alert_active && audioEnabled) {
        const now = Date.now();
        if (now - lastBeepTimeRef.current > 2500) {
          playAlertChime();
          lastBeepTimeRef.current = now;
        }
      }
    } catch (e) {
      setIsConnected(false);
    }
  };

  useEffect(() => {
    fetchData();
    const interval = setInterval(fetchData, 1500);
    return () => clearInterval(interval);
  }, [audioEnabled]);

  // One-Click Posture Calibration
  const handleCalibrate = async () => {
    try {
      const latestReading = history[history.length - 1];
      const payload = latestReading ? {
        tilt: latestReading.tilt,
        roll: latestReading.roll,
        distance: latestReading.distance
      } : {};

      const res = await axios.post(`${API_BASE}/calibrate`, payload);
      setSettings(prev => ({
        ...prev,
        baseline_tilt: res.data.baseline.tilt,
        baseline_roll: res.data.baseline.roll,
        baseline_distance: res.data.baseline.distance
      }));

      setCalibrateMsg(`✅ Calibrated! Baseline set: Tilt ${res.data.baseline.tilt}°, Distance ${Math.round(res.data.baseline.distance)}cm`);
      setTimeout(() => setCalibrateMsg(""), 4500);
    } catch (e) {
      setCalibrateMsg("❌ Failed to calibrate. Is backend running?");
    }
  };

  const latest = history[history.length - 1] || {
    tilt: settings.baseline_tilt,
    roll: settings.baseline_roll,
    distance: settings.baseline_distance,
    is_close: false,
    is_tilted: false,
    is_rotated: false,
    slouching: false,
    duration_sec: 0,
    alert_active: false
  };

  const durationSec = latest.duration_sec || 0;
  const timerPercent = Math.min(100, Math.round((durationSec / 20.0) * 100));

  const chartData = {
    labels: history.map((d) => new Date(d.timestamp).toLocaleTimeString()),
    datasets: [
      {
        label: "Torso Tilt (°)",
        data: history.map((d) => d.tilt),
        borderColor: "rgb(239, 68, 68)",
        backgroundColor: "rgba(239, 68, 68, 0.4)",
        yAxisID: "y1",
      },
      {
        label: "Torso Rotation (°)",
        data: history.map((d) => Math.abs(d.roll || 0)),
        borderColor: "rgb(168, 85, 247)",
        backgroundColor: "rgba(168, 85, 247, 0.4)",
        yAxisID: "y1",
      },
      {
        label: "Chest Distance (cm)",
        data: history.map((d) => d.distance),
        borderColor: "rgb(59, 130, 246)",
        backgroundColor: "rgba(59, 130, 246, 0.4)",
        yAxisID: "y2",
      }
    ]
  };

  const chartOptions = {
    responsive: true,
    scales: {
      y1: {
        type: "linear",
        position: "left",
        title: { display: true, text: "Angle (° Tilt / Roll)" },
        min: 0,
        max: 60,
      },
      y2: {
        type: "linear",
        position: "right",
        title: { display: true, text: "Distance (cm)" },
        min: 0,
        max: 80,
        grid: { drawOnChartArea: false },
      }
    }
  };

  return (
    <div style={{ padding: "2rem", fontFamily: "Segoe UI, sans-serif", maxWidth: "1000px", margin: "auto" }}>
      {/* Header & Controls */}
      <header style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: "1.5rem", flexWrap: "wrap", gap: "1rem" }}>
        <div>
          <h1 style={{ margin: 0, color: "#111827" }}>SitSense 🧘‍♂️</h1>
          <p style={{ color: "#6b7280", margin: "4px 0 0 0" }}>
            Smart Multi-Sensor Posture Ergonomics (Desktop Audio & SQLite Persistence)
          </p>
        </div>

        <div style={{ display: "flex", alignItems: "center", gap: "10px" }}>
          {/* Audio Alert Toggle */}
          <button
            onClick={() => {
              setAudioEnabled(!audioEnabled);
              if (!audioEnabled) playAlertChime();
            }}
            style={{
              padding: "0.5rem 0.9rem",
              borderRadius: "6px",
              border: "1px solid #d1d5db",
              background: audioEnabled ? "#ecfdf5" : "#f3f4f6",
              color: audioEnabled ? "#065f46" : "#6b7280",
              fontWeight: "600",
              cursor: "pointer"
            }}
          >
            {audioEnabled ? "🔊 Desktop Audio: ON" : "🔇 Sound: MUTED"}
          </button>

          {/* Test Sound Button */}
          <button
            onClick={playAlertChime}
            style={{
              padding: "0.5rem 0.75rem",
              borderRadius: "6px",
              border: "1px solid #d1d5db",
              background: "#ffffff",
              color: "#374151",
              fontSize: "0.85rem",
              cursor: "pointer"
            }}
            title="Click to test the application chime"
          >
            🔔 Test Chime
          </button>

          {/* Calibration Button */}
          <button
            onClick={handleCalibrate}
            style={{
              padding: "0.5rem 1rem",
              borderRadius: "6px",
              border: "none",
              background: "#2563eb",
              color: "#ffffff",
              fontWeight: "600",
              cursor: "pointer"
            }}
          >
            🎯 Calibrate My Posture
          </button>

          {/* Status Indicator */}
          <div style={{ display: "flex", alignItems: "center", gap: "6px", marginLeft: "6px" }}>
            <span style={{
              width: "12px",
              height: "12px",
              borderRadius: "50%",
              backgroundColor: isConnected ? "#22c55e" : "#ef4444",
              display: "inline-block"
            }} />
            <span style={{ fontSize: "0.85rem", color: "#4b5563" }}>
              {isConnected ? "Live" : "Offline"}
            </span>
          </div>
        </div>
      </header>

      {/* Calibration Notification Toast */}
      {calibrateMsg && (
        <div style={{
          padding: "0.85rem 1.25rem",
          marginBottom: "1.25rem",
          borderRadius: "8px",
          background: "#eff6ff",
          border: "1px solid #93c5fd",
          color: "#1e40af",
          fontWeight: "600"
        }}>
          {calibrateMsg}
        </div>
      )}

      {/* 20-Second Slouch Countdown & Sound Alert Banner */}
      {latest.slouching && (
        <div style={{
          padding: "1rem 1.25rem",
          marginBottom: "1.5rem",
          borderRadius: "8px",
          background: latest.alert_active ? "#fef2f2" : "#fffbeb",
          border: `1px solid ${latest.alert_active ? "#f87171" : "#fcd34d"}`
        }}>
          <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: "8px" }}>
            <span style={{ fontWeight: "bold", color: latest.alert_active ? "#991b1b" : "#92400e" }}>
              {latest.alert_active
                ? "🚨 SUSTAINED SLOUCH ALERT (> 20s) — DESKTOP AUDIO CHIME BEEPING!"
                : "⏳ Multi-Sensor Slouch Detected (20s Grace Period Running)"}
            </span>
            <span style={{ fontWeight: "600", color: latest.alert_active ? "#991b1b" : "#92400e" }}>
              {durationSec}s / 20.0s
            </span>
          </div>
          <div style={{ width: "100%", height: "8px", background: "#e5e7eb", borderRadius: "4px", overflow: "hidden" }}>
            <div style={{
              width: `${timerPercent}%`,
              height: "100%",
              background: latest.alert_active ? "#ef4444" : "#f59e0b",
              transition: "width 0.3s ease"
            }} />
          </div>
        </div>
      )}

      {/* Baseline Calibration Card */}
      <div style={{ display: "flex", gap: "1rem", padding: "0.75rem 1rem", background: "#f8fafc", borderRadius: "8px", border: "1px solid #e2e8f0", marginBottom: "1.5rem", fontSize: "0.85rem", color: "#475569" }}>
        <span><strong>Active Baseline:</strong> Tilt: {settings.baseline_tilt}° | Rotation: {settings.baseline_roll}° | Distance: {Math.round(settings.baseline_distance)}cm</span>
        <span>•</span>
        <span><strong>Alert Rule:</strong> Distance &lt; {settings.distance_threshold}cm AND (Tilt &gt; {settings.tilt_threshold}° OR Roll &gt; {settings.roll_threshold}°) sustained for &gt; 20s</span>
      </div>

      {/* Multi-Sensor Intersection Status Cards */}
      <div style={{ display: "grid", gridTemplateColumns: "repeat(auto-fit, minmax(220px, 1fr))", gap: "1rem", marginBottom: "2rem" }}>
        {/* Proximity */}
        <div style={{
          padding: "1.25rem",
          background: latest.is_close ? "#fee2e2" : "#f0fdf4",
          borderRadius: "10px",
          border: `1px solid ${latest.is_close ? "#fca5a5" : "#bbf7d0"}`
        }}>
          <div style={{ fontSize: "0.85rem", color: "#6b7280" }}>1. HC-SR04 Proximity</div>
          <p style={{ fontSize: "1.5rem", fontWeight: "bold", margin: "0.25rem 0" }}>{Math.round(latest.distance)} cm</p>
          <span style={{ fontSize: "0.75rem", fontWeight: "600", color: latest.is_close ? "#dc2626" : "#16a34a" }}>
            {latest.is_close ? "⚠️ Closer than 35 cm" : "✅ Safe Distance (> 35 cm)"}
          </span>
        </div>

        {/* Tilt */}
        <div style={{
          padding: "1.25rem",
          background: latest.is_tilted ? "#fee2e2" : "#f0fdf4",
          borderRadius: "10px",
          border: `1px solid ${latest.is_tilted ? "#fca5a5" : "#bbf7d0"}`
        }}>
          <div style={{ fontSize: "0.85rem", color: "#6b7280" }}>2. MPU-6050 Forward Tilt</div>
          <p style={{ fontSize: "1.5rem", fontWeight: "bold", margin: "0.25rem 0" }}>{Math.round(latest.tilt)}°</p>
          <span style={{ fontSize: "0.75rem", fontWeight: "600", color: latest.is_tilted ? "#dc2626" : "#16a34a" }}>
            {latest.is_tilted ? "⚠️ Exceeds 15° Pitch" : "✅ Good Spinal Alignment"}
          </span>
        </div>

        {/* Rotation */}
        <div style={{
          padding: "1.25rem",
          background: latest.is_rotated ? "#fee2e2" : "#f0fdf4",
          borderRadius: "10px",
          border: `1px solid ${latest.is_rotated ? "#fca5a5" : "#bbf7d0"}`
        }}>
          <div style={{ fontSize: "0.85rem", color: "#6b7280" }}>3. MPU-6050 Torso Rotation</div>
          <p style={{ fontSize: "1.5rem", fontWeight: "bold", margin: "0.25rem 0" }}>{Math.round(Math.abs(latest.roll || 0))}°</p>
          <span style={{ fontSize: "0.75rem", fontWeight: "600", color: latest.is_rotated ? "#dc2626" : "#16a34a" }}>
            {latest.is_rotated ? "⚠️ Shoulder Twist > 12°" : "✅ Square Shoulder Alignment"}
          </span>
        </div>

        {/* Overall Status */}
        <div style={{
          padding: "1.25rem",
          background: latest.alert_active ? "#fef2f2" : (latest.slouching ? "#fffbeb" : "#dcfce7"),
          borderRadius: "10px",
          border: `1px solid ${latest.alert_active ? "#ef4444" : (latest.slouching ? "#f59e0b" : "#86efac")}`
        }}>
          <div style={{ fontSize: "0.85rem", color: "#6b7280" }}>Desktop Audio Alert</div>
          <p style={{ fontSize: "1.25rem", fontWeight: "bold", margin: "0.4rem 0", color: latest.alert_active ? "#991b1b" : (latest.slouching ? "#b45309" : "#166534") }}>
            {latest.alert_active ? "🚨 SOUND BEEPING" : (latest.slouching ? "⏳ Grace Counting" : "✅ Good Posture")}
          </p>
          <span style={{ fontSize: "0.75rem", color: "#4b5563" }}>
            Database Records: <strong>{stats.total_samples || 0}</strong>
          </span>
        </div>
      </div>

      {/* Multi-Channel Live Chart */}
      <div style={{ background: "#ffffff", padding: "1.5rem", borderRadius: "10px", border: "1px solid #e5e7eb", boxShadow: "0 1px 3px rgba(0,0,0,0.05)" }}>
        <h3 style={{ margin: "0 0 1rem 0", color: "#1f2937" }}>
          Live Multi-Sensor Feed (Tilt vs. Rotation vs. Distance)
        </h3>
        <Line data={chartData} options={chartOptions} />
      </div>
    </div>
  );
}
