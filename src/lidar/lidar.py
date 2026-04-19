"""
LiDAR module for GEMBOT
Handles RPLidar scanning for obstacle detection and navigation
"""

import threading
import time
import numpy as np
from typing import Optional, List, Tuple
from ..utils import get_logger, get_config

logger = get_logger("lidar.lidar")


class LiDAR:
    """LiDAR sensor for obstacle detection"""
    
    def __init__(self):
        """Initialize LiDAR"""
        self.config = get_config()
        self.port = self.config.get('lidar.port', '/dev/ttyUSB0')
        self.baudrate = self.config.get('lidar.baudrate', 256000)
        self.max_distance = self.config.get('lidar.max_distance', 5.0)
        self.min_distance = self.config.get('lidar.min_distance', 0.15)
        
        self.is_running = False
        self.scan_data = []
        self.data_lock = threading.Lock()
        self.last_scan_time = None
        
        # Simulate LiDAR data in demo mode
        self.demo_mode = True
        self.serial_port = None
        
        logger.info(f"LiDAR initialized on {self.port}")
    
    def initialize(self) -> bool:
        """Initialize LiDAR connection"""
        try:
            if self.demo_mode:
                logger.info("LiDAR running in demo mode (simulation)")
                return True
            
            try:
                import serial
                self.serial_port = serial.Serial(
                    port=self.port,
                    baudrate=self.baudrate,
                    timeout=1
                )
                logger.info("LiDAR connected successfully")
                return True
            except ImportError:
                logger.warning("pyserial not installed, using demo mode")
                self.demo_mode = True
                return True
                
        except Exception as e:
            logger.error(f"LiDAR initialization failed: {e}")
            return False
    
    def start_scanning(self):
        """Start continuous LiDAR scanning"""
        if self.is_running:
            logger.warning("LiDAR scanning already active")
            return
        
        self.is_running = True
        scan_thread = threading.Thread(target=self._scan_loop, daemon=True)
        scan_thread.start()
        logger.info("LiDAR scanning started")
    
    def _scan_loop(self):
        """Continuous scanning loop"""
        while self.is_running:
            try:
                if self.demo_mode:
                    scan_data = self._generate_demo_scan()
                else:
                    scan_data = self._read_real_scan()
                
                if scan_data:
                    with self.data_lock:
                        self.scan_data = scan_data
                        self.last_scan_time = time.time()
                
                time.sleep(0.05)  # 20 Hz scan rate
                
            except Exception as e:
                logger.error(f"Scan error: {e}")
                time.sleep(0.1)
    
    def _generate_demo_scan(self) -> List[Tuple[float, float]]:
        """Generate simulated LiDAR scan data"""
        # Simulate scan with some obstacles
        scan_points = []
        angles = np.linspace(0, 360, 360)
        
        for angle in angles:
            # Add some variation and obstacles
            distance = 2.0 + np.random.normal(0, 0.1)
            
            # Simulate obstacle at 90 degrees
            if 80 <= angle <= 100:
                distance = 0.5 + np.random.normal(0, 0.05)
            
            # Simulate obstacle at 270 degrees
            if 260 <= angle <= 280:
                distance = 0.8 + np.random.normal(0, 0.05)
            
            if self.min_distance <= distance <= self.max_distance:
                scan_points.append((angle, distance))
        
        return scan_points
    
    def _read_real_scan(self) -> List[Tuple[float, float]]:
        """Read real scan from LiDAR"""
        # This would read from actual RPLidar device
        # Implementation depends on rplidar-python library
        return []
    
    def get_scan_data(self) -> List[Tuple[float, float]]:
        """Get latest scan data (angle, distance)"""
        with self.data_lock:
            return self.scan_data.copy() if self.scan_data else []
    
    def get_obstacles(self) -> List[Tuple[float, float]]:
        """Get obstacles detected in front of robot"""
        threshold = self.config.get('lidar.obstacle_threshold', 2.0)
        
        with self.data_lock:
            obstacles = []
            for angle, distance in self.scan_data:
                if distance < threshold:
                    # Front is 0 degrees (with 45 degree tolerance)
                    if angle < 45 or angle > 315:
                        obstacles.append((angle, distance))
            
            return obstacles
    
    def is_path_clear(self) -> bool:
        """Check if path ahead is clear"""
        obstacles = self.get_obstacles()
        return len(obstacles) == 0
    
    def get_closest_obstacle_distance(self) -> Optional[float]:
        """Get distance to closest obstacle"""
        obstacles = self.get_obstacles()
        if not obstacles:
            return None
        
        distances = [d for _, d in obstacles]
        return min(distances) if distances else None
    
    def get_scan_stats(self) -> dict:
        """Get scan statistics"""
        scan = self.get_scan_data()
        obstacles = self.get_obstacles()
        
        return {
            'total_points': len(scan),
            'obstacle_count': len(obstacles),
            'path_clear': self.is_path_clear(),
            'closest_obstacle': self.get_closest_obstacle_distance(),
            'last_scan_time': self.last_scan_time
        }
    
    def stop_scanning(self):
        """Stop LiDAR scanning"""
        self.is_running = False
        logger.info("LiDAR scanning stopped")
    
    def release(self):
        """Release LiDAR resources"""
        self.stop_scanning()
        if self.serial_port:
            self.serial_port.close()
            self.serial_port = None
        logger.info("LiDAR released")
