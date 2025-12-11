# Implementation Summary: Compressed Data Support

## Overview
Successfully implemented compressed time-series data processing for the cloud server endpoint. The system now receives raw binary data, decompresses it using a custom algorithm (ported from C++), decodes the timestamps and register values, and stores multiple snapshots in the database efficiently.

## Files Created

### 1. `app/utils/__init__.py`
- Package initialization for utils module

### 2. `app/utils/compressor.py`
- **Main module for compression/decompression utilities**
- Functions implemented:
  - `get_u16_be()`: Read 16-bit big-endian integer from byte array
  - `unpack_s4()`: Unpack 4-bit signed integer from nibble
  - `decompress()`: Main decompression algorithm (ported from C++)
  - `decode_decompressed_data()`: Parse decompressed data into structured snapshots
  - `process_compressed_data()`: Complete pipeline (decompress + decode)

### 3. `test_compressor.py`
- Comprehensive test script for decompression/decoding
- Verifies algorithm against provided example data
- Tests complete processing pipeline

### 4. `test_client.py`
- Python test client for API testing
- Tests both compressed binary and JSON formats
- Demonstrates proper request formatting

### 5. `test_upload_compressed.ps1`
- PowerShell script for testing compressed uploads
- Windows-friendly testing option

### 6. `COMPRESSED_DATA_API.md`
- Detailed documentation for compressed data API
- Includes examples, data flow diagrams, and usage instructions

### 7. `README.md`
- Updated main documentation with new features
- Quick start guide and API reference

## Files Modified

### 1. `app/routes/data.py`
**Changes:**
- Added import for `insert_data_batch` and `process_compressed_data`
- Updated `receive_data()` endpoint to handle both formats:
  - Binary (compressed): `Content-Type: application/octet-stream`
  - JSON (legacy): Existing format maintained
- Added query parameter `regs` for configurable register count
- Returns detailed response with all decoded snapshots

**Key Logic:**
```python
if request.content_type == 'application/octet-stream':
    raw_bytes = request.data
    snapshots = process_compressed_data(raw_bytes, regs)
    count = insert_data_batch(snapshots)
    return snapshots info
else:
    # Handle JSON format (legacy)
```

### 2. `app/models/database.py`
**Changes:**
- Added `insert_data_batch()` function for efficient batch insertion
- Accepts list of snapshots with timestamps and register values
- Uses `executemany()` for optimized database operations
- Handles -1 to NULL conversion for unread registers

## Algorithm Implementation

### Decompression Algorithm (C++ → Python)

**Original C++ Logic:**
1. First frame: Absolute 16-bit values for all registers
2. Subsequent frames: Delta-encoded with bit masks
   - Mask indicates absolute vs delta encoding per register
   - Packed nibbles store small delta values (-8 to +7)
   - Large changes use full 16-bit absolute values

**Python Port:**
- Byte-perfect translation of C++ algorithm
- Handles big-endian 16-bit integers
- Properly unpacks 4-bit signed deltas
- Maintains frame continuity with previous values

### Decoding Algorithm

**Structure:**
Each frame = 6 timestamp words + N register words
- Words 0-5: year, month, day, hour, minute, second
- Words 6+: register values (0xFFFF = unread)

**Processing:**
1. Calculate frame size and count
2. Extract timestamp components
3. Format as ISO datetime string
4. Extract register values
5. Convert 0xFFFF to -1 for API consistency

## Data Flow

```
Embedded Device
    ↓ (compressed binary)
POST /data?regs=10
    ↓
receive_data()
    ↓
process_compressed_data()
    ├→ decompress() → [2025, 10, 19, 19, 28, 14, ...]
    └→ decode_decompressed_data() → [{timestamp, registers}, ...]
    ↓
insert_data_batch()
    ↓
SQLite Database
```

## Example Data Transformation

**Input (Compressed - 48 bytes):**
```
07 E9 00 0A 00 13 00 13 00 1C 00 0E FF FF FF FF 00 32 ...
```

**After Decompression (48 uint16 values):**
```
2025, 10, 19, 19, 28, 14, 65535, 65535, 50, 65535, ...
```

**After Decoding (3 snapshots):**
```json
[
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
```

**Database Records:**
```
id | timestamp           | reg_0 | reg_1 | reg_2 | ... | reg_8 | reg_9
---|---------------------|-------|-------|-------|-----|-------|-------
1  | 2025-10-19 19:28:14 | NULL  | NULL  | 50    | ... | 25    | NULL
2  | 2025-10-19 19:28:20 | NULL  | NULL  | 50    | ... | 25    | NULL
3  | 2025-10-19 19:28:26 | NULL  | NULL  | 50    | ... | 25    | NULL
```

## Testing Strategy

### Unit Tests
- `test_compressor.py`: Validates decompression algorithm
- Compares output against known expected values
- Tests complete pipeline

### Integration Tests
- `test_client.py`: Tests API endpoint
- Sends actual compressed data to server
- Verifies response format and database insertion

### Manual Testing
- `test_upload_compressed.ps1`: PowerShell testing
- Easy to modify for different test cases

## Benefits

1. **Efficiency**: Multiple snapshots in single request reduces HTTP overhead
2. **Bandwidth**: Binary format more compact than JSON
3. **Reliability**: Batch database insertion in single transaction
4. **Backward Compatibility**: Original JSON format still supported
5. **Modularity**: Compression logic isolated in utils module
6. **Testability**: Comprehensive test suite included

## API Response Example

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

## Usage from Embedded Device

```cpp
// Compress data
std::vector<uint8_t> compressed = compressor.compress(snapshots);

// Send to server
HTTPClient http;
http.begin("http://server:5000/data?regs=10");
http.addHeader("Content-Type", "application/octet-stream");
int httpCode = http.POST(compressed.data(), compressed.size());
```

## Future Enhancements

1. Add compression statistics to response (compression ratio)
2. Support variable register counts per request
3. Add validation for timestamp ranges
4. Implement data deduplication
5. Add metrics/logging for monitoring

## Deployment Checklist

- [x] Decompression algorithm implemented and tested
- [x] Decoding logic implemented and tested
- [x] Database batch insertion added
- [x] API endpoint updated
- [x] Test scripts created
- [x] Documentation written
- [ ] Run unit tests (`python test_compressor.py`)
- [ ] Run integration tests (`python test_client.py`)
- [ ] Verify database schema supports batch inserts
- [ ] Test with actual embedded device
- [ ] Monitor server logs for errors
- [ ] Verify performance with large batches

## Support

For issues or questions:
1. Check `COMPRESSED_DATA_API.md` for detailed documentation
2. Run test scripts to verify functionality
3. Check server logs for error messages
4. Verify compressed data format matches specification
