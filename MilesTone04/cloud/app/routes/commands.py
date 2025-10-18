import json
from flask import Blueprint, request, jsonify
from app.models.database import queue_command, get_queued_commands, clear_command_queue

commands_bp = Blueprint('commands', __name__)

@commands_bp.route('/commands', methods=['GET'])
def get_commands():
    """
    GET /commands
    Returns all queued commands
    """
    commands = get_queued_commands()
    return jsonify(commands), 200

@commands_bp.route('/commands', methods=['POST'])
def post_command():
    """
    POST /commands
    Accepts a command to queue or a result to clear queue
    """
    data = request.get_json()
    if not data:
        return jsonify({'status': 'error', 'message': 'No data provided'}), 400
    if 'action' in data and data['action'] == 'write_register':
        # Queue the command
        queue_command(data)
        return jsonify({'status': 'success', 'message': 'Command queued'}), 201
    elif 'command_result' in data:
        # Clear the queue
        clear_command_queue()
        return jsonify({'status': 'success', 'message': 'Command queue cleared'}), 200
    else:
        return jsonify({'status': 'error', 'message': 'Invalid command format'}), 400
