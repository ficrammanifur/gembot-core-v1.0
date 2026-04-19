"""
MQTT client for GEMBOT
Handles pub/sub communication with message broker
"""

import paho.mqtt.client as mqtt
import json
import threading
import time
from typing import Callable, Dict, Any
from ..utils import get_logger, get_config

logger = get_logger("comm.mqtt")


class MQTTClient:
    """MQTT client for distributed communication"""
    
    def __init__(self):
        """Initialize MQTT client"""
        self.config = get_config()
        self.broker = self.config.get('mqtt.broker', 'localhost')
        self.port = self.config.get('mqtt.port', 1883)
        self.username = self.config.get('mqtt.username', '')
        self.password = self.config.get('mqtt.password', '')
        self.keepalive = self.config.get('mqtt.keepalive', 60)
        
        self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)
        self.is_connected = False
        self.subscriptions = {}
        self.connect_retries = 0
        self.max_retries = 5
        
        # Set callbacks
        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect
        self.client.on_message = self._on_message
        
        logger.info(f"MQTT client initialized: {self.broker}:{self.port}")
    
    def connect(self) -> bool:
        """Connect to MQTT broker"""
        try:
            if self.username:
                self.client.username_pw_set(self.username, self.password)
            
            self.client.connect(self.broker, self.port, self.keepalive)
            self.client.loop_start()
            
            # Wait for connection
            for _ in range(20):
                if self.is_connected:
                    logger.info("Connected to MQTT broker")
                    return True
                time.sleep(0.1)
            
            logger.error("MQTT connection timeout")
            return False
            
        except Exception as e:
            logger.error(f"MQTT connection failed: {e}")
            return False
    
    def _on_connect(self, client, userdata, flags, rc):
        """MQTT connection callback"""
        if rc == 0:
            self.is_connected = True
            self.connect_retries = 0
            logger.info("MQTT connected successfully")
            
            # Resubscribe to all topics
            for topic in self.subscriptions.keys():
                client.subscribe(topic)
        else:
            logger.error(f"MQTT connection failed with code {rc}")
            self.is_connected = False
    
    def _on_disconnect(self, client, userdata, rc):
        """MQTT disconnection callback"""
        self.is_connected = False
        if rc != 0:
            logger.warning(f"Unexpected MQTT disconnection: {rc}")
    
    def _on_message(self, client, userdata, msg):
        """MQTT message received callback"""
        try:
            topic = msg.topic
            payload = msg.payload.decode()
            
            # Call registered callback
            if topic in self.subscriptions:
                callback = self.subscriptions[topic]
                try:
                    data = json.loads(payload)
                except:
                    data = payload
                
                callback(data)
            
        except Exception as e:
            logger.error(f"Error processing message: {e}")
    
    def publish(self, topic: str, data: Any, qos: int = 0) -> bool:
        """Publish message to topic"""
        try:
            if not self.is_connected:
                logger.warning("MQTT not connected, queuing message")
                return False
            
            payload = json.dumps(data) if isinstance(data, dict) else str(data)
            result = self.client.publish(topic, payload, qos=qos)
            
            if result.rc != mqtt.MQTT_ERR_SUCCESS:
                logger.error(f"Publish failed: {result.rc}")
                return False
            
            return True
            
        except Exception as e:
            logger.error(f"Publish error: {e}")
            return False
    
    def subscribe(self, topic: str, callback: Callable, qos: int = 0) -> bool:
        """Subscribe to topic with callback"""
        try:
            self.subscriptions[topic] = callback
            
            if self.is_connected:
                result = self.client.subscribe(topic, qos=qos)
                if result[0] == mqtt.MQTT_ERR_SUCCESS:
                    logger.info(f"Subscribed to {topic}")
                    return True
                else:
                    logger.error(f"Subscribe failed: {result[0]}")
                    return False
            else:
                logger.warning(f"MQTT not connected, will subscribe on connect: {topic}")
                return True
                
        except Exception as e:
            logger.error(f"Subscribe error: {e}")
            return False
    
    def unsubscribe(self, topic: str) -> bool:
        """Unsubscribe from topic"""
        try:
            if topic in self.subscriptions:
                del self.subscriptions[topic]
            
            if self.is_connected:
                result = self.client.unsubscribe(topic)
                if result[0] == mqtt.MQTT_ERR_SUCCESS:
                    logger.info(f"Unsubscribed from {topic}")
                    return True
            
            return True
            
        except Exception as e:
            logger.error(f"Unsubscribe error: {e}")
            return False
    
    def publish_status(self, status: Dict[str, Any]) -> bool:
        """Publish robot status"""
        return self.publish('gembot/status', status)
    
    def disconnect(self):
        """Disconnect from MQTT broker"""
        try:
            self.client.loop_stop()
            self.client.disconnect()
            self.is_connected = False
            logger.info("MQTT disconnected")
        except Exception as e:
            logger.error(f"Disconnect error: {e}")
