"""
HTTP client for GEMBOT
Handles heavy data transfer (audio, images) between devices
"""

import requests
import json
from typing import Optional, Dict, Any
from ..utils import get_logger, get_config

logger = get_logger("comm.http")


class HTTPClient:
    """HTTP client for inter-device communication"""
    
    def __init__(self):
        """Initialize HTTP client"""
        self.config = get_config()
        self.esp32_host = self.config.get('esp32.host', 'localhost')
        self.esp32_port = self.config.get('esp32.port', 80)
        self.esp32_url = f"http://{self.esp32_host}:{self.esp32_port}"
        self.timeout = self.config.get('esp32.timeout', 10)
    
    def send_tts_request(self, text: str) -> bool:
        """
        Send text-to-speech request to ESP32
        Text will be converted to speech and played on speaker
        """
        try:
            if not text or not isinstance(text, str):
                logger.error("Invalid text for TTS")
                return False
            
            payload = {'text': text}
            response = requests.post(
                f"{self.esp32_url}/speak",
                json=payload,
                timeout=self.timeout
            )
            
            if response.status_code == 200:
                logger.info(f"TTS sent: {text[:50]}")
                return True
            else:
                logger.error(f"TTS request failed: {response.status_code}")
                return False
                
        except requests.Timeout:
            logger.error(f"TTS request timeout ({self.timeout}s)")
            return False
        except Exception as e:
            logger.error(f"TTS error: {e}")
            return False
    
    def get_esp32_status(self) -> Optional[Dict[str, Any]]:
        """Get ESP32 status"""
        try:
            response = requests.get(
                f"{self.esp32_url}/status",
                timeout=5
            )
            
            if response.status_code == 200:
                return response.json()
            else:
                logger.warning(f"Status request failed: {response.status_code}")
                return None
                
        except Exception as e:
            logger.warning(f"Status request error: {e}")
            return None
    
    def send_motor_command(self, left_speed: int, right_speed: int) -> bool:
        """Send motor control command to ESP32"""
        try:
            payload = {
                'left_speed': max(-255, min(255, left_speed)),
                'right_speed': max(-255, min(255, right_speed))
            }
            
            response = requests.post(
                f"{self.esp32_url}/motor",
                json=payload,
                timeout=self.timeout
            )
            
            return response.status_code == 200
            
        except Exception as e:
            logger.error(f"Motor command error: {e}")
            return False
    
    def is_esp32_available(self) -> bool:
        """Check if ESP32 is available"""
        status = self.get_esp32_status()
        return status is not None
