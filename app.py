from flask import Flask, render_template, request, jsonify, url_for, Response
from flask_cors import CORS
from flask_socketio import SocketIO, emit
from werkzeug.utils import secure_filename
from datetime import datetime
import google.generativeai as genai
import os
import uuid
import re
import socket
import threading
import queue
import time
import wave
import json
import cv2
import numpy as np
import requests
from dotenv import load_dotenv
from gtts import gTTS
import speech_recognition as sr
from pydub import AudioSegment
from pydub.effects import normalize
import paho.mqtt.client as mqtt

load_dotenv()

app = Flask(__name__)
CORS(app)
app.config['SECRET_KEY'] = os.getenv('SECRET_KEY', 'gembot-secret-key-2024')
socketio = SocketIO(app, cors_allowed_origins="*")

# ==================== KONFIGURASI ====================
GEMINI_API_KEY = os.getenv("GEMINI_API_KEY", "")
MQTT_BROKER = os.getenv('MQTT_BROKER', 'localhost')
MQTT_PORT = int(os.getenv('MQTT_PORT', 1883))
ESP32_AUDIO_HOST = os.getenv("ESP32_AUDIO_HOST", "192.168.1.100")
ESP32_AUDIO_PORT = 3333
ESP32_MQTT_CMD_TOPIC = "gembot/command"
ESP32_MQTT_STATUS_TOPIC = "gembot/status"
ESP32_MQTT_SENSOR_TOPIC = "gembot/sensor"

# ==================== DIREKTORI ====================
os.makedirs('static/audio', exist_ok=True)
os.makedirs('static/images', exist_ok=True)
os.makedirs('logs', exist_ok=True)

# ==================== GLOBAL VARIABLES ====================
mqtt_client = None
mqtt_connected = False
sensor_data = {
    "accel": [0, 0, 0],
    "gyro": [0, 0, 0],
    "lat": 0,
    "lng": 0,
    "speed": 0,
    "satellites": 0,
    "command": "STOP",
    "last_update": None
}
status_log = []
command_queue = queue.Queue()
camera = None
camera_lock = threading.Lock()
start_time = time.time()

# ==================== GEMINI AI INITIALIZATION ====================
def initialize_gemini():
    """Initialize Gemini AI with multiple model fallbacks."""
    global model, model_name
    try:
        if not GEMINI_API_KEY:
            print("[AI] GEMINI_API_KEY not set")
            return None, None
            
        genai.configure(api_key=GEMINI_API_KEY)
        
        # List available models
        available_models = []
        try:
            for m in genai.list_models():
                if 'gemini' in m.name:
                    available_models.append(m.name)
                    print(f"[AI] Available: {m.name}")
        except Exception as e:
            print(f"[AI] Could not list models: {e}")
        
        # Try models in order
        model_names = [
            'gemini-2.5-flash',
            'gemini-2.0-flash-exp', 
            'gemini-1.5-flash',
            'gemini-1.5-flash-8b',
            'models/gemini-2.5-flash',
            'models/gemini-2.0-flash-exp',
            'models/gemini-1.5-flash'
        ]
        
        for model_name_try in model_names:
            try:
                # Adjust model name format
                if not model_name_try.startswith('models/') and 'models/' not in model_name_try:
                    model_name_try = f"models/{model_name_try}"
                
                print(f"[AI] Trying model: {model_name_try}")
                test_model = genai.GenerativeModel(
                    model_name_try,
                    generation_config={
                        "temperature": 0.7,
                        "max_output_tokens": 100,
                    }
                )
                # Quick test
                test_response = test_model.generate_content("Hi")
                if test_response and test_response.text:
                    print(f"[AI] ✅ Success with model: {model_name_try}")
                    return test_model, model_name_try
            except Exception as e:
                print(f"[AI] Failed {model_name_try}: {str(e)[:50]}")
                continue
        
        print("[AI] ❌ No working Gemini model found")
        return None, None
        
    except Exception as e:
        print(f"[AI] Init error: {e}")
        return None, None

# Initialize Gemini AI
model, model_name = initialize_gemini()

def ask_gemini(prompt):
    """Ask Gemini AI with fallback"""
    global model
    
    # First try with direct REST API (more reliable)
    if GEMINI_API_KEY:
        try:
            url = f"https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash-exp:generateContent?key={GEMINI_API_KEY}"
            
            payload = {
                "contents": [{
                    "parts": [{
                        "text": f"Kamu adalah asisten robot pintar bernama Gembot. Jawab dengan singkat, jelas, dan ramah dalam bahasa Indonesia.\n\nPertanyaan: {prompt}\n\nJawaban:"
                    }]
                }],
                "generationConfig": {
                    "temperature": 0.7,
                    "maxOutputTokens": 300,
                }
            }
            
            response = requests.post(url, json=payload, timeout=10)
            if response.status_code == 200:
                data = response.json()
                if "candidates" in data and data["candidates"]:
                    answer = data["candidates"][0]["content"]["parts"][0]["text"]
                    return answer.strip()
        except Exception as e:
            print(f"[AI] REST API error: {e}")
    
    # Fallback to library
    if model:
        try:
            full_prompt = f"Kamu adalah asisten robot pintar bernama Gembot. Jawab dengan singkat, jelas, dan ramah dalam bahasa Indonesia.\n\nPertanyaan: {prompt}\n\nJawaban:"
            response = model.generate_content(full_prompt)
            if response and response.text:
                return response.text.strip()
        except Exception as e:
            print(f"[AI] Library error: {e}")
    
    # Final fallback
    return get_fallback_response(prompt)

def get_fallback_response(prompt):
    """Fallback responses"""
    prompt_lower = prompt.lower()
    
    # Command detection
    if any(word in prompt_lower for word in ['maju', 'jalan', 'forward', 'w']):
        send_mqtt_command("maju")
        return "Baik, saya akan maju."
    elif any(word in prompt_lower for word in ['mundur', 'backward', 's']):
        send_mqtt_command("mundur")
        return "Baik, saya akan mundur."
    elif any(word in prompt_lower for word in ['kiri', 'left', 'a']):
        send_mqtt_command("kiri")
        return "Baik, belok kiri."
    elif any(word in prompt_lower for word in ['kanan', 'right', 'd']):
        send_mqtt_command("kanan")
        return "Baik, belok kanan."
    elif any(word in prompt_lower for word in ['stop', 'berhenti', 'x']):
        send_mqtt_command("stop")
        return "Berhenti."
    elif any(word in prompt_lower for word in ['nama', 'siapa', 'kamu siapa']):
        return "Saya Gembot, asisten robot pintar Anda!"
    elif any(word in prompt_lower for word in ['kabar', 'gimana', 'bagaimana']):
        return "Saya baik-baik saja! Terima kasih sudah bertanya."
    elif any(word in prompt_lower for word in ['lokasi', 'posisi', 'dimana']):
        if sensor_data['lat'] != 0:
            return f"Saya berada di koordinat {sensor_data['lat']:.6f}, {sensor_data['lng']:.6f} dengan kecepatan {sensor_data['speed']:.1f} km/jam."
        return "Saya belum mendapat sinyal GPS."
    elif any(word in prompt_lower for word in ['terima kasih', 'thank']):
        return "Sama-sama! Senang bisa membantu."
    else:
        return f"Saya mendengar: '{prompt[:50]}'. Untuk kontrol robot, katakan: maju, mundur, kiri, kanan, atau stop."

# ==================== MQTT ====================
def on_mqtt_connect(client, userdata, flags, rc):
    global mqtt_connected
    if rc == 0:
        mqtt_connected = True
        print("✅ MQTT Connected")
        client.subscribe(ESP32_MQTT_STATUS_TOPIC)
        client.subscribe(ESP32_MQTT_SENSOR_TOPIC)
        log_status("MQTT Connected", "success")
    else:
        print(f"❌ MQTT Failed: {rc}")

def on_mqtt_disconnect(client, userdata, rc):
    global mqtt_connected
    mqtt_connected = False
    print("⚠️ MQTT Disconnected")

def on_mqtt_message(client, userdata, msg):
    global sensor_data
    try:
        payload = msg.payload.decode()
        if msg.topic == ESP32_MQTT_SENSOR_TOPIC:
            data = json.loads(payload)
            sensor_data.update(data)
            sensor_data["last_update"] = datetime.now().strftime("%H:%M:%S")
            socketio.emit('sensor_update', sensor_data)
        elif msg.topic == ESP32_MQTT_STATUS_TOPIC:
            socketio.emit('robot_status', {'command': payload})
        print(f"[MQTT] {msg.topic}: {payload[:50]}")
    except Exception as e:
        print(f"MQTT error: {e}")

def init_mqtt():
    global mqtt_client
    try:
        mqtt_client = mqtt.Client()
        mqtt_client.on_connect = on_mqtt_connect
        mqtt_client.on_disconnect = on_mqtt_disconnect
        mqtt_client.on_message = on_mqtt_message
        mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
        mqtt_client.loop_start()
        return True
    except Exception as e:
        print(f"MQTT init failed: {e}")
        return False

def send_mqtt_command(command):
    if mqtt_client and mqtt_connected:
        mqtt_client.publish(ESP32_MQTT_CMD_TOPIC, command)
        log_status(f"Cmd: {command}", "info")
        return True
    return False

# ==================== AUDIO ====================
def send_pcm_to_esp32(raw_pcm):
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect((ESP32_AUDIO_HOST, ESP32_AUDIO_PORT))
        sock.sendall(raw_pcm)
        sock.close()
        return True
    except Exception as e:
        print(f"Audio error: {e}")
        return False

def text_to_speech(text):
    try:
        clean_text = re.sub(r'\*\*(.*?)\*\*', r'\1', text)
        tts = gTTS(text=clean_text, lang='id', slow=False)
        temp_file = 'static/audio/temp.mp3'
        tts.save(temp_file)
        
        audio = AudioSegment.from_mp3(temp_file)
        audio = audio.set_frame_rate(16000).set_channels(1)
        audio = normalize(audio)
        
        success = send_pcm_to_esp32(audio.raw_data)
        os.remove(temp_file)
        return success
    except Exception as e:
        print(f"TTS error: {e}")
        return False

# ==================== CAMERA ====================
def init_camera():
    global camera
    with camera_lock:
        for i in range(3):
            cap = cv2.VideoCapture(i)
            if cap.isOpened():
                camera = cap
                camera.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
                camera.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
                print(f"✅ Camera ready (index {i})")
                return True
        print("⚠️ No camera found")
        return False

def generate_frames():
    while True:
        with camera_lock:
            if camera is None or not camera.isOpened():
                init_camera()
                frame = np.zeros((480, 640, 3), dtype=np.uint8)
                cv2.putText(frame, "Camera Not Available", (150, 240), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
                ret, buffer = cv2.imencode('.jpg', frame)
                yield (b'--frame\r\nContent-Type: image/jpeg\r\n\r\n' + buffer.tobytes() + b'\r\n')
                time.sleep(0.1)
                continue
            
            success, frame = camera.read()
            if not success:
                continue
            
            # Add HUD
            h, w = frame.shape[:2]
            cv2.rectangle(frame, (0, 0), (w, 60), (0, 0, 0), -1)
            cv2.putText(frame, f"CMD: {sensor_data['command']}", (10, 25), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)
            cv2.putText(frame, f"SPD: {sensor_data['speed']:.1f} km/h", (10, 50), 
                       cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
            
            ret, buffer = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, 70])
            yield (b'--frame\r\nContent-Type: image/jpeg\r\n\r\n' + buffer.tobytes() + b'\r\n')
        
        time.sleep(0.03)

# ==================== LOGGING ====================
def log_status(message, level="info"):
    timestamp = datetime.now().strftime("%H:%M:%S")
    log_entry = {"timestamp": timestamp, "message": message, "level": level}
    status_log.append(log_entry)
    if len(status_log) > 100:
        status_log.pop(0)
    socketio.emit('status_log', log_entry)
    print(f"[{timestamp}] {message}")

# ==================== ROUTES ====================
@app.route('/')
def index():
    return render_template('index.html')

@app.route('/api/status')
def api_status():
    return jsonify({
        "mqtt_connected": mqtt_connected,
        "sensor_data": sensor_data,
        "ai_ready": model is not None or bool(GEMINI_API_KEY),
        "uptime": time.time() - start_time
    })

@app.route('/api/command', methods=['POST'])
def api_command():
    data = request.json
    command = data.get('command', '').lower()
    if command in ['maju', 'mundur', 'kiri', 'kanan', 'stop']:
        return jsonify({"success": send_mqtt_command(command), "command": command})
    return jsonify({"success": False, "error": "Invalid command"})

@app.route('/api/chat', methods=['POST'])
def api_chat():
    data = request.json
    message = data.get('message', '').strip()
    if not message:
        return jsonify({"error": "Empty message"}), 400
    
    response = ask_gemini(message)
    text_to_speech(response)
    
    return jsonify({
        "success": True,
        "response": response,
        "timestamp": datetime.now().isoformat()
    })

@app.route('/api/audio/upload', methods=['POST'])
def upload_audio():
    try:
        audio_data = request.get_data()
        temp_wav = 'static/audio/temp.wav'
        
        with wave.open(temp_wav, 'wb') as wf:
            wf.setnchannels(1)
            wf.setsampwidth(2)
            wf.setframerate(16000)
            wf.writeframes(audio_data)
        
        recognizer = sr.Recognizer()
        with sr.AudioFile(temp_wav) as source:
            audio = recognizer.record(source)
        
        text = recognizer.recognize_google(audio, language='id-ID')
        print(f"[STT] {text}")
        
        response = ask_gemini(text)
        text_to_speech(response)
        
        os.remove(temp_wav)
        
        return jsonify({
            "success": True,
            "text": text,
            "response": response
        })
        
    except sr.UnknownValueError:
        return jsonify({"success": False, "error": "Tidak terdengar jelas"}), 400
    except Exception as e:
        return jsonify({"success": False, "error": str(e)}), 500

@app.route('/api/camera/stream')
def camera_stream():
    return Response(generate_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/api/sensors')
def api_sensors():
    return jsonify(sensor_data)

@app.route('/api/logs')
def api_logs():
    return jsonify(status_log)

@app.route('/api/health')
def health():
    return jsonify({
        "status": "ok",
        "mqtt": mqtt_connected,
        "ai": model is not None or bool(GEMINI_API_KEY),
        "camera": camera is not None and camera.isOpened()
    })

@app.route('/api/test/gemini', methods=['GET'])
def test_gemini():
    test_prompt = "Halo, apa kabar?"
    response = ask_gemini(test_prompt)
    return jsonify({
        "test_prompt": test_prompt,
        "response": response,
        "api_key_configured": bool(GEMINI_API_KEY),
        "model_initialized": model is not None
    })

# ==================== SOCKETIO ====================
@socketio.on('connect')
def handle_connect():
    log_status("Client connected", "info")
    emit('connected', {'message': 'Connected'})

@socketio.on('command')
def handle_command(data):
    send_mqtt_command(data.get('command', '').lower())

@socketio.on('chat_message')
def handle_chat_message(data):
    message = data.get('message', '')
    response = ask_gemini(message)
    text_to_speech(response)
    emit('chat_response', {'response': response})

# ==================== MAIN ====================
if __name__ == '__main__':
    print("\n" + "="*60)
    print("🤖 GEMBOT Server")
    print("="*60)
    
    # Initialize
    init_mqtt()
    init_camera()
    
    print(f"\n✅ Server Ready!")
    print(f"🌐 http://localhost:5000")
    print(f"📡 MQTT: {MQTT_BROKER}:{MQTT_PORT}")
    print(f"🔊 ESP32 Audio: {ESP32_AUDIO_HOST}:{ESP32_AUDIO_PORT}")
    print(f"🤖 Gemini: {'Active' if (model or GEMINI_API_KEY) else 'Fallback'}")
    print("="*60 + "\n")
    
    # Run without allow_unsafe_werkzeug
    socketio.run(app, host='0.0.0.0', port=5000, debug=True)
