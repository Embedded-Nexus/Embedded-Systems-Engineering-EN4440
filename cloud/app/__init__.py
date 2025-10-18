from flask import Flask, request
from app.config.settings import config
from app.models.database import init_db, close_db

def create_app():

    from app.config.logging_config import setup_logging
    app = Flask(__name__)
    app.config.from_object(config)
    # Setup logging
    setup_logging(
        log_level=app.config.get('LOG_LEVEL', 'INFO'),
        log_to_file=app.config.get('LOG_TO_FILE', False),
        log_file_path=app.config.get('LOG_FILE_PATH', 'server.log')
    )
    import logging
    logger = logging.getLogger('cloud.server')

    # CORS preflight handler (must be after app is defined)
    @app.after_request
    def add_cors_headers(response):
        response.headers['Access-Control-Allow-Origin'] = '*'
        response.headers['Access-Control-Allow-Methods'] = 'GET, POST, OPTIONS'
        response.headers['Access-Control-Allow-Headers'] = 'Content-Type'
        return response

    @app.route('/<path:path>', methods=['OPTIONS'])
    @app.route('/', methods=['OPTIONS'])
    def options_handler(path=None):
        response = app.response_class()
        response.status_code = 200
        response.headers['Access-Control-Allow-Origin'] = '*'
        response.headers['Access-Control-Allow-Methods'] = 'GET, POST, OPTIONS'
        response.headers['Access-Control-Allow-Headers'] = 'Content-Type'
        logger.info(f'OPTIONS request for {request.path}')
        return response

    # Initialize database
    with app.app_context():
        logger.info('Initializing database...')
        init_db()
        logger.info('Database initialized.')

    # Register teardown function
    app.teardown_appcontext(close_db)
    logger.info('App teardown registered.')

    # Register blueprints
    from app.routes.config import config_bp
    from app.routes.data import data_bp
    from app.routes.firmware import firmware_bp
    from app.routes.commands import commands_bp
    app.register_blueprint(config_bp)
    app.register_blueprint(data_bp)
    app.register_blueprint(firmware_bp)
    app.register_blueprint(commands_bp)
    logger.info('Blueprints registered.')

    # Health check endpoint
    @app.route('/')
    def index():
        logger.info('Health check (/) endpoint called.')
        return {
            'status': 'running',
            'message': 'Cloud Server API is running',
            'endpoints': {
                'config': {
                    'GET /config': 'Get current configuration',
                    'POST /config': 'Update configuration'
                },
                'data': {
                    'POST /data': 'Submit sensor data',
                    'GET /data': 'Get latest or ranged data',
                    'GET /data/count': 'Get data count'
                },
                'firmware': {
                    'POST /firmware': 'Upload new firmware',
                    'GET /firmware': 'Download latest firmware'
                },
                'commands': {
                    'GET /commands': 'Get queued commands',
                    'POST /commands': 'Queue or clear commands'
                }
            }
        }

    @app.route('/health')
    def health():
        logger.info('Health check (/health) endpoint called.')
        return {'status': 'healthy'}, 200

    return app
