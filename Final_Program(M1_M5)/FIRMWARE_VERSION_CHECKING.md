# Firmware OTA Update - Version Checking Integration

## Cloud Endpoint Implementation (firmware.py)

### GET /firmware Endpoint

**Response Header Format:**
```
X-Config-Version: "version_level"
Example: "1.0.0_2"
```

**Response Details:**
- Header contains version in format: `{version}_{level}`
  - Version: semantic version (e.g., "1.0.0")
  - Level: update level (1, 2, or 3)
  - Separator: underscore `_`

### POST /firmware Endpoint

**Request Parameters:**
```
- file: binary firmware file (.bin)
- version: semantic version string (e.g., "1.0.0")
- level: update level (int: 1=minor, 2=standard, 3=major)
```

---

## Device Implementation (firmware_updater.cpp)

### Version Check Cycle

```
1. Check WiFi connection
2. Make HEAD request to /firmware endpoint
3. Parse X-Config-Version header ("version_level" format)
4. Extract version part (before underscore)
5. Compare with device version
6. If server version > device version:
   → Download and flash firmware
   → Restart device
7. Else:
   → Continue normally
```

### Key Functions

#### `fetchServerVersion()`
```cpp
// Makes HEAD request to firmware endpoint
// Parses header: "1.0.0_2" → extracts "1.0.0"
// Returns: version string (e.g., "1.0.0")
```

**Processing:**
```
Header received: "1.0.0_2"
                   ↓
Find underscore at position 5
                   ↓
Extract substring [0:5] = "1.0.0"
                   ↓
Extract level from [6:] = "2"
                   ↓
Log: "Server version: 1.0.0 (update level: 2)"
                   ↓
Return: "1.0.0"
```

#### `isVersionNewer(serverVersion, deviceVersion)`
```cpp
// Parses "major.minor.patch" format
// Compares semantically (1.1.0 > 1.0.0)
// Returns: true if server > device
```

**Comparison Logic:**
```
Server: "1.2.3" vs Device: "1.1.0"
         ↓
Compare Major: 1 == 1 (equal, continue)
Compare Minor: 2 > 1 (server newer)
         ↓
Return: true (update available)
```

#### `checkForUpdate()`
```cpp
// Main update check function
// 1. Fetch server version
// 2. Compare versions
// 3. If newer: download and flash
// 4. Auto-restart device
```

**Flow:**
```
checkForUpdate()
    ↓
fetchServerVersion() → "1.1.0"
    ↓
isVersionNewer("1.1.0", "1.1.0") → false
    ↓
Log: "No newer version available"
    ↓
Return false (no update)

---

checkForUpdate()
    ↓
fetchServerVersion() → "1.2.0"
    ↓
isVersionNewer("1.2.0", "1.1.0") → true
    ↓
Download firmware from /firmware
    ↓
Flash to device
    ↓
Update currentVersion = "1.2.0"
    ↓
ESP.restart()
```

---

## Semantic Version Comparison

### Format: `major.minor.patch`

**Examples:**
```
1.0.0  → major=1, minor=0, patch=0
1.1.0  → major=1, minor=1, patch=0
1.2.3  → major=1, minor=2, patch=3
2.0.0  → major=2, minor=0, patch=0
```

### Comparison Rules

1. **Major Version**: Highest priority
   - 2.0.0 > 1.9.9 (always)
   - 1.0.0 = 1.0.0 (equal)

2. **Minor Version** (if major equal):
   - 1.2.0 > 1.1.9
   - 1.1.0 = 1.1.0

3. **Patch Version** (if major and minor equal):
   - 1.1.5 > 1.1.4
   - 1.1.0 = 1.1.0

### Downgrade Prevention

If device has newer version than server:
```
Server: "1.0.5"
Device: "1.1.0"

Compare major: 1 == 1 (equal)
Compare minor: 0 < 1 (server is older)
         ↓
Downgrade prevented
Device keeps 1.1.0
```

---

## Implementation Alignment

### ✅ Header Parsing
Device correctly parses `X-Config-Version` header in `version_level` format:
```cpp
String versionInfo = http.header("X-Config-Version");  // "1.0.0_2"
int underscoreIndex = versionInfo.indexOf('_');         // position 5
String serverVersion = versionInfo.substring(0, underscoreIndex);  // "1.0.0"
int updateLevel = versionInfo.substring(underscoreIndex + 1).toInt();  // 2
```

### ✅ Version Comparison
Semantic version comparison correctly handles:
- Major.minor.patch format
- Proper numerical comparison
- Downgrade prevention

### ✅ Update Flow
When update needed:
1. Device recognizes server version is newer
2. Downloads firmware binary via GET /firmware
3. Device auto-flashes and restarts
4. Updates internal version to new version

---

## Example Scenarios

### Scenario 1: Device has older firmware
```
Device version: 1.0.0
Server version: 1.1.0

1. Device requests GET /firmware
2. Server responds with X-Config-Version: 1.1.0_2
3. Device parses: serverVersion="1.1.0", level=2
4. isVersionNewer("1.1.0", "1.0.0") → true
5. Device downloads firmware
6. Device flashes firmware
7. Device restarts
8. Device now runs 1.1.0
✅ Update successful
```

### Scenario 2: Device has latest firmware
```
Device version: 1.1.0
Server version: 1.1.0

1. Device requests GET /firmware
2. Server responds with X-Config-Version: 1.1.0_2
3. Device parses: serverVersion="1.1.0", level=2
4. isVersionNewer("1.1.0", "1.1.0") → false
5. Device logs: "No newer version available"
6. Device continues normal operation
✅ No action needed
```

### Scenario 3: Device has newer firmware than server
```
Device version: 1.2.0
Server version: 1.1.0

1. Device requests GET /firmware
2. Server responds with X-Config-Version: 1.1.0_2
3. Device parses: serverVersion="1.1.0", level=2
4. isVersionNewer("1.1.0", "1.2.0") → false
5. Device logs: "Device has newer version (downgrade prevented)"
6. Device continues normal operation
✅ Downgrade prevented
```

### Scenario 4: Network error
```
Device requests GET /firmware
Network error → connection timeout

1. fetchServerVersion() returns ""
2. checkForUpdate() detects empty version
3. Device logs: "Failed to retrieve server version"
4. Device continues normal operation
✅ Error handled gracefully
```

---

## Main Loop Integration

**In `src/main.cpp` setup():**
```cpp
FirmwareUpdater::begin("http://192.168.137.1:5000/firmware", "1.1.0");
```

**In `src/main.cpp` loop():**
```cpp
FirmwareUpdater::handle();  // Called every cycle
// Internally checks every 5 minutes with version comparison
```

---

## Testing the Version Check

### Upload New Firmware to Cloud
```bash
curl -X POST \
  -F "file=@firmware.bin" \
  -F "version=1.2.0" \
  -F "level=1" \
  http://192.168.137.1:5000/firmware

# Server stores version "1.2.0" in database
# GET /firmware will return header: X-Config-Version: 1.2.0_1
```

### Monitor Device
```
Device logs:
[FirmwareUpdater] ✅ Initialized with endpoint: http://192.168.137.1:5000/firmware
[FirmwareUpdater] Current version: 1.1.0

... (5 minute wait) ...

[FirmwareUpdater] 🌐 Fetching version info from: http://192.168.137.1:5000/firmware
[FirmwareUpdater] 📊 Version comparison:
[FirmwareUpdater]    Server:  1.2.0
[FirmwareUpdater]    Device:  1.1.0
[FirmwareUpdater] ✅ Minor version higher on server
[FirmwareUpdater] ✅ Newer firmware available! (server: 1.2.0 > device: 1.1.0)
[FirmwareUpdater] 📥 Starting firmware download and flash...
[FirmwareUpdater] ✅ Firmware flashed successfully!
[FirmwareUpdater] Updating version from 1.1.0 to 1.2.0
[FirmwareUpdater] 🔄 Rebooting device with new firmware...

[Device restarts with new firmware 1.2.0]
```

---

## Summary

✅ **Implementation correctly:**
- Parses `X-Config-Version` header format (`version_level`)
- Extracts semantic version for comparison
- Compares versions correctly (major.minor.patch)
- Prevents downgrade attacks
- Integrates with firmware.py endpoint
- Handles errors gracefully
- Auto-restarts device after successful update
