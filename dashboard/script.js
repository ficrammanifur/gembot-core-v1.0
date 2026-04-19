/*
GEMBOT Control Dashboard JavaScript
Handles all UI interactions and communication
*/

// Configuration
const CONFIG = {
    raspiHost: 'localhost:5001',
    aiServerHost: 'localhost:5000',
    esp32Host: 'localhost:80',
    streamUpdateInterval: 100,
    statusUpdateInterval: 1000,
    detectionRefreshInterval: 500
};

// State
const state = {
    isConnected: false,
    isStreaming: false,
    isSpeaking: false,
    speed: 150,
    lastFrameTime: 0,
    frameCount: 0,
    currentDetections: []
};

// ==================== Initialization ====================

document.addEventListener('DOMContentLoaded', () => {
    console.log('GEMBOT Dashboard initialized');
    
    // Setup event listeners
    setupEventListeners();
    
    // Check connection
    checkConnection();
    
    // Start status updates
    setInterval(updateStatus, CONFIG.statusUpdateInterval);
    
    // Start stream if available
    setTimeout(() => {
        if (state.isConnected) {
            startStream();
        }
    }, 1000);
});

// ==================== Event Listeners ====================

function setupEventListeners() {
    // Stream control
    document.getElementById('streamToggle').addEventListener('click', toggleStream);
    
    // Movement buttons
    document.querySelectorAll('[data-direction]').forEach(btn => {
        btn.addEventListener('mousedown', () => {
            const direction = btn.dataset.direction;
            moveRobot(direction);
        });
        btn.addEventListener('mouseup', () => stopRobot());
    });
    
    document.getElementById('moveStop').addEventListener('click', stopRobot);
    
    // Speed control
    document.getElementById('speedSlider').addEventListener('input', (e) => {
        state.speed = parseInt(e.target.value);
        document.getElementById('speedValue').textContent = 
            Math.round((state.speed / 255) * 100);
    });
    
    // Voice control
    document.getElementById('speakButton').addEventListener('mousedown', startListening);
    document.getElementById('speakButton').addEventListener('mouseup', stopListening);
    
    // Text to speech
    document.getElementById('speakSubmit').addEventListener('click', () => {
        const text = document.getElementById('speakInput').value;
        if (text.trim()) {
            makeRobotSpeak(text);
            document.getElementById('speakInput').value = '';
        }
    });
}

// ==================== Connection Management ====================

async function checkConnection() {
    try {
        // Try to connect to Raspberry Pi
        const response = await fetch(`http://${CONFIG.raspiHost}/health`, {
            timeout: 5000
        }).catch(() => null);
        
        if (response && response.ok) {
            setConnected(true);
            console.log('✓ Connected to Raspberry Pi');
        } else {
            setConnected(false);
            console.log('✗ Raspberry Pi not available');
        }
    } catch (error) {
        setConnected(false);
        console.log('Connection check failed:', error.message);
    }
    
    // Check other services
    checkServiceStatus();
}

function setConnected(connected) {
    state.isConnected = connected;
    const dot = document.getElementById('connectionStatus');
    const text = document.getElementById('statusText');
    
    if (connected) {
        dot.classList.add('connected');
        text.textContent = 'Connected';
        document.getElementById('raspiStatus').textContent = 'Online';
    } else {
        dot.classList.remove('connected');
        text.textContent = 'Disconnected';
        document.getElementById('raspiStatus').textContent = 'Offline';
    }
}

async function checkServiceStatus() {
    // Check AI Server
    try {
        const aiResponse = await fetch(
            `http://${CONFIG.aiServerHost}/health`,
            { timeout: 3000 }
        ).catch(() => null);
        
        document.getElementById('aiStatus').textContent = 
            aiResponse && aiResponse.ok ? 'Online' : 'Offline';
    } catch (error) {
        document.getElementById('aiStatus').textContent = 'Offline';
    }
    
    // Check ESP32
    try {
        const esp32Response = await fetch(
            `http://${CONFIG.esp32Host}/status`,
            { timeout: 3000 }
        ).catch(() => null);
        
        document.getElementById('esp32Status').textContent = 
            esp32Response && esp32Response.ok ? 'Online' : 'Offline';
    } catch (error) {
        document.getElementById('esp32Status').textContent = 'Offline';
    }
}

// ==================== Video Streaming ====================

function toggleStream() {
    if (state.isStreaming) {
        stopStream();
    } else {
        startStream();
    }
}

function startStream() {
    if (state.isStreaming || !state.isConnected) return;
    
    state.isStreaming = true;
    document.getElementById('streamToggle').textContent = 'Stop Stream';
    
    console.log('Starting video stream...');
    updateStream();
}

function stopStream() {
    state.isStreaming = false;
    document.getElementById('streamToggle').textContent = 'Start Stream';
    console.log('Stopping video stream');
}

async function updateStream() {
    if (!state.isStreaming) return;
    
    try {
        const timestamp = Date.now();
        const imageUrl = `http://${CONFIG.raspiHost}/stream?t=${timestamp}`;
        
        const response = await fetch(imageUrl, { timeout: 5000 }).catch(() => null);
        
        if (response && response.ok) {
            const blob = await response.blob();
            const url = URL.createObjectURL(blob);
            
            const img = document.getElementById('streamImage');
            img.src = url;
            
            // Update FPS
            const currentTime = performance.now();
            if (state.lastFrameTime > 0) {
                const fps = 1000 / (currentTime - state.lastFrameTime);
                document.getElementById('fps').textContent = fps.toFixed(1) + ' FPS';
            }
            state.lastFrameTime = currentTime;
            state.frameCount++;
        }
    } catch (error) {
        console.error('Stream error:', error.message);
    }
    
    setTimeout(updateStream, CONFIG.streamUpdateInterval);
}

// ==================== Robot Control ====================

async function moveRobot(direction) {
    if (!state.isConnected) return;
    
    const speedMap = {
        'forward': { left: state.speed, right: state.speed },
        'backward': { left: -state.speed, right: -state.speed },
        'left': { left: -state.speed / 2, right: state.speed },
        'right': { left: state.speed, right: -state.speed / 2 }
    };
    
    const command = speedMap[direction];
    if (!command) return;
    
    try {
        const response = await fetch(
            `http://${CONFIG.raspiHost}/motor`,
            {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(command),
                timeout: 3000
            }
        ).catch(() => null);
        
        if (response && response.ok) {
            console.log(`Moving ${direction}`);
        }
    } catch (error) {
        console.error('Motor command error:', error.message);
    }
}

async function stopRobot() {
    if (!state.isConnected) return;
    
    try {
        await fetch(
            `http://${CONFIG.raspiHost}/motor`,
            {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ left: 0, right: 0 }),
                timeout: 3000
            }
        ).catch(() => null);
    } catch (error) {
        console.error('Stop error:', error.message);
    }
}

// ==================== Voice Control ====================

function startListening() {
    if (!state.isConnected || state.isSpeaking) return;
    
    document.getElementById('speakButton').textContent = '🎤 Listening...';
    document.getElementById('transcriptionText').textContent = 'Listening...';
    
    console.log('Started listening for speech');
}

function stopListening() {
    document.getElementById('speakButton').textContent = '🎤 Hold to Speak';
    console.log('Stopped listening');
}

async function makeRobotSpeak(text) {
    if (!state.isConnected) return;
    
    try {
        const response = await fetch(
            `http://${CONFIG.esp32Host}/speak`,
            {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ text: text }),
                timeout: 5000
            }
        ).catch(() => null);
        
        if (response && response.ok) {
            console.log('Robot speaking:', text);
            showNotification('Robot speaking: ' + text);
        }
    } catch (error) {
        console.error('TTS error:', error.message);
        showNotification('Failed to speak', 'error');
    }
}

// ==================== Status Updates ====================

async function updateStatus() {
    if (!state.isConnected) return;
    
    try {
        const response = await fetch(
            `http://${CONFIG.raspiHost}/status`,
            { timeout: 3000 }
        ).catch(() => null);
        
        if (response && response.ok) {
            const data = await response.json();
            updateStatusDisplay(data);
        }
    } catch (error) {
        console.error('Status update error:', error.message);
    }
}

function updateStatusDisplay(data) {
    // Update robot state
    document.getElementById('robotState').textContent = 
        data.state || 'Unknown';
    
    // Update path status
    document.getElementById('pathStatus').textContent = 
        data.path_clear ? 'Yes' : 'No - Obstacle!';
    
    // Update detection count
    document.getElementById('objectCount').textContent = 
        data.object_count || 0;
    
    // Update obstacle distance
    const obstacleText = data.closest_obstacle !== null ? 
        `${data.closest_obstacle.toFixed(2)}m` : 'None';
    document.getElementById('obstacleDistance').textContent = obstacleText;
    
    // Update detection list
    if (data.detections && data.detections.length > 0) {
        updateDetectionsList(data.detections);
    }
}

function updateDetectionsList(detections) {
    const list = document.getElementById('detectionsList');
    
    if (detections.length === 0) {
        list.innerHTML = '<p class="empty-state">No objects detected</p>';
        return;
    }
    
    list.innerHTML = detections.map(det => `
        <div class="detection-item">
            <span class="label">${det.label}</span>
            <span class="confidence">${(det.confidence * 100).toFixed(1)}%</span>
        </div>
    `).join('');
}

// ==================== Utilities ====================

function showNotification(message, type = 'info') {
    console.log(`[${type.toUpperCase()}] ${message}`);
    
    // Could implement toast notifications here
    const colors = {
        'info': '#3498db',
        'success': '#2ecc71',
        'error': '#e74c3c',
        'warning': '#f39c12'
    };
    
    // Simple visual feedback
    const notification = document.createElement('div');
    notification.style.cssText = `
        position: fixed;
        top: 20px;
        right: 20px;
        background: ${colors[type]};
        color: white;
        padding: 15px 20px;
        border-radius: 4px;
        box-shadow: 0 4px 6px rgba(0,0,0,0.3);
        z-index: 1000;
        animation: slideIn 0.3s ease;
    `;
    notification.textContent = message;
    
    document.body.appendChild(notification);
    
    setTimeout(() => {
        notification.remove();
    }, 3000);
}

// Periodic reconnection attempt
setInterval(() => {
    if (!state.isConnected) {
        checkConnection();
    }
}, 5000);
