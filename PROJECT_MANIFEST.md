# GEMBOT Project Manifest

Complete file listing and documentation of the GEMBOT distributed AI robot system.

## Project Overview

**GEMBOT** is a sophisticated distributed AI robot system combining:
- **Raspberry Pi**: Main control unit with vision, LiDAR, and decision-making
- **AI Server (PC)**: YOLOv8-based object detection engine
- **ESP32**: Audio output and motor control peripherals
- **Web Dashboard**: Real-time monitoring and control interface

---

## Directory Structure

### Root Configuration Files

```
gembot/
├── README.md                  # Main project documentation
├── QUICKSTART.md              # 5-minute setup guide
├── ARCHITECTURE.md            # Technical architecture details
├── PROJECT_MANIFEST.md        # This file
├── requirements.txt           # Python dependencies (Raspberry Pi)
└── config/
    └── config.yaml            # System configuration (edit before running)
```

### Source Code Structure

```
src/                           # Raspberry Pi main application
├── main.py                    # Entry point & orchestration (200 lines)
├── __init__.py                # Package initialization
├── utils/                     # Utility modules
│   ├── __init__.py
│   ├── logger.py              # Logging system (70 lines)
│   └── config_loader.py       # Configuration management (73 lines)
├── vision/                    # Camera & object detection
│   ├── __init__.py
│   ├── camera.py              # OpenCV camera capture (139 lines)
│   ├── stream.py              # MJPEG streaming (149 lines)
│   └── detect_client.py       # AI server communication (136 lines)
├── lidar/                     # Navigation & obstacle detection
│   ├── __init__.py
│   ├── lidar.py               # LiDAR sensor interface (180 lines)
│   └── obstacle.py            # Obstacle avoidance logic (77 lines)
├── audio/                     # Speech processing
│   ├── __init__.py
│   ├── tts.py                 # Text-to-speech (106 lines)
│   └── stt.py                 # Speech recognition (91 lines)
├── comm/                      # Communication protocols
│   ├── __init__.py
│   ├── mqtt_client.py         # MQTT pub/sub (175 lines)
│   ├── http_client.py         # HTTP communication (98 lines)
│   └── protocol.py            # Message definitions (135 lines)
└── control/                   # Decision making & behavior
    ├── __init__.py
    └── brain.py               # AI decision engine (170 lines)
```

**Total Raspberry Pi Code**: ~1,500 lines of modular Python

### AI Server

```
server/                        # YOLOv8 detection server
├── app.py                     # FastAPI application (213 lines)
├── detect.py                  # Detection engine (150 lines)
└── requirements.txt           # Server dependencies
```

**Total AI Server Code**: ~360 lines of Python

### ESP32 Firmware

```
esp32/                         # Arduino firmware
└── main.ino                   # Complete firmware (380 lines)
```

**Total ESP32 Code**: ~380 lines of Arduino C++

### Web Dashboard

```
dashboard/                     # Browser interface
├── index.html                 # HTML structure (146 lines)
├── style.css                  # Styling (429 lines)
└── script.js                  # Interactivity (408 lines)
```

**Total Dashboard Code**: ~980 lines of web code

### Scripts & Documentation

```
scripts/                       # Utility scripts
└── install.sh                 # Installation script
```

---

## Module Documentation

### Core Modules

#### `src/main.py` - Robot Orchestrator
- **Purpose**: Main entry point for Raspberry Pi
- **Key Classes**: `GEMBOTRobot`
- **Features**: 
  - Initializes all subsystems
  - Manages main execution loop
  - Handles graceful shutdown
  - Signal handling for cleanup
- **Dependencies**: All src modules

#### `src/vision/camera.py` - Camera Management
- **Purpose**: OpenCV camera capture and processing
- **Key Classes**: `Camera`
- **Features**:
  - Threaded capture loop
  - Frame rotation support
  - JPEG encoding for streaming
  - Resolution and FPS configuration
- **Thread-safe**: Yes (frame_lock)

#### `src/vision/detect_client.py` - AI Communication
- **Purpose**: HTTP client for detection server
- **Key Classes**: `DetectionClient`
- **Features**:
  - Async frame sending
  - Confidence threshold filtering
  - Detection caching
  - Server health checks
- **Non-blocking**: Yes (threading support)

#### `src/lidar/lidar.py` - Navigation Sensor
- **Purpose**: LiDAR scanning for obstacle detection
- **Key Classes**: `LiDAR`
- **Features**:
  - Serial communication with RPLidar
  - Demo mode for testing
  - Continuous scan loop
  - Obstacle detection
- **Demo Mode**: Yes (simulated data generation)

#### `src/control/brain.py` - Decision Engine
- **Purpose**: AI brain for robot behavior
- **Key Classes**: `Brain`, `RobotState` (Enum)
- **Features**:
  - State machine (IDLE, MOVING, STOPPED, etc.)
  - Safety checks
  - Sensor fusion
  - Decision making
- **Safety**: Enabled by default

#### `src/comm/mqtt_client.py` - Message Broker
- **Purpose**: Pub/sub communication with MQTT
- **Key Classes**: `MQTTClient`
- **Features**:
  - Auto-reconnection
  - Topic subscription
  - Callback handling
  - Status publishing
- **Thread-safe**: Yes

### Server Modules

#### `server/app.py` - Detection API
- **Purpose**: FastAPI server for object detection
- **Endpoints**: 
  - `POST /detect` - Detect objects
  - `POST /detect_and_visualize` - With annotations
  - `GET /health` - Status check
  - `GET /info` - Server info
- **Features**: CORS support, error handling, JSON responses

#### `server/detect.py` - YOLOv8 Engine
- **Purpose**: YOLOv8 inference wrapper
- **Key Classes**: `ObjectDetector`
- **Features**:
  - Model loading
  - Batch inference
  - Visualization
  - Confidence filtering
- **GPU Support**: CUDA/MPS optional

### Configuration System

#### `config/config.yaml`
- **MQTT**: Broker address, port, auth
- **AI Server**: Host, port, confidence threshold
- **ESP32**: Host, port, timeout
- **Camera**: Resolution, FPS, rotation
- **LiDAR**: Serial port, max distance
- **Motor**: GPIO pins, PWM settings
- **Safety**: Distance thresholds, limits

### Logging System

#### `src/utils/logger.py`
- **Features**:
  - Console and file logging
  - Colored output
  - Log rotation
  - Singleton pattern
- **Log File**: `logs/gembot.log`
- **Max Size**: 10MB with 5 backups

---

## Data Flow & Communication

### Message Protocol (JSON)

**Robot Status**:
```json
{
  "type": "status",
  "state": "moving",
  "battery": 85.5,
  "uptime": 3600,
  "error": null
}
```

**Detection Results**:
```json
{
  "success": true,
  "detection_count": 3,
  "detections": [
    {
      "box": [100, 150, 200, 300],
      "label": "person",
      "confidence": 0.95,
      "class_id": 0
    }
  ]
}
```

**Motor Command**:
```json
{
  "left_speed": 150,
  "right_speed": 150
}
```

---

## Configuration Reference

### MQTT Topics
- `gembot/status` - Robot status
- `gembot/motor/control` - Motor commands
- `gembot/esp32/speak` - TTS requests
- `gembot/alert/obstacle` - Obstacle warnings

### HTTP Endpoints

**Raspberry Pi**:
- `http://raspi:5001/stream` - MJPEG stream
- `http://raspi:5001/status` - Robot status
- `http://raspi:5001/motor` - Motor control

**AI Server**:
- `http://ai-server:5000/detect` - Object detection
- `http://ai-server:5000/health` - Health check

**ESP32**:
- `http://esp32:80/speak` - TTS output
- `http://esp32:80/motor` - Motor control
- `http://esp32:80/status` - Status

---

## Installation & Setup

### Prerequisites
- Python 3.8+
- Raspberry Pi OS (or Linux)
- pip / pip3
- Optional: MQTT broker, GPU (for AI server)

### Quick Install
```bash
# Clone and setup
cd gembot
chmod +x scripts/install.sh
./scripts/install.sh

# Configure
nano config/config.yaml

# Run
python src/main.py
```

### Detailed Setup
See `QUICKSTART.md` for step-by-step instructions.

---

## Testing & Debugging

### Debug Logging
```bash
# View real-time logs
tail -f logs/gembot.log

# Run with debug level
LOGLEVEL=DEBUG python src/main.py
```

### Test Individual Modules
```python
# Test camera
from src.vision import Camera
cam = Camera()
cam.initialize()
cam.start_capture()

# Test detection
from src.vision import DetectionClient
detector = DetectionClient()
print(detector.is_server_available())

# Test LiDAR
from src.lidar import LiDAR
lidar = LiDAR()
lidar.initialize()
lidar.start_scanning()
```

### Health Checks
```bash
# Check Raspberry Pi
curl http://localhost:5001/status

# Check AI Server
curl http://ai-server:5000/health

# Check ESP32
curl http://esp32/status

# Check MQTT
mosquitto_sub -h broker -t "gembot/#"
```

---

## Performance & Optimization

### Bottlenecks
1. **Detection**: 50-100ms per frame (AI server)
2. **Streaming**: Network bandwidth dependent
3. **MQTT**: Can be slow for large payloads
4. **LiDAR**: ~20Hz sample rate

### Optimization Tips
1. Reduce stream quality: `dashboard.stream_quality: 60`
2. Increase detection confidence: `ai_server.detection_confidence: 0.7`
3. Use GPU on AI server for 10x speedup
4. Enable object tracking to reduce detection frequency

### Resource Usage
- **Raspberry Pi**: 40-60% CPU, 400-600MB RAM
- **AI Server**: Variable (CPU: 50-100%, GPU: 80-100% if used)
- **Dashboard**: ~50MB RAM in browser
- **Network**: 2-5 Mbps for video stream

---

## Troubleshooting Guide

### Common Issues

**Camera not working**
```bash
# Test camera
libcamera-hello --t=2000

# Check device
ls /dev/video*
```

**Detection server unreachable**
```bash
# Check server is running
curl http://ai-server:5000/health

# Check firewall
sudo ufw allow 5000
```

**Motor not responding**
```bash
# Verify GPIO pins in config
# Test with direct GPIO control
gpio mode 12 output
gpio write 12 1
```

**MQTT not connecting**
```bash
# Check broker is running
mosquitto_sub -h broker -v

# Verify config IP
cat config/config.yaml | grep mqtt.broker
```

---

## Contributing & Extension

### Adding New Sensors
1. Create module in `src/` directory
2. Implement with same interface pattern
3. Add to `main.py` initialization
4. Update `config.yaml` with settings

### Custom Decision Logic
1. Override `Brain` class methods
2. Implement custom `_get_movement_command()`
3. Add state machine transitions

### New Communication Methods
1. Add to `src/comm/`
2. Implement publish/subscribe interface
3. Update protocol definitions

---

## File Statistics

```
Total Lines of Code:
├── Raspberry Pi (Python):     ~1,500
├── AI Server (Python):        ~360
├── ESP32 (Arduino):           ~380
├── Web Dashboard (HTML/CSS/JS): ~980
├── Configuration:             ~79
└── Documentation:             ~2,000+

Total Project: ~5,300+ lines (code + docs)
```

---

## Support & Resources

- **Documentation**: See README.md
- **Quick Start**: See QUICKSTART.md
- **Architecture**: See ARCHITECTURE.md
- **Logs**: Check `logs/gembot.log`
- **Config**: Edit `config/config.yaml`

---

**Project Version**: 1.0.0  
**Last Updated**: 2026-04-19  
**Status**: Production Ready  
**License**: MIT (or your chosen license)
