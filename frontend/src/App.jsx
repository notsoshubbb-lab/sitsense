import { useEffect, useState } from "react";
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
  const [isConnected, setIsConnected] = useState(false);

  useEffect(() => {
    const fetchData = async () => {
      try {
        const [histRes, statsRes] = await Promise.all([
          axios.get(`${API_BASE}/history`),
          axios.get(`${API_BASE}/stats`)
        ]);
        setHistory(histRes.data.slice(-30));
        setStats(statsRes.data);
        setIsConnected(true);
      } catch (e) {
        setIsConnected(false);
      }
    };
    fetchData();
    const interval = setInterval(fetchData, 1500);
    return () => clearInterval(interval);
  }, []);

  const latest = history[history.length - 1] || {
    tilt: 0,
    roll: 0,
    distance: 0,
    is_close: false,
    is_tilted: false,
    is_rotated: false,
    slouching: false,
    duration_sec: 0,
    buzzer: false
  };

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
        label: "Desk Proximity (cm)",
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
        title: { display: true, text: "Angle (° Tilt & Roll)" },
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

  // Progress toward 20-second alarm threshold
  const durationSec = latest.duration_sec || 0;
  const timerPercent = Math.min(100, Math.round((durationSec / 20.0) * 100));

  return (
    <div style={{ padding: "2rem", fontFamily: "Segoe UI, sans-serif", maxWidth: "980px", margin: "auto" }}>
      {/* Header */}
      <header style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: "1.5rem" }}>
        <div>
          <h1 style={{ margin: 0, color: "#111827" }}>SitSense 🧘‍♂️</h1>
          <p style={{ color: "#6b7280", margin: "4px 0 0 0" }}>
            Multi-Sensor Intersection Tracking (Proximity + Tilt + Rotation)
          </p>
        </div>
        <div style={{ display: "flex", alignItems: "center", gap: "8px" }}>
          <span style={{
            width: "12px",
            height: "12px",
            borderRadius: "50%",
            backgroundColor: isConnected ? "#22c55e" : "#ef4444",
            display: "inline-block"
          }} />
          <span style={{ fontSize: "0.875rem", color: "#4b5563" }}>
            {isConnected ? "Device Streaming" : "Waiting for Telemetry"}
          </span>
        </div>
      </header>

      {/* 20-Second Slouch Timer Alert Banner */}
      {latest.slouching && (
        <div style={{
          padding: "1rem 1.25rem",
          marginBottom: "1.5rem",
          borderRadius: "8px",
          background: latest.buzzer ? "#fef2f2" : "#fffbeb",
          border: `1px solid ${latest.buzzer ? "#f87171" : "#fcd34d"}`
        }}>
          <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: "8px" }}>
            <span style={{ fontWeight: "bold", color: latest.buzzer ? "#991b1b" : "#92400e" }}>
              {latest.buzzer ? "🚨 SUSTAINED SLOUCH ALERT (> 20 SECONDS) — BUZZER ACTIVE!" : "⏳ Slouch Intersection Met (Grace Period Running)"}
            </span>
            <span style={{ fontWeight: "600", color: latest.buzzer ? "#991b1b" : "#92400e" }}>
              {durationSec}s / 20.0s
            </span>
          </div>
          <div style={{ width: "100%", height: "8px", background: "#e5e7eb", borderRadius: "4px", overflow: "hidden" }}>
            <div style={{
              width: `${timerPercent}%`,
              height: "100%",
              background: latest.buzzer ? "#ef4444" : "#f59e0b",
              transition: "width 0.3s ease"
            }} />
          </div>
        </div>
      )}

      {/* Multi-Sensor Intersection Status Cards */}
      <h3 style={{ margin: "0 0 0.75rem 0", color: "#374151" }}>Sensor Intersection Matrix</h3>
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
            {latest.is_tilted ? "⚠️ Exceeds 15° Threshold" : "✅ Normal Alignment (≤ 15°)"}
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
            {latest.is_rotated ? "⚠️ Shoulder Twist > 12°" : "✅ Square Shoulders (≤ 12°)"}
          </span>
        </div>

        {/* Overall Status */}
        <div style={{
          padding: "1.25rem",
          background: latest.buzzer ? "#fef2f2" : (latest.slouching ? "#fffbeb" : "#dcfce7"),
          borderRadius: "10px",
          border: `1px solid ${latest.buzzer ? "#ef4444" : (latest.slouching ? "#f59e0b" : "#86efac")}`
        }}>
          <div style={{ fontSize: "0.85rem", color: "#6b7280" }}>Intersection Result</div>
          <p style={{ fontSize: "1.25rem", fontWeight: "bold", margin: "0.4rem 0", color: latest.buzzer ? "#991b1b" : (latest.slouching ? "#b45309" : "#166534") }}>
            {latest.buzzer ? "🚨 ALARM BEEPING" : (latest.slouching ? "⏳ Grace Counting" : "✅ Good Ergonomics")}
          </p>
          <span style={{ fontSize: "0.75rem", color: "#4b5563" }}>
            Alarms Triggered: <strong>{stats.alarms_triggered || 0}</strong>
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
