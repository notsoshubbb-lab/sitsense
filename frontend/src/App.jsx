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
  const [stats, setStats] = useState({ total_samples: 0, slouch_percent: 0 });

  useEffect(() => {
    const fetchData = async () => {
      try {
        const [histRes, statsRes] = await Promise.all([
          axios.get(`${API_BASE}/history`),
          axios.get(`${API_BASE}/stats`)
        ]);
        setHistory(histRes.data.slice(-30));
        setStats(statsRes.data);
      } catch (e) {
        console.error("Backend offline or waiting for data...", e);
      }
    };
    fetchData();
    const interval = setInterval(fetchData, 2000);
    return () => clearInterval(interval);
  }, []);

  const chartData = {
    labels: history.map((d) => new Date(d.timestamp).toLocaleTimeString()),
    datasets: [
      {
        label: "Torso Tilt (°)",
        data: history.map((d) => d.tilt),
        borderColor: "rgb(239, 68, 68)",
        backgroundColor: "rgba(239, 68, 68, 0.5)",
      },
      {
        label: "Desk Distance (cm)",
        data: history.map((d) => d.distance),
        borderColor: "rgb(59, 130, 246)",
        backgroundColor: "rgba(59, 130, 246, 0.5)",
      }
    ]
  };

  return (
    <div style={{ padding: "2rem", fontFamily: "sans-serif", maxWidth: "900px", margin: "auto" }}>
      <h1>SitSense Posture Monitor</h1>
      <div style={{ display: "flex", gap: "1rem", marginBottom: "1.5rem" }}>
        <div style={{ padding: "1rem", background: "#f3f4f6", borderRadius: "8px", flex: 1 }}>
          <h3>Total Readings</h3>
          <p style={{ fontSize: "1.5rem", fontWeight: "bold" }}>{stats.total_samples}</p>
        </div>
        <div style={{ padding: "1rem", background: stats.slouch_percent > 30 ? "#fee2e2" : "#dcfce7", borderRadius: "8px", flex: 1 }}>
          <h3>Slouch Ratio</h3>
          <p style={{ fontSize: "1.5rem", fontWeight: "bold" }}>{stats.slouch_percent}%</p>
        </div>
      </div>
      <div style={{ background: "#ffffff", padding: "1rem", borderRadius: "8px", border: "1px solid #e5e7eb" }}>
        <Line data={chartData} />
      </div>
    </div>
  );
}
