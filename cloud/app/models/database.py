import sqlite3
import json
from datetime import datetime
from flask import g
import os

DATABASE = 'cloud_data.db'

def get_db():
    """Get database connection from Flask's g object"""
    db = getattr(g, '_database', None)
    if db is None:
        db = g._database = sqlite3.connect(DATABASE)
        db.row_factory = sqlite3.Row
    return db

def close_db(e=None):
    """Close database connection"""
    db = getattr(g, '_database', None)
    if db is not None:
        db.close()

def init_db():
    """Initialize the database with tables"""
    db = sqlite3.connect(DATABASE)
    cursor = db.cursor()
    
    # Create config table
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS config (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            reg_read TEXT NOT NULL,
            interval INTEGER NOT NULL,
            version TEXT NOT NULL,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    ''')
    
    # Create data table
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS sensor_data (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            reg_0 INTEGER,
            reg_1 INTEGER,
            reg_2 INTEGER,
            reg_3 INTEGER,
            reg_4 INTEGER,
            reg_5 INTEGER,
            reg_6 INTEGER,
            reg_7 INTEGER,
            reg_8 INTEGER,
            reg_9 INTEGER,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    ''')

    # Create firmware table
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS firmware (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_path TEXT NOT NULL,
            update_level INTEGER NOT NULL,
            version TEXT NOT NULL,
            uploaded_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    ''')

    # Create commands table
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS commands (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            command_json TEXT NOT NULL
        )
    ''')
    
    # Insert default config if not exists
    cursor.execute('SELECT COUNT(*) FROM config')
    if cursor.fetchone()[0] == 0:
        default_reg_read = json.dumps([1, 1, 1, 1, 1, 1, 1, 1, 1, 1])
        cursor.execute(
            'INSERT INTO config (reg_read, interval, version) VALUES (?, ?, ?)',
            (default_reg_read, 1000, '1.0.0')
        )

    # Insert default firmware if not exists
    cursor.execute('SELECT COUNT(*) FROM firmware')
    if cursor.fetchone()[0] == 0:
        cursor.execute(
            'INSERT INTO firmware (file_path, update_level, version) VALUES (?, ?, ?)',
            ('', 1, '1.0.0')
        )
# Command queue operations
def queue_command(command):
    db = get_db()
    cursor = db.cursor()
    timestamp = datetime.utcnow().isoformat()
    cursor.execute(
        'INSERT INTO commands (timestamp, command_json) VALUES (?, ?)',
        (timestamp, json.dumps(command))
    )
    db.commit()

def get_queued_commands():
    db = get_db()
    cursor = db.cursor()
    cursor.execute('SELECT timestamp, command_json FROM commands ORDER BY id ASC')
    rows = cursor.fetchall()
    commands = []
    for row in rows:
        commands.append({
            'timestamp': row['timestamp'],
            'command': json.loads(row['command_json'])
        })
    return {
        'timestamp': datetime.utcnow().isoformat(),
        'commands': [c['command'] for c in commands]
    }

def clear_command_queue():
    db = get_db()
    cursor = db.cursor()
    cursor.execute('DELETE FROM commands')
    db.commit()
    
    db.commit()
    db.close()

# Configuration operations
def get_config():
    """Get current configuration"""
    db = get_db()
    cursor = db.cursor()
    cursor.execute('SELECT reg_read, interval, version FROM config ORDER BY id DESC LIMIT 1')
    row = cursor.fetchone()
    if row:
        return {
            'reg_read': json.loads(row['reg_read']),
            'interval': row['interval'],
            'version': row['version']
        }
    return None

def get_firmware():
    """Get latest firmware info"""
    db = get_db()
    cursor = db.cursor()
    cursor.execute('SELECT file_path, update_level, version FROM firmware ORDER BY id DESC LIMIT 1')
    row = cursor.fetchone()
    if row:
        return {
            'file_path': row['file_path'],
            'update_level': row['update_level'],
            'version': row['version']
        }
    return None

def insert_firmware(file_path, update_level):
    """Insert new firmware and update config version"""
    db = get_db()
    cursor = db.cursor()
    # Get current firmware version
    cursor.execute('SELECT version FROM firmware ORDER BY id DESC LIMIT 1')
    row = cursor.fetchone()
    current_version = row['version'] if row else '1.0.0'
    # Increment version (last digit)
    version_parts = current_version.split('.')
    version_parts[-1] = str(int(version_parts[-1]) + 1)
    new_version = '.'.join(version_parts)
    # Insert new firmware record
    cursor.execute(
        'INSERT INTO firmware (file_path, update_level, version) VALUES (?, ?, ?)',
        (file_path, update_level, new_version)
    )
    # Also update config version
    cursor.execute('SELECT reg_read, interval FROM config ORDER BY id DESC LIMIT 1')
    config_row = cursor.fetchone()
    reg_read = json.loads(config_row['reg_read']) if config_row else [1]*10
    interval = config_row['interval'] if config_row else 1000
    cursor.execute(
        'INSERT INTO config (reg_read, interval, version) VALUES (?, ?, ?)',
        (json.dumps(reg_read), interval, new_version)
    )
    db.commit()
    return new_version

def update_config(reg_read=None, interval=None):
    """Update configuration"""
    db = get_db()
    cursor = db.cursor()
    
    # Get current config
    current_config = get_config()
    if not current_config:
        return False
    
    # Update with new values if provided
    new_reg_read = reg_read if reg_read is not None else current_config['reg_read']
    new_interval = interval if interval is not None else current_config['interval']
    
    # Keep version unchanged
    current_version = current_config['version']
    # Insert new config with same version
    cursor.execute(
        'INSERT INTO config (reg_read, interval, version) VALUES (?, ?, ?)',
        (json.dumps(new_reg_read), new_interval, current_version)
    )
    db.commit()
    return True

# Data operations
def insert_data(timestamp, data):
    """Insert sensor data"""
    db = get_db()
    cursor = db.cursor()
    
    # Convert -1 to NULL for unreaded registers
    reg_values = [None if val == -1 else val for val in data]
    
    cursor.execute('''
        INSERT INTO sensor_data 
        (timestamp, reg_0, reg_1, reg_2, reg_3, reg_4, reg_5, reg_6, reg_7, reg_8, reg_9)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    ''', (timestamp, *reg_values))
    
    db.commit()
    return cursor.lastrowid

def get_latest_data():
    """Get the latest value for each register"""
    db = get_db()
    cursor = db.cursor()
    
    latest_values = []
    for i in range(10):
        reg_col = f'reg_{i}'
        cursor.execute(f'''
            SELECT {reg_col}, timestamp 
            FROM sensor_data 
            WHERE {reg_col} IS NOT NULL 
            ORDER BY id DESC 
            LIMIT 1
        ''')
        row = cursor.fetchone()
        if row:
            latest_values.append({
                'register': i,
                'value': row[0],
                'timestamp': row[1]
            })
        else:
            latest_values.append({
                'register': i,
                'value': None,
                'timestamp': None
            })
    
    return latest_values

def get_data_by_id_range(start_id, end_id):
    """Get data by ID range"""
    db = get_db()
    cursor = db.cursor()
    
    cursor.execute('''
        SELECT id, timestamp, reg_0, reg_1, reg_2, reg_3, reg_4, 
               reg_5, reg_6, reg_7, reg_8, reg_9
        FROM sensor_data
        WHERE id BETWEEN ? AND ?
        ORDER BY id
    ''', (start_id, end_id))
    
    rows = cursor.fetchall()
    result = []
    for row in rows:
        data_entry = {
            'id': row['id'],
            'timestamp': row['timestamp'],
            'data': [row[f'reg_{i}'] if row[f'reg_{i}'] is not None else -1 for i in range(10)]
        }
        result.append(data_entry)
    
    return result

def get_data_by_timestamp_range(start_time, end_time):
    """Get data by timestamp range"""
    db = get_db()
    cursor = db.cursor()
    
    cursor.execute('''
        SELECT id, timestamp, reg_0, reg_1, reg_2, reg_3, reg_4, 
               reg_5, reg_6, reg_7, reg_8, reg_9
        FROM sensor_data
        WHERE timestamp BETWEEN ? AND ?
        ORDER BY timestamp
    ''', (start_time, end_time))
    
    rows = cursor.fetchall()
    result = []
    for row in rows:
        data_entry = {
            'id': row['id'],
            'timestamp': row['timestamp'],
            'data': [row[f'reg_{i}'] if row[f'reg_{i}'] is not None else -1 for i in range(10)]
        }
        result.append(data_entry)
    
    return result

def get_data_count():
    """Get total count of data records"""
    db = get_db()
    cursor = db.cursor()
    cursor.execute('SELECT COUNT(*) as count FROM sensor_data')
    row = cursor.fetchone()
    return row['count'] if row else 0
