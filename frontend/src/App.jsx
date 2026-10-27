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
  const [stats, setStats] = useState({ total_samples: 0, slouch_percent: 0, slouch_count: 0 });
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
    const interval = setInterval(fetchData, 2000);
    return () => clearInterval(interval);
  }, []);

  const latest = history[history.length - 1] || { tilt: 0, roll: 0, distance: 0, slouching: false };

  const chartData = {
    labels: history.map((d) => new Date(d.timestamp).toLocaleTimeString()),
    datasets: [
      {
        label: "Torso Tilt (°)",
        data: history.map((d) => d.tilt),
        borderColor: "rgb(239, 68, 68)",
        backgroundColor: "rgba(239, 68, 68, 0.5)",
        yAxisID: "y1",
      },
      {
        label: "Desk Proximity (cm)",
        data: history.map((d) => d.distance),
        borderColor: "rgb(59, 130, 246)",
        backgroundColor: "rgba(59, 130, 246, 0.5)",
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
        title: { display: true, text: "Tilt (°)" },
        min: 0,
        max: 60,
      },
      y2: {
        type: "linear",
        position: "right",
        title: { display: true, text: "Distance (cm)" },
        min: 0,
        max: 100,
        grid: { drawOnChartArea: false },
      }
    }
  };

  return (
    <div style={{ padding: "2rem", fontFamily: "Segoe UI, sans-serif", maxWidth: "960px", margin: "auto" }}>
      <header style={{ display: "flex", justifyContent: "space-between", alignItems: "center", marginBottom: "2rem" }}>
        <div>
          <h1 style={{ margin: 0, color: "#111827" }}>SitSense 🧘‍♂️</h1>
          <p style={{ color: "#6b7280", margin: "4px 0 0 0" }}>Real-time ergonomic telemetry & posture habit tracking</p>
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
            {isConnected ? "Device Connected" : "Awaiting Data"}
          </span>
        </div>
      </header>

      <div style={{ display: "grid", gridTemplateColumns: "repeat(auto-fit, minmax(200px, 1fr))", gap: "1rem", marginBottom: "2rem" }}>
        <div style={{ padding: "1.25rem", background: latest.slouching ? "#fee2e2" : "#dcfce7", borderRadius: "10px", border: "1px solid " + (latest.slouching ? "#fca5a5" : "#86efac") }}>
          <h4 style={{ margin: 0, color: "#374151" }}>Current Posture</h4>
          <p style={{ fontSize: "1.5rem", fontWeight: "bold", margin: "0.5rem 0 0", color: latest.slouching ? "#991b1b" : "#166534" }}>
            {latest.slouching ? "⚠️ Slouching" : "✅ Good Posture"}
          </p>
        </div>
        <div style={{ padding: "1.25rem", background: "#f9fafb", borderRadius: "10px", border: "1px solid #e5e7eb" }}>
          <h4 style={{ margin: 0, color: "#374151" }}>Torso Tilt</h4>
          <p style={{ fontSize: "1.5rem", fontWeight: "bold", margin: "0.5rem 0 0" }}>{Math.round(latest.tilt)}°</p>
        </div>
        <div style={{ padding: "1.25rem", background: "#f9fafb", borderRadius: "10px", border: "1px solid #e5e7eb" }}>
          <h4 style={{ margin: 0, color: "#374151" }}>Chest Proximity</h4>
          <p style={{ fontSize: "1.5rem", fontWeight: "bold", margin: "0.5rem 0 0" }}>{Math.round(latest.distance)} cm</p>
        </div>
        <div style={{ padding: "1.25rem", background: "#f9fafb", borderRadius: "10px", border: "1px solid #e5e7eb" }}>
          <h4 style={{ margin: 0, color: "#374151" }}>Slouch Rate</h4>
          <p style={{ fontSize: "1.5rem", fontWeight: "bold", margin: "0.5rem 0 0" }}>{stats.slouch_percent}%</p>
        </div>
      </div>

      <div style={{ background: "#ffffff", padding: "1.5rem", borderRadius: "10px", border: "1px solid #e5e7eb", boxShadow: "0 1px 3px rgba(0,0,0,0.05)" }}>
        <h3 style={{ margin: "0 0 1rem 0", color: "#1f2937" }}>Live Ergonomic Feed (Tilt vs Distance)</h3>
        <Line data={chartData} options={chartOptions} />
      </div>
    </div>
  );
}
