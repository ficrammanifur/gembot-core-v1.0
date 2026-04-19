# GEMBOT Quick Start Guide

Get GEMBOT up and running in 5 minutes!

## Prerequisites

- Raspberry Pi 4/5 with Raspberry Pi OS
- AI Server (PC with GPU recommended for YOLOv8)
- ESP32 with USB connection
- WiFi network with internet access
- Python 3.8+

## Step 1: Initial Setup

### On Raspberry Pi

```bash
# Clone project
cd ~/
git clone <your-repo> gembot
cd gembot

# Run installation
chmod +x scripts/install.sh
./scripts/install.sh

# Configure your system
nano config/config.yaml
# Edit:
# - mqtt.broker: Your MQTT server IP
# - ai_server.host: PC with YOLOv8 IP
# - esp32.host: ESP32 IP
```

### On AI Server PC

```bash
# Install dependencies
cd server
pip install -r requirements.txt

# Download YOLOv8 model (first run only)
python -c "from ultralytics import YOLO; YOLO('yolov8n.pt')"
```

### On ESP32

1. Open Arduino IDE
2. Install ESP32 board support
3. Open `esp32/main.ino`
4. Update WiFi credentials:
   ```cpp
   const char* WIFI_SSID = "YOUR_SSID";
   const char* WIFI_PASSWORD = "YOUR_PASSWORD";
   ```
5. Upload to ESP32

## Step 2: Network Configuration

Update `config/config.yaml` with your network details:

```yaml
mqtt:
  broker: "192.168.1.100"      # MQTT broker address
  
ai_server:
  host: "192.168.1.100"        # AI Server PC IP
  port: 5000
  
esp32:
  host: "192.168.1.50"         # ESP32 IP
  port: 80
```

## Step 3: Start Services

### Terminal 1 - Raspberry Pi Main Program
```bash
cd ~/gembot
source venv/bin/activate
python src/main.py
```

### Terminal 2 - AI Server
```bash
cd ~/gembot/server
source venv/bin/activate
python app.py
```

### Terminal 3 - MQTT Broker (optional)
```bash
docker run -p 1883:1883 eclipse-mosquitto
```

## Step 4: Access Dashboard

1. Open web browser
2. Navigate to `http://localhost:5001`
3. Or run simple server: `cd dashboard && python -m http.server 8000`
4. Visit `http://localhost:8000`

## Troubleshooting

### Camera Not Detected
```bash
# Check camera
libcamera-hello --t=5000

# List USB devices
lsusb
```

### LiDAR Not Detected
```bash
# Check serial connection
ls /dev/ttyUSB*

# Update port in config.yaml if needed
```

### AI Server Connection Failed
```bash
# Test connection
curl http://192.168.1.100:5000/health

# Check firewall
sudo ufw allow 5000
```

### Motor Not Moving
```bash
# Check GPIO pins in config
# Test motor directly with GPIO
```

## Basic Commands

### From Dashboard
- **Move**: Use arrow buttons or keyboard
- **Speak**: Enter text in "Make Robot Speak" section
- **Speed**: Adjust with speed slider
- **Stream**: Click "Start Stream" for live video

### From Terminal
```python
# Import and test
from src.control import Brain
from src.vision import DetectionClient

# Test detection
detector = DetectionClient()
if detector.is_server_available():
    print("AI Server is ready!")
```

## Performance Optimization

### For Raspberry Pi
1. Reduce camera resolution: `camera.resolution: [480, 360]`
2. Reduce stream FPS: `dashboard.fps: 10`
3. Increase detection confidence: `ai_server.detection_confidence: 0.7`

### For Better Detection
1. Improve lighting in environment
2. Use GPU-accelerated inference (nvidia, apple)
3. Use YOLOv8 medium model: `yolov8m.pt` (slower but more accurate)

## Safety Considerations

1. Always keep emergency stop ready
2. Start in open area with obstacles
3. Keep robot away from edges
4. Test in safe environment first
5. Never disable safety checks in production

## Next Steps

1. Read full documentation in `README.md`
2. Explore configuration options in `config/config.yaml`
3. Check module-specific documentation in `src/*/README.md`
4. Join community for support

## Support

- Check logs: `tail -f logs/gembot.log`
- Enable debug mode in config
- Run individual tests: `python scripts/test_*.py`

Happy robotics!
