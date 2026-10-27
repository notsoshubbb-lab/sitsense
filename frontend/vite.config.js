import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

export default defineConfig({
  plugins: [react()],
  server: {
    host: true, // Exposes on local Wi-Fi network for mobile/teammate access
    port: 5173
  }
});
