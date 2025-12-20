# Firmware OTA Update - Code Implementation Summary

## Files Created

### 1. `include/firmware_updater.h`
Header file defining the FirmwareUpdater namespace with functions:
- `begin()` - Initialize with endpoint and version
- `checkForUpdate()` - Check and apply firmware updates
- `getCurrentVersion()` - Get current firmware version
- `setCurrentVersion()` - Update version after OTA
- `handle()` - Periodic handler (call from main loop)

### 2. `src/firmware_updater.cpp`
Implementation file containing:
- State management (current version, last check time, check interval)
- OTA update logic using ESP8266httpUpdate library
- Error handling and logging
- Non-blocking periodic check mechanism (every 5 minutes)
- Automatic device restart after successful update

---

## Files Modified

### 1. `src/main.cpp`

#### Change 1: Include Header
```cpp
// ADDED
#include "firmware_updater.h"
```

#### Change 2: Initialize in setup()
```cpp
void setup() {
    // ... existing code ...
    
    PollingManager::begin(pollingInterval);
    UploadManager::begin("http://192.168.137.1:5000/data",
                        "http://192.168.137.1:5000/config",
                        "http://192.168.137.1:5000/commands");
    
    // ADDED: Initialize firmware updater
    FirmwareUpdater::begin("http://192.168.137.1:5000/firmware", "1.0.0");

    DEBUG_PRINTLN("[System] ✅ Setup complete.");
    pe_begin(5000);
}
```

#### Change 3: Call handler in loop()
```cpp
void loop() {
    // ... existing code ...
    
    PollingManager::handle();
    UploadManager::handle();
    FirmwareUpdater::handle();  // ADDED: Check for firmware updates
    pe_tickAndMaybePrint();
    
    // ... rest of loop code ...
}
```

---

## How It Works in the Cycle

### Main Loop Flow:
```
loop()
  ├─ PollingManager::handle()      [Poll inverter every 10s]
  ├─ UploadManager::handle()       [Upload data every 30s]
  ├─ FirmwareUpdater::handle()     [Check firmware every 5min]
  │  └─ Checks WiFi connection
  │  └─ Calls ESPhttpUpdate.update() if check interval elapsed
  │  └─ Automatically restarts on successful update
  └─ Sleep/low-power mode
```

### Update Check Sequence:
1. **Timer Check**: Is it time to check? (every 5 minutes)
2. **WiFi Check**: Is WiFi connected?
3. **HTTP Request**: GET /firmware from cloud
4. **Response Processing**:
   - 200 OK + binary → Download and flash → Restart
   - 204 No Content → No update available → Continue
   - 404 or error → Log error → Continue

---

## Configuration Points

### Cloud Server URL
**Location**: `src/main.cpp` line ~76
```cpp
FirmwareUpdater::begin("http://192.168.137.1:5000/firmware", "1.0.0");
                       ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
```
- Update `192.168.137.1` to your cloud server IP
- Keep `/firmware` path (matches firmware.py endpoint)

### Firmware Version
**Location**: `src/main.cpp` line ~76
```cpp
FirmwareUpdater::begin("http://192.168.137.1:5000/firmware", "1.0.0");
                                                              ^^^^^^^
```
- Update `1.0.0` to match your current firmware version
- Must be string format (quoted)

### Check Interval
**Location**: `src/firmware_updater.cpp` line ~15
```cpp
static const unsigned long checkInterval = 300000; // 5 minutes
```
- Change milliseconds value:
  - 30000 = 30 seconds (for testing)
  - 60000 = 1 minute
  - 300000 = 5 minutes (default)
  - 1800000 = 30 minutes

---

## Cloud Server Reference

The firmware endpoint is defined in `cloud/app/routes/firmware.py`:

```python
@firmware_bp.route('/firmware', methods=['GET'])
def get_firmware_file():
    """
    Returns the latest firmware binary (.bin file)
    Device calls this endpoint to download updates
    """
    info = get_firmware()  # Get from database
    
    if not info or not os.path.exists(info['file_path']):
        return jsonify({'status': 'error', ...}), 404
    
    response = send_file(info['file_path'], as_attachment=True)
    response.headers['X-Config-Version'] = config['version']
    return response
```

### Endpoint Characteristics:
- **URL**: `GET /firmware`
- **Returns**: Binary firmware file (`.bin`)
- **Content-Type**: `application/octet-stream`
- **On No Update**: HTTP 204 (No Content)
- **On Error**: HTTP 404 (Not Found)

---

## Update Process Timeline

### When Device Boots:
```
Time: 0s
- Device initializes
- FirmwareUpdater::begin() called
- Timer set to 0 (will check immediately on first cycle)
```

### First Cycle (1st loop iteration):
```
Time: ~10s (first polling interval)
- PollingManager::handle() → Poll inverter
- UploadManager::handle() → Upload data
- FirmwareUpdater::handle() → Check firmware (interval elapsed)
  └─ Makes GET /firmware request
  └─ Gets "No update" or downloads new firmware
```

### Subsequent Cycles (if no update):
```
Time: 10-300s
- PollingManager, UploadManager run normally
- FirmwareUpdater::handle() returns immediately (timer not elapsed)

Time: 300s (5 minutes)
- FirmwareUpdater::handle() → Check firmware again
- Repeat cycle
```

### If Update Available:
```
Time: 300s
- FirmwareUpdater::handle() → Check firmware
- Server returns binary data
- Device flashes firmware to memory
- Device calls ESP.restart()
- Device boots with new firmware
- All systems re-initialize
- Next check in 5 minutes shows "No update"
```

---

## Example Serial Output

### Normal Operation (No Update):
```
[FirmwareUpdater] ✅ Initialized with endpoint: http://192.168.137.1:5000/firmware
[FirmwareUpdater] Current version: 1.0.0
...
[FirmwareUpdater] 🔍 Checking for firmware updates at: http://192.168.137.1:5000/firmware
[FirmwareUpdater] ℹ️ No new firmware available (current version is up-to-date)
```

### Update Available:
```
[FirmwareUpdater] 🔍 Checking for firmware updates...
[FirmwareUpdater] ✅ Firmware update successful!
[FirmwareUpdater] 🔄 Rebooting device with new firmware...
[Device restarts]
```

### Update Error:
```
[FirmwareUpdater] ❌ Update failed! Error: ... 
[FirmwareUpdater] Error code: -1
[FirmwareUpdater] Continuing with current firmware
```

---

## Testing Procedure

### Step 1: Build New Firmware
```bash
cd <project-root>
pio run -e nodemcuv2
# Generates: .pio/build/nodemcuv2/firmware.bin
```

### Step 2: Upload to Cloud Server
```bash
curl -X POST \
  -F "file=@.pio/build/nodemcuv2/firmware.bin" \
  -F "level=1" \
  http://192.168.137.1:5000/firmware

# Response:
# {"status": "success", "version": "1.0.1", ...}
```

### Step 3: Wait for Device Check
- Device checks every 5 minutes automatically
- Monitor serial console for update messages
- Device will download and restart

### Step 4: Verify Update
- Device restarts with new firmware
- Serial output shows new firmware running
- Next check shows "No update available"

---

## Integration Benefits

✅ **Automatic**: No manual intervention needed
✅ **Periodic**: Checks every 5 minutes (configurable)
✅ **Non-blocking**: Doesn't interfere with normal operations
✅ **Seamless**: Integrated into main event loop
✅ **Safe**: Error handling prevents bricking
✅ **Efficient**: Only downloads when update available
✅ **Scalable**: Can deploy to multiple devices

---

## Key Files Reference

| File | Purpose |
|------|---------|
| `include/firmware_updater.h` | Public API for firmware updates |
| `src/firmware_updater.cpp` | Core OTA update logic |
| `src/main.cpp` | Integration with main loop |
| `cloud/app/routes/firmware.py` | Cloud endpoint for firmware delivery |

---

## Important Notes

1. **Binary File Format**: Always use `.bin` files from PlatformIO
2. **WiFi Required**: Updates only check when WiFi is connected
3. **Automatic Restart**: Device restarts automatically after update
4. **Error Recovery**: Device continues with old firmware if update fails
5. **Check Interval**: Default is 5 minutes, adjustable for testing
6. **Version Tracking**: Helps prevent re-downloading same firmware
