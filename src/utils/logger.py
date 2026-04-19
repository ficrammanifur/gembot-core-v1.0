"""
Logging utility for GEMBOT
Provides colored logging for console and file output
"""

import logging
import logging.handlers
import os
from pathlib import Path


class GembotLogger:
    """Centralized logging configuration for GEMBOT"""
    
    _instance = None
    
    def __new__(cls):
        if cls._instance is None:
            cls._instance = super().__new__(cls)
            cls._instance._initialized = False
        return cls._instance
    
    def __init__(self):
        if self._initialized:
            return
        
        self._initialized = True
        self.logger = logging.getLogger("gembot")
        self.logger.setLevel(logging.DEBUG)
        
        # Create logs directory
        log_dir = Path("logs")
        log_dir.mkdir(exist_ok=True)
        
        # Console handler with color
        console_handler = logging.StreamHandler()
        console_handler.setLevel(logging.INFO)
        console_format = logging.Formatter(
            '%(asctime)s - %(name)s - %(levelname)s - %(message)s',
            datefmt='%Y-%m-%d %H:%M:%S'
        )
        console_handler.setFormatter(console_format)
        
        # File handler with rotation
        file_handler = logging.handlers.RotatingFileHandler(
            'logs/gembot.log',
            maxBytes=10485760,  # 10 MB
            backupCount=5
        )
        file_handler.setLevel(logging.DEBUG)
        file_format = logging.Formatter(
            '%(asctime)s - %(name)s - %(levelname)s - [%(filename)s:%(lineno)d] - %(message)s',
            datefmt='%Y-%m-%d %H:%M:%S'
        )
        file_handler.setFormatter(file_format)
        
        self.logger.addHandler(console_handler)
        self.logger.addHandler(file_handler)
    
    def get_logger(self, name):
        """Get a logger instance for a module"""
        return logging.getLogger(f"gembot.{name}")


# Global logger instance
def get_logger(name):
    """Get logger for a module"""
    logger_instance = GembotLogger()
    return logger_instance.get_logger(name)
