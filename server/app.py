#!/usr/bin/env python3
"""
GEMBOT AI Server
FastAPI server for YOLOv8 object detection
"""

import logging
import io
import cv2
import numpy as np
from fastapi import FastAPI, UploadFile, File, Query
from fastapi.responses import JSONResponse
from fastapi.middleware.cors import CORSMiddleware

from detect import ObjectDetector

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger("api")

# Initialize FastAPI app
app = FastAPI(title="GEMBOT AI Server", version="1.0.0")

# Add CORS middleware
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Initialize detector
detector = None


@app.on_event("startup")
def startup_event():
    """Initialize on startup"""
    global detector
    logger.info("GEMBOT AI Server starting...")
    
    try:
        detector = ObjectDetector(model_name="yolov8n.pt", confidence=0.5)
        logger.info("ObjectDetector initialized")
    except Exception as e:
        logger.error(f"Failed to initialize detector: {e}")


@app.get("/health")
def health_check():
    """Health check endpoint"""
    return {
        "status": "healthy",
        "service": "GEMBOT AI Server",
        "detector_ready": detector is not None
    }


@app.get("/info")
def server_info():
    """Get server information"""
    if detector is None:
        return {"error": "Detector not initialized"}
    
    return {
        "service": "GEMBOT AI Server",
        "version": "1.0.0",
        "model_info": detector.get_model_info()
    }


@app.post("/detect")
def detect_objects(
    image: UploadFile = File(...),
    confidence: float = Query(0.5, ge=0, le=1)
):
    """
    Detect objects in uploaded image
    
    Parameters:
    - image: Image file (JPEG/PNG)
    - confidence: Detection confidence threshold (0-1)
    
    Returns:
    - detections: List of detected objects with bounding boxes
    """
    try:
        if detector is None:
            return JSONResponse(
                status_code=500,
                content={"error": "Detector not initialized"}
            )
        
        # Read image
        image_data = image.file.read()
        nparr = np.frombuffer(image_data, np.uint8)
        frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
        
        if frame is None:
            return JSONResponse(
                status_code=400,
                content={"error": "Invalid image"}
            )
        
        logger.debug(f"Processing image: {frame.shape}")
        
        # Run detection
        detections = detector.detect(frame, confidence=confidence)
        
        logger.info(f"Detection complete: {len(detections)} objects found")
        
        return {
            "success": True,
            "detection_count": len(detections),
            "detections": detections,
            "image_shape": frame.shape
        }
    
    except Exception as e:
        logger.error(f"Detection error: {e}")
        return JSONResponse(
            status_code=500,
            content={"error": str(e)}
        )


@app.post("/detect_and_visualize")
def detect_and_visualize(
    image: UploadFile = File(...),
    confidence: float = Query(0.5, ge=0, le=1)
):
    """
    Detect objects and return annotated image
    
    Returns:
    - Image with bounding boxes drawn
    """
    try:
        if detector is None:
            return JSONResponse(
                status_code=500,
                content={"error": "Detector not initialized"}
            )
        
        # Read image
        image_data = image.file.read()
        nparr = np.frombuffer(image_data, np.uint8)
        frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
        
        if frame is None:
            return JSONResponse(
                status_code=400,
                content={"error": "Invalid image"}
            )
        
        # Run detection
        detections = detector.detect(frame, confidence=confidence)
        
        # Visualize
        annotated_frame = detector.visualize(frame, detections)
        
        # Encode to JPEG
        _, buffer = cv2.imencode('.jpg', annotated_frame)
        image_bytes = buffer.tobytes()
        
        logger.info(f"Visualized detection: {len(detections)} objects")
        
        return {
            "success": True,
            "detection_count": len(detections),
            "detections": detections
        }
    
    except Exception as e:
        logger.error(f"Visualization error: {e}")
        return JSONResponse(
            status_code=500,
            content={"error": str(e)}
        )


@app.get("/")
def root():
    """Root endpoint"""
    return {
        "name": "GEMBOT AI Server",
        "version": "1.0.0",
        "endpoints": {
            "/health": "Health check",
            "/info": "Server information",
            "/detect": "Detect objects in image (POST)",
            "/detect_and_visualize": "Detect and annotate image (POST)"
        }
    }


if __name__ == "__main__":
    import uvicorn
    
    logger.info("Starting GEMBOT AI Server...")
    logger.info("Server running on http://0.0.0.0:5000")
    
    uvicorn.run(
        app,
        host="0.0.0.0",
        port=5000,
        log_level="info"
    )
