"""
Decision-making AI brain for GEMBOT
Processes sensor data and makes movement decisions
"""

from enum import Enum
from typing import Optional, Dict, Any
from ..utils import get_logger, get_config
from ..comm import MessageProtocol

logger = get_logger("control.brain")


class RobotState(Enum):
    """Robot operational states"""
    IDLE = "idle"
    MOVING = "moving"
    AVOIDING = "avoiding"
    STOPPED = "stopped"
    ERROR = "error"


class Brain:
    """Main decision-making system for robot behavior"""
    
    def __init__(self, lidar, detection_client, http_client):
        """Initialize robot brain"""
        self.config = get_config()
        self.lidar = lidar
        self.detection_client = detection_client
        self.http_client = http_client
        
        self.current_state = RobotState.IDLE
        self.detected_humans = []
        self.safety_enabled = True
        
        logger.info("Robot brain initialized")
    
    def process_sensor_data(self) -> Dict[str, Any]:
        """Process all sensor data and make decisions"""
        decisions = {
            'state': self.current_state.value,
            'movement': self._get_movement_command(),
            'alerts': self._get_alerts(),
            'status': self._get_status()
        }
        
        return decisions
    
    def _get_movement_command(self) -> Optional[Dict[str, int]]:
        """Determine movement command based on sensors"""
        
        # Check obstacle status
        closest_distance = self.lidar.get_closest_obstacle_distance()
        should_stop = closest_distance is not None and closest_distance < self.config.get('safety.obstacle_stop_distance', 0.5)
        
        # Check for detected humans
        humans = self.detection_client.filter_detections_by_class('person')
        self.detected_humans = humans
        
        if should_stop or not self.safety_enabled:
            # Safety override - stop movement
            self.current_state = RobotState.STOPPED
            return {'left': 0, 'right': 0}
        
        if len(humans) > 0:
            # Human detected - prepare for interaction
            self.current_state = RobotState.IDLE
            return {'left': 0, 'right': 0}
        
        # Normal state - ready to move
        self.current_state = RobotState.IDLE
        return None
    
    def _get_alerts(self) -> list:
        """Get system alerts"""
        alerts = []
        
        # Check obstacles
        obstacle_info = self.lidar.get_scan_stats()
        if not obstacle_info['path_clear']:
            closest = self.lidar.get_closest_obstacle_distance()
            if closest:
                alerts.append({
                    'type': 'obstacle',
                    'severity': 'critical' if closest < 0.5 else 'warning',
                    'distance': closest
                })
        
        # Check detections
        detections = self.detection_client.get_latest_detections()
        if len(detections) > 0:
            alerts.append({
                'type': 'detection',
                'count': len(detections),
                'classes': list(set([d.get('label') for d in detections]))
            })
        
        return alerts
    
    def _get_status(self) -> Dict[str, Any]:
        """Get comprehensive system status"""
        return {
            'state': self.current_state.value,
            'safety_enabled': self.safety_enabled,
            'humans_detected': len(self.detected_humans),
            'objects_detected': len(self.detection_client.get_latest_detections()),
            'path_clear': self.lidar.is_path_clear()
        }
    
    def handle_human_interaction(self, human_detection: Dict) -> str:
        """
        Handle interaction with detected human
        Returns response text
        """
        confidence = human_detection.get('confidence', 0)
        
        if confidence > 0.9:
            response = "Hello! I'm GEMBOT. How can I help you?"
        else:
            response = "Hello there!"
        
        return response
    
    def move(self, direction: str, speed: int = 150) -> bool:
        """
        Move robot in specified direction
        direction: 'forward', 'backward', 'left', 'right'
        """
        if self.current_state == RobotState.STOPPED:
            logger.warning("Robot is stopped due to safety")
            return False
        
        speed = max(-255, min(255, speed))
        
        commands = {
            'forward': {'left': speed, 'right': speed},
            'backward': {'left': -speed, 'right': -speed},
            'left': {'left': -speed // 2, 'right': speed},
            'right': {'left': speed, 'right': -speed // 2},
            'stop': {'left': 0, 'right': 0}
        }
        
        if direction not in commands:
            logger.error(f"Unknown direction: {direction}")
            return False
        
        cmd = commands[direction]
        self.current_state = RobotState.MOVING if direction != 'stop' else RobotState.IDLE
        
        return self.http_client.send_motor_command(cmd['left'], cmd['right'])
    
    def stop(self) -> bool:
        """Stop robot movement"""
        return self.move('stop')
    
    def enable_safety(self):
        """Enable safety checks"""
        self.safety_enabled = True
        logger.info("Safety enabled")
    
    def disable_safety(self):
        """Disable safety checks"""
        self.safety_enabled = False
        logger.warning("Safety disabled - use with caution!")
    
    def get_state(self) -> str:
        """Get current robot state"""
        return self.current_state.value
