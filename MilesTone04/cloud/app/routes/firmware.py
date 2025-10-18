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
      - level: update level (int)
    """
    if 'file' not in request.files:
        return jsonify({'status': 'error', 'message': 'No file part'}), 400
    file = request.files['file']
    if file.filename == '':
        return jsonify({'status': 'error', 'message': 'No selected file'}), 400
    level = request.form.get('level', type=int)
    if not level or level < 1:
        return jsonify({'status': 'error', 'message': 'Invalid update level'}), 400
    filename = secure_filename(file.filename)
    save_path = os.path.join(FIRMWARE_DIR, filename)
    os.makedirs(FIRMWARE_DIR, exist_ok=True)
    file.save(save_path)
    # Insert firmware record and update config version
    new_version = insert_firmware(save_path, level)
    return jsonify({'status': 'success', 'message': 'Firmware uploaded', 'version': new_version, 'file_path': save_path, 'update_level': level}), 201

@firmware_bp.route('/firmware', methods=['GET'])
def get_firmware_file():
    """
    GET /firmware
    Returns the latest firmware binary file
    """
    from app.models.database import get_config
    info = get_firmware()
    config = get_config()
    if not info or not info['file_path'] or not os.path.exists(info['file_path']):
        return jsonify({'status': 'error', 'message': 'Firmware file not found'}), 404
    # Add current config version to response headers
    response = send_file(info['file_path'], as_attachment=True)
    if config and 'version' in config:
        response.headers['X-Config-Version'] = config['version']
    return response
