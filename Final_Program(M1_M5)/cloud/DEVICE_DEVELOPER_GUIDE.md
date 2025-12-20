# Quick Reference: Sending Compressed Data to Cloud Server

## For Embedded Device Developers

### Endpoint
```
POST http://server:5000/data?regs=10
Content-Type: application/octet-stream
```

### Steps

1. **Compress your data** using TimeSeriesCompressor
2. **Send binary data** via HTTP POST
3. **Receive confirmation** with decoded snapshots

### Code Example (ESP32/Arduino)

```cpp
#include <HTTPClient.h>
#include "TimeSeriesCompressor.h"

// Server configuration
const char* serverUrl = "http://192.168.1.100:5000/data?regs=10";

void sendCompressedData() {
    // Assume you have compressed data
    std::vector<uint8_t> compressedData = myCompressor.compress(snapshots);
    
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/octet-stream");
    
    // Send compressed bytes
    int httpCode = http.POST(compressedData.data(), compressedData.size());
    
    if (httpCode == 201) {
        String response = http.getString();
        Serial.println("✓ Data sent successfully!");
        Serial.println(response);
    } else {
        Serial.printf("✗ Error: HTTP %d\n", httpCode);
    }
    
    http.end();
}
```

### Expected Response

**Success (201 Created):**
```json
{
  "status": "success",
  "message": "Compressed data received and processed successfully",
  "snapshots_count": 3,
  "snapshots": [...]
}
```

**Error (400 Bad Request):**
```json
{
  "status": "error",
  "message": "Failed to decompress/decode data"
}
```

### Data Format Requirements

#### Compressed Data Structure
- First frame: Absolute values for all registers
- Subsequent frames: Delta-encoded with masks
- Each frame includes timestamp (year, month, day, hour, minute, second)
- Registers are 16-bit unsigned integers
- Use 0xFFFF (65535) for unread/invalid values

#### Query Parameters
- `regs`: Number of registers per frame (default: 10)

### Testing Your Implementation

#### 1. Test with Example Data

```cpp
// Example compressed data (3 snapshots)
uint8_t testData[] = {
    0x07, 0xE9, 0x00, 0x0A, 0x00, 0x13, 0x00, 0x13, 0x00, 0x1C, 
    0x00, 0x0E, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x32, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x19,
    0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
    0x00, 0x00
};

sendCompressedData(testData, sizeof(testData));
```

Expected: 3 snapshots at 2025-10-19 19:28:14, 19:28:20, 19:28:26

#### 2. Verify Server Receives Data

Check server response for `snapshots_count` matching your expected count.

#### 3. Query Database

```
GET http://server:5000/data
```

Verify your data appears in the response.

### Troubleshooting

#### "Failed to decompress/decode data"
- Verify data format matches TimeSeriesCompressor specification
- Check that `regs` parameter matches your data
- Ensure first frame has absolute values
- Verify big-endian byte order for 16-bit values

#### Connection Timeout
- Verify server is running: `curl http://server:5000/`
- Check network connectivity
- Verify server URL and port

#### HTTP 500 Error
- Check server logs for detailed error message
- Verify data is not corrupted during transmission
- Ensure Content-Type header is set correctly

### Debugging Tips

1. **Print compressed data before sending:**
```cpp
Serial.print("Sending ");
Serial.print(compressedData.size());
Serial.println(" bytes");
for (size_t i = 0; i < compressedData.size(); i++) {
    Serial.printf("%02X ", compressedData[i]);
}
Serial.println();
```

2. **Check server response:**
```cpp
if (httpCode > 0) {
    String response = http.getString();
    Serial.println("Response: " + response);
}
```

3. **Test with Python script first:**
```bash
cd cloud
python test_client.py
```

### Best Practices

1. **Batch multiple snapshots** in one request to reduce overhead
2. **Limit batch size** to ~100 snapshots to avoid timeouts
3. **Implement retry logic** for network failures
4. **Validate compression** before sending
5. **Log transmission statistics** for monitoring

### Compression Ratio Example

| Format | Size | Description |
|--------|------|-------------|
| Uncompressed | 480 bytes | 3 snapshots × (6 timestamp + 10 registers) × 2 bytes |
| Compressed | 52 bytes | Delta encoding + bit masks |
| **Ratio** | **~9:1** | Significant bandwidth savings |

### Alternative: JSON Format (Legacy)

If you need to send single snapshot without compression:

```cpp
// JSON format (less efficient)
String json = "{";
json += "\"timestamp\":\"2025-10-19 19:28:14\",";
json += "\"data\":[100,200,-1,150,-1,300,250,-1,180,220]";
json += "}";

HTTPClient http;
http.begin("http://server:5000/data");
http.addHeader("Content-Type", "application/json");
http.POST(json);
```

### Performance Metrics

| Metric | Value |
|--------|-------|
| Typical compression ratio | 8:1 to 10:1 |
| Max snapshots per request | 100-200 (depends on registers) |
| Typical latency | 100-300ms |
| Max payload size | 10KB recommended |

### Contact & Support

For issues:
1. Check `COMPRESSED_DATA_API.md` for details
2. Review server logs
3. Test with `test_client.py`
4. Verify data format matches specification

### Summary Checklist

- [ ] Compress data using TimeSeriesCompressor
- [ ] Set Content-Type to "application/octet-stream"
- [ ] Send to /data?regs=N endpoint
- [ ] Check for HTTP 201 response
- [ ] Parse JSON response for confirmation
- [ ] Verify data in database via GET /data
- [ ] Implement error handling and retry logic
- [ ] Monitor transmission statistics
