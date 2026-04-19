"""
Text-to-Speech module for GEMBOT
Sends text to ESP32 for audio output
"""

import threading
from typing import Optional
from ..utils import get_logger, get_config
from ..comm import HTTPClient

logger = get_logger("audio.tts")


class TextToSpeech:
    """Handles text-to-speech conversion via ESP32"""
    
    def __init__(self, http_client: Optional[HTTPClient] = None):
        """Initialize TTS module"""
        self.config = get_config()
        self.http_client = http_client or HTTPClient()
        self.language = self.config.get('audio.tts_language', 'en')
        self.queue = []
        self.queue_lock = threading.Lock()
        self.is_speaking = False
        
        logger.info("TTS module initialized")
    
    def speak(self, text: str, async_mode: bool = True) -> bool:
        """
        Speak text through ESP32 speaker
        async_mode: If True, non-blocking; if False, wait for completion
        """
        if not text or not isinstance(text, str):
            logger.error("Invalid text for TTS")
            return False
        
        text = text.strip()
        if len(text) == 0:
            return False
        
        logger.info(f"TTS request: {text[:100]}")
        
        if async_mode:
            thread = threading.Thread(
                target=self._speak_sync,
                args=(text,),
                daemon=True
            )
            thread.start()
            return True
        else:
            return self._speak_sync(text)
    
    def _speak_sync(self, text: str) -> bool:
        """Synchronously send text to ESP32"""
        try:
            with self.queue_lock:
                self.is_speaking = True
            
            success = self.http_client.send_tts_request(text)
            
            with self.queue_lock:
                self.is_speaking = False
            
            return success
            
        except Exception as e:
            logger.error(f"TTS error: {e}")
            return False
    
    def queue_speech(self, text: str) -> bool:
        """Queue text for speech output"""
        if not text or not isinstance(text, str):
            return False
        
        with self.queue_lock:
            self.queue.append(text.strip())
            logger.debug(f"Queued text (queue size: {len(self.queue)})")
        
        return True
    
    def process_queue(self):
        """Process queued speech items"""
        with self.queue_lock:
            if not self.queue or self.is_speaking:
                return
            
            text = self.queue.pop(0)
        
        self._speak_sync(text)
    
    def is_available(self) -> bool:
        """Check if ESP32 TTS is available"""
        return self.http_client.is_esp32_available()
    
    def get_queue_size(self) -> int:
        """Get number of queued items"""
        with self.queue_lock:
            return len(self.queue)
    
    def clear_queue(self):
        """Clear speech queue"""
        with self.queue_lock:
            self.queue.clear()
            logger.info("Speech queue cleared")
