"""
Video streaming module for GEMBOT
Provides MJPEG streaming for web dashboard
"""

import cv2
import threading
import time
from typing import Optional, Callable
from ..utils import get_logger, get_config

logger = get_logger("vision.stream")


class StreamProcessor:
    """Handles video streaming with optional frame processing"""
    
    def __init__(self, camera):
        """Initialize stream processor"""
        self.camera = camera
        self.config = get_config()
        self.stream_quality = self.config.get('dashboard.stream_quality', 80)
        self.stream_fps = self.config.get('dashboard.fps', 15)
        self.frame_processors = []
        self.is_streaming = False
        self.stream_lock = threading.Lock()
        self.last_stream_frame = None
        
    def add_frame_processor(self, processor_func: Callable):
        """
        Add a frame processing function
        processor_func should accept and return a frame
        """
        self.frame_processors.append(processor_func)
        logger.info(f"Added frame processor: {processor_func.__name__}")
    
    def _process_frame(self, frame):
        """Apply all registered frame processors"""
        processed = frame.copy() if frame is not None else None
        
        for processor in self.frame_processors:
            try:
                if processed is not None:
                    processed = processor(processed)
            except Exception as e:
                logger.error(f"Frame processor error: {e}")
        
        return processed
    
    def start_streaming(self):
        """Start continuous stream generation"""
        if self.is_streaming:
            logger.warning("Streaming already active")
            return
        
        self.is_streaming = True
        stream_thread = threading.Thread(target=self._streaming_loop, daemon=True)
        stream_thread.start()
        logger.info("Stream processing started")
    
    def _streaming_loop(self):
        """Continuous streaming loop"""
        frame_interval = 1.0 / self.stream_fps
        last_frame_time = time.time()
        
        while self.is_streaming:
            try:
                # Frame rate control
                current_time = time.time()
                if current_time - last_frame_time < frame_interval:
                    time.sleep(frame_interval - (current_time - last_frame_time))
                
                # Get and process frame
                frame = self.camera.get_frame()
                if frame is not None:
                    processed_frame = self._process_frame(frame)
                    
                    with self.stream_lock:
                        self.last_stream_frame = processed_frame
                
                last_frame_time = time.time()
                
            except Exception as e:
                logger.error(f"Error in streaming loop: {e}")
                time.sleep(0.1)
    
    def get_mjpeg_frame(self, quality: Optional[int] = None) -> Optional[bytes]:
        """Get current frame as MJPEG bytes"""
        try:
            quality = quality or self.stream_quality
            
            with self.stream_lock:
                frame = self.last_stream_frame
            
            if frame is None:
                return None
            
            _, buffer = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, quality])
            return buffer.tobytes()
            
        except Exception as e:
            logger.error(f"MJPEG encoding error: {e}")
            return None
    
    def draw_bounding_boxes(self, detections: list) -> Callable:
        """
        Create a frame processor that draws bounding boxes
        detections format: [{'box': (x1,y1,x2,y2), 'label': 'person', 'confidence': 0.95}, ...]
        """
        def processor(frame):
            if frame is None:
                return frame
            
            for detection in detections:
                try:
                    box = detection.get('box')
                    label = detection.get('label', 'unknown')
                    confidence = detection.get('confidence', 0.0)
                    
                    x1, y1, x2, y2 = box
                    
                    # Draw rectangle
                    cv2.rectangle(frame, (int(x1), int(y1)), (int(x2), int(y2)), 
                                 (0, 255, 0), 2)
                    
                    # Draw label with background
                    text = f"{label} {confidence:.2f}"
                    text_size = cv2.getTextSize(text, cv2.FONT_HERSHEY_SIMPLEX, 0.6, 2)[0]
                    
                    cv2.rectangle(frame, 
                                 (int(x1), int(y1) - text_size[1] - 5),
                                 (int(x1) + text_size[0], int(y1)),
                                 (0, 255, 0), -1)
                    
                    cv2.putText(frame, text, (int(x1), int(y1) - 5),
                               cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 0), 2)
                    
                except Exception as e:
                    logger.warning(f"Error drawing box: {e}")
            
            return frame
        
        return processor
    
    def stop_streaming(self):
        """Stop stream processing"""
        self.is_streaming = False
        logger.info("Stream processing stopped")
