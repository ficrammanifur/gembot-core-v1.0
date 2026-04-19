"""
Speech-to-Text module for GEMBOT
Converts speech input to text using available STT engine
"""

import threading
from typing import Optional, Callable
from ..utils import get_logger, get_config

logger = get_logger("audio.stt")


class SpeechToText:
    """Handles speech-to-text conversion"""
    
    def __init__(self):
        """Initialize STT module"""
        self.config = get_config()
        self.language = self.config.get('audio.tts_language', 'en')
        self.recognizer = None
        self.microphone = None
        self.last_text = None
        self.stt_callback = None
        
        self._initialize_recognizer()
        logger.info("STT module initialized")
    
    def _initialize_recognizer(self):
        """Initialize speech recognition engine"""
        try:
            from speech_recognition import Recognizer, Microphone
            self.recognizer = Recognizer()
            self.microphone = Microphone()
            
            # Calibrate for ambient noise
            with self.microphone as source:
                self.recognizer.adjust_for_ambient_noise(source, duration=1)
            
            logger.info("Speech recognizer initialized")
        except ImportError:
            logger.warning("SpeechRecognition not installed, STT disabled")
            self.recognizer = None
        except Exception as e:
            logger.error(f"Recognizer initialization failed: {e}")
            self.recognizer = None
    
    def set_stt_callback(self, callback: Callable):
        """Set callback function for recognized text"""
        self.stt_callback = callback
    
    def listen_once(self) -> Optional[str]:
        """Listen once and return recognized text"""
        if self.recognizer is None:
            logger.error("Recognizer not available")
            return None
        
        try:
            logger.info("Listening for speech...")
            
            with self.microphone as source:
                audio = self.recognizer.listen(source, timeout=10)
            
            logger.info("Processing audio...")
            text = self.recognizer.recognize_google(audio, language=self.language)
            
            self.last_text = text
            logger.info(f"Recognized: {text}")
            
            # Call callback if registered
            if self.stt_callback:
                self.stt_callback(text)
            
            return text
        
        except Exception as e:
            logger.error(f"Recognition error: {e}")
            return None
    
    def listen_async(self):
        """Listen in background thread"""
        if not self.recognizer:
            logger.error("Recognizer not available")
            return
        
        thread = threading.Thread(target=self.listen_once, daemon=True)
        thread.start()
    
    def get_last_recognized(self) -> Optional[str]:
        """Get last recognized text"""
        return self.last_text
