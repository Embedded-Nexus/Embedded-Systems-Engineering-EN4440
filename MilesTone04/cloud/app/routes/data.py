from flask import jsonify, request
from app.routes import data_bp
from app.models.database import (
    insert_data, 
    get_latest_data, 
    get_data_by_id_range, 
    get_data_by_timestamp_range,
    get_data_count
)

@data_bp.route('/data', methods=['POST'])
def receive_data():
    """
    POST /data
    Receives data from the embedded device
    Expected JSON body:
    {
        "timestamp": "2025-10-18T12:00:00",
        "data": [100, 200, -1, 150, -1, 300, 250, -1, 180, 220]
    }
    """
    try:
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
