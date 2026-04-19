"""
Communication protocol definitions for GEMBOT
JSON message formats and MQTT topics
"""

from enum import Enum
from typing import Dict, Any
import json


class MessageType(Enum):
    """Message types for MQTT communication"""
    STATUS = "status"
    CONTROL = "control"
    ALERT = "alert"
    RESPONSE = "response"
    DATA = "data"


class MQTTTopics:
    """MQTT topic definitions"""
    # Status topics
    ROBOT_STATUS = "gembot/status"
    CAMERA_STATUS = "gembot/camera/status"
    LIDAR_STATUS = "gembot/lidar/status"
    MOTOR_STATUS = "gembot/motor/status"
    
    # Control topics
    MOTOR_CONTROL = "gembot/motor/control"
    CAMERA_CONTROL = "gembot/camera/control"
    
    # ESP32 topics
    ESP32_SPEAK = "gembot/esp32/speak"
    ESP32_STATUS = "gembot/esp32/status"
    
    # Alert topics
    OBSTACLE_ALERT = "gembot/alert/obstacle"
    EMERGENCY_STOP = "gembot/emergency/stop"


class MessageProtocol:
    """Protocol helper for message creation and parsing"""
    
    @staticmethod
    def create_status_message(
        robot_state: str,
        battery: float,
        uptime: float,
        error: str = None
    ) -> Dict[str, Any]:
        """Create robot status message"""
        return {
            'type': MessageType.STATUS.value,
            'state': robot_state,
            'battery': battery,
            'uptime': uptime,
            'error': error
        }
    
    @staticmethod
    def create_obstacle_alert(
        distance: float,
        angle: float,
        direction: str
    ) -> Dict[str, Any]:
        """Create obstacle alert message"""
        return {
            'type': MessageType.ALERT.value,
            'alert_type': 'obstacle',
            'distance': distance,
            'angle': angle,
            'avoidance_direction': direction
        }
    
    @staticmethod
    def create_motor_command(
        left_speed: int,
        right_speed: int
    ) -> Dict[str, Any]:
        """Create motor control command"""
        return {
            'type': MessageType.CONTROL.value,
            'command': 'motor',
            'left_speed': max(-255, min(255, left_speed)),
            'right_speed': max(-255, min(255, right_speed))
        }
    
    @staticmethod
    def create_detection_data(
        detections: list,
        frame_count: int
    ) -> Dict[str, Any]:
        """Create detection data message"""
        return {
            'type': MessageType.DATA.value,
            'data_type': 'detection',
            'frame_count': frame_count,
            'detections': detections,
            'detection_count': len(detections)
        }
    
    @staticmethod
    def create_lidar_data(
        obstacles: list,
        path_clear: bool,
        closest_distance: float = None
    ) -> Dict[str, Any]:
        """Create LiDAR scan data message"""
        return {
            'type': MessageType.DATA.value,
            'data_type': 'lidar',
            'obstacles': obstacles,
            'path_clear': path_clear,
            'closest_obstacle_distance': closest_distance
        }
    
    @staticmethod
    def create_speak_command(text: str, language: str = 'en') -> Dict[str, Any]:
        """Create text-to-speech command"""
        return {
            'type': MessageType.CONTROL.value,
            'command': 'speak',
            'text': text,
            'language': language
        }
    
    @staticmethod
    def create_emergency_stop() -> Dict[str, Any]:
        """Create emergency stop message"""
        return {
            'type': MessageType.CONTROL.value,
            'command': 'emergency_stop',
            'priority': 'critical'
        }
