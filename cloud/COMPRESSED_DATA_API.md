# Data Endpoint - Compressed Data Processing

## Overview

The `/data` POST endpoint now supports receiving **compressed time-series data** from embedded devices using the TimeSeriesCompressor format. This allows efficient transmission of multiple sensor snapshots in a single request.

## How It Works

### 1. Data Compression (Embedded Device)
The embedded device compresses sensor data using the TimeSeriesCompressor algorithm before transmission.

### 2. Data Transmission
The device sends raw binary data to the `/data` endpoint with `Content-Type: application/octet-stream`.

### 3. Server Processing Pipeline

```
Raw Bytes → Decompression → Decoding → Database Insertion
```

#### Step 1: Decompression (`decompress()`)
- Reads compressed byte stream
- Extracts first frame (absolute values)
- Decodes subsequent frames using delta encoding with bit masks
- Returns list of 16-bit unsigned integers

#### Step 2: Decoding (`decode_decompressed_data()`)
- Parses decompressed integers into structured snapshots
- Each snapshot contains:
  - Timestamp (6 words: year, month, day, hour, minute, second)
  - Register values (10 words)
- Converts 0xFFFF (65535) to -1 for unread registers

#### Step 3: Batch Insertion (`insert_data_batch()`)
- Inserts all snapshots into database in a single transaction
- Converts -1 values to NULL in database
- Returns count of inserted records

## API Usage

### Binary Format (Compressed)

**Request:**
```http
POST /data?regs=10
Content-Type: application/octet-stream

[Raw binary data]
```

**Query Parameters:**
- `regs` (optional): Number of registers per frame (default: 10)

**Response:**
```json
{
  "status": "success",
  "message": "Compressed data received and processed successfully",
  "snapshots_count": 3,
  "snapshots": [
    {
      "timestamp": "2025-10-19 19:28:14",
      "registers": [-1, -1, 50, -1, -1, -1, -1, -1, 25, -1]
    },
    {
      "timestamp": "2025-10-19 19:28:20",
      "registers": [-1, -1, 50, -1, -1, -1, -1, -1, 25, -1]
    },
    {
      "timestamp": "2025-10-19 19:28:26",
      "registers": [-1, -1, 50, -1, -1, -1, -1, -1, 25, -1]
    }
  ]
}
```

### JSON Format (Legacy)

The endpoint still supports the original JSON format:

**Request:**
```http
POST /data
Content-Type: application/json

{
  "timestamp": "2025-10-18T12:00:00",
  "data": [100, 200, -1, 150, -1, 300, 250, -1, 180, 220]
}
```

## Example Compressed Data

**Compressed (hex):**
```
07 E9 00 0A 00 13 00 13 00 1C 00 0E FF FF FF FF 00 32 FF FF FF FF FF FF FF FF FF FF 00 19 FF FF 00 00 00 00 06 00 00 00 00 00 00 00 00 00 06 00 00 00 00 00
```

**Decompressed (integers):**
```
2025 10 19 19 28 14 65535 65535 50 65535 65535 65535 65535 65535 25 65535
2025 10 19 19 28 20 65535 65535 50 65535 65535 65535 65535 65535 25 65535
2025 10 19 19 28 26 65535 65535 50 65535 65535 65535 65535 65535 25 65535
```

**Decoded (structured):**
```
Snapshot 1 @ 2025-10-19 19:28:14
  R0-R1 = (unread)
  R2 = 50
  R3-R7 = (unread)
  R8 = 25
  R9 = (unread)

Snapshot 2 @ 2025-10-19 19:28:20
  R0-R1 = (unread)
  R2 = 50
  R3-R7 = (unread)
  R8 = 25
  R9 = (unread)

Snapshot 3 @ 2025-10-19 19:28:26
  R0-R1 = (unread)
  R2 = 50
  R3-R7 = (unread)
  R8 = 25
  R9 = (unread)
```

## Module Structure

### `app/utils/compressor.py`
Contains the compression/decompression utilities:
- `get_u16_be()` - Read 16-bit big-endian integer
- `unpack_s4()` - Unpack 4-bit signed integer
- `decompress()` - Main decompression algorithm
- `decode_decompressed_data()` - Convert integers to structured snapshots
- `process_compressed_data()` - Complete pipeline (decompress + decode)

### `app/routes/data.py`
Updated POST endpoint:
- Detects binary vs JSON content
- Processes compressed data
- Inserts batch records to database

### `app/models/database.py`
New function:
- `insert_data_batch()` - Batch insert multiple snapshots efficiently

## Testing

Use the included test script to verify the implementation:

```bash
python test_compressor.py
```

This will:
1. Load the example compressed data
2. Decompress it
3. Decode it
4. Verify against expected output
5. Test the complete pipeline

## Benefits

1. **Bandwidth Efficiency**: Send multiple snapshots in one request
2. **Reduced Overhead**: Binary format is more compact than JSON
3. **Transaction Safety**: Batch insertion in single database transaction
4. **Backward Compatible**: Original JSON format still supported
5. **Modular Design**: Compression logic separated into utility module
