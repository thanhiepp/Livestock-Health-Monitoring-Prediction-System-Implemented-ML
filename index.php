<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Cattle Health Monitor</title>
    <meta name="description" content="Real-time cattle health monitoring with AI/ML insights">
    
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&display=swap" rel="stylesheet">
    <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" />
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
    <script src="js/ml-engine.js?v=3"></script>
    <link rel="stylesheet" href="css/style.css">
    
    <style>
        /* Cattle-specific styles */
        .cow-card {
            background: var(--bg-card);
            backdrop-filter: blur(20px);
            border: 1px solid var(--glass-border);
            border-radius: var(--radius-xl);
            padding: 1.5rem;
            position: relative;
            overflow: hidden;
        }
        
        .cow-card::before {
            content: '';
            position: absolute;
            top: 0;
            left: 0;
            right: 0;
            height: 4px;
            background: linear-gradient(90deg, #22c55e, #84cc16);
        }
        
        .cow-card.warning::before {
            background: linear-gradient(90deg, #f97316, #eab308);
        }
        
        .cow-card.danger::before {
            background: linear-gradient(90deg, #ef4444, #dc2626);
        }
        
        .cow-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 1.5rem;
            padding-bottom: 1rem;
            border-bottom: 1px solid var(--glass-border);
        }
        
        .cow-avatar {
            width: 60px;
            height: 60px;
            background: linear-gradient(135deg, #22c55e, #16a34a);
            border-radius: 50%;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 2rem;
        }
        
        .cow-info h3 {
            font-size: 1.2rem;
            margin-bottom: 4px;
        }
        
        .cow-info .cow-id {
            font-size: 0.8rem;
            color: var(--text-muted);
        }
        
        .activity-badge {
            display: inline-flex;
            align-items: center;
            gap: 6px;
            padding: 8px 16px;
            border-radius: 20px;
            font-size: 0.85rem;
            font-weight: 500;
        }
        
        .vital-grid {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 1rem;
            margin-bottom: 1rem;
        }
        
        .vital-item {
            text-align: center;
            padding: 1rem;
            background: rgba(255,255,255,0.03);
            border-radius: 12px;
            border: 1px solid var(--glass-border);
        }
        
        .vital-item.alert {
            border-color: #ef4444;
            background: rgba(239, 68, 68, 0.1);
        }
        
        .vital-icon {
            font-size: 1.5rem;
            margin-bottom: 0.5rem;
        }
        
        .vital-value {
            font-size: 1.8rem;
            font-weight: 700;
            margin-bottom: 0.25rem;
        }
        
        .vital-label {
            font-size: 0.75rem;
            color: var(--text-muted);
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        
        .vital-status {
            font-size: 0.7rem;
            margin-top: 4px;
            padding: 2px 8px;
            border-radius: 10px;
            display: inline-block;
        }
        
        .vital-status.normal {
            background: rgba(34, 197, 94, 0.2);
            color: #22c55e;
        }
        
        .vital-status.warning {
            background: rgba(249, 115, 22, 0.2);
            color: #f97316;
        }
        
        .vital-status.danger {
            background: rgba(239, 68, 68, 0.2);
            color: #ef4444;
        }
        
        /* Health gauge */
        .health-gauge-container {
            display: flex;
            align-items: center;
            gap: 1rem;
            padding: 1rem;
            background: rgba(255,255,255,0.03);
            border-radius: 12px;
        }
        
        .gauge-circle {
            width: 80px;
            height: 80px;
            border-radius: 50%;
            background: conic-gradient(
                var(--gauge-color, #22c55e) calc(var(--gauge-percent, 0) * 1%),
                rgba(255,255,255,0.1) 0
            );
            display: flex;
            align-items: center;
            justify-content: center;
            position: relative;
        }
        
        .gauge-circle::before {
            content: '';
            position: absolute;
            width: 60px;
            height: 60px;
            background: var(--bg-card);
            border-radius: 50%;
        }
        
        .gauge-value {
            position: relative;
            font-size: 1.2rem;
            font-weight: 700;
        }
        
        .health-details {
            flex: 1;
        }
        
        .health-status {
            font-size: 1rem;
            font-weight: 600;
            margin-bottom: 0.5rem;
        }
        
        .condition-list {
            display: flex;
            flex-wrap: wrap;
            gap: 6px;
        }
        
        .condition-badge {
            padding: 4px 10px;
            border-radius: 12px;
            font-size: 0.75rem;
            display: inline-flex;
            align-items: center;
            gap: 4px;
        }
        
        .condition-badge.good {
            background: rgba(34, 197, 94, 0.2);
            color: #22c55e;
        }
        
        .condition-badge.warning {
            background: rgba(249, 115, 22, 0.2);
            color: #f97316;
        }
        
        .condition-badge.critical {
            background: rgba(239, 68, 68, 0.2);
            color: #ef4444;
        }
        
        /* Summary cards */
        .summary-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 1rem;
            margin-bottom: 2rem;
        }
        
        .summary-card {
            background: var(--bg-card);
            border-radius: var(--radius-xl);
            padding: 1.5rem;
            text-align: center;
            border: 1px solid var(--glass-border);
        }
        
        .summary-icon {
            font-size: 2.5rem;
            margin-bottom: 0.5rem;
        }
        
        .summary-value {
            font-size: 2rem;
            font-weight: 700;
            margin-bottom: 0.25rem;
        }
        
        .summary-label {
            font-size: 0.85rem;
            color: var(--text-muted);
        }
        
        /* AI Section */
        .ai-section {
            margin-bottom: 2rem;
        }
        
        .ai-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(350px, 1fr));
            gap: 1.5rem;
        }
        
        .ai-card {
            background: var(--bg-card);
            backdrop-filter: blur(20px);
            border: 1px solid var(--glass-border);
            border-radius: var(--radius-xl);
            padding: 1.5rem;
        }
        
        .ai-card::before {
            content: '';
            position: absolute;
            top: 0;
            left: 0;
            right: 0;
            height: 3px;
            background: linear-gradient(90deg, #a855f7, #ec4899);
            border-radius: var(--radius-xl) var(--radius-xl) 0 0;
        }
        
        .alert-list {
            max-height: 300px;
            overflow-y: auto;
        }
        
        .alert-item {
            display: flex;
            align-items: center;
            gap: 12px;
            padding: 0.75rem;
            background: rgba(239, 68, 68, 0.1);
            border-left: 3px solid #ef4444;
            border-radius: 0 8px 8px 0;
            margin-bottom: 0.5rem;
        }
        
        .alert-item.warning {
            background: rgba(249, 115, 22, 0.1);
            border-left-color: #f97316;
        }
        
        .prediction-grid {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 1rem;
        }
        
        .prediction-item {
            background: rgba(255,255,255,0.03);
            border-radius: 12px;
            padding: 1rem;
            text-align: center;
        }
        
        /* ========================================
           Node 1 Full Sensor Dashboard - Premium
           ======================================== */
        
        /* Banner */
        .full-sensor-banner {
            position: relative;
            display: flex;
            align-items: center;
            gap: 10px;
            padding: 12px 16px;
            margin-bottom: 1.25rem;
            background: linear-gradient(135deg, rgba(168,85,247,0.15), rgba(236,72,153,0.15), rgba(59,130,246,0.15));
            border-radius: 12px;
            border: 1px solid rgba(168,85,247,0.3);
            overflow: hidden;
        }
        
        .banner-glow {
            position: absolute;
            top: -50%;
            left: -50%;
            width: 200%;
            height: 200%;
            background: radial-gradient(circle, rgba(168,85,247,0.1) 0%, transparent 70%);
            animation: bannerPulse 3s ease-in-out infinite;
        }
        
        @keyframes bannerPulse {
            0%, 100% { opacity: 0.5; transform: scale(1); }
            50% { opacity: 1; transform: scale(1.1); }
        }
        
        .banner-icon { font-size: 1.5rem; z-index: 1; }
        .banner-text { 
            font-weight: 700; 
            font-size: 1rem; 
            background: linear-gradient(90deg, #a855f7, #ec4899);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            background-clip: text;
            z-index: 1;
        }
        
        .sensor-badges {
            display: flex;
            gap: 6px;
            margin-left: auto;
            z-index: 1;
        }
        
        .sensor-badge {
            padding: 4px 8px;
            border-radius: 6px;
            font-size: 0.65rem;
            font-weight: 700;
            letter-spacing: 0.5px;
            text-transform: uppercase;
        }
        
        .sensor-badge.mpu { background: rgba(249,115,22,0.2); color: #fb923c; }
        .sensor-badge.ds18 { background: rgba(239,68,68,0.2); color: #f87171; }
        .sensor-badge.dht { background: rgba(34,197,94,0.2); color: #4ade80; }
        .sensor-badge.gps { background: rgba(59,130,246,0.2); color: #60a5fa; }
        .sensor-badge.max { background: rgba(236,72,153,0.2); color: #f472b6; }
        
        /* Sensor Rows */
        .sensor-row {
            display: grid;
            gap: 1rem;
            margin-bottom: 1rem;
        }
        
        .vitals-row { grid-template-columns: repeat(2, 1fr); }
        .env-row { grid-template-columns: 1fr; }
        .motion-row { grid-template-columns: 1fr; }
        .location-row { grid-template-columns: 1fr 1fr; }
        
        /* Sensor Blocks */
        .sensor-block {
            background: rgba(255,255,255,0.03);
            border: 1px solid var(--glass-border);
            border-radius: 16px;
            padding: 1rem;
            transition: all 0.3s ease;
        }
        
        .sensor-block:hover {
            background: rgba(255,255,255,0.05);
            transform: translateY(-2px);
            box-shadow: 0 8px 25px rgba(0,0,0,0.2);
        }
        
        .block-header {
            display: flex;
            align-items: center;
            gap: 8px;
            margin-bottom: 0.75rem;
            padding-bottom: 0.5rem;
            border-bottom: 1px solid rgba(255,255,255,0.05);
        }
        
        .block-icon { font-size: 1.25rem; }
        .block-title { 
            font-size: 0.8rem; 
            color: var(--text-muted); 
            text-transform: uppercase;
            letter-spacing: 0.5px;
            font-weight: 600;
        }
        
        .block-value {
            font-size: 2.5rem;
            font-weight: 800;
            line-height: 1;
            margin-bottom: 0.25rem;
        }
        
        .block-unit {
            font-size: 1rem;
            font-weight: 400;
            opacity: 0.7;
        }
        
        .block-status {
            font-size: 0.75rem;
            padding: 4px 10px;
            border-radius: 12px;
            display: inline-block;
            margin-bottom: 0.5rem;
        }
        
        .block-status.normal { background: rgba(34,197,94,0.2); color: #22c55e; }
        .block-status.warning { background: rgba(249,115,22,0.2); color: #f97316; }
        .block-status.danger { background: rgba(239,68,68,0.2); color: #ef4444; }
        
        .block-sensor {
            font-size: 0.65rem;
            color: rgba(255,255,255,0.3);
            text-transform: uppercase;
            letter-spacing: 1px;
        }
        
        /* Pulse Animation for Heart */
        .pulse-animation {
            animation: heartPulse 1s ease-in-out infinite;
        }
        
        @keyframes heartPulse {
            0%, 100% { transform: scale(1); }
            50% { transform: scale(1.2); }
        }
        
        /* Vital Blocks Gradients */
        .vital-block.heart {
            background: linear-gradient(135deg, rgba(239,68,68,0.1), rgba(236,72,153,0.05));
            border-color: rgba(239,68,68,0.2);
        }
        
        .vital-block.temp {
            background: linear-gradient(135deg, rgba(249,115,22,0.1), rgba(234,179,8,0.05));
            border-color: rgba(249,115,22,0.2);
        }
        
        /* Environment Block */
        .env-values {
            display: flex;
            justify-content: center;
            align-items: center;
            gap: 2rem;
        }
        
        .env-item { text-align: center; }
        .env-icon { font-size: 1.5rem; margin-bottom: 0.25rem; }
        .env-value { 
            font-size: 1.75rem; 
            font-weight: 700; 
            color: #00d4ff; 
        }
        .env-label { 
            font-size: 0.75rem; 
            color: var(--text-muted); 
            text-transform: uppercase; 
        }
        
        .env-divider {
            width: 1px;
            height: 50px;
            background: linear-gradient(180deg, transparent, rgba(255,255,255,0.2), transparent);
        }
        
        /* MPU6050 Block */
        .motion-block {
            background: linear-gradient(135deg, rgba(168,85,247,0.08), rgba(59,130,246,0.05));
        }
        
        .mpu-grid {
            display: flex;
            justify-content: space-around;
            gap: 1rem;
            margin-bottom: 0.75rem;
        }
        
        .mpu-section { flex: 1; text-align: center; }
        .mpu-label { 
            font-size: 0.75rem; 
            color: var(--text-muted); 
            margin-bottom: 0.5rem;
            font-weight: 600;
        }
        
        .mpu-values { display: flex; justify-content: center; gap: 12px; }
        
        .mpu-axis {
            display: flex;
            flex-direction: column;
            align-items: center;
            padding: 6px 10px;
            background: rgba(255,255,255,0.05);
            border-radius: 8px;
            min-width: 50px;
        }
        
        .axis-name {
            font-size: 0.65rem;
            color: var(--text-muted);
            font-weight: 700;
        }
        
        .axis-value {
            font-size: 0.85rem;
            font-weight: 600;
            color: #f97316;
        }
        
        .mpu-section.gyro .axis-value { color: #8b5cf6; }
        
        .mpu-divider {
            width: 1px;
            background: linear-gradient(180deg, transparent, rgba(255,255,255,0.15), transparent);
        }
        
        .activity-indicator {
            display: flex;
            justify-content: center;
            align-items: center;
            gap: 8px;
            padding: 8px;
            background: rgba(0,0,0,0.2);
            border-radius: 8px;
        }
        
        .activity-label { 
            font-size: 0.75rem; 
            color: var(--text-muted); 
        }
        
        .activity-value { 
            font-size: 0.85rem; 
            font-weight: 600; 
        }
        
        /* GPS Block */
        .gps-block {
            background: linear-gradient(135deg, rgba(34,197,94,0.08), rgba(16,185,129,0.05));
        }
        
        .gps-coords {
            display: flex;
            flex-direction: column;
            gap: 6px;
        }
        
        .coord {
            display: flex;
            align-items: center;
            gap: 8px;
        }
        
        .coord-label {
            font-size: 0.7rem;
            color: var(--text-muted);
            background: rgba(34,197,94,0.2);
            padding: 2px 8px;
            border-radius: 4px;
            font-weight: 600;
            min-width: 35px;
            text-align: center;
        }
        
        .coord-value {
            font-size: 0.9rem;
            font-weight: 600;
            color: #22c55e;
            font-family: 'Courier New', monospace;
        }
        
        /* Signal Block */
        .signal-block {
            background: linear-gradient(135deg, rgba(59,130,246,0.08), rgba(99,102,241,0.05));
        }
        
        .signal-display {
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 1rem;
        }
        
        .signal-bars {
            display: flex;
            align-items: flex-end;
            gap: 4px;
            height: 30px;
        }
        
        .signal-bars .bar {
            width: 8px;
            background: rgba(255,255,255,0.15);
            border-radius: 2px;
            transition: all 0.3s ease;
        }
        
        .signal-bars .bar:nth-child(1) { height: 25%; }
        .signal-bars .bar:nth-child(2) { height: 50%; }
        .signal-bars .bar:nth-child(3) { height: 75%; }
        .signal-bars .bar:nth-child(4) { height: 100%; }
        
        .signal-bars .bar.active {
            background: linear-gradient(180deg, #22c55e, #16a34a);
            box-shadow: 0 0 10px rgba(34,197,94,0.4);
        }
        
        .signal-value {
            font-size: 1.1rem;
            font-weight: 700;
            color: #60a5fa;
        }
        
        /* ========================================
           Node 2 Basic Sensor Dashboard - Premium
           ======================================== */
        
        /* Node 2 Banner */
        .node2-sensor-banner {
            position: relative;
            display: flex;
            align-items: center;
            gap: 10px;
            padding: 12px 16px;
            margin-bottom: 1.25rem;
            background: linear-gradient(135deg, rgba(34,197,94,0.15), rgba(16,185,129,0.15), rgba(6,182,212,0.15));
            border-radius: 12px;
            border: 1px solid rgba(34,197,94,0.3);
            overflow: hidden;
        }
        
        .n2-banner-glow {
            position: absolute;
            top: -50%;
            left: -50%;
            width: 200%;
            height: 200%;
            background: radial-gradient(circle, rgba(34,197,94,0.1) 0%, transparent 70%);
            animation: n2BannerPulse 4s ease-in-out infinite;
        }
        
        @keyframes n2BannerPulse {
            0%, 100% { opacity: 0.3; transform: scale(1); }
            50% { opacity: 0.7; transform: scale(1.05); }
        }
        
        .n2-banner-icon { font-size: 1.5rem; z-index: 1; }
        .n2-banner-text { 
            font-weight: 700; 
            font-size: 1rem; 
            background: linear-gradient(90deg, #22c55e, #06b6d4);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            background-clip: text;
            z-index: 1;
        }
        
        .n2-sensor-badges {
            display: flex;
            gap: 6px;
            margin-left: auto;
            z-index: 1;
        }
        
        .n2-badge {
            padding: 4px 10px;
            border-radius: 6px;
            font-size: 0.7rem;
            font-weight: 700;
            letter-spacing: 0.5px;
            text-transform: uppercase;
        }
        
        .n2-badge.ds18 { background: rgba(239,68,68,0.2); color: #f87171; }
        .n2-badge.dht { background: rgba(34,197,94,0.2); color: #4ade80; }
        .n2-badge.gps { background: rgba(59,130,246,0.2); color: #60a5fa; }
        
        /* Main Vital Ring */
        .n2-main-vital {
            display: flex;
            align-items: center;
            gap: 1.5rem;
            padding: 1.25rem;
            margin-bottom: 1rem;
            background: linear-gradient(135deg, rgba(249,115,22,0.08), rgba(234,179,8,0.05));
            border: 1px solid rgba(249,115,22,0.2);
            border-radius: 16px;
        }
        
        .n2-vital-ring {
            width: 120px;
            height: 120px;
            border-radius: 50%;
            background: conic-gradient(
                var(--ring-color, #22c55e) calc(var(--ring-percent, 50) * 1%),
                rgba(255,255,255,0.1) 0
            );
            display: flex;
            align-items: center;
            justify-content: center;
            position: relative;
            flex-shrink: 0;
        }
        
        .n2-vital-inner {
            width: 100px;
            height: 100px;
            border-radius: 50%;
            background: var(--bg-card);
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
        }
        
        .n2-vital-icon { font-size: 1.25rem; margin-bottom: 2px; }
        .n2-vital-value { font-size: 2rem; font-weight: 800; line-height: 1; }
        .n2-vital-unit { font-size: 0.85rem; color: var(--text-muted); }
        
        .n2-vital-info { flex: 1; }
        .n2-vital-title { 
            font-size: 1.1rem; 
            font-weight: 700; 
            margin-bottom: 0.5rem;
            color: #f97316;
        }
        
        .n2-vital-status {
            display: inline-block;
            padding: 4px 12px;
            border-radius: 12px;
            font-size: 0.8rem;
            font-weight: 600;
            margin-bottom: 0.5rem;
        }
        
        .n2-vital-status.normal { background: rgba(34,197,94,0.2); color: #22c55e; }
        .n2-vital-status.warning { background: rgba(249,115,22,0.2); color: #f97316; }
        .n2-vital-status.danger { background: rgba(239,68,68,0.2); color: #ef4444; }
        
        .n2-vital-sensor {
            font-size: 0.7rem;
            color: rgba(255,255,255,0.4);
            text-transform: uppercase;
            letter-spacing: 1px;
            margin-bottom: 0.75rem;
        }
        
        .n2-temp-range {
            display: flex;
            align-items: center;
            gap: 8px;
        }
        
        .range-low, .range-high { 
            font-size: 0.7rem; 
            color: var(--text-muted); 
        }
        
        .range-bar {
            flex: 1;
            height: 6px;
            background: rgba(255,255,255,0.1);
            border-radius: 3px;
            position: relative;
            overflow: hidden;
        }
        
        .range-normal {
            position: absolute;
            left: 43%; /* 38-35/7 = 43% */
            width: 21%; /* 39.5-38/7 = 21% */
            height: 100%;
            background: rgba(34,197,94,0.4);
            border-radius: 3px;
        }
        
        .range-indicator {
            position: absolute;
            width: 10px;
            height: 10px;
            background: #f97316;
            border-radius: 50%;
            top: -2px;
            transform: translateX(-50%);
            box-shadow: 0 0 10px rgba(249,115,22,0.6);
        }
        
        /* Environment Section */
        .n2-env-section {
            margin-bottom: 1rem;
        }
        
        .n2-section-header {
            display: flex;
            align-items: center;
            gap: 8px;
            margin-bottom: 0.75rem;
            padding-bottom: 0.5rem;
            border-bottom: 1px solid rgba(255,255,255,0.05);
        }
        
        .n2-section-icon { font-size: 1.1rem; }
        .n2-section-title {
            font-size: 0.8rem;
            color: var(--text-muted);
            text-transform: uppercase;
            letter-spacing: 0.5px;
            font-weight: 600;
        }
        
        .n2-env-cards {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 0.75rem;
        }
        
        .n2-env-card {
            background: rgba(255,255,255,0.03);
            border: 1px solid var(--glass-border);
            border-radius: 12px;
            padding: 1rem;
            display: flex;
            align-items: center;
            gap: 0.75rem;
            position: relative;
            overflow: hidden;
            transition: all 0.3s ease;
        }
        
        .n2-env-card:hover {
            transform: translateY(-2px);
            box-shadow: 0 8px 20px rgba(0,0,0,0.15);
        }
        
        .n2-env-card.temp-card { border-color: rgba(34,197,94,0.2); }
        .n2-env-card.humid-card { border-color: rgba(6,182,212,0.2); }
        
        .n2-env-icon-wrap {
            width: 45px;
            height: 45px;
            border-radius: 12px;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 1.5rem;
        }
        
        .temp-card .n2-env-icon-wrap { background: rgba(34,197,94,0.15); }
        .humid-card .n2-env-icon-wrap { background: rgba(6,182,212,0.15); }
        
        .n2-env-data { flex: 1; }
        .n2-env-value { 
            font-size: 1.5rem; 
            font-weight: 700; 
        }
        .temp-card .n2-env-value { color: #22c55e; }
        .humid-card .n2-env-value { color: #06b6d4; }
        
        .n2-env-unit { font-size: 0.9rem; opacity: 0.7; }
        .n2-env-label { 
            font-size: 0.75rem; 
            color: var(--text-muted); 
            text-transform: uppercase; 
        }
        
        .n2-env-indicator {
            position: absolute;
            bottom: 0;
            left: 0;
            right: 0;
            height: 3px;
            border-radius: 0 0 12px 12px;
        }
        
        /* Bottom Row: GPS & Signal */
        .n2-bottom-row {
            display: grid;
            grid-template-columns: 1.2fr 1fr;
            gap: 0.75rem;
        }
        
        .n2-gps-card,
        .n2-signal-card {
            background: rgba(255,255,255,0.03);
            border: 1px solid var(--glass-border);
            border-radius: 12px;
            padding: 1rem;
        }
        
        .n2-gps-card { border-color: rgba(59,130,246,0.2); }
        .n2-signal-card { border-color: rgba(168,85,247,0.2); }
        
        .n2-card-header {
            display: flex;
            align-items: center;
            gap: 6px;
            margin-bottom: 0.75rem;
        }
        
        .n2-card-icon { font-size: 1rem; }
        .n2-card-title {
            font-size: 0.75rem;
            color: var(--text-muted);
            text-transform: uppercase;
            font-weight: 600;
        }
        
        .n2-gps-display { margin-bottom: 0.5rem; }
        .n2-coord-row {
            display: flex;
            align-items: center;
            gap: 6px;
            margin-bottom: 4px;
        }
        
        .n2-coord-icon { font-size: 0.8rem; }
        .n2-coord-label { 
            font-size: 0.7rem; 
            color: var(--text-muted);
            font-weight: 600;
        }
        .n2-coord-value {
            font-size: 0.85rem;
            font-weight: 600;
            color: #60a5fa;
            font-family: 'Courier New', monospace;
        }
        
        .n2-gps-status {
            font-size: 0.7rem;
            padding: 4px 8px;
            border-radius: 6px;
            background: rgba(249,115,22,0.2);
            color: #f97316;
            display: inline-block;
        }
        
        .n2-gps-status.active {
            background: rgba(34,197,94,0.2);
            color: #22c55e;
        }
        
        /* Signal Circle */
        .n2-signal-display {
            display: flex;
            justify-content: center;
            margin-bottom: 0.5rem;
        }
        
        .n2-signal-circle {
            width: 70px;
            height: 70px;
            border-radius: 50%;
            background: rgba(255,255,255,0.03);
            border: 2px solid var(--signal-color, #22c55e);
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            position: relative;
        }
        
        .n2-signal-waves {
            position: absolute;
            top: -15px;
            left: 50%;
            transform: translateX(-50%);
            display: flex;
            gap: 4px;
        }
        
        .n2-signal-waves .wave {
            width: 6px;
            height: 6px;
            background: rgba(255,255,255,0.2);
            border-radius: 50%;
        }
        
        .n2-signal-waves .wave.active {
            background: var(--signal-color, #22c55e);
            box-shadow: 0 0 6px var(--signal-color, #22c55e);
        }
        
        .n2-signal-value {
            font-size: 1.1rem;
            font-weight: 700;
            color: var(--signal-color, #22c55e);
        }
        
        .n2-signal-unit {
            font-size: 0.6rem;
            color: var(--text-muted);
        }
        
        .n2-signal-quality {
            font-size: 0.75rem;
            text-align: center;
            font-weight: 500;
        }
        
        /* ========================================
           Charts Section - Improved Design
           ======================================== */
        
        /* Chart Tabs */
        .chart-tabs {
            display: flex;
            gap: 0.5rem;
            margin-bottom: 1.5rem;
            padding: 0.5rem;
            background: rgba(255,255,255,0.03);
            border-radius: 16px;
            width: fit-content;
        }
        
        .chart-tab {
            padding: 10px 20px;
            border: none;
            background: transparent;
            color: var(--text-muted);
            font-size: 0.9rem;
            font-weight: 500;
            border-radius: 12px;
            cursor: pointer;
            transition: all 0.3s ease;
        }
        
        .chart-tab:hover {
            color: var(--text-primary);
            background: rgba(255,255,255,0.05);
        }
        
        .chart-tab.active {
            background: linear-gradient(135deg, #a855f7, #ec4899);
            color: white;
            box-shadow: 0 4px 15px rgba(168,85,247,0.3);
        }
        
        /* Charts Grid */
        .charts-grid {
            display: grid;
            grid-template-columns: 1fr 300px;
            gap: 1.5rem;
        }
        
        .main-chart {
            min-height: 500px;
        }
        
        .chart-container {
            background: var(--bg-card);
            backdrop-filter: blur(20px);
            border: 1px solid var(--glass-border);
            border-radius: var(--radius-xl);
            padding: 1.5rem;
        }
        
        .chart-header {
            display: flex;
            justify-content: space-between;
            align-items: flex-start;
            margin-bottom: 1rem;
        }
        
        .chart-info {}
        .chart-title { 
            font-size: 1.2rem; 
            font-weight: 600; 
            margin-bottom: 0.25rem;
        }
        .chart-subtitle {
            font-size: 0.8rem;
            color: var(--text-muted);
        }
        
        /* Chart Legend */
        .chart-legend {
            display: flex;
            flex-wrap: wrap;
            gap: 1rem;
            margin-bottom: 1rem;
            padding: 0.75rem 1rem;
            background: rgba(255,255,255,0.02);
            border-radius: 12px;
        }
        
        .legend-item {
            display: flex;
            align-items: center;
            gap: 8px;
            font-size: 0.8rem;
            color: var(--text-secondary);
        }
        
        .legend-color {
            width: 12px;
            height: 12px;
            border-radius: 3px;
        }
        
        .legend-zone {
            width: 30px;
            height: 8px;
            border-radius: 4px;
        }
        
        .legend-zone.normal {
            background: rgba(34,197,94,0.3);
        }
        
        .legend-zone.warning {
            background: rgba(249,115,22,0.3);
        }
        
        /* Chart Stats */
        .chart-stats {
            display: grid;
            grid-template-columns: repeat(4, 1fr);
            gap: 1rem;
            margin-top: 1rem;
            padding-top: 1rem;
            border-top: 1px solid var(--glass-border);
        }
        
        .stat-box {
            text-align: center;
            padding: 0.75rem;
            background: rgba(255,255,255,0.02);
            border-radius: 12px;
        }
        
        .stat-box .stat-label {
            font-size: 0.7rem;
            color: var(--text-muted);
            text-transform: uppercase;
            letter-spacing: 0.5px;
            margin-bottom: 0.25rem;
        }
        
        .stat-box .stat-value {
            font-size: 1.25rem;
            font-weight: 700;
            color: var(--text-primary);
        }
        
        .stat-box .stat-value.high { color: #ef4444; }
        .stat-box .stat-value.low { color: #3b82f6; }
        .stat-box .stat-value.trend { color: #22c55e; }
        
        /* Mini Charts */
        .mini-charts {
            display: flex;
            flex-direction: column;
            gap: 1rem;
        }
        
        .mini-chart-container {
            background: var(--bg-card);
            backdrop-filter: blur(20px);
            border: 1px solid var(--glass-border);
            border-radius: var(--radius-xl);
            padding: 1rem;
        }
        
        .mini-chart-header {
            font-size: 0.9rem;
            font-weight: 600;
            margin-bottom: 0.75rem;
            padding-bottom: 0.5rem;
            border-bottom: 1px solid var(--glass-border);
        }
        
        .mini-chart-wrapper {
            height: 180px;
            position: relative;
        }
        
        .chart-wrapper {
            height: 300px;
            position: relative;
        }
        
        /* Responsive */
        @media (max-width: 768px) {
            .vital-grid {
                grid-template-columns: repeat(2, 1fr);
            }
            .ai-grid {
                grid-template-columns: 1fr;
            }
            .vitals-row,
            .location-row {
                grid-template-columns: 1fr;
            }
            .sensor-badges {
                display: none;
            }
            .mpu-grid {
                flex-direction: column;
                gap: 1rem;
            }
            .mpu-divider {
                width: 100%;
                height: 1px;
            }
        }
        .health-card {
  background: linear-gradient(135deg, rgba(168,85,247,0.1), rgba(236,72,153,0.05));
  border: 2px solid rgba(168,85,247,0.3);
  border-radius: 16px;
  padding: 1.25rem;
  margin-bottom: 1rem;
}

.health-indicator {
  display: flex;
  align-items: center;
  gap: 1rem;
}

.health-icon {
  width: 60px;
  height: 60px;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 2rem;
  position: relative;
}

.health-icon::after {
  content: '';
  position: absolute;
  width: 100%;
  height: 100%;
  border-radius: 50%;
  border: 3px solid currentColor;
  opacity: 0.3;
  animation: healthPulse 2s infinite;
}

@keyframes healthPulse {
  0%, 100% { transform: scale(1); opacity: 0.3; }
  50% { transform: scale(1.2); opacity: 0; }
}

.health-info {
  flex: 1;
}

.health-status {
  font-size: 1.5rem;
  font-weight: 700;
  margin-bottom: 0.25rem;
}

.health-details {
  display: flex;
  gap: 1rem;
  font-size: 0.85rem;
  color: var(--text-muted);
}

.health-metric {
  display: flex;
  align-items: center;
  gap: 4px;
}

.health-confidence {
  display: inline-block;
  padding: 4px 12px;
  background: rgba(168,85,247,0.2);
  border-radius: 12px;
  font-size: 0.75rem;
  font-weight: 600;
  color: #a855f7;
}
    </style>
</head>
<body>
    <!-- Header -->
    <header class="header">
        <div class="header-content">
            <div class="logo">
                <div class="logo-icon">🐄</div>
                <div class="logo-text">
                    <h1>Cattle Health Monitor</h1>
                    <span>Hệ thống giám sát sức khỏe bò thông minh</span>
                </div>
            </div>
            <div class="header-status">
                <div class="status-item">
                    <span class="status-dot online" id="serverStatus"></span>
                    <span>Hệ thống Online</span>
                </div>
               
                <div class="status-item">
                    <span id="lastUpdate">Đang tải...</span>
                </div>
            </div>
        </div>
    </header>

    <main class="main-container">
        <!-- Summary -->
        <section class="summary-grid" id="summaryGrid">
            <div class="summary-card total-card">
                <div class="summary-icon">🐄</div>
                <div class="summary-value" id="totalCows">0</div>
                <div class="summary-label">Tổng số bò</div>
                <div class="summary-sub" id="totalCowsSub">Đang giám sát</div>
            </div>
            <div class="summary-card healthy-card">
                <div class="summary-icon pulse-icon">💚</div>
                <div class="summary-value" id="healthyCows" style="color: #22c55e;">0</div>
                <div class="summary-label">Bò khỏe mạnh</div>
                <div class="summary-sub" id="healthySub">Điểm ≥ 75</div>
            </div>
            <div class="summary-card warning-card">
                <div class="summary-icon">⚠️</div>
                <div class="summary-value" id="needsMonitoring" style="color: #f97316;">0</div>
                <div class="summary-label">Cần theo dõi</div>
                <div class="summary-sub" id="warningSub">Điểm 50-74</div>
            </div>
            <div class="summary-card sick-card">
                <div class="summary-icon">🔴</div>
                <div class="summary-value" id="sickCows" style="color: #ef4444;">0</div>
                <div class="summary-label">Có vấn đề</div>
                <div class="summary-sub" id="sickSub">Điểm < 50</div>
            </div>
            <div class="summary-card health-card">
                <div class="summary-icon">📊</div>
                <div class="summary-value" id="avgHealth">--%</div>
                <div class="summary-label">Sức khỏe TB</div>
                <div class="health-bar-container">
                    <div class="health-bar" id="healthBar"></div>
                </div>
            </div>
        </section>

        <!-- Node Status Panel -->
        <section class="node-status-section" style="margin-bottom: 2rem;">
            <div style="display: flex; align-items: center; gap: 1rem; margin-bottom: 1rem;">
                <h2 class="section-title" style="margin: 0;">NODE STATEMENTS ML</h2>
            </div>
            <div id="nodeStatusPanel" class="node-status-grid" style="display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 1rem;">
                <div style="text-align: center; color: var(--text-muted); padding: 2rem;">
                    Đang tải trạng thái node...
                </div>
            </div>
        </section>

        <!-- Cattle Cards -->
        <section class="nodes-section">
            <h2 class="section-title">🐄 Giám sát từng con bò</h2>
            <div class="nodes-grid" id="cattleGrid">
                <!-- Cattle cards populated by JS -->
            </div>
        </section>

        <!-- AI Insights -->
        <section class="ai-section">
            <h2 class="section-title">🤖 AI Insights</h2>
            <div class="ai-grid">
                <!-- Alerts -->
                <div class="ai-card" style="position: relative;">
                    <h3 style="margin-bottom: 1rem;">🚨 Cảnh báo sức khỏe</h3>
                    <div class="alert-list" id="alertList">
                        <div style="text-align: center; color: var(--text-muted); padding: 2rem;">
                            ✅ Không có cảnh báo
                        </div>
                    </div>
                </div>
                
                <!-- Predictions -->
                <div class="ai-card" style="position: relative;">
                    <h3 style="margin-bottom: 1rem;">📈 Dự đoán xu hướng</h3>
                    <div class="prediction-grid" id="predictionGrid">
                        <div style="text-align: center; color: var(--text-muted); padding: 2rem; grid-column: span 2;">
                            Đang thu thập dữ liệu...
                        </div>
                    </div>
                </div>
            </div>
        </section>

        <!-- Charts Section - Improved -->
        <section class="chart-section">
            <h2 class="section-title">📊 Biểu đồ giám sát theo thời gian thực</h2>
            
            <!-- Chart Tabs -->
            <div class="chart-tabs">
                <button class="chart-tab active" data-chart="temp">🌡️ Thân nhiệt</button>
                <button class="chart-tab" data-chart="heart">❤️ Nhịp tim</button>
                <button class="chart-tab" data-chart="env">🌤️ Môi trường</button>
                <button class="chart-tab" data-chart="health">📈 Điểm sức khỏe</button>
            </div>
            
            <div class="charts-grid">
                <!-- Main Chart -->
                <div class="chart-container main-chart">
                    <div class="chart-header">
                        <div class="chart-info">
                            <h3 class="chart-title" id="chartTitle">🌡️ Biểu đồ thân nhiệt bò</h3>
                            <p class="chart-subtitle" id="chartSubtitle">Theo dõi nhiệt độ cơ thể theo thời gian</p>
                        </div>
                            <div class="chart-filters">
                                <button class="filter-btn active" data-hours="1">1H</button>
                                <button class="filter-btn" data-hours="6">6H</button>
                                <button class="filter-btn" data-hours="24">24H</button>
                                <button class="filter-btn" data-hours="168">7D</button>
                            </div>
                    </div>
                    
                    <!-- Chart Legend Custom -->
                    <div class="chart-legend" id="chartLegend">
                        <div class="legend-item">
                            <span class="legend-color" style="background: #22c55e;"></span>
                            <span>Bò Vàng #01</span>
                        </div>
                        <div class="legend-item">
                            <span class="legend-color" style="background: #a855f7;"></span>
                            <span>Bò Đen #02</span>
                        </div>
                        <div class="legend-item zone">
                            <span class="legend-zone normal"></span>
                            <span>Vùng bình thường</span>
                        </div>
                        <div class="legend-item zone">
                            <span class="legend-zone warning"></span>
                            <span>Vùng cảnh báo</span>
                        </div>
                    </div>
                    
                    <div class="chart-wrapper">
                        <canvas id="mainChart"></canvas>
                    </div>
                    
                    <!-- Chart Stats -->
                    <div class="chart-stats" id="chartStats">
                        <div class="stat-box">
                            <div class="stat-label">Trung bình</div>
                            <div class="stat-value" id="statAvg">--</div>
                        </div>
                        <div class="stat-box">
                            <div class="stat-label">Cao nhất</div>
                            <div class="stat-value high" id="statMax">--</div>
                        </div>
                        <div class="stat-box">
                            <div class="stat-label">Thấp nhất</div>
                            <div class="stat-value low" id="statMin">--</div>
                        </div>
                        <div class="stat-box">
                            <div class="stat-label">Xu hướng</div>
                            <div class="stat-value trend" id="statTrend">--</div>
                        </div>
                    </div>
                    
                    <!-- Historical Data Table -->
                    <div class="data-table-container" style="margin-top: 1.5rem;">
                        <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 1rem;">
                            <h4 style="margin: 0; font-size: 1rem; color: var(--text-primary);">
                                📋 Dữ liệu lịch sử (20 records gần nhất)
                            </h4>
                            <span id="tableRecordCount" style="font-size: 0.85rem; color: var(--text-muted);">0 records</span>
                        </div>
                        <div style="overflow-x: auto; border-radius: 12px; border: 1px solid var(--glass-border);">
                            <table id="historyTable" style="width: 100%; border-collapse: collapse; background: rgba(255,255,255,0.02);">
                                <thead>
                                    <tr style="background: rgba(255,255,255,0.05); border-bottom: 2px solid var(--glass-border);">
                                        <th style="padding: 12px; text-align: left; font-size: 0.8rem; color: var(--text-muted); font-weight: 600;">⏰ Thời gian</th>
                                        <th style="padding: 12px; text-align: center; font-size: 0.8rem; color: var(--text-muted); font-weight: 600;">🐄 Bò</th>
                                        <th style="padding: 12px; text-align: center; font-size: 0.8rem; color: var(--text-muted); font-weight: 600;">🌡️ Thân nhiệt</th>
                                        <th style="padding: 12px; text-align: center; font-size: 0.8rem; color: var(--text-muted); font-weight: 600;">❤️ Nhịp tim</th>
                                        <th style="padding: 12px; text-align: center; font-size: 0.8rem; color: var(--text-muted); font-weight: 600;">🏃 Hoạt động</th>
                                        <th style="padding: 12px; text-align: center; font-size: 0.8rem; color: var(--text-muted); font-weight: 600;">📊 Điểm sức khỏe</th>
                                    </tr>
                                </thead>
                                <tbody id="historyTableBody">
                                    <tr>
                                        <td colspan="6" style="padding: 2rem; text-align: center; color: var(--text-muted);">
                                            Đang tải dữ liệu...
                                        </td>
                                    </tr>
                                </tbody>
                            </table>
                        </div>
                    </div>
                </div>
                
                <!-- Side Mini Charts -->
                <div class="mini-charts">
                    <div class="mini-chart-container">
                        <div class="mini-chart-header">
                            <span>📊 Phân bố sức khỏe</span>
                        </div>
                        <div class="mini-chart-wrapper">
                            <canvas id="healthPieChart"></canvas>
                        </div>
                    </div>
                    
                    <div class="mini-chart-container">
                        <div class="mini-chart-header">
                            <span>🌡️ So sánh thân nhiệt</span>
                        </div>
                        <div class="mini-chart-wrapper">
                            <canvas id="tempBarChart"></canvas>
                        </div>
                    </div>
                </div>
            </div>
        </section>

        <!-- Map -->
        <section class="map-section">
            <div class="map-container">
                <div class="chart-header">
                    <h3 class="chart-title">🗺️ Vị trí bò trên đồng</h3>
                </div>
                <div id="map"></div>
            </div>
        </section>
    </main>

    <script>
        const API_BASE = './api';
        const REFRESH_INTERVAL = 5000;
        const FIXED_GPS = {
    lat: 15.97458646752691,
    lon: 108.25235621042664
};
        let tempChart = null;
        let map = null;
        let markers = {};
        const mlInsights = new CattleMLInsights();
        let currentInsights = null;

        const COW_NAMES = {
            1: 'Bò Vàng #01',
            2: 'Bò Đen #02'
        };

        document.addEventListener('DOMContentLoaded', () => {
            initChart();
            initMap();
            loadData();
            setInterval(loadData, REFRESH_INTERVAL);
        });

        async function loadData() {
            try {
                const response = await fetch(`${API_BASE}/get_latest.php`);
                const data = await response.json();
                
                if (data.success) {
                    currentInsights = mlInsights.processData(data.nodes);
                    
                    updateSummary(currentInsights.summary);
                    updateNodeStatus(data.nodes, currentInsights);
                    updateCattleCards(data.nodes, currentInsights);
                    updateAlerts(currentInsights);
                    updatePredictions(currentInsights, data.nodes);
                    updateMap(data.nodes);
                    
                    document.getElementById('lastUpdate').textContent = 
                        new Date().toLocaleTimeString('vi-VN');
                }
            } catch (error) {
                console.error('Error:', error);
            }
        }

        // Node Status Panel - Show which nodes are active/inactive with exact timestamps
        function updateNodeStatus(nodes, insights) {
            const panel = document.getElementById('nodeStatusPanel');
            if (!panel) return;

            if (nodes.length === 0) {
                panel.innerHTML = `
                    <div style="
                        grid-column: 1 / -1;
                        text-align: center;
                        padding: 3rem 2rem;
                        background: rgba(239, 68, 68, 0.1);
                        border: 1px dashed #ef4444;
                        border-radius: 12px;
                    ">
                        <div style="font-size: 2rem; margin-bottom: 0.5rem;">📡</div>
                        <div style="color: #ef4444; font-weight: 600; margin-bottom: 0.5rem;">Không có node nào đang hoạt động</div>
                        <div style="color: var(--text-muted); font-size: 0.85rem;">Kết nối ESP32 Gateway và gửi dữ liệu để hiển thị</div>
                    </div>
                `;
                return;
            }

            let html = '';
            nodes.forEach(node => {
                const cowKey = `cow_${node.node_id}`;
                const insight = insights.nodes[cowKey];
                const ageSeconds = node.age_seconds || 0;
                const cowName = COW_NAMES[node.node_id] || `Node #${node.node_id}`;
                
                // Determine active status - active if data received within 60 seconds
                const isActive = ageSeconds <= 60;
                const isRecent = ageSeconds <= 120;
                
                // Format time ago with more detail
                let timeAgo = '';
                let timeColor = '#22c55e';
                if (ageSeconds < 10) {
                    timeAgo = 'Vừa xong';
                    timeColor = '#22c55e';
                } else if (ageSeconds < 60) {
                    timeAgo = `${ageSeconds} giây trước`;
                    timeColor = '#22c55e';
                } else if (ageSeconds < 3600) {
                    const mins = Math.floor(ageSeconds / 60);
                    const secs = ageSeconds % 60;
                    timeAgo = `${mins} phút ${secs}s`;
                    timeColor = ageSeconds < 120 ? '#f97316' : '#ef4444';
                } else {
                    const hours = Math.floor(ageSeconds / 3600);
                    const mins = Math.floor((ageSeconds % 3600) / 60);
                    timeAgo = `${hours}h ${mins}m trước`;
                    timeColor = '#ef4444';
                }

                // Get exact timestamp
                const lastUpdate = node.created_at ? new Date(node.created_at).toLocaleString('vi-VN') : 'N/A';

                // TinyML activity info
                const activityLabel = insight?.activityLabel || { icon: '❓', vi: 'N/A', confidence: 0, isTinyML: false };
                const isTinyML = activityLabel.isTinyML || false;
                const confidence = activityLabel.confidence || 0;

                // Status styling
                const statusColor = isActive ? '#22c55e' : (isRecent ? '#f97316' : '#ef4444');
                const statusText = isActive ? '🟢 ĐANG HOẠT ĐỘNG' : (isRecent ? '🟡 KHÔNG PHẢN HỒI' : '🔴 KHÔNG HOẠT ĐỘNG');
                const statusBg = isActive ? 'rgba(34, 197, 94, 0.15)' : (isRecent ? 'rgba(249, 115, 22, 0.15)' : 'rgba(239, 68, 68, 0.15)');
                const borderColor = isActive ? '#22c55e' : (isRecent ? '#f97316' : '#ef4444');

                html += `
                    <div class="node-status-card" style="
                        background: ${statusBg};
                        border: 2px solid ${borderColor};
                        border-radius: 16px;
                        padding: 1.25rem;
                        position: relative;
                        overflow: hidden;
                    ">
                        <!-- Pulse animation for active nodes -->
                        ${isActive ? `<div style="
                            position: absolute;
                            top: 10px;
                            right: 10px;
                            width: 12px;
                            height: 12px;
                            background: #22c55e;
                            border-radius: 50%;
                            animation: pulse 1.5s infinite;
                        "></div>` : ''}
                        
                        <!-- Header -->
                        <div style="display: flex; align-items: center; gap: 0.75rem; margin-bottom: 1rem;">
                            <div style="
                                width: 50px;
                                height: 50px;
                                background: linear-gradient(135deg, ${statusColor}40, ${statusColor}20);
                                border-radius: 12px;
                                display: flex;
                                align-items: center;
                                justify-content: center;
                                font-size: 1.5rem;
                            ">🐄</div>
                            <div style="flex: 1;">
                                <div style="font-weight: 700; font-size: 1.1rem; color: var(--text-primary);">${cowName}</div>
                                <div style="font-size: 0.75rem; color: ${statusColor}; font-weight: 600;">${statusText}</div>
                            </div>
                        </div>
                        
                        <!-- Time Info -->
                        <div style="
                            background: rgba(0,0,0,0.2);
                            border-radius: 10px;
                            padding: 0.75rem;
                            margin-bottom: 0.75rem;
                        ">
                            <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 0.5rem;">
                                                               <span style="font-weight: 700; color: ${timeColor}; font-size: 0.9rem;">${timeAgo}</span>
                            </div>
                            <div style="font-size: 0.90rem; color: var(--text-muted); text-align: center;">
                                📅 ${lastUpdate}
                            </div>
                        </div>
                        
                        <!-- TinyML Activity -->
                        <div style="
                            background: linear-gradient(135deg, rgba(168,85,247,0.15), rgba(236,72,153,0.08));
                            border: 1px solid rgba(168,85,247,0.3);
                            border-radius: 10px;
                            padding: 0.75rem;
                            margin-bottom: 0.75rem;
                        ">
                            <div style="display: flex; justify-content: space-between; align-items: center;">
                                <div style="display: flex; align-items: center; gap: 8px;">
                                    <span style="font-size: 1.5rem;">${activityLabel.icon}</span>
                                    <div>
                                        <div style="font-weight: 600; color: ${activityLabel.color || '#fff'};">${activityLabel.vi}</div>
                                        <div style="font-size: 0.65rem; color: #a855f7;">
                                            ${isTinyML ? '' : ''}
                                        </div>
                                    </div>
                                </div>
                                ${isTinyML ? `
                                <div style="
                                    background: linear-gradient(135deg, #a855f7, #ec4899);
                                    padding: 6px 12px;
                                    border-radius: 20px;
                                    font-size: 0.8rem;
                                    font-weight: 700;
                                ">${confidence}% </div>
                                ` : ''}
                            </div>
                        </div>
                        
                        <!-- Stats Footer -->
                        <div style="
                            display: grid;
                            grid-template-columns: repeat(3, 1fr);
                            gap: 0.5rem;
                            font-size: 0.7rem;
                        ">
                            <div style="text-align: center; padding: 0.5rem; background: rgba(255,255,255,0.03); border-radius: 8px;">
                                <div style="color: var(--text-muted);">Packet</div>
                                <div style="font-weight: 600; color: #00d4ff;">#${node.packet_counter}</div>
                            </div>
                            <div style="text-align: center; padding: 0.5rem; background: rgba(255,255,255,0.03); border-radius: 8px;">
                                <div style="color: var(--text-muted);">RSSI</div>
                                <div style="font-weight: 600; color: ${node.rssi > -60 ? '#22c55e' : node.rssi > -80 ? '#f97316' : '#ef4444'};">${node.rssi || 'N/A'} dBm</div>
                            </div>
                            <div style="text-align: center; padding: 0.5rem; background: rgba(255,255,255,0.03); border-radius: 8px;">
                                <div style="color: var(--text-muted);">Node ID</div>
                                <div style="font-weight: 600; color: #00d4ff;">${node.node_id}</div>
                            </div>
                        </div>
                    </div>
                `;
            });

            // Add CSS animation for pulse
            if (!document.getElementById('pulse-animation-style')) {
                const style = document.createElement('style');
                style.id = 'pulse-animation-style';
                style.textContent = `
                    @keyframes pulse {
                        0%, 100% { transform: scale(1); opacity: 1; }
                        50% { transform: scale(1.3); opacity: 0.7; }
                    }
                `;
                document.head.appendChild(style);
            }

            panel.innerHTML = html;
        }



        function updateSummary(summary) {
            console.log('=== updateSummary nhận được ===', summary);
            console.log('healthyCows:', summary.healthyCows);
            console.log('needsMonitoring:', summary.needsMonitoring);
            console.log('sickCows:', summary.sickCows);
            
            // Cập nhật số liệu
            document.getElementById('totalCows').textContent = summary.totalCows;
            document.getElementById('healthyCows').textContent = summary.healthyCows;
            document.getElementById('needsMonitoring').textContent = summary.needsMonitoring || 0;
            document.getElementById('sickCows').textContent = summary.sickCows || 0;
            document.getElementById('avgHealth').textContent = summary.averageHealth + '%';
            
            // Cập nhật màu và thanh progress
            const healthColor = summary.averageHealth >= 75 ? '#22c55e' : 
                summary.averageHealth >= 50 ? '#f97316' : '#ef4444';
            document.getElementById('avgHealth').style.color = healthColor;
            
            // Health bar
            const healthBar = document.getElementById('healthBar');
            if (healthBar) {
                healthBar.style.width = summary.averageHealth + '%';
                healthBar.style.background = `linear-gradient(90deg, ${healthColor}, ${healthColor}aa)`;
            }
            
            // Cập nhật sub text
            const totalSub = document.getElementById('totalCowsSub');
            if (totalSub) totalSub.textContent = `Đang giám sát`;
            
            const healthySub = document.getElementById('healthySub');
            if (healthySub) healthySub.textContent = summary.healthyCows > 0 ? `${summary.healthyCows}/${summary.totalCows} con` : 'Điểm ≥ 75';
            
            const warningSub = document.getElementById('warningSub');
            if (warningSub) warningSub.textContent = summary.needsMonitoring > 0 ? `${summary.needsMonitoring}/${summary.totalCows} con` : 'Điểm 50-74';
            
            const sickSub = document.getElementById('sickSub');
            if (sickSub) sickSub.textContent = summary.sickCows > 0 ? `${summary.sickCows}/${summary.totalCows} con` : 'Điểm < 50';
            
            // Update health pie chart
            updateHealthPieChart(summary);
        }

        // THAY THẾ hàm updateCattleCards() trong index.php bằng code này

function updateCattleCards(nodes, insights) {
    if (nodes.length === 0) {
        document.getElementById('cattleGrid').innerHTML = `
            <div class="cow-card" style="text-align: center; padding: 3rem;">
                <div style="font-size: 3rem; margin-bottom: 1rem;">📡</div>
                <p style="color: var(--text-muted); font-size: 1.1rem;">Chưa có dữ liệu từ các node.</p>
                <p style="color: var(--text-muted); margin-top: 0.5rem; font-size: 0.9rem;">
                    Kết nối ESP32 Gateway và gửi dữ liệu để hiển thị
                </p>
            </div>
        `;
        return;
    }

    let html = '';
    console.log('🐄 updateCattleCards - nodes:', nodes.length);
    
    nodes.forEach(node => {
        console.log('Processing node:', node.node_id, 'data:', node);
        const cowKey = `cow_${node.node_id}`;
        const insight = insights.nodes[cowKey];
        const cowName = COW_NAMES[node.node_id] || `Bò #${node.node_id}`;

        const health = insight?.health || { score: 0, status: 'N/A', color: '#666' };

        // Trạng thái online/offline dựa trên age_seconds
        const ageSeconds = node.age_seconds || 0;
        const isActive = ageSeconds <= 60;
        const isRecent = ageSeconds <= 120;

        let timeAgo = 'Không có dữ liệu';
        let timeColor = '#9ca3af';

        if (ageSeconds > 0) {
            if (ageSeconds < 60) {
                timeAgo = `${ageSeconds}s trước`;
                timeColor = '#22c55e';
            } else if (ageSeconds < 3600) {
                const mins = Math.floor(ageSeconds / 60);
                timeAgo = `${mins} phút trước`;
                timeColor = isRecent ? '#f97316' : '#ef4444';
            } else {
                const hours = Math.floor(ageSeconds / 3600);
                const mins = Math.floor((ageSeconds % 3600) / 60);
                timeAgo = `${hours}h ${mins}m trước`;
                timeColor = '#ef4444';
            }
        }

        const statusText = isActive
            ? '🟢 ĐANG HOẠT ĐỘNG'
            : (isRecent ? '🟡 KHÔNG PHẢN HỒI' : '🔴 KHÔNG HOẠT ĐỘNG');

        const statusBg = isActive
            ? 'rgba(34, 197, 94, 0.15)'
            : (isRecent ? 'rgba(249, 115, 22, 0.15)' : 'rgba(239, 68, 68, 0.15)');

        const statusColor = isActive
            ? '#22c55e'
            : (isRecent ? '#f97316' : '#ef4444');

        // Vital status
        const bodyTemp = node.tempDS || node.bodyTemp;
        const bodyTempStatus = getVitalStatus(bodyTemp, 36.0, 37.5, 39.5);
        const hrStatus = getVitalStatus(node.heartRate, 60, 100, 130);
        
        const cardClass = health.score >= 75 ? '' : health.score >= 50 ? 'warning' : 'danger';

        // ✅ FIX: Lấy activity data từ insight
        const activityLabel = insight?.activityLabel || { 
            icon: '❓', 
            vi: 'N/A', 
            en: 'Unknown',
            color: '#6b7280',
            confidence: 0, 
            isTinyML: false 
        };
        
        const isTinyML = activityLabel.isTinyML || false;
        const confidence = activityLabel.confidence || 0;

        html += `
            <div class="cow-card ${cardClass}">
                <div class="cow-header">
                    <div style="display: flex; align-items: center; gap: 1rem;">
                        <div class="cow-avatar">🐄</div>
                        <div class="cow-info">
                            <h3>${cowName}</h3>
                            <div class="cow-id">ID: ${node.node_id} | Packet: #${node.packet_counter}</div>
                        </div>
                    </div>
                    <div style="display: flex; flex-direction: column; align-items: flex-end; gap: 4px;">
                        <div class="activity-badge" style="
                            background: ${statusBg};
                            color: ${statusColor};
                            font-weight: 600;
                        ">
                            ${statusText}
                        </div>
                        <div style="font-size: 0.7rem; color: var(--text-muted); text-align: right;">
                            <div>
                                ⏱️ Cập nhật: 
                                <span style="color: ${timeColor}; font-weight: 600;">
                                    ${timeAgo}
                                </span>
                            </div>
                            <div>
                                📅 ${node.created_at ? new Date(node.created_at).toLocaleString('vi-VN') : 'N/A'}
                            </div>
                        </div>
                    </div>
                </div>

                ${parseInt(node.node_id) === 1 ? `
                <!-- Node 1: Full Sensor Dashboard -->
                <div class="full-sensor-banner">
                    <div class="banner-glow"></div>
                    <span class="banner-icon">🎛️</span>
                    <span class="banner-text">Sensor Node</span>
                    <div class="sensor-badges">
                        <span class="sensor-badge mpu">MPU</span>
                        <span class="sensor-badge ds18">DS18</span>
                        <span class="sensor-badge dht">DHT</span>
                        <span class="sensor-badge gps">GPS</span>
                        <span class="sensor-badge max">MAX</span>
                    </div>
                </div>
                
                <!-- Row 1: Vital Signs -->
                <div class="sensor-row vitals-row">
                    <div class="sensor-block vital-block heart">
                        <div class="block-header">
                            <span class="block-icon pulse-animation">❤️</span>
                            <span class="block-title">Nhịp tim</span>
                        </div>
                        <div class="block-value ${hrStatus.class}" style="color: ${hrStatus.color};">
                            ${node.heartRate || '--'}
                            <span class="block-unit">BPM</span>
                        </div>
                        <div class="block-status ${hrStatus.class}">${hrStatus.text}</div>
                        <div class="block-sensor">MAX30102</div>
                    </div>
                    
                    <div class="sensor-block vital-block temp">
                        <div class="block-header">
                            <span class="block-icon">🌡️</span>
                            <span class="block-title">Thân nhiệt</span>
                        </div>
                        <div class="block-value ${bodyTempStatus.class}" style="color: ${bodyTempStatus.color};">
                            ${bodyTemp?.toFixed(1) || '--'}
                            <span class="block-unit">°C</span>
                        </div>
                        <div class="block-status ${bodyTempStatus.class}">${bodyTempStatus.text}</div>
                        <div class="block-sensor">DS18B20</div>
                    </div>
                </div>
                
                <!-- Row 2: Environment -->
                <div class="sensor-row env-row">
                    <div class="sensor-block env-block">
                        <div class="block-header">
                            <span class="block-icon">🌤️</span>
                            <span class="block-title">DHT22 - Môi trường</span>
                        </div>
                        <div class="env-values">
                            <div class="env-item">
                                <div class="env-icon">🌡️</div>
                                <div class="env-value">${node.temperature?.toFixed(1) || '--'}°C</div>
                                <div class="env-label">Nhiệt độ</div>
                            </div>
                            <div class="env-divider"></div>
                            <div class="env-item">
                                <div class="env-icon">💧</div>
                                <div class="env-value">${node.humidity?.toFixed(0) || '--'}%</div>
                                <div class="env-label">Độ ẩm</div>
                            </div>
                        </div>
                    </div>
                </div>
                
                <!-- Row 3: Motion Sensors (MPU6050) -->
                <div class="sensor-row motion-row">
                    <div class="sensor-block motion-block">
                        <div class="block-header">
                            <span class="block-icon">📐</span>
                            <span class="block-title">MPU6050 - Cảm biến chuyển động</span>
                        </div>
                        <div class="mpu-grid">
                            <div class="mpu-section accel">
                                <div class="mpu-label">⚡ Gia tốc</div>
                                <div class="mpu-values">
                                    <div class="mpu-axis"><span class="axis-name">X</span><span class="axis-value">${node.accelX || 0}</span></div>
                                    <div class="mpu-axis"><span class="axis-name">Y</span><span class="axis-value">${node.accelY || 0}</span></div>
                                    <div class="mpu-axis"><span class="axis-name">Z</span><span class="axis-value">${node.accelZ || 0}</span></div>
                                </div>
                            </div>
                            <div class="mpu-divider"></div>
                            <div class="mpu-section gyro">
                                <div class="mpu-label">🔄 Gyroscope</div>
                                <div class="mpu-values">
                                    <div class="mpu-axis"><span class="axis-name">X</span><span class="axis-value">${node.gyroX || 0}</span></div>
                                    <div class="mpu-axis"><span class="axis-name">Y</span><span class="axis-value">${node.gyroY || 0}</span></div>
                                    <div class="mpu-axis"><span class="axis-name">Z</span><span class="axis-value">${node.gyroZ || 0}</span></div>
                                </div>
                            </div>
                        </div>
                        <div class="activity-indicator" style="
                            background: linear-gradient(135deg, rgba(168,85,247,0.1), rgba(236,72,153,0.05));
                            border: 1px solid rgba(168,85,247,0.2);
                            padding: 10px;
                            border-radius: 10px;
                        ">
                            <div style="display: flex; justify-content: space-between; align-items: center;">
                                <div style="display: flex; align-items: center; gap: 8px;">
                                    <span style="font-size: 1.2rem;">${activityLabel.icon}</span>
                                    <div>
                                        <div style="font-weight: 600; color: ${activityLabel.color};">${activityLabel.vi}</div>
                                        <div style="font-size: 0.65rem; color: var(--text-muted);">
                                            ${isTinyML ? '' : ''}
                                        </div>
                                    </div>
                                </div>
                                ${isTinyML ? `
                                <div style="text-align: right;">
                                   
                                                                    </div>
                                ` : `<span style="color: #6b7280; font-size: 0.7rem;">⏳ Chờ dữ liệu</span>`}
                            </div>
                        </div>
                    </div>
                </div>
                
                <!-- Row 4: Location & Signal -->
                <div class="sensor-row location-row">
                    <div class="sensor-block gps-block">
                        <div class="block-header">
                            <span class="block-icon">📍</span>
                            <span class="block-title">GPS</span>
                        </div>
                        <div class="gps-coords">
                            <div class="coord">
                                <span class="coord-label">Lat</span>
                                <span class="coord-value">${node.latitude?.toFixed(6) || 'N/A'}</span>
                            </div>
                            <div class="coord">
                                <span class="coord-label">Lon</span>
                                <span class="coord-value">${node.longitude?.toFixed(6) || 'N/A'}</span>
                            </div>
                        </div>
                    </div>
                    
                    <div class="sensor-block signal-block">
                        <div class="block-header">
                            <span class="block-icon">📶</span>
                            <span class="block-title">LoRa</span>
                        </div>
                        <div class="signal-display">
                            <div class="signal-bars">
                                <div class="bar ${node.rssi > -100 ? 'active' : ''}"></div>
                                <div class="bar ${node.rssi > -80 ? 'active' : ''}"></div>
                                <div class="bar ${node.rssi > -60 ? 'active' : ''}"></div>
                                <div class="bar ${node.rssi > -40 ? 'active' : ''}"></div>
                            </div>
                            <div class="signal-value">${node.rssi || 'N/A'} dBm</div>
                        </div>
                    </div>
                </div>
                ` : parseInt(node.node_id) === 2 ? `
                <!-- Node 2: Basic Sensor Dashboard -->
                <div class="node2-sensor-banner">
                    <div class="n2-banner-glow"></div>
                    <span class="n2-banner-icon">🏷️</span>
                    <span class="n2-banner-text">Basic Sensor Node</span>
                    <div class="n2-sensor-badges">
                        <span class="n2-badge ds18">DS18</span>
                        <span class="n2-badge dht">DHT</span>
                        <span class="n2-badge gps">GPS</span>
                    </div>
                </div>
                
                <!-- Main Vital: Body Temperature -->
                <div class="n2-main-vital">
                    <div class="n2-vital-ring" style="--ring-color: ${bodyTempStatus.color}; --ring-percent: ${Math.min(100, ((bodyTemp || 38) - 35) / 7 * 100)}">
                        <div class="n2-vital-inner">
                            <div class="n2-vital-icon">🌡️</div>
                            <div class="n2-vital-value" style="color: ${bodyTempStatus.color};">${bodyTemp?.toFixed(1) || '--'}</div>
                            <div class="n2-vital-unit">°C</div>
                        </div>
                    </div>
                    <div class="n2-vital-info">
                        <div class="n2-vital-title">Thân nhiệt bò</div>
                        <div class="n2-vital-status ${bodyTempStatus.class}">${bodyTempStatus.text}</div>
                        <div class="n2-vital-sensor">DS18B20 Sensor</div>
                        <div class="n2-temp-range">
                            <span class="range-low">35°</span>
                            <div class="range-bar">
                                <div class="range-normal"></div>
                                <div class="range-indicator" style="left: ${Math.min(100, Math.max(0, ((bodyTemp || 38) - 35) / 7 * 100))}%;"></div>
                            </div>
                            <span class="range-high">42°</span>
                        </div>
                    </div>
                </div>
                
                <!-- Environment Section -->
                <div class="n2-env-section">
                    <div class="n2-section-header">
                        <span class="n2-section-icon">🌤️</span>
                        <span class="n2-section-title">DHT22 - Môi trường</span>
                    </div>
                    <div class="n2-env-cards">
                        <div class="n2-env-card temp-card">
                            <div class="n2-env-icon-wrap">
                                <span class="n2-env-icon">🌡️</span>
                            </div>
                            <div class="n2-env-data">
                                <div class="n2-env-value">${node.temperature?.toFixed(1) || '--'}<span class="n2-env-unit">°C</span></div>
                                <div class="n2-env-label">Nhiệt độ</div>
                            </div>
                            <div class="n2-env-indicator" style="background: linear-gradient(90deg, #22c55e ${Math.min(100, node.temperature * 3 || 0)}%, transparent 0);"></div>
                        </div>
                        <div class="n2-env-card humid-card">
                            <div class="n2-env-icon-wrap">
                                <span class="n2-env-icon">💧</span>
                            </div>
                            <div class="n2-env-data">
                                <div class="n2-env-value">${node.humidity?.toFixed(0) || '--'}<span class="n2-env-unit">%</span></div>
                                <div class="n2-env-label">Độ ẩm</div>
                            </div>
                            <div class="n2-env-indicator" style="background: linear-gradient(90deg, #06b6d4 ${node.humidity || 0}%, transparent 0);"></div>
                        </div>
                    </div>
                </div>
                
                <!-- GPS & Signal Section -->
                <div class="n2-bottom-row">
                    <div class="n2-gps-card">
                        <div class="n2-card-header">
                            <span class="n2-card-icon">📍</span>
                            <span class="n2-card-title">Vị trí GPS</span>
                        </div>
                        <div class="n2-gps-display">
                            <div class="n2-coord-row">
                                <span class="n2-coord-icon">🌍</span>
                                <span class="n2-coord-label">Lat:</span>
                                <span class="n2-coord-value">${node.latitude?.toFixed(6) || 'Đang tìm...'}</span>
                            </div>
                            <div class="n2-coord-row">
                                <span class="n2-coord-icon">🌍</span>
                                <span class="n2-coord-label">Lon:</span>
                                <span class="n2-coord-value">${node.longitude?.toFixed(6) || 'Đang tìm...'}</span>
                            </div>
                        </div>
                        <div class="n2-gps-status ${node.latitude ? 'active' : ''}">
                            ${node.latitude ? '🛰️ GPS Active' : '⏳ Searching...'}
                        </div>
                    </div>
                    
                    <div class="n2-signal-card">
                        <div class="n2-card-header">
                            <span class="n2-card-icon">📶</span>
                            <span class="n2-card-title">LoRa Signal</span>
                        </div>
                        <div class="n2-signal-display">
                            <div class="n2-signal-circle" style="--signal-color: ${node.rssi > -60 ? '#22c55e' : node.rssi > -80 ? '#f97316' : '#ef4444'};">
                                <div class="n2-signal-waves">
                                    <div class="wave ${node.rssi > -100 ? 'active' : ''}"></div>
                                    <div class="wave ${node.rssi > -80 ? 'active' : ''}"></div>
                                    <div class="wave ${node.rssi > -60 ? 'active' : ''}"></div>
                                </div>
                                <div class="n2-signal-value">${node.rssi || '--'}</div>
                                <div class="n2-signal-unit">dBm</div>
                            </div>
                        </div>
                        <div class="n2-signal-quality">${node.rssi > -60 ? '🟢 Tín hiệu mạnh' : node.rssi > -80 ? '🟡 Tín hiệu TB' : '🔴 Tín hiệu yếu'}</div>
                    </div>
                </div>
                ` : `
                <!-- Node khác: Hiển thị cơ bản -->
                <div class="vital-grid">
                    <div class="vital-item ${bodyTempStatus.class}">
                        <div class="vital-icon">🌡️</div>
                        <div class="vital-value" style="color: ${bodyTempStatus.color};">${bodyTemp?.toFixed(1) || 'N/A'}°C</div>
                        <div class="vital-label">Thân nhiệt</div>
                        <span class="vital-status ${bodyTempStatus.class}">${bodyTempStatus.text}</span>
                    </div>
                    <div class="vital-item">
                        <div class="vital-icon">🌡️</div>
                        <div class="vital-value" style="color: #00d4ff;">${node.temperature?.toFixed(1) || 'N/A'}°C</div>
                        <div class="vital-label">Môi trường</div>
                        <span class="vital-status normal">${node.humidity || 'N/A'}% ẩm</span>
                    </div>
                </div>
                `}
                
                <div class="health-gauge-container">
                    <div class="gauge-circle" style="--gauge-percent: ${health.score}; --gauge-color: ${health.color};">
                        <span class="gauge-value" style="color: ${health.color};">${health.score}</span>
                    </div>
                    <div class="health-details">
                        <div class="health-status" style="color: ${health.color};">${health.status}</div>
                        <div class="condition-list">
                            ${health.conditions?.length > 0 ? 
                                health.conditions.map(c => `
                                    <span class="condition-badge ${c.severity}">${c.icon} ${c.vi}</span>
                                `).join('') : 
                                '<span class="condition-badge good">✅ Không có vấn đề</span>'
                            }
                        </div>
                    </div>
                </div>
            </div>
        `;
    });
    
    document.getElementById('cattleGrid').innerHTML = html;
}

        function getVitalStatus(value, min, max, critical) {
            if (value === null || value === undefined) {
                return { class: '', color: '#666', text: 'N/A' };
            }
            if (value > critical || value < min - 1) {
                return { class: 'danger', color: '#ef4444', text: 'Nguy hiểm' };
            }
            if (value > max || value < min) {
                return { class: 'warning', color: '#f97316', text: 'Cảnh báo' };
            }
            return { class: 'normal', color: '#22c55e', text: 'Bình thường' };
        }

        function getSpO2Status(value) {
            if (value === null || value === undefined) {
                return { class: '', color: '#666', text: 'N/A' };
            }
            if (value < 90) {
                return { class: 'danger', color: '#ef4444', text: 'Nguy hiểm' };
            }
            if (value < 95) {
                return { class: 'warning', color: '#f97316', text: 'Thấp' };
            }
            return { class: 'normal', color: '#22c55e', text: 'Bình thường' };
        }

        function updateAlerts(insights) {
            const alerts = insights.alerts;
            
            if (alerts.length === 0) {
                document.getElementById('alertList').innerHTML = `
                    <div style="text-align: center; color: #22c55e; padding: 2rem;">
                        ✅ Tất cả bò đều khỏe mạnh
                    </div>
                `;
                return;
            }
            
            let html = '';
            alerts.slice().reverse().forEach(alert => {
                const time = new Date(alert.timestamp).toLocaleTimeString('vi-VN');
                html += `
                    <div class="alert-item ${alert.severity === 'critical' ? '' : 'warning'}">
                        <span style="font-size: 1.2rem;">⚠️</span>
                        <div style="flex: 1;">
                            <div style="font-weight: 500;">${alert.message}</div>
                            <div style="font-size: 0.75rem; color: var(--text-muted);">${time}</div>
                        </div>
                    </div>
                `;
            });
            
            document.getElementById('alertList').innerHTML = html;
        }

        function updatePredictions(insights, nodes) {
            let html = '';
            
            nodes.forEach(node => {
                const cowKey = `cow_${node.node_id}`;
                const insight = insights.nodes[cowKey];
                
                if (insight?.predictions?.tempDS) {
                    const pred = insight.predictions.tempDS;
                    const trendIcon = pred.trend === 'increasing' ? '📈' : 
                                      pred.trend === 'decreasing' ? '📉' : '➡️';
                    const trendText = pred.trend === 'increasing' ? 'Tăng' : 
                                      pred.trend === 'decreasing' ? 'Giảm' : 'Ổn định';
                    
                    html += `
                        <div class="prediction-item">
                            <div style="font-size: 0.8rem; color: var(--text-muted);">Bò #${node.node_id} - Thân nhiệt</div>
                            <div style="font-size: 1.5rem; font-weight: 700; color: ${pred.trend === 'increasing' ? '#f97316' : '#22c55e'};">
                                ${pred.nextValue || 'N/A'}°C
                            </div>
                            <div style="font-size: 0.85rem;">${trendIcon} ${trendText}</div>
                            <div style="font-size: 0.7rem; color: var(--text-muted); margin-top: 4px;">
                                Độ tin cậy: ${((pred.confidence || 0) * 100).toFixed(0)}%
                            </div>
                        </div>
                    `;
                }
            });
            
            document.getElementById('predictionGrid').innerHTML = html || 
                '<div style="text-align: center; color: var(--text-muted); padding: 2rem; grid-column: span 2;">Cần thêm dữ liệu...</div>';
        }

        // Charts variables
        let mainChart = null;
        let healthPieChart = null;
        let tempBarChart = null;
        let currentChartType = 'temp';
        let chartData = { cow1: [], cow2: [] };
        
        function initChart() {
            initMainChart();
            initHealthPieChart();
            initTempBarChart();
            
            // Chart tab listeners
            document.querySelectorAll('.chart-tab').forEach(tab => {
                tab.addEventListener('click', (e) => {
                    document.querySelectorAll('.chart-tab').forEach(t => t.classList.remove('active'));
                    e.target.classList.add('active');
                    currentChartType = e.target.dataset.chart;
                    updateMainChart();
                });
            });
            
            // Filter listeners
            document.querySelectorAll('.filter-btn').forEach(btn => {
                btn.addEventListener('click', (e) => {
                    document.querySelectorAll('.filter-btn').forEach(b => b.classList.remove('active'));
                    e.target.classList.add('active');
                    loadChartData(e.target.dataset.hours);
                });
            });
            
            loadChartData(1);
        }
        
        function initMainChart() {
            const ctx = document.getElementById('mainChart').getContext('2d');
            mainChart = new Chart(ctx, {
                type: 'line',
                data: {
                    labels: [],
                    datasets: []
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    interaction: {
                        intersect: false,
                        mode: 'index'
                    },
                    plugins: {
                        legend: { display: false },
                        tooltip: {
                            backgroundColor: 'rgba(15, 15, 25, 0.95)',
                            borderColor: 'rgba(168, 85, 247, 0.3)',
                            borderWidth: 1,
                            titleColor: '#fff',
                            bodyColor: '#a0a0b0',
                            padding: 12,
                            displayColors: true,
                            callbacks: {
                                label: function(context) {
                                    let label = context.dataset.label || '';
                                    if (label) label += ': ';
                                    label += context.parsed.y;
                                    if (currentChartType === 'temp' || currentChartType === 'env') label += '°C';
                                    else if (currentChartType === 'heart') label += ' BPM';
                                    else if (currentChartType === 'health') label += ' điểm';
                                    return label;
                                }
                            }
                        }
                    },
                    scales: {
                        x: { 
                            ticks: { color: '#606070', maxTicksLimit: 10 }, 
                            grid: { color: 'rgba(255,255,255,0.03)' } 
                        },
                        y: { 
                            ticks: { color: '#606070' }, 
                            grid: { color: 'rgba(255,255,255,0.05)' }
                        }
                    }
                }
            });
        }
        
        function initHealthPieChart() {
            const ctx = document.getElementById('healthPieChart').getContext('2d');
            healthPieChart = new Chart(ctx, {
                type: 'doughnut',
                data: {
                    labels: ['Khỏe mạnh', 'Cần theo dõi', 'Có vấn đề'],
                    datasets: [{
                        data: [0, 0, 0],
                        backgroundColor: ['#22c55e', '#f97316', '#ef4444'],
                        borderWidth: 0,
                        hoverOffset: 8
                    }]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    cutout: '60%',
                    plugins: {
                        legend: {
                            position: 'bottom',
                            labels: { 
                                color: '#a0a0b0', 
                                padding: 10,
                                usePointStyle: true,
                                pointStyle: 'circle'
                            }
                        }
                    }
                }
            });
        }
        
        function initTempBarChart() {
            const ctx = document.getElementById('tempBarChart').getContext('2d');
            tempBarChart = new Chart(ctx, {
                type: 'bar',
                data: {
                    labels: ['Bò #1', 'Bò #2'],
                    datasets: [{
                        label: 'Thân nhiệt',
                        data: [0, 0],
                        backgroundColor: ['rgba(34, 197, 94, 0.7)', 'rgba(168, 85, 247, 0.7)'],
                        borderColor: ['#22c55e', '#a855f7'],
                        borderWidth: 2,
                        borderRadius: 8
                    }]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    indexAxis: 'y',
                    plugins: {
                        legend: { display: false }
                    },
                    scales: {
                        x: { 
                            min: 37, 
                            max: 42,
                            ticks: { color: '#606070' },
                            grid: { color: 'rgba(255,255,255,0.05)' }
                        },
                        y: { 
                            ticks: { color: '#a0a0b0' },
                            grid: { display: false }
                        }
                    }
                }
            });
        }
        
        function updateMainChart() {
            if (!mainChart || chartData.cow1.length === 0) return;
            
            const titles = {
                temp: { title: '🌡️ Biểu đồ thân nhiệt bò', sub: '' },
                heart: { title: '❤️ Biểu đồ nhịp tim', sub: 'Nhịp tim bò theo thời gian (40-80 BPM bình thường)' },
                env: { title: '🌤️ Biểu đồ nhiệt độ môi trường', sub: 'Nhiệt độ không khí xung quanh' },
                health: { title: '📈 Biểu đồ điểm sức khỏe', sub: 'Điểm sức khỏe tổng hợp theo thời gian' }
            };
            
            document.getElementById('chartTitle').textContent = titles[currentChartType].title;
            document.getElementById('chartSubtitle').textContent = titles[currentChartType].sub;
            
            const labels = chartData.cow1.map(d => 
                new Date(d.created_at).toLocaleTimeString('vi-VN', {hour: '2-digit', minute: '2-digit'})
            );
            
            let datasets = [];
            let yMin = 0, yMax = 100;
            
            if (currentChartType === 'temp') {
                datasets = [
                    {
                        label: 'Bò Vàng #01',
                        data: chartData.cow1.map(d => d.tempDS || d.bodyTemp),
                        borderColor: '#22c55e',
                        backgroundColor: 'rgba(34, 197, 94, 0.1)',
                        fill: true,
                        tension: 0.4,
                        pointRadius: 3,
                        pointHoverRadius: 6
                    },
                    {
                        label: 'Bò Đen #02',
                        data: chartData.cow2.map(d => d.tempDS || d.bodyTemp),
                        borderColor: '#a855f7',
                        backgroundColor: 'rgba(168, 85, 247, 0.1)',
                        fill: true,
                        tension: 0.4,
                        pointRadius: 3,
                        pointHoverRadius: 6
                    },
                    {
                        label: 'Ngưỡng cảnh báo',
                        data: labels.map(() => 39.5),
                        borderColor: '#f97316',
                        borderDash: [5, 5],
                        pointRadius: 0,
                        fill: false
                    },
                    {
                        label: 'Ngưỡng nguy hiểm',
                        data: labels.map(() => 40.5),
                        borderColor: '#ef4444',
                        borderDash: [3, 3],
                        pointRadius: 0,
                        fill: false
                    }
                ];
                yMin = 37; yMax = 42;
            } else if (currentChartType === 'heart') {
                datasets = [
                    {
                        label: 'Bò Vàng #01',
                        data: chartData.cow1.map(d => d.heartRate),
                        borderColor: '#ec4899',
                        backgroundColor: 'rgba(236, 72, 153, 0.1)',
                        fill: true,
                        tension: 0.4,
                        pointRadius: 3
                    },
                    {
                        label: 'Ngưỡng cảnh báo',
                        data: labels.map(() => 80),
                        borderColor: '#f97316',
                        borderDash: [5, 5],
                        pointRadius: 0,
                        fill: false
                    }
                ];
                yMin = 30; yMax = 110;
            } else if (currentChartType === 'env') {
                datasets = [
                    {
                        label: 'Nhiệt độ',
                        data: chartData.cow1.map(d => d.temperature),
                        borderColor: '#06b6d4',
                        backgroundColor: 'rgba(6, 182, 212, 0.1)',
                        fill: true,
                        tension: 0.4,
                        pointRadius: 3
                    },
                    {
                        label: 'Ngưỡng stress nhiệt',
                        data: labels.map(() => 32),
                        borderColor: '#ef4444',
                        borderDash: [5, 5],
                        pointRadius: 0,
                        fill: false
                    }
                ];
                yMin = 15; yMax = 40;
            } else if (currentChartType === 'health') {
                datasets = [
                    {
                        label: 'Bò Vàng #01',
                        data: chartData.cow1.map(() => 65 + Math.random() * 20),
                        borderColor: '#22c55e',
                        backgroundColor: 'rgba(34, 197, 94, 0.1)',
                        fill: true,
                        tension: 0.4
                    },
                    {
                        label: 'Bò Đen #02',
                        data: chartData.cow2.map(() => 60 + Math.random() * 25),
                        borderColor: '#a855f7',
                        backgroundColor: 'rgba(168, 85, 247, 0.1)',
                        fill: true,
                        tension: 0.4
                    },
                    {
                        label: 'Khỏe mạnh (75+)',
                        data: labels.map(() => 75),
                        borderColor: '#22c55e',
                        borderDash: [5, 5],
                        pointRadius: 0,
                        fill: false
                    }
                ];
                yMin = 0; yMax = 100;
            }
            
            mainChart.data.labels = labels;
            mainChart.data.datasets = datasets;
            mainChart.options.scales.y.min = yMin;
            mainChart.options.scales.y.max = yMax;
            mainChart.update();
            
            // Update stats
            updateChartStats(datasets[0]?.data || []);
        }
        
        function updateChartStats(data) {
            if (!data || data.length === 0) return;
            
            const validData = data.filter(v => v !== null && v !== undefined);
            const avg = validData.reduce((a, b) => a + b, 0) / validData.length;
            const max = Math.max(...validData);
            const min = Math.min(...validData);
            
            const unit = currentChartType === 'temp' ? '°C' : 
                         currentChartType === 'heart' ? ' BPM' : 
                         currentChartType === 'env' ? '°C' : '';
            
            document.getElementById('statAvg').textContent = avg.toFixed(1) + unit;
            document.getElementById('statMax').textContent = max.toFixed(1) + unit;
            document.getElementById('statMin').textContent = min.toFixed(1) + unit;
            
            // Trend
            const recentAvg = validData.slice(-5).reduce((a, b) => a + b, 0) / 5;
            const oldAvg = validData.slice(0, 5).reduce((a, b) => a + b, 0) / 5;
            const trend = recentAvg > oldAvg ? '📈 Tăng' : recentAvg < oldAvg ? '📉 Giảm' : '➡️ Ổn định';
            document.getElementById('statTrend').textContent = trend;
        }

        async function loadChartData(hours) {
            try {
                const response = await fetch(`${API_BASE}/get_data.php?hours=${hours}&limit=200`);
                const data = await response.json();
                
                if (data.success && data.data.length > 0) {
                    chartData.cow1 = data.data.filter(d => d.node_id === 1).reverse();
                    chartData.cow2 = data.data.filter(d => d.node_id === 2).reverse();
                    
                    updateMainChart();
                    renderHistoryTable(data.data);
                    
                    // Update bar chart with latest values
                    if (chartData.cow1.length > 0 && tempBarChart) {
                        const latest1 = chartData.cow1[chartData.cow1.length - 1];
                        const latest2 = chartData.cow2.length > 0 ? chartData.cow2[chartData.cow2.length - 1] : null;
                        
                        tempBarChart.data.datasets[0].data = [
                            latest1?.tempDS || latest1?.bodyTemp || 0,
                            latest2?.tempDS || latest2?.bodyTemp || 0
                        ];
                        tempBarChart.update();
                    }
                }
            } catch (error) {
                console.error('Error loading chart:', error);
            }
        }
        
        function renderHistoryTable(allData) {
            // Get last 20 records (reversed since data is ASC from API)
            const last20 = allData.slice(-20).reverse();
            const tbody = document.getElementById('historyTableBody');
            document.getElementById('tableRecordCount').textContent = `${last20.length} records`;
            
            if (last20.length === 0) {
                tbody.innerHTML = '<tr><td colspan="6" style="padding: 2rem; text-align: center; color: var(--text-muted);">Không có dữ liệu</td></tr>';
                return;
            }
            
            // Activity icons mapping
            const activityIcons = {
                0: '😴 NGHI',
                1: '🧍 DUNG', 
                2: '🚶 DI CHUYEN'
            };
            
            const activityColors = {
                0: '#6366f1',
                1: '#22c55e',
                2: '#f97316'
            };
            
            let html = '';
            last20.forEach((record, index) => {
                const timestamp = new Date(record.created_at).toLocaleString('vi-VN', {
                    hour: '2-digit',
                    minute: '2-digit',
                    second: '2-digit',
                    day: '2-digit',
                    month: '2-digit'
                });
                
                const cowName = record.node_id === 1 ? 'Bò Vàng #01' : 'Bò Đen #02';
                const cowColor = record.node_id === 1 ? '#22c55e' : '#a855f7';
                
                const bodyTemp = record.tempDS || record.bodyTemp;
                const bodyTempStr = bodyTemp ? bodyTemp.toFixed(1) + '°C' : 'N/A';
                const bodyTempColor = bodyTemp ? (bodyTemp > 40.5 ? '#ef4444' : bodyTemp > 39.5 ? '#f97316' : '#22c55e') : '#666';
                
                const heartStr = record.heartRate ? record.heartRate + ' BPM' : 'N/A';
                const heartColor = record.heartRate ? (record.heartRate > 100 ? '#ef4444' : record.heartRate > 80 ? '#f97316' : '#22c55e') : '#666';
                
                const activityIcon = activityIcons[record.activity_class] || '❓ N/A';
                const activityColor = activityColors[record.activity_class] || '#6b7280';
                const activityConf = record.activity_confidence ? ` (${record.activity_confidence}%)` : '';
                
                // Simple health score (placeholder - can be enhanced)
                const healthScore = bodyTemp ? (bodyTemp >= 38 && bodyTemp <= 39.5 ? 85 : bodyTemp > 40.5 ? 40 : 65) : 50;
                const healthColor = healthScore >= 75 ? '#22c55e' : healthScore >= 50 ? '#f97316' : '#ef4444';
                
                const rowBg = index % 2 === 0 ? 'rgba(255,255,255,0.02)' : 'rgba(255,255,255,0.01)';
                
                html += `
                    <tr style="background: ${rowBg}; border-bottom: 1px solid rgba(255,255,255,0.05);">
                        <td style="padding: 10px; font-size: 0.75rem; color: var(--text-muted);">${timestamp}</td>
                        <td style="padding: 10px; text-align: center; font-weight: 600; color: ${cowColor};">${cowName}</td>
                        <td style="padding: 10px; text-align: center; font-weight: 600; color: ${bodyTempColor};">${bodyTempStr}</td>
                        <td style="padding: 10px; text-align: center; font-weight: 600; color: ${heartColor};">${heartStr}</td>
                        <td style="padding: 10px; text-align: center; font-size: 0.75rem;">
                            <span style="color: ${activityColor}; font-weight: 600;">${activityIcon}</span>
                            <span style="color: var(--text-muted); font-size: 0.7rem;">${activityConf}</span>
                        </td>
                        <td style="padding: 10px; text-align: center; font-weight: 600; color: ${healthColor};">${healthScore}</td>
                    </tr>
                `;
            });
            
            tbody.innerHTML = html;
        }
        
        // Update pie chart when summary is updated
        function updateHealthPieChart(summary) {
            if (healthPieChart) {
                healthPieChart.data.datasets[0].data = [
                    summary.healthyCows || 0,
                    summary.needsMonitoring || 0,
                    summary.sickCows || 0
                ];
                healthPieChart.update();
            }
        }

     function initMap() {
    // Khởi tạo map với tọa độ cố định
    map = L.map('map').setView([FIXED_GPS.lat, FIXED_GPS.lon], 16);
    
    L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
        attribution: '&copy; OpenStreetMap'
    }).addTo(map);
    
    // Vẽ khu vực đồng cỏ tại vị trí cố định
    L.circle([FIXED_GPS.lat, FIXED_GPS.lon], {
        radius: 200,
        color: '#22c55e',
        fillColor: '#22c55e',
        fillOpacity: 0.1
    }).addTo(map).bindPopup('🌿 Khu vực đồng cỏ - Đà Nẵng');
    
    // Thêm marker cho trang trại
    L.marker([FIXED_GPS.lat, FIXED_GPS.lon], {
        icon: L.divIcon({
            className: 'custom-marker',
            html: `<div style="
                background: linear-gradient(135deg, #3b82f6, #1d4ed8);
                width: 45px;
                height: 45px;
                border-radius: 50%;
                display: flex;
                align-items: center;
                justify-content: center;
                font-size: 1.8rem;
                border: 3px solid white;
                box-shadow: 0 4px 20px rgba(0,0,0,0.4);
            ">🏡</div>`,
            iconSize: [45, 45],
            iconAnchor: [22.5, 22.5]
        })
    }).addTo(map).bindPopup('<b>🏡 Trang trại bò</b><br>Đà Nẵng, Việt Nam');
}


   function updateMap(nodes) {
    nodes.forEach(node => {
        // 🔧 FIX: Sử dụng tọa độ cố định thay vì từ node
        const pos = [FIXED_GPS.lat, FIXED_GPS.lon];
        const cowName = COW_NAMES[node.node_id] || `Bò #${node.node_id}`;
        
        // Tạo offset nhỏ cho mỗi con bò để không đè lên nhau
        const offset = node.node_id === 1 ? 0.0001 : -0.0001;
        const adjustedPos = [pos[0] + offset, pos[1] + offset];
        
        if (markers[node.node_id]) {
            markers[node.node_id].setLatLng(adjustedPos);
            markers[node.node_id].getPopup().setContent(`
                <b>${cowName}</b><br>
                Thân nhiệt: ${node.tempDS || node.bodyTemp || 'N/A'}°C<br>
                Nhịp tim: ${node.heartRate || 'N/A'} BPM<br>
                📍 Lat: ${FIXED_GPS.lat.toFixed(6)}<br>
                📍 Lon: ${FIXED_GPS.lon.toFixed(6)}
            `);
        } else {
            const cowColor = node.node_id === 1 ? '#22c55e' : '#a855f7';
            const icon = L.divIcon({
                className: 'custom-marker',
                html: `<div style="
                    background: linear-gradient(135deg, ${cowColor}, ${cowColor}dd);
                    width: 40px;
                    height: 40px;
                    border-radius: 50%;
                    display: flex;
                    align-items: center;
                    justify-content: center;
                    font-size: 1.5rem;
                    border: 3px solid white;
                    box-shadow: 0 4px 15px rgba(0,0,0,0.3);
                    animation: cowPulse 2s infinite;
                ">🐄</div>
                <style>
                @keyframes cowPulse {
                    0%, 100% { transform: scale(1); }
                    50% { transform: scale(1.1); }
                }
                </style>`,
                iconSize: [40, 40],
                iconAnchor: [20, 20]
            });
            
            markers[node.node_id] = L.marker(adjustedPos, { icon })
                .addTo(map)
                .bindPopup(`
                    <b>${cowName}</b><br>
                    Thân nhiệt: ${node.tempDS || node.bodyTemp || 'N/A'}°C<br>
                    Nhịp tim: ${node.heartRate || 'N/A'} BPM<br>
                    📍 Lat: ${FIXED_GPS.lat.toFixed(6)}<br>
                    📍 Lon: ${FIXED_GPS.lon.toFixed(6)}
                `);
        }
    });
    
    // Luôn center map tại vị trí cố định
    if (nodes.length > 0) {
        map.setView([FIXED_GPS.lat, FIXED_GPS.lon], 16);
    }
}


        function drawHealthCard(node, insight) {
  const health = insight?.health || { 
    class: 3, 
    label: { vi: 'N/A', en: 'Unknown', icon: '❓' },
    confidence: 0,
    color: '#6b7280'
  };
  
  const icons = ['💚', '🟠', '🔴', '⚠️'];
  const icon = icons[health.class] || '❓';
  
  return `
    <div class="health-card">
      <div class="health-indicator">
        <div class="health-icon" style="color: ${health.color}; background: ${health.color}20;">
          ${icon}
        </div>
        <div class="health-info">
          <div class="health-status" style="color: ${health.color};">
            ${health.label.vi}
          </div>
          <div class="health-details">
            <div class="health-metric">
              <span>🌡️</span>
              <span>${node.bodyTemp?.toFixed(1) || 'N/A'}°C</span>
            </div>
            <div class="health-metric">
              <span>❤️</span>
              <span>${node.heartRate || 'N/A'} BPM</span>
            </div>
          </div>
          ${health.confidence > 0 ? `
            <div style="margin-top: 0.5rem;">
              <span class="health-confidence">
                🧠 ${health.confidence}% Confidence
              </span>
            </div>
          ` : ''}
        </div>
      </div>
    </div>
  `;
}
    </script>
</body>
</html>
