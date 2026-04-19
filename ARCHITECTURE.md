# GEMBOT System Architecture

Comprehensive technical documentation of GEMBOT's distributed architecture.

## System Overview

GEMBOT is a distributed AI robot system with three main components:

```
┌─────────────────────────────────────────────────────────┐
│                   Web Dashboard (Browser)               │
│                   Live Stream + Controls                │
└────────────────────┬────────────────────────────────────┘
                     │ WebSocket/HTTP
        ┌────────────┴──────────────────┐
        │                               │
┌───────▼──────────────────┐   ┌────────▼────────────────┐
│   Raspberry Pi            │   │   AI Server (PC)       │
│  - Camera (OpenCV)        │   │   - YOLOv8 Detection   │
│  - LiDAR Navigation       │   │   - FastAPI Endpoint   │
│  - STT Processing         │   │   - GPU Inference      │
│  - MQTT Client            │   └────────────────────────┘
│  - HTTP Server            │
└────────┬─────────────────┘
         │ MQTT + HTTP
    ┌────▼──────────────┐
    │   ESP32           │
    │   - TTS (Audio)   │
    │   - Motor Control │
    │   - I2S to Speaker│
    └───────────────────┘
```

## Component Details

### 1. Raspberry Pi (Distributed Control Unit)

**Purpose**: Main orchestrator of robot functions

**Key Modules**:
```
src/
├── main.py                    # Entry point & orchestration
├── vision/                    # Camera & detection
│   ├── camera.py             # OpenCV capture
│   ├── stream.py             # MJPEG streaming
│   └── detect_client.py       # HTTP client to AI server
├── lidar/                     # Navigation
│   ├── lidar.py              # Sensor interface
│   └── obstacle.py           # Avoidance logic
├── audio/                     # Voice I/O
│   ├── stt.py                # Speech recognition
│   └── tts.py                # Text-to-speech via HTTP
├── comm/                      # Communication
│   ├── mqtt_client.py        # Pub/sub messaging
│   ├── http_client.py        # Device communication
│   └── protocol.py           # Message formats
├── control/                   # Decision making
│   └── brain.py              # State & behavior
└── utils/                     # Utilities
    ├── logger.py             # Logging
    └── config_loader.py      # Config management
```

**Execution Flow**:
```
main.py
  ├── Initialize all subsystems
  ├── Start hardware capture threads
  │   ├── camera.start_capture()
  │   └── lidar.start_scanning()
  ├── Main loop (1000 Hz)
  │   ├── Get frame from camera
  │   ├── Send for detection (async)
  │   ├── Process sensor data via brain
  │   ├── Publish status via MQTT
  │   └── Handle user commands
  └── Graceful shutdown on signal
```

**Threading Model**:
- Main thread: Orchestration & MQTT
- Camera thread: Continuous frame capture
- LiDAR thread: Continuous scanning
- Stream thread: MJPEG generation
- Detection thread: HTTP requests (async)

### 2. AI Server (Detection Engine)

**Purpose**: Centralized object detection using YOLOv8

**API Endpoints**:

```python
POST /detect
├── Input: Image file (JPEG/PNG)
├── Query: ?confidence=0.5
└── Output: [
    {
        'box': (x1, y1, x2, y2),
        'label': 'person',
        'confidence': 0.95,
        'class_id': 0
    },
    ...
]

POST /detect_and_visualize
└── Output: Same as /detect but image has bounding boxes

GET /health
└── Status check endpoint

GET /info
└── Model information
```

**Implementation**:
```python
detect.py
├── ObjectDetector class
│   ├── load_model()         # Load YOLOv8
│   ├── detect()             # Run inference
│   ├── visualize()          # Draw boxes
│   └── get_model_info()

app.py (FastAPI)
├── startup_event()          # Initialize detector
├── /detect handler          # Main detection endpoint
├── /detect_and_visualize    # With visualization
└── CORS middleware          # Cross-origin support
```

**Performance Characteristics**:
- Model: YOLOv8 Nano (2.6MB, ~80 FPS on CPU)
- Latency: 50-100ms per frame
- Memory: ~500MB RAM
- GPU optimized (CUDA/MPS support)

### 3. ESP32 (Peripheral Controller)

**Purpose**: Audio output & motor control

**Features**:
- I2S audio output via MAX98357A
- Dual DC motor PWM control
- WiFi connectivity
- Simple HTTP server
- Real-time responsiveness

**Endpoints**:

```cpp
POST /speak
├── Input: {"text": "Hello"}
└── Action: Convert to speech & play

POST /motor
├── Input: {"left_speed": 150, "right_speed": 150}
├── Range: -255 to 255
└── Action: Control motor speeds

GET /status
└── Output: {"uptime": 12345, "is_speaking": false}
```

**Hardware Configuration**:
```
GPIO Pins:
├── Motor Control
│   ├── Left: GPIO 12,14 (direction), GPIO 5 (PWM)
│   └── Right: GPIO 19,21 (direction), GPIO 18 (PWM)
└── I2S Audio
    ├── BCLK: GPIO 26
    ├── LRC: GPIO 25
    └── DIN: GPIO 27
```

## Communication Protocols

### MQTT Topics

```
gembot/
├── status/                    # Robot status updates
├── motor/control             # Motor commands
├── camera/status             # Camera info
├── lidar/status              # LiDAR data
├── esp32/speak               # TTS requests
├── alert/obstacle            # Obstacle alerts
└── emergency/stop            # Emergency stop
```

**Message Format** (JSON):
```json
{
    "type": "status",           // message type
    "state": "moving",          // robot state
    "battery": 85.5,            // battery %
    "uptime": 3600,             // seconds
    "timestamp": 1704067200000  // milliseconds
}
```

### HTTP Communication

**Raspberry Pi → AI Server**:
- Endpoint: `POST /detect`
- Payload: Image file (multipart/form-data)
- Response: JSON with detections
- Async non-blocking calls

**Raspberry Pi → ESP32**:
- Motor: `POST /motor` with JSON payload
- Audio: `POST /speak` with text
- Status: `GET /status` for heartbeat

## Data Flow Examples

### Example 1: Object Detection Pipeline

```
1. Camera captures frame (30 FPS)
   └─> camera.py: capture_loop()

2. Frame sent to AI server (async, non-blocking)
   └─> detect_client.py: send_frame_async()

3. AI Server runs YOLOv8 inference
   └─> server/detect.py: detect()

4. Detections returned to Raspberry Pi
   └─> detect_client.py: _send_frame_sync()

5. Detections stored in latest_detections
   └─> stream_processor.py: draw_bounding_boxes()

6. Stream includes detection overlays
   └─> dashboard: receives updated MJPEG

7. Brain processes for decision making
   └─> control/brain.py: process_sensor_data()
```

### Example 2: Motor Control Flow

```
1. User clicks movement button in dashboard
   └─> script.js: moveRobot('forward')

2. HTTP request to Raspberry Pi
   └─> /motor endpoint (Flask/HTTP server)

3. Brain receives motor request
   └─> http_client.py: send_motor_command()

4. Command sent to ESP32
   └─> ESP32: POST /motor

5. ESP32 sets PWM values
   └─> GPIO PWM output to motor driver

6. Motor physically moves
   └─> Dual DC motor rotation
```

### Example 3: Voice Interaction Flow

```
1. User speaks into microphone
   └─> audio/stt.py: listen_once()

2. Speech converted to text
   └─> SpeechRecognition library

3. Text sent to AI (Gemini/OpenAI)
   └─> AI generates response

4. Response text sent to ESP32
   └─> http_client.py: send_tts_request()

5. ESP32 generates audio
   └─> Audio output via I2S to MAX98357A

6. Speaker plays response
   └─> Physical speaker output
```

## Configuration System

**Hierarchy**:
```
config.yaml (default)
    ├─ mqtt.broker
    ├─ ai_server.host
    ├─ esp32.host
    ├─ camera.*
    ├─ lidar.*
    ├─ motor.*
    └─ safety.*
```

**Usage**:
```python
from src.utils import get_config

config = get_config()

# Dot notation access
broker = config.get('mqtt.broker')
ai_host = config.get('ai_server.host')

# Get entire section
mqtt_config = config.get_dict('mqtt')

# Reload configuration
config.reload()
```

## Error Handling & Resilience

### Graceful Degradation

1. **AI Server Offline**
   - Detection disabled
   - Robot operates without vision
   - Warnings logged

2. **MQTT Unavailable**
   - System continues operating
   - Status not published
   - Normal operation resumed when available

3. **ESP32 Disconnected**
   - No motor/audio output
   - Core robot functions unaffected
   - Reconnection attempted

### Safety Mechanisms

```python
# Obstacle detection
if closest_distance < STOP_DISTANCE:
    stop_motors()  # Emergency stop

# Path clearing
if not is_path_clear():
    avoid_obstacle()  # Automatic avoidance

# Timeout handling
if mqtt_timeout > MAX_TIMEOUT:
    logger.warning("MQTT timeout")
    # Continue with degraded functionality
```

## Performance Characteristics

### Raspberry Pi
- CPU: ARM (BCM2712 on Pi 5)
- RAM: 4-8GB recommended
- Storage: microSD 32GB+ recommended
- Network: Gigabit Ethernet recommended
- FPS: 20-30 FPS detection processing

### AI Server (PC)
- CPU: i7 or equivalent
- GPU: NVIDIA (10GB+ VRAM) recommended
- RAM: 16GB+ recommended
- Inference: 50-100ms per 640x480 frame
- Throughput: 10-20 FPS with YOLOv8 Nano

### Network
- MQTT: Low latency required (<50ms)
- HTTP: Can tolerate higher latency
- Bandwidth: ~2-5 Mbps for video stream

## Security Considerations

### Current Implementation
- No authentication on HTTP endpoints
- No encryption on MQTT
- Open network access

### For Production
1. Add API authentication (JWT tokens)
2. Enable MQTT username/password
3. Use HTTPS/TLS encryption
4. Implement rate limiting
5. Add input validation
6. Network isolation (VPN/firewall)

## Extension Points

### Adding New Sensors
```python
# In src/lidar/ or similar
class NewSensor:
    def initialize(self):
        pass
    
    def read_data(self):
        return data
    
    def get_stats(self):
        return stats

# In main.py
new_sensor = NewSensor()
new_sensor.initialize()
# Use in brain processing
```

### Custom Decision Logic
```python
# Override brain.py methods
class CustomBrain(Brain):
    def _get_movement_command(self):
        # Custom logic here
        return movement_command
```

### New Communication Methods
```python
# Add to comm/
class CustomComm:
    def publish(self, topic, data):
        # Custom implementation
        pass
```

## Deployment Checklist

- [ ] All IP addresses configured correctly
- [ ] Firewall rules allowing traffic
- [ ] MQTT broker running and accessible
- [ ] AI Server running with YOLOv8 loaded
- [ ] ESP32 WiFi connected
- [ ] Raspberry Pi camera detected
- [ ] LiDAR serial port configured
- [ ] Motor pins tested with GPIO test
- [ ] Dashboard accessible in browser
- [ ] Safety limits verified
- [ ] Emergency stop tested
- [ ] Logs monitoring configured

---

**Last Updated**: 2026-04-19
**Version**: 1.0.0
