class Config:
    """Base configuration"""
    SECRET_KEY = 'dev-secret-key-change-in-production'
    DATABASE = 'cloud_data.db'
    LOG_LEVEL = 'INFO'  # Default logging level
    LOG_TO_FILE = True # Set to True to log to file
    LOG_FILE_PATH = 'server.log'

class DevelopmentConfig(Config):
    """Development configuration"""
    DEBUG = True
    LOG_LEVEL = 'DEBUG'

class ProductionConfig(Config):
    """Production configuration"""
    DEBUG = False
    LOG_LEVEL = 'INFO'

# Default config
config = DevelopmentConfig()
