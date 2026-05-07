import os
import re
import socket
import cv2
import serial
import time
import threading
import numpy as np
import subprocess
from queue import Queue
from dotenv import load_dotenv
from flask import Flask, render_template, Response, jsonify, request
from gtts import gTTS
import google.generativeai as genai
from ultralytics import YOLO
import logging
import io
from pydub import AudioSegment
import tempfile
import speech_recognition as sr

load_dotenv()

logging.getLogger('ultralytics').setLevel(logging.WARNING)

app = Flask(__name__, static_folder="static", template_folder="templates")

# ================= KONFIGURASI =================
GEMINI_API_KEY = os.getenv("GEMINI_API_KEY", "")
SERIAL_PORT = os.getenv("SERIAL_PORT", "/dev/ttyUSB0")
BAUD_RATE = int(os.getenv("BAUDRATE", "115200"))

# Wake word dan end word
WAKE_WORD = "halo"
END_WORD = "selesai"

# Camera & YOLO
FRAME_WIDTH = 640
FRAME_HEIGHT = 480
CONF_THRESHOLD = 0.5
IOU_THRESHOLD = 0.45
MODEL_PATH = "yolov8n.pt"

# Global variables
ser = None
current_command = "STOP"
camera_active = True
fps = 0
model_ai = None
conversation_active = False
voice_listening = False

# Queue
frame_queue = Queue(maxsize=1)
result_queue = Queue(maxsize=1)

# ================= AUDIO OUTPUT =================
def play_audio_simple(audio_bytes: bytes, sample_rate=16000):
    """Play audio using aplay command (Linux)"""
    try:
        with tempfile.NamedTemporaryFile(suffix='.raw', delete=False) as f:
            f.write(audio_bytes)
            temp_file = f.name
        
        cmd = f'aplay -f S16_LE -r {sample_rate} -c 1 "{temp_file}" 2>/dev/null'
        subprocess.run(cmd, shell=True)
        
        os.unlink(temp_file)
        return True
    except Exception as e:
        print(f"[AUDIO] Error: {e}")
        return False

def text_to_speech(text: str):
    try:
        if not text:
            return False
        
        clean_text = re.sub(r'\*\*(.*?)\*\*', r'\1', text)
        clean_text = re.sub(r'\*(.*?)\*', r'\1', clean_text)
        
        print(f"[TTS] Speaking: {clean_text[:50]}...")
        
        tts = gTTS(clean_text, lang="id", slow=False)
        mp3_fp = io.BytesIO()
        tts.write_to_fp(mp3_fp)
        mp3_fp.seek(0)
        
        audio = AudioSegment.from_mp3(mp3_fp)
        audio = audio.set_frame_rate(16000).set_channels(1).set_sample_width(2)
        
        return play_audio_simple(audio.raw_data)
        
    except Exception as e:
        print(f"[TTS] Error: {e}")
        return False

# ================= VOICE INPUT =================
def listen_for_voice_command():
    """Listen for voice command from microphone"""
    global conversation_active
    
    recognizer = sr.Recognizer()
    
    # Gunakan microphone USB Webcam
    try:
        mic = sr.Microphone(device_index=0)  # Device 0 (USB Webcam)
    except:
        mic = sr.Microphone()
    
    with mic as source:
        print("[VOICE] Adjusting for ambient noise...")
        recognizer.adjust_for_ambient_noise(source, duration=0.5)
        print("[VOICE] Listening...")
        
        try:
            audio = recognizer.listen(source, timeout=5, phrase_time_limit=5)
            text = recognizer.recognize_google(audio, language='id-ID')
            print(f"[VOICE] Heard: '{text}'")
            return text.lower()
        except sr.WaitTimeoutError:
            return None
        except sr.UnknownValueError:
            print("[VOICE] Could not understand")
            return None
        except Exception as e:
            print(f"[VOICE] Error: {e}")
            return None

def voice_loop():
    """Background thread for voice recognition"""
    global conversation_active
    
    while True:
        if conversation_active:
            command = listen_for_voice_command()
            if command:
                process_voice_command(command)
        time.sleep(0.5)

def process_voice_command(command: str):
    """Process voice command"""
    global conversation_active
    
    if not command:
        return
    
    # Check for end word
    if END_WORD in command:
        conversation_active = False
        response = "Selesai. Sampai jumpa!"
        text_to_speech(response)
        return
    
    # Motor commands
    if 'maju' in command:
        send_motor_command('FORWARD')
        threading.Timer(0.5, lambda: send_motor_command('STOP')).start()
        response = "Maju sebentar"
        text_to_speech(response)
    elif 'mundur' in command:
        send_motor_command('BACKWARD')
        threading.Timer(0.5, lambda: send_motor_command('STOP')).start()
        response = "Mundur"
        text_to_speech(response)
    elif 'kiri' in command:
        send_motor_command('LEFT')
        threading.Timer(0.3, lambda: send_motor_command('STOP')).start()
        response = "Belok kiri"
        text_to_speech(response)
    elif 'kanan' in command:
        send_motor_command('RIGHT')
        threading.Timer(0.3, lambda: send_motor_command('STOP')).start()
        response = "Belok kanan"
        text_to_speech(response)
    elif 'stop' in command or 'berhenti' in command:
        send_motor_command('STOP')
        response = "Berhenti"
        text_to_speech(response)
    else:
        # Tanya AI
        response = call_gemini(command)
        text_to_speech(response)

# ================= GEMINI AI =================
def initialize_gemini():
    global model_ai
    if not GEMINI_API_KEY:
        print("[AI] ⚠️ GEMINI_API_KEY not set")
        return False
    
    try:
        genai.configure(api_key=GEMINI_API_KEY)
        model_names = ['gemini-flash-latest', 'gemini-1.5-flash-latest']
        
        for model_name in model_names:
            try:
                model_ai = genai.GenerativeModel(model_name)
                test_response = model_ai.generate_content("Hi", generation_config={"max_output_tokens": 5})
                print(f"[AI] ✅ Gemini ready with model: {model_name}")
                return True
            except:
                continue
        
        print("[AI] ⚠️ Using fallback responses")
        return False
    except Exception as e:
        print(f"[AI] ❌ Error: {e}")
        return False

def call_gemini(user_text: str) -> str:
    global model_ai
    
    cmd_responses = {
        'maju': 'Siap, robot maju',
        'mundur': 'Siap, robot mundur',
        'kiri': 'Belok kiri',
        'kanan': 'Belok kanan',
        'stop': 'Robot berhenti',
    }
    
    text_lower = user_text.lower()
    for key, response in cmd_responses.items():
        if key in text_lower:
            return response
    
    if model_ai:
        try:
            prompt = f"""Kamu adalah Gembot, asisten robot yang ramah.
Pengguna: "{user_text}"
Jawab singkat dalam bahasa Indonesia (maks 2 kalimat)."""
            
            response = model_ai.generate_content(prompt)
            return response.text.strip()
        except Exception as e:
            print(f"[AI] Error: {e}")
    
    return "Saya Gembot. Ada yang bisa saya bantu?"

# ================= SERIAL MOTOR =================
def init_serial():
    global ser
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        time.sleep(2)
        print(f"✅ Serial connected to {SERIAL_PORT}")
        return True
    except Exception as e:
        print(f"⚠️ Serial error: {e}")
        return False

def send_motor_command(cmd):
    global ser, current_command
    if cmd == current_command:
        return True
    if ser:
        try:
            ser.write((cmd + '\n').encode('utf-8'))
            ser.flush()
            current_command = cmd
            print(f"→ Command sent: {cmd}")
            return True
        except:
            return False
    return False

# ================= YOLO =================
def capture_camera():
    global camera_active, fps
    cap = cv2.VideoCapture(0)
    
    if not cap.isOpened():
        print("[ERROR] Cannot open camera!")
        camera_active = False
        return
    
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, FRAME_WIDTH)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT)
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
    
    print(f"[CAM] Started at {FRAME_WIDTH}x{FRAME_HEIGHT}")
    
    frame_count = 0
    last_time = time.time()
    
    while camera_active:
        ret, frame = cap.read()
        if not ret:
            continue
        
        frame_count += 1
        if time.time() - last_time >= 1.0:
            fps = frame_count
            frame_count = 0
            last_time = time.time()
        
        if frame_queue.full():
            try:
                frame_queue.get_nowait()
            except:
                pass
        frame_queue.put(frame)
    
    cap.release()

def yolo_detection():
    global camera_active
    
    print("[YOLO] Loading model...")
    model = YOLO(MODEL_PATH)
    print("[YOLO] Ready!")
    
    while camera_active:
        try:
            frame = frame_queue.get(timeout=0.5)
        except:
            continue
        
        frame_resized = cv2.resize(frame, (416, 416))
        results = model(frame_resized, conf=CONF_THRESHOLD, iou=IOU_THRESHOLD, verbose=False)
        
        annotated_frame = cv2.resize(frame_resized, (FRAME_WIDTH, FRAME_HEIGHT))
        detection_count = 0
        
        for r in results:
            for box in r.boxes:
                conf = float(box.conf[0])
                cls = int(box.cls[0])
                label = model.names[cls]
                
                x1, y1, x2, y2 = map(int, box.xyxy[0])
                scale_x = FRAME_WIDTH / 416
                scale_y = FRAME_HEIGHT / 416
                x1, y1, x2, y2 = int(x1*scale_x), int(y1*scale_y), int(x2*scale_x), int(y2*scale_y)
                detection_count += 1
                
                if label == 'person':
                    color = (0, 255, 0)
                elif label in ['car', 'truck']:
                    color = (0, 165, 255)
                else:
                    color = (255, 0, 0)
                
                cv2.rectangle(annotated_frame, (x1, y1), (x2, y2), color, 2)
                cv2.putText(annotated_frame, f"{label}", (x1, y1-5), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 2)
        
        overlay = annotated_frame.copy()
        cv2.rectangle(overlay, (0, 0), (300, 80), (0, 0, 0), -1)
        annotated_frame = cv2.addWeighted(overlay, 0.5, annotated_frame, 0.5, 0)
        cv2.putText(annotated_frame, f"FPS: {fps}", (5, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
        cv2.putText(annotated_frame, f"Objects: {detection_count}", (5, 50), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
        
        status_text = "AI: ACTIVE" if conversation_active else "AI: SLEEP"
        status_color = (0, 255, 0) if conversation_active else (255, 165, 0)
        cv2.putText(annotated_frame, status_text, (5, 75), cv2.FONT_HERSHEY_SIMPLEX, 0.5, status_color, 2)
        
        if result_queue.full():
            try:
                result_queue.get_nowait()
            except:
                pass
        result_queue.put(annotated_frame)

# ================= FLASK ROUTES =================
@app.route('/')
def index():
    return render_template('gembot_dashboard.html')

@app.route('/video_feed')
def video_feed():
    def generate():
        while camera_active:
            try:
                frame = result_queue.get(timeout=0.1)
                ret, buffer = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, 80])
                if ret:
                    yield (b'--frame\r\n'
                           b'Content-Type: image/jpeg\r\n\r\n' + buffer.tobytes() + b'\r\n')
            except:
                time.sleep(0.05)
    return Response(generate(), mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/api/control', methods=['POST'])
def control():
    data = request.get_json()
    command = data.get('command', '').upper()
    is_touch_start = data.get('touch_start', True)
    
    valid = ['FORWARD', 'BACKWARD', 'LEFT', 'RIGHT', 'STOP']
    
    if command in valid:
        if is_touch_start:
            success = send_motor_command(command)
            return jsonify({'status': 'success' if success else 'error'})
        else:
            send_motor_command("STOP")
            return jsonify({'status': 'success'})
    return jsonify({'status': 'error'}), 400

@app.route('/api/command', methods=['POST'])
def command_endpoint():
    data = request.get_json()
    cmd = data.get('command', '').upper()
    
    cmd_map = {'MAJU': 'FORWARD', 'MUNDUR': 'BACKWARD', 'KIRI': 'LEFT', 'KANAN': 'RIGHT', 'STOP': 'STOP'}
    command = cmd_map.get(cmd, 'STOP')
    send_motor_command(command)
    
    if command != 'STOP':
        threading.Timer(0.5, lambda: send_motor_command('STOP')).start()
    
    return jsonify({'status': 'success'})

@app.route('/api/emergency_stop', methods=['POST'])
def emergency_stop():
    send_motor_command("STOP")
    return jsonify({'status': 'success'})

@app.route('/api/status', methods=['GET'])
def status():
    return jsonify({
        'motor_connected': ser is not None and ser.is_open,
        'current_command': current_command,
        'camera_active': camera_active,
        'fps': fps,
        'ai_ready': model_ai is not None,
        'conversation_active': conversation_active
    })

@app.route('/api/chat', methods=['POST'])
def api_chat():
    global conversation_active
    
    data = request.get_json() or {}
    message = data.get('message', '').strip().lower()
    
    if not message:
        return jsonify({'response': 'Pesan kosong'}), 400
    
    print(f"[CHAT] User: {message}")
    
    # WAKE WORD
    if WAKE_WORD in message and not conversation_active:
        conversation_active = True
        response = "Halo! Ada yang bisa saya bantu? Silakan bicara atau ketik perintah."
        text_to_speech(response)
        print("[CHAT] AI activated")
        return jsonify({'response': response})
    
    # END WORD
    if END_WORD in message and conversation_active:
        conversation_active = False
        response = "Selesai. Sampai jumpa!"
        text_to_speech(response)
        print("[CHAT] AI deactivated")
        return jsonify({'response': response})
    
    # Process commands if AI active
    if conversation_active:
        if 'maju' in message:
            send_motor_command('FORWARD')
            threading.Timer(0.5, lambda: send_motor_command('STOP')).start()
            response = "Maju sebentar!"
        elif 'mundur' in message:
            send_motor_command('BACKWARD')
            threading.Timer(0.5, lambda: send_motor_command('STOP')).start()
            response = "Mundur!"
        elif 'kiri' in message:
            send_motor_command('LEFT')
            threading.Timer(0.3, lambda: send_motor_command('STOP')).start()
            response = "Belok kiri!"
        elif 'kanan' in message:
            send_motor_command('RIGHT')
            threading.Timer(0.3, lambda: send_motor_command('STOP')).start()
            response = "Belok kanan!"
        elif 'stop' in message or 'berhenti' in message:
            send_motor_command('STOP')
            response = "Berhenti!"
        else:
            response = call_gemini(message)
            text_to_speech(response)
        
        return jsonify({'response': response})
    else:
        return jsonify({'response': f'Ketik "{WAKE_WORD}" untuk memulai percakapan.'})

def get_tailscale_ip():
    try:
        result = subprocess.run(['tailscale', 'ip'], capture_output=True, text=True)
        if result.returncode == 0:
            ips = result.stdout.strip().split()
            return ips[0] if ips else None
    except:
        pass
    return None

# ================= MAIN =================
if __name__ == '__main__':
    init_serial()
    initialize_gemini()
    
    # Start threads
    threading.Thread(target=capture_camera, daemon=True).start()
    threading.Thread(target=yolo_detection, daemon=True).start()
    threading.Thread(target=voice_loop, daemon=True).start()
    
    local_ip = socket.gethostbyname(socket.gethostname())
    tailscale_ip = get_tailscale_ip()
    
    print("\n" + "="*60)
    print("🤖 GEMBOT - Voice + Text Control")
    print("="*60)
    print(f"📍 Local:     http://{local_ip}:5000")
    if tailscale_ip:
        print(f"📍 Tailscale:  http://{tailscale_ip}:5000")
    print("="*60)
    print("\n🎤 VOICE COMMANDS (bicara ke mic webcam):")
    print("   1. Ucapkan 'Halo' untuk memulai")
    print("   2. 'Maju', 'Mundur', 'Kiri', 'Kanan', 'Stop'")
    print("   3. Tanya apa saja ke AI")
    print("   4. 'Selesai' untuk mengakhiri")
    print("\n💬 Atau ketik di chat box")
    print("🔊 Suara keluar dari speaker\n")
    print("[READY] Menunggu perintah...\n")
    
    app.run(host='0.0.0.0', port=5000, debug=False, threaded=True)
