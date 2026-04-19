"""
Obstacle avoidance logic for GEMBOT
Processes LiDAR data for navigation decisions
"""

from typing import Tuple, Optional
from ..utils import get_logger, get_config

logger = get_logger("lidar.obstacle")


class ObstacleAvoidance:
    """Handles obstacle detection and avoidance logic"""
    
    def __init__(self, lidar):
        """Initialize obstacle avoidance"""
        self.lidar = lidar
        self.config = get_config()
        self.stop_distance = self.config.get('safety.obstacle_stop_distance', 0.5)
        self.warning_distance = self.config.get('lidar.obstacle_threshold', 2.0)
    
    def should_stop(self) -> bool:
        """Check if robot should stop due to obstacle"""
        closest = self.lidar.get_closest_obstacle_distance()
        return closest is not None and closest < self.stop_distance
    
    def should_warn(self) -> bool:
        """Check if there's a warning (obstacle nearby)"""
        closest = self.lidar.get_closest_obstacle_distance()
        return closest is not None and closest < self.warning_distance
    
    def get_avoidance_direction(self) -> Optional[str]:
        """
        Determine direction to avoid obstacle
        Returns: 'left', 'right', or None if no obstacle
        """
        obstacles = self.lidar.get_obstacles()
        
        if not obstacles:
            return None
        
        # Count obstacles on left (angles 0-180) vs right (180-360)
        left_obstacles = [o for o in obstacles if 0 < o[0] <= 180]
        right_obstacles = [o for o in obstacles if 180 < o[0] < 360]
        
        # Avoid toward side with fewer obstacles
        if len(left_obstacles) > len(right_obstacles):
            return 'right'
        elif len(right_obstacles) > len(left_obstacles):
            return 'left'
        
        # If equal, check distances
        left_min = min([d for _, d in left_obstacles], default=float('inf'))
        right_min = min([d for _, d in right_obstacles], default=float('inf'))
        
        return 'right' if left_min < right_min else 'left'
    
    def get_obstacle_info(self) -> dict:
        """Get detailed obstacle information"""
        stats = self.lidar.get_scan_stats()
        closest = self.lidar.get_closest_obstacle_distance()
        avoidance = self.get_avoidance_direction()
        
        return {
            'obstacle_detected': not stats['path_clear'],
            'should_stop': self.should_stop(),
            'should_warn': self.should_warn(),
            'closest_distance': closest,
            'obstacle_count': stats['obstacle_count'],
            'avoidance_direction': avoidance,
            'stats': stats
        }
    
    def is_emergency_stop_needed(self) -> bool:
        """Check if emergency stop is required"""
        return self.should_stop() and self.lidar.is_path_clear() == False
