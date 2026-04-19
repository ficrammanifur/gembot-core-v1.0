"""Vision and detection modules for GEMBOT"""

from .camera import Camera
from .stream import StreamProcessor
from .detect_client import DetectionClient

__all__ = ['Camera', 'StreamProcessor', 'DetectionClient']
