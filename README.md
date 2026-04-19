# GEMBOT - Distributed AI Robot System

A sophisticated distributed AI robot system using Raspberry Pi, ESP32, and AI Server (PC) for real-time object detection, voice interaction, and autonomous navigation.

## System Architecture

```
┌─────────────────────┐
│   Web Dashboard     │
│  (Live Stream + UI) │
└──────────┬──────────┘
           │ WebSocket/HTTP
┌──────────▼────────────────┐
│   Raspberry Pi            │
│  - Camera + OpenCV        │
│  - LiDAR Navigation       │
│  - STT Processing         │
│  - MQTT + HTTP Comm       │
└──────────┬────────────────┘
           │ HTTP/MQTT
     ┌─────┴─────┐
     │           │
┌────▼────┐  ┌──▼──────────────┐
│  ESP32  │  │  AI Server (PC) │
│  - TTS  │  │  - YOLOv8       │
│  - I2S  │  │  - FastAPI      │
└────┬────┘  └─────────────────┘
     │
  Speaker
(MAX98357A)
```

## Features

- **Real-time Vision**: Live camera streaming with YOLOv8 object detection
- **Smart Navigation**: LiDAR-based obstacle avoidance
- **Voice Interaction**: Speech-to-Text → AI Chat → Text-to-Speech
- **Web Dashboard**: Live stream with overlaid bounding boxes
- **Distributed Architecture**: MQTT for control, HTTP for heavy data

## Project Structure

```
gembot/
├── config/                 # Configuration files
│   ├── config.yaml        # Main configuration
│   └── mqtt.yaml          # MQTT settings
├── src/                   # Raspberry Pi source code
│   ├── main.py            # Entry point
│   ├── vision/            # Camera & detection
│   ├── lidar/             # Navigation
│   ├── audio/             # Voice processing
│   ├── comm/              # Communication
│   ├── control/           # Decision making
│   └── utils/             # Utilities
├── server/                # AI Server (PC)
│   ├── app.py             # FastAPI server
│   ├── detect.py          # YOLOv8 inference
│   └── models/            # Model files
├── esp32/                 # ESP32 firmware
│   └── main.ino           # Arduino sketch
├── dashboard/             # Web interface
│   ├── index.html
│   ├── style.css
│   └── script.js
├── scripts/               # Utility scripts
│   ├── install.sh
│   └── test_detection.py
├── requirements.txt       # Python dependencies
└── README.md
```

## Quick Start

### 1. Raspberry Pi Setup
```bash
cd gembot
pip install -r requirements.txt
python src/main.py
```

### 2. AI Server Setup
```bash
cd server
pip install -r requirements.txt
python app.py
```

### 3. ESP32 Setup
- Use Arduino IDE
- Load `esp32/main.ino`
- Configure WiFi credentials and server IP

### 4. Access Dashboard
- Open `dashboard/index.html` in browser
- Or run with Flask: `python -m http.server 8000` in dashboard folder

## Configuration

Edit `config/config.yaml`:

```yaml
mqtt:
  broker: "192.168.1.100"
  port: 1883
  
ai_server:
  host: "192.168.1.100"
  port: 5000
  
esp32:
  host: "192.168.1.50"
  port: 80
  
camera:
  resolution: [640, 480]
  fps: 30
  
lidar:
  port: "/dev/ttyUSB0"
  baudrate: 256000
```

## Key Dependencies

- **OpenCV**: Video capture and processing
- **Ultralytics**: YOLOv8 object detection
- **FastAPI**: REST API server
- **Paho-MQTT**: Message broker communication
- **SpeechRecognition**: STT conversion

## Communication Protocol

### MQTT Topics
- `gembot/status` - Robot status updates
- `gembot/motor/control` - Motor commands
- `gembot/esp32/speak` - TTS requests

### HTTP Endpoints
- `/detect` - Send image for detection
- `/speak` - Request TTS output
- `/stream` - MJPEG video stream

## License

MIT License - Feel free to use and modify

## Support

For issues and questions, refer to the documentation in each module.
