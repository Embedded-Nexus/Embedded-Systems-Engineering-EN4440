from flask import jsonify, request
from app.routes import data_bp
from app.models.database import (
    insert_data,
    insert_data_batch,
    get_latest_data, 
    get_data_by_id_range, 
    get_data_by_timestamp_range,
    get_data_count
)
from app.utils.compressor import process_compressed_data, reset_sequence_counter

@data_bp.route('/data', methods=['POST'])
def receive_data():
    """
    POST /data
    Receives data from the embedded device
    
    Supports two formats:
    
    1. JSON format (legacy):
    {
        "timestamp": "2025-10-18T12:00:00",
        "data": [100, 200, -1, 150, -1, 300, 250, -1, 180, 220]
    }
    
    2. Binary format (compressed):
    Content-Type: application/octet-stream
    Raw compressed bytes (TimeSeriesCompressor format)
    """
    try:
        # Check if request contains binary data (compressed)
        if request.content_type == 'application/octet-stream' or \
           (request.data and not request.is_json):
            # Read raw bytes
            raw_bytes = request.data
            
            if not raw_bytes:
                return jsonify({
                    'status': 'error',
                    'message': 'No binary data provided'
                }), 400
            
            # Get number of registers and optional key from query parameters
            regs = request.args.get('regs', default=10, type=int)
            # key parameter is optional: if provided, uses legacy XOR decryption
            # if omitted (None), uses new authenticated encryption
            key = request.args.get('key', default=None, type=int)
            reset_seq = request.args.get('reset_seq', default='false').lower() == 'true'
            
            # Reset sequence counter if requested (use when device restarts)
            if reset_seq:
                reset_sequence_counter()
                print("[Data Route] Sequence counter reset")
            
            print(f"[Data Route] Received {len(raw_bytes)} bytes, regs={regs}")
            
            # Process compressed data (decrypt + decompress + decode)
            snapshots = process_compressed_data(raw_bytes, regs, key)
            
            if not snapshots:
                return jsonify({
                    'status': 'error',
                    'message': 'Failed to decompress/decode data (likely decryption failure or replay attack)',
                    'packet_size': len(raw_bytes),
                    'hint': 'If device restarted, use ?reset_seq=true'
                }), 400
            
            # Insert all snapshots to database
            count = insert_data_batch(snapshots)
            
            return jsonify({
                'status': 'success',
                'message': f'Compressed data received and processed successfully',
                'snapshots_count': count,
                'snapshots': snapshots
            }), 201
        
        # Otherwise, handle JSON format (legacy)
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
