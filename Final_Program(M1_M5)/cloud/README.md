# Cloud Server for Embedded Systems

RESTful API server for receiving, storing, and managing sensor data from embedded devices.

## Features

- ✅ **Compressed Data Support**: Receives time-series compressed binary data
- ✅ **Batch Processing**: Efficiently processes multiple snapshots in single request
- ✅ **Configuration Management**: Dynamic device configuration via REST API
- ✅ **Firmware Management**: OTA firmware update support
- ✅ **Command Queue**: Remote command execution on devices
- ✅ **Data Retrieval**: Query data by ID range or timestamp
- ✅ **SQLite Database**: Lightweight persistent storage

## Quick Start

### Installation

```bash
# Create virtual environment
python -m venv .venv

# Activate virtual environment
# Windows:
.venv\Scripts\activate
# Linux/Mac:
source .venv/bin/activate

# Install dependencies
pip install -r requirements.txt
```

### Run Server

```bash
python main.py
```

Server runs on `http://localhost:5000`

## API Endpoints

### Data Endpoints

#### POST /data (Compressed Binary)
Receive compressed time-series data from embedded devices.

**Request:**
```http
POST /data?regs=10
Content-Type: application/octet-stream

[Raw binary data]
```

**Response:**
```json
{
  "status": "success",
  "message": "Compressed data received and processed successfully",
  "snapshots_count": 3,
  "snapshots": [...]
}
```

#### POST /data (JSON - Legacy)
Receive single data snapshot in JSON format.

**Request:**
```json
{
  "timestamp": "2025-10-19 19:28:14",
  "data": [100, 200, -1, 150, -1, 300, 250, -1, 180, 220]
}
```

#### GET /data
Retrieve sensor data with various query options.

**Examples:**
- `GET /data` - Latest values for each register
- `GET /data?start_id=1&end_id=100` - Data by ID range
- `GET /data?start_time=2025-10-19T00:00:00&end_time=2025-10-19T23:59:59` - Data by timestamp

#### GET /data/count
Get total count of data records.

### Configuration Endpoints

#### GET /config
Get current device configuration.

#### POST /config
Update device configuration.

**Request:**
```json
{
  "reg_read": [1, 1, 0, 1, 1, 0, 0, 1, 1, 0],
  "interval": 5000
}
```

### Firmware Endpoints

#### GET /firmware
Get latest firmware information.

#### POST /firmware
Upload new firmware binary.

### Command Endpoints

#### GET /commands
Get queued commands for device.

#### POST /commands
Queue new command for device.

#### POST /commands/clear
Clear command queue.

## Compressed Data Processing

The server uses a custom **TimeSeriesCompressor** algorithm to efficiently handle compressed sensor data from embedded devices.

### Processing Pipeline

```
Raw Bytes → Decompression → Decoding → Database Insertion
```

1. **Decompression**: Extracts compressed time-series data using delta encoding
2. **Decoding**: Parses timestamps and register values from decompressed data
3. **Batch Insertion**: Efficiently stores multiple snapshots in database

### Example

**Input (48 bytes compressed):**
```
07 E9 00 0A 00 13 00 13 00 1C 00 0E ...
```

**Output (3 snapshots):**
- Snapshot 1 @ 2025-10-19 19:28:14
- Snapshot 2 @ 2025-10-19 19:28:20  
- Snapshot 3 @ 2025-10-19 19:28:26

See [COMPRESSED_DATA_API.md](COMPRESSED_DATA_API.md) for detailed documentation.

## Testing

### Test Decompression Algorithm

```bash
python test_compressor.py
```

### Test API Client

```bash
python test_client.py
```

### Test with PowerShell

```powershell
.\test_upload_compressed.ps1
```

## Project Structure

```
cloud/
├── app/
│   ├── config/         # Application configuration
│   ├── models/         # Database models and operations
│   ├── routes/         # API endpoints
│   └── utils/          # Utility functions (compressor, etc.)
├── firmware/           # Firmware storage directory
├── main.py            # Application entry point
├── test_compressor.py # Decompression test script
├── test_client.py     # API client test script
└── README.md          # This file
```

## Database Schema

### sensor_data
- `id`: Auto-incrementing primary key
- `timestamp`: Data timestamp
- `reg_0` to `reg_9`: 10 register values (NULL for unread)
- `created_at`: Record creation timestamp

### config
- `reg_read`: JSON array of register read flags
- `interval`: Data collection interval (ms)
- `version`: Configuration version

### firmware
- `file_path`: Path to firmware binary
- `update_level`: Update severity level
- `version`: Firmware version

### commands
- `timestamp`: Command timestamp
- `command_json`: Command data in JSON format

## Development

### Adding New Endpoints

1. Create route function in `app/routes/`
2. Register blueprint in `main.py`
3. Add database operations in `app/models/database.py`

### Modifying Compression Algorithm

Edit `app/utils/compressor.py` and update test cases in `test_compressor.py`.

## License

[Add your license here]

## Contributors

[Add contributors here]
