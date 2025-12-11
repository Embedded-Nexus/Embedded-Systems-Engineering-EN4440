from flask import jsonify, request
from app.routes import data_bp
from app.models.database import (
    insert_data, 
    get_latest_data, 
    get_data_by_id_range, 
    get_data_by_timestamp_range,
    get_data_count
)
from app.utils.security import decrypt_buffer
from datetime import datetime
import struct

@data_bp.route('/data', methods=['POST'])
def receive_data():
    """
    POST /data
    Receives data from the embedded device
    
    Supports two formats:
    1. Binary encrypted data (Content-Type: application/octet-stream)
       - Sends raw encrypted payload from security layer
       - Will be decrypted, decompressed, and stored
    
    2. JSON format (Content-Type: application/json):
       {
           "timestamp": "2025-10-18T12:00:00",
           "data": [100, 200, -1, 150, -1, 300, 250, -1, 180, 220]
       }
    """
    try:
        content_type = request.content_type or ""
        
        # ========== Handle Binary Encrypted Data ==========
        if "application/octet-stream" in content_type:
            encrypted_data = request.get_data()
            
            if not encrypted_data:
                return jsonify({
                    'status': 'error',
                    'message': 'No binary data provided'
                }), 400
            
            print(f"[Data Route] Received {len(encrypted_data)} bytes of encrypted data")
            print(f"[Data Route] Hex: {encrypted_data.hex()}")
            
            # Decrypt the data
            decrypted = decrypt_buffer(encrypted_data)
            if decrypted is None:
                return jsonify({
                    'status': 'error',
                    'message': 'Decryption failed or authentication error'
                }), 400
            
            print(f"[Data Route] Decrypted {len(decrypted)} bytes")
            print(f"[Data Route] Decrypted hex: {decrypted.hex()}")
            
            # TODO: Decompress and parse the decrypted data
            # For now, store raw decrypted data with current timestamp
            timestamp = datetime.utcnow().isoformat()
            
            # Parse decrypted data - expecting 10 register values (2 bytes each)
            if len(decrypted) >= 20:
                # Extract 10 16-bit values (little-endian)
                data = []
                for i in range(10):
                    offset = i * 2
                    val = struct.unpack('<h', decrypted[offset:offset+2])[0]
                    data.append(val)
                
                print(f"[Data Route] Parsed registers: {data}")
                
                # Insert data
                record_id = insert_data(timestamp, data)
                
                return jsonify({
                    'status': 'success',
                    'message': 'Binary data received and decrypted successfully',
                    'id': record_id,
                    'decrypted_size': len(decrypted)
                }), 201
            else:
                return jsonify({
                    'status': 'error',
                    'message': f'Decrypted data too small: {len(decrypted)} bytes (need >= 20)'
                }), 400
        
        # ========== Handle JSON Data ==========
        else:
            payload = request.get_json()
            
            if not payload:
                return jsonify({
                    'status': 'error',
                    'message': 'No data provided'
                }), 400
            
            timestamp = payload.get('timestamp')
            data = payload.get('data')
            
            # Validate inputs
            if not timestamp:
                return jsonify({
                    'status': 'error',
                    'message': 'timestamp is required'
                }), 400
            
            if not data or not isinstance(data, list) or len(data) != 10:
                return jsonify({
                    'status': 'error',
                    'message': 'data must be a list of 10 elements'
                }), 400
            
            # Insert data
            record_id = insert_data(timestamp, data)
            
            return jsonify({
                'status': 'success',
                'message': 'Data received successfully',
                'id': record_id
            }), 201
        
    except Exception as e:
        print(f"[Data Route] Exception: {str(e)}")
        import traceback
        traceback.print_exc()
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 500

@data_bp.route('/data', methods=['GET'])
def get_data():
    """
    GET /data
    Returns data based on query parameters
    
    Query parameters:
    - No params: Returns latest value for each register
    - start_id & end_id: Returns data in ID range
    - start_time & end_time: Returns data in timestamp range
    
    Examples:
    - GET /data (latest values)
    - GET /data?start_id=1&end_id=100
    - GET /data?start_time=2025-10-18T00:00:00&end_time=2025-10-18T23:59:59
    """
    try:
        start_id = request.args.get('start_id', type=int)
        end_id = request.args.get('end_id', type=int)
        start_time = request.args.get('start_time')
        end_time = request.args.get('end_time')
        
        # Case 1: ID range query
        if start_id is not None and end_id is not None:
            if start_id < 0 or end_id < start_id:
                return jsonify({
                    'status': 'error',
                    'message': 'Invalid ID range'
                }), 400
            
            data = get_data_by_id_range(start_id, end_id)
            return jsonify({
                'status': 'success',
                'count': len(data),
                'data': data
            }), 200
        
        # Case 2: Timestamp range query
        elif start_time is not None and end_time is not None:
            data = get_data_by_timestamp_range(start_time, end_time)
            return jsonify({
                'status': 'success',
                'count': len(data),
                'data': data
            }), 200
        
        # Case 3: Latest values (default)
        else:
            data = get_latest_data()
            return jsonify({
                'status': 'success',
                'data': data
            }), 200
            
    except Exception as e:
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 500

@data_bp.route('/data/count', methods=['GET'])
def count_data():
    """
    GET /data/count
    Returns the total count of data records
    """
    try:
        count = get_data_count()
        return jsonify({
            'status': 'success',
            'count': count
        }), 200
    except Exception as e:
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 500
