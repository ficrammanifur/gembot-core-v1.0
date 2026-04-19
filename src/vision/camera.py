"""
Camera module for GEMBOT
Handles video capture, frame processing, and streaming
"""

import cv2
import numpy as np
import threading
import time
from typing import Optional, Tuple
from ..utils import get_logger, get_config

logger = get_logger("vision.camera")


class Camera:
    """Manages camera capture and frame processing"""
    
    def __init__(self):
        """Initialize camera with configuration"""
        self.config = get_config()
        self.cap = None
        self.is_running = False
        self.current_frame = None
        self.frame_lock = threading.Lock()
        self.fps = self.config.get('camera.fps', 30)
        self.resolution = self.config.get('camera.resolution', [640, 480])
        self.frame_count = 0
        
    def initialize(self) -> bool:
        """Initialize camera and set properties"""
        try:
            logger.info("Initializing camera...")
            self.cap = cv2.VideoCapture(0)
            
            # Set resolution
            self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, self.resolution[0])
            self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, self.resolution[1])
            self.cap.set(cv2.CAP_PROP_FPS, self.fps)
            
            # Set other properties
            self.cap.set(cv2.CAP_PROP_BRIGHTNESS, self.config.get('camera.brightness', 0))
            self.cap.set(cv2.CAP_PROP_CONTRAST, self.config.get('camera.contrast', 0))
            self.cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)  # Reduce buffering for real-time
            
            # Test capture
            ret, frame = self.cap.read()
            if not ret:
                logger.error("Failed to read from camera")
                return False
            
            logger.info(f"Camera initialized: {self.resolution[0]}x{self.resolution[1]}@{self.fps}fps")
            return True
            
        except Exception as e:
            logger.error(f"Camera initialization failed: {e}")
            return False
    
    def start_capture(self):
        """Start continuous frame capture in background thread"""
        if self.is_running:
            logger.warning("Camera capture already running")
            return
        
        self.is_running = True
        capture_thread = threading.Thread(target=self._capture_loop, daemon=True)
        capture_thread.start()
        logger.info("Camera capture started")
    
    def _capture_loop(self):
        """Continuous frame capture loop"""
        while self.is_running and self.cap is not None:
            try:
                ret, frame = self.cap.read()
                if ret:
                    # Rotate if configured
                    rotation = self.config.get('camera.rotation', 0)
                    if rotation != 0:
                        h, w = frame.shape[:2]
                        center = (w // 2, h // 2)
                        matrix = cv2.getRotationMatrix2D(center, rotation, 1.0)
                        frame = cv2.warpAffine(frame, matrix, (w, h))
                    
                    with self.frame_lock:
                        self.current_frame = frame
                        self.frame_count += 1
                else:
                    logger.warning("Failed to read frame from camera")
                    time.sleep(0.1)
            except Exception as e:
                logger.error(f"Error in capture loop: {e}")
                time.sleep(0.1)
    
    def get_frame(self) -> Optional[np.ndarray]:
        """Get current frame"""
        with self.frame_lock:
            return self.current_frame.copy() if self.current_frame is not None else None
    
    def get_frame_info(self) -> dict:
        """Get frame information"""
        with self.frame_lock:
            return {
                'count': self.frame_count,
                'resolution': self.resolution,
                'has_frame': self.current_frame is not None
            }
    
    def encode_frame_jpeg(self, frame: Optional[np.ndarray] = None, quality: int = 80) -> Optional[bytes]:
        """Encode frame as JPEG bytes"""
        try:
            if frame is None:
                frame = self.get_frame()
            
            if frame is None:
                return None
            
            _, buffer = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, quality])
            return buffer.tobytes()
        except Exception as e:
            logger.error(f"JPEG encoding failed: {e}")
            return None
    
    def stop_capture(self):
        """Stop camera capture"""
        self.is_running = False
        logger.info("Camera capture stopped")
    
    def release(self):
        """Release camera resources"""
        self.stop_capture()
        if self.cap is not None:
            self.cap.release()
            self.cap = None
        logger.info("Camera released")
    
    def __del__(self):
        """Cleanup on deletion"""
        self.release()
