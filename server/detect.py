"""
YOLOv8 Object Detection module
Handles model loading and inference
"""

import cv2
import numpy as np
from typing import List, Dict, Any, Tuple, Optional
import logging

logger = logging.getLogger("detect")


class ObjectDetector:
    """YOLOv8 object detection engine"""
    
    def __init__(self, model_name: str = "yolov8n.pt", confidence: float = 0.5):
        """Initialize object detector"""
        self.model_name = model_name
        self.confidence_threshold = confidence
        self.model = None
        self.device = "cpu"
        self.load_model()
    
    def load_model(self):
        """Load YOLOv8 model"""
        try:
            from ultralytics import YOLO
            
            logger.info(f"Loading YOLOv8 model: {self.model_name}")
            self.model = YOLO(self.model_name)
            self.model.to(self.device)
            logger.info("Model loaded successfully")
            
        except ImportError:
            logger.error("Ultralytics not installed. Install with: pip install ultralytics")
            raise
        except Exception as e:
            logger.error(f"Failed to load model: {e}")
            raise
    
    def detect(self, image: np.ndarray, confidence: Optional[float] = None) -> List[Dict[str, Any]]:
        """
        Run detection on image
        Returns list of detections with format:
        [{'box': (x1,y1,x2,y2), 'label': 'person', 'confidence': 0.95}, ...]
        """
        if self.model is None:
            logger.error("Model not loaded")
            return []
        
        try:
            conf = confidence or self.confidence_threshold
            
            # Run inference
            results = self.model(image, conf=conf, verbose=False)
            
            detections = []
            if results and len(results) > 0:
                result = results[0]
                
                # Extract boxes and classes
                if result.boxes is not None:
                    for box in result.boxes:
                        # Get bounding box coordinates
                        x1, y1, x2, y2 = box.xyxy[0].cpu().numpy()
                        
                        # Get class and confidence
                        class_id = int(box.cls[0].item())
                        confidence_val = float(box.conf[0].item())
                        
                        # Get class name
                        class_name = result.names[class_id] if class_id in result.names else f"class_{class_id}"
                        
                        detection = {
                            'box': (float(x1), float(y1), float(x2), float(y2)),
                            'label': class_name,
                            'confidence': confidence_val,
                            'class_id': class_id
                        }
                        
                        detections.append(detection)
            
            logger.debug(f"Detected {len(detections)} objects")
            return detections
        
        except Exception as e:
            logger.error(f"Detection error: {e}")
            return []
    
    def detect_from_file(self, image_path: str, confidence: Optional[float] = None) -> List[Dict[str, Any]]:
        """Detect objects in image file"""
        try:
            image = cv2.imread(image_path)
            if image is None:
                logger.error(f"Failed to read image: {image_path}")
                return []
            
            return self.detect(image, confidence)
        
        except Exception as e:
            logger.error(f"File detection error: {e}")
            return []
    
    def visualize(self, image: np.ndarray, detections: List[Dict], 
                  thickness: int = 2, font_scale: float = 0.6) -> np.ndarray:
        """Draw bounding boxes on image"""
        output = image.copy()
        
        for detection in detections:
            try:
                box = detection['box']
                label = detection['label']
                confidence = detection['confidence']
                
                x1, y1, x2, y2 = map(int, box)
                
                # Draw rectangle
                cv2.rectangle(output, (x1, y1), (x2, y2), (0, 255, 0), thickness)
                
                # Draw label
                text = f"{label} {confidence:.2f}"
                text_size = cv2.getTextSize(text, cv2.FONT_HERSHEY_SIMPLEX, font_scale, thickness)[0]
                
                # Background for text
                cv2.rectangle(output, 
                            (x1, y1 - text_size[1] - 5),
                            (x1 + text_size[0], y1),
                            (0, 255, 0), -1)
                
                # Text
                cv2.putText(output, text, (x1, y1 - 5),
                           cv2.FONT_HERSHEY_SIMPLEX, font_scale, (0, 0, 0), thickness)
            
            except Exception as e:
                logger.warning(f"Error drawing detection: {e}")
        
        return output
    
    def get_model_info(self) -> Dict[str, Any]:
        """Get model information"""
        if self.model is None:
            return {}
        
        return {
            'model': self.model_name,
            'task': getattr(self.model, 'task', 'detect'),
            'device': self.device
        }
