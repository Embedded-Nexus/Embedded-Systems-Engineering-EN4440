import os
from flask import Blueprint, request, jsonify, send_file
from werkzeug.utils import secure_filename
from app.models.database import insert_firmware, get_firmware

FIRMWARE_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../firmware')
firmware_bp = Blueprint('firmware', __name__)

@firmware_bp.route('/firmware', methods=['POST'])
def upload_firmware():
    """
    POST /firmware
    Upload a new firmware binary file and update version
    Multipart/form-data:
      - file: binary file
      - version: semantic version string (e.g., "1.0.0")
      - level: update level (int)
    """
    if 'file' not in request.files:
        return jsonify({'status': 'error', 'message': 'No file part'}), 400
    file = request.files['file']
    if file.filename == '':
        return jsonify({'status': 'error', 'message': 'No selected file'}), 400
    
    # Get version from form (semantic version like "1.0.0")
    version = request.form.get('version', '').strip()
    if not version or not is_valid_semantic_version(version):
        return jsonify({'status': 'error', 'message': 'Invalid version format. Use semantic versioning (e.g., 1.0.0)'}), 400
    
    level = request.form.get('level', type=int)
    if not level or level < 1:
        return jsonify({'status': 'error', 'message': 'Invalid update level'}), 400
    
    filename = secure_filename(file.filename)
    save_path = os.path.join(FIRMWARE_DIR, filename)
    os.makedirs(FIRMWARE_DIR, exist_ok=True)
    file.save(save_path)
    
    # Insert firmware record and update config version
    new_version = insert_firmware(save_path, version, level)
    return jsonify({'status': 'success', 'message': 'Firmware uploaded', 'version': new_version, 'file_path': save_path, 'update_level': level}), 201

@firmware_bp.route('/firmware', methods=['GET'])
def get_firmware_file():
    """
    GET /firmware
    Returns the latest firmware binary file
    Header format: X-Config-Version = "version_level" (e.g., "1.0.0_2")
    """
    from app.models.database import get_config
    info = get_firmware()
    config = get_config()
    if not info or not info['file_path'] or not os.path.exists(info['file_path']):
        return jsonify({'status': 'error', 'message': 'Firmware file not found'}), 404
    
    # Add current config version and level to response headers
    response = send_file(info['file_path'], as_attachment=True)
    if config and 'version' in config and 'level' in config:
        # Format: "version_level" (e.g., "1.0.0_2")
        version_info = f"{config['version']}_{config['level']}"
        response.headers['X-Config-Version'] = version_info
    return response

@firmware_bp.route('/firmware/version', methods=['GET'])
def get_firmware_version():
    """
    GET /firmware/version
    Returns current firmware version without downloading the binary
    Response JSON:
      - version: semantic version string (e.g., "1.0.0")
      - level: update level (int)
      - version_string: combined format (e.g., "1.0.0_2")
    """
    firmware_info = get_firmware()
    if not firmware_info or 'version' not in firmware_info:
        return jsonify({'status': 'error', 'message': 'No firmware config found'}), 404
    
    return jsonify({
        'status': 'success',
        'version': firmware_info['version'],
        'level': firmware_info['update_level'],
        'version_string': f"{firmware_info['version']}_{firmware_info['update_level']}"
    }), 200


def is_valid_semantic_version(version_str):
    """
    Validate semantic version format (e.g., 1.0.0)
    Format: MAJOR.MINOR.PATCH
    """
    parts = version_str.split('.')
    if len(parts) != 3:
        return False
    try:
        for part in parts:
            int(part)  # Must be numeric
        return True
    except ValueError:
        return False
