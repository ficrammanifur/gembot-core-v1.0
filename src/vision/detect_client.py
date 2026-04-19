"""
Detection client for GEMBOT
Sends frames to AI Server for YOLOv8 processing
"""

import requests
import numpy as np
import cv2
import threading
import time
from typing import Optional, List, Dict
from ..utils import get_logger, get_config

logger = get_logger("vision.detect_client")


class DetectionClient:
    """Client for communicating with AI Server for object detection"""
    
    def __init__(self):
        """Initialize detection client"""
        self.config = get_config()
        self.server_host = self.config.get('ai_server.host', 'localhost')
        self.server_port = self.config.get('ai_server.port', 5000)
        self.server_url = f"http://{self.server_host}:{self.server_port}"
        self.timeout = self.config.get('ai_server.timeout', 30)
        self.confidence_threshold = self.config.get('ai_server.detection_confidence', 0.5)
        
        self.latest_detections = []
        self.detection_lock = threading.Lock()
        self.last_detection_time = None
        self.detection_enabled = True
        
        logger.info(f"DetectionClient initialized: {self.server_url}")
    
    def is_server_available(self) -> bool:
        """Check if AI Server is available"""
        try:
            response = requests.get(f"{self.server_url}/health", timeout=5)
            return response.status_code == 200
        except Exception as e:
            logger.warning(f"AI Server health check failed: {e}")
            return False
    
    def send_frame_async(self, frame: np.ndarray, callback=None):
        """Send frame to server asynchronously"""
        if not self.detection_enabled:
            return
        
        thread = threading.Thread(
            target=self._send_frame_sync,
            args=(frame, callback),
            daemon=True
        )
        thread.start()
    
    def _send_frame_sync(self, frame: np.ndarray, callback=None):
        """Synchronously send frame to detection server"""
        try:
            if frame is None:
                return
            
            # Encode frame as JPEG
            success, buffer = cv2.imencode('.jpg', frame)
            if not success:
                logger.error("Failed to encode frame")
                return
            
            # Prepare request
            files = {'image': ('frame.jpg', buffer.tobytes(), 'image/jpeg')}
            
            # Send to server
            response = requests.post(
                f"{self.server_url}/detect",
                files=files,
                timeout=self.timeout,
                params={'confidence': self.confidence_threshold}
            )
            
            if response.status_code == 200:
                detections = response.json().get('detections', [])
                
                with self.detection_lock:
                    self.latest_detections = detections
                    self.last_detection_time = time.time()
                
                logger.debug(f"Detected {len(detections)} objects")
                
                # Call callback if provided
                if callback:
                    callback(detections)
            else:
                logger.error(f"Detection failed: {response.status_code}")
        
        except requests.Timeout:
            logger.warning(f"Detection request timeout ({self.timeout}s)")
        except Exception as e:
            logger.error(f"Detection error: {e}")
    
    def get_latest_detections(self) -> List[Dict]:
        """Get latest detections"""
        with self.detection_lock:
            return self.latest_detections.copy() if self.latest_detections else []
    
    def get_detection_stats(self) -> Dict:
        """Get detection statistics"""
        with self.detection_lock:
            return {
                'detection_count': len(self.latest_detections),
                'last_detection_time': self.last_detection_time,
                'detections': self.latest_detections
            }
    
    def enable_detection(self):
        """Enable object detection"""
        self.detection_enabled = True
        logger.info("Object detection enabled")
    
    def disable_detection(self):
        """Disable object detection"""
        self.detection_enabled = False
        logger.info("Object detection disabled")
    
    def filter_detections_by_class(self, class_name: str) -> List[Dict]:
        """Filter detections by class name"""
        with self.detection_lock:
            return [d for d in self.latest_detections 
                   if d.get('label', '').lower() == class_name.lower()]
    
    def get_detection_by_confidence(self, min_confidence: float = None) -> List[Dict]:
        """Get detections above confidence threshold"""
        threshold = min_confidence or self.confidence_threshold
        with self.detection_lock:
            return [d for d in self.latest_detections 
                   if d.get('confidence', 0) >= threshold]
