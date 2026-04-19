"""
Configuration loader for GEMBOT
Loads and manages YAML configuration files
"""

import yaml
from pathlib import Path
from typing import Any, Dict, Optional


class ConfigLoader:
    """Load and manage configuration from YAML files"""
    
    _instance = None
    
    def __new__(cls):
        if cls._instance is None:
            cls._instance = super().__new__(cls)
            cls._instance._initialized = False
        return cls._instance
    
    def __init__(self):
        if self._initialized:
            return
        
        self._initialized = True
        self.config = {}
        self._load_config()
    
    def _load_config(self):
        """Load configuration from YAML file"""
        config_path = Path(__file__).parent.parent.parent / "config" / "config.yaml"
        
        try:
            with open(config_path, 'r') as f:
                self.config = yaml.safe_load(f)
            if self.config is None:
                self.config = {}
        except FileNotFoundError:
            raise FileNotFoundError(f"Config file not found: {config_path}")
        except yaml.YAMLError as e:
            raise ValueError(f"Invalid YAML config: {e}")
    
    def get(self, key: str, default: Any = None) -> Any:
        """
        Get configuration value using dot notation
        Example: config.get('mqtt.broker') returns broker IP
        """
        keys = key.split('.')
        value = self.config
        
        for k in keys:
            if isinstance(value, dict):
                value = value.get(k, default)
            else:
                return default
        
        return value if value is not None else default
    
    def get_dict(self, key: str) -> Dict[str, Any]:
        """Get entire dictionary section"""
        return self.get(key, {})
    
    def reload(self):
        """Reload configuration from file"""
        self._load_config()


# Global config instance
def get_config() -> ConfigLoader:
    """Get global config instance"""
    return ConfigLoader()
