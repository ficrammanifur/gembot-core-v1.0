#!/usr/bin/env python3
"""
GEMBOT Main Entry Point
Orchestrates all robot systems and processes
"""

import sys
import time
import signal
from pathlib import Path

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent.parent))

from src.utils import get_logger, get_config
from src.vision import Camera, StreamProcessor, DetectionClient
from src.lidar import LiDAR, ObstacleAvoidance
from src.comm import MQTTClient, HTTPClient, MessageProtocol
from src.audio import TextToSpeech, SpeechToText
from src.control import Brain

logger = get_logger("main")


class GEMBOTRobot:
    """Main robot orchestration class"""
    
    def __init__(self):
        """Initialize robot systems"""
        logger.info("=" * 50)
        logger.info("GEMBOT - Distributed AI Robot System")
        logger.info("=" * 50)
        
        self.config = get_config()
        self.running = False
        
        # Initialize all subsystems
        self.camera = Camera()
        self.stream_processor = None
        self.detection_client = DetectionClient()
        self.lidar = LiDAR()
        self.obstacle_avoidance = ObstacleAvoidance(self.lidar)
        self.mqtt_client = MQTTClient()
        self.http_client = HTTPClient()
        self.tts = TextToSpeech(self.http_client)
        self.stt = SpeechToText()
        self.brain = Brain(self.lidar, self.detection_client, self.http_client)
        
        # Setup signal handlers
        signal.signal(signal.SIGINT, self._signal_handler)
        signal.signal(signal.SIGTERM, self._signal_handler)
    
    def _signal_handler(self, sig, frame):
        """Handle shutdown signals"""
        logger.info("Shutdown signal received")
        self.shutdown()
        sys.exit(0)
    
    def initialize(self) -> bool:
        """Initialize all robot systems"""
        logger.info("Initializing robot systems...")
        
        # Initialize camera
        if not self.camera.initialize():
            logger.error("Camera initialization failed")
            return False
        
        # Initialize LiDAR
        if not self.lidar.initialize():
            logger.error("LiDAR initialization failed")
            return False
        
        # Initialize MQTT
        if not self.mqtt_client.connect():
            logger.warning("MQTT connection failed - continuing without it")
        
        # Initialize stream processor
        self.stream_processor = StreamProcessor(self.camera)
        
        logger.info("All systems initialized")
        return True
    
    def start(self):
        """Start robot systems"""
        logger.info("Starting robot systems...")
        
        self.running = True
        
        # Start hardware capture
        self.camera.start_capture()
        self.lidar.start_scanning()
        self.stream_processor.start_streaming()
        
        logger.info("Robot systems started")
        logger.info("Starting main loop...")
    
    def run_main_loop(self):
        """Main execution loop"""
        loop_count = 0
        
        try:
            while self.running:
                loop_count += 1
                
                try:
                    # Get current frame
                    frame = self.camera.get_frame()
                    if frame is None:
                        time.sleep(0.01)
                        continue
                    
                    # Send frame for detection (non-blocking)
                    self.detection_client.send_frame_async(frame)
                    
                    # Get and add detection overlay
                    detections = self.detection_client.get_latest_detections()
                    if detections:
                        processor = self.stream_processor.draw_bounding_boxes(detections)
                        self.stream_processor.add_frame_processor(processor)
                    
                    # Process sensor data through brain
                    brain_output = self.brain.process_sensor_data()
                    
                    # Publish status every 10 loops
                    if loop_count % 10 == 0:
                        status = MessageProtocol.create_status_message(
                            robot_state=self.brain.get_state(),
                            battery=100.0,  # Placeholder
                            uptime=time.time()
                        )
                        self.mqtt_client.publish_status(status)
                    
                    # Check for alerts
                    for alert in brain_output['alerts']:
                        logger.warning(f"ALERT: {alert}")
                    
                    time.sleep(0.01)
                
                except Exception as e:
                    logger.error(f"Error in main loop: {e}")
                    time.sleep(0.1)
        
        except KeyboardInterrupt:
            logger.info("Interrupted by user")
        except Exception as e:
            logger.error(f"Fatal error in main loop: {e}")
    
    def shutdown(self):
        """Shutdown robot systems"""
        logger.info("Shutting down robot systems...")
        
        self.running = False
        
        # Stop all capture threads
        if self.camera:
            self.camera.stop_capture()
            self.camera.release()
        
        if self.lidar:
            self.lidar.stop_scanning()
            self.lidar.release()
        
        if self.stream_processor:
            self.stream_processor.stop_streaming()
        
        # Disconnect communication
        if self.mqtt_client:
            self.mqtt_client.disconnect()
        
        logger.info("Robot shutdown complete")
    
    def run(self):
        """Full startup and run sequence"""
        try:
            if not self.initialize():
                logger.error("Initialization failed")
                return False
            
            self.start()
            self.run_main_loop()
            
        finally:
            self.shutdown()
        
        return True


def main():
    """Application entry point"""
    logger.info("GEMBOT starting...")
    
    robot = GEMBOTRobot()
    success = robot.run()
    
    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
