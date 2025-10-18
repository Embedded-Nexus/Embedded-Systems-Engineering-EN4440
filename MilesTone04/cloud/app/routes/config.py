from flask import jsonify, request
from app.routes import config_bp
from app.models.database import get_config, update_config

@config_bp.route('/config', methods=['GET'])
def get_device_config():
    """
    GET /config
    Returns current configuration for the device
    """
    try:
        config = get_config()
        if config:
            return jsonify({
                'status': 'success',
                'config': config
            }), 200
        else:
            return jsonify({
                'status': 'error',
                'message': 'Configuration not found'
            }), 404
    except Exception as e:
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 500

@config_bp.route('/config', methods=['POST'])
def update_device_config():
    """
    POST /config
    Updates configuration
    Expected JSON body:
    {
        "reg_read": [1, 0, 1, 0, 1, 0, 1, 0, 1, 0],  // optional
        "interval": 2000  // optional
    }
    """
    try:
        data = request.get_json()
        
        if not data:
            return jsonify({
                'status': 'error',
                'message': 'No data provided'
            }), 400
        
        reg_read = data.get('reg_read')
        interval = data.get('interval')
        
        # Validate reg_read if provided
        if reg_read is not None:
            if not isinstance(reg_read, list) or len(reg_read) != 10:
                return jsonify({
                    'status': 'error',
                    'message': 'reg_read must be a list of 10 elements'
                }), 400
            if not all(x in [0, 1] for x in reg_read):
                return jsonify({
                    'status': 'error',
                    'message': 'reg_read elements must be 0 or 1'
                }), 400
        
        # Validate interval if provided
        if interval is not None:
            if not isinstance(interval, int) or interval <= 0:
                return jsonify({
                    'status': 'error',
                    'message': 'interval must be a positive integer'
                }), 400
        
        # Update config
        success = update_config(reg_read=reg_read, interval=interval)
        
        if success:
            new_config = get_config()
            return jsonify({
                'status': 'success',
                'message': 'Configuration updated successfully',
                'config': new_config
            }), 200
        else:
            return jsonify({
                'status': 'error',
                'message': 'Failed to update configuration'
            }), 500
            
    except Exception as e:
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 500
