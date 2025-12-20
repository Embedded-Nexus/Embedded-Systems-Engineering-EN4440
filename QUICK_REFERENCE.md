# Quick Reference: Firmware OTA with Version Checking

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     DEVICE (ESP8266)                        │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  Main Loop (every cycle):                                   │
│  ├─ FirmwareUpdater::handle()                              │
│  │  └─ Every 5 minutes:                                    │
│  │     1. Check WiFi                                       │
│  │     2. Fetch server version via HEAD /firmware           │
│  │     3. Parse header: "1.2.0_2" → "1.2.0"              │
│  │     4. Compare: 1.2.0 > 1.1.0?                         │
│  │     5. If YES: Download & Flash                         │
│  │     6. If YES: Restart device                           │
│  │                                                           │
│  └─ PollingManager::handle()                               │
│  └─ UploadManager::handle()                                │
│                                                               │
└─────────────────────────────────────────────────────────────┘
           │
           │ HTTP Requests
           ↓
┌─────────────────────────────────────────────────────────────┐
│                  CLOUD SERVER (Python)                      │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  GET /firmware (firmware.py):                              │
│  ├─ Retrieve latest firmware from database                 │
│  ├─ Set header: X-Config-Version = "1.2.0_2"             │
│  └─ Send binary file                                        │
│                                                               │
│  POST /firmware:                                            │
│  ├─ Accept version parameter (e.g., "1.2.0")             │
│  ├─ Accept level parameter (1, 2, or 3)                   │
│  └─ Store in database                                      │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

---

## Device Version: 1.1.0

**Location:** `src/main.cpp`
```cpp
FirmwareUpdater::begin("http://192.168.137.1:5000/firmware", "1.1.0");
```

Update this when you change firmware:
- Build v1.2.0 firmware → change to "1.2.0"
- Build v2.0.0 firmware → change to "2.0.0"

---

## Version Comparison Logic

```
isVersionNewer(serverVersion, deviceVersion)

Parse both strings: "major.minor.patch"
Compare step by step:

if (server.major > device.major) → Update
if (server.major < device.major) → Skip (downgrade)
if (server.major == device.major):
    if (server.minor > device.minor) → Update
    if (server.minor < device.minor) → Skip
    if (server.minor == device.minor):
        if (server.patch > device.patch) → Update
        if (server.patch < device.patch) → Skip
        if (server.patch == device.patch) → No action

Examples:
1.2.0 > 1.1.0 → Update ✅
1.1.0 > 1.0.5 → Update ✅
1.0.5 = 1.0.5 → No action ❌
1.0.5 < 1.1.0 → Downgrade blocked ❌
```

---

## Update Process

### Manual Trigger

**Step 1: Build new firmware**
```bash
pio run -e nodemcuv2
# Output: .pio/build/nodemcuv2/firmware.bin
```

**Step 2: Update version in source**
```cpp
// src/main.cpp
FirmwareUpdater::begin("...", "X.Y.Z");  // Change version here
```

**Step 3: Rebuild**
```bash
pio run -e nodemcuv2
```

**Step 4: Upload to cloud**
```bash
curl -X POST \
  -F "file=@.pio/build/nodemcuv2/firmware.bin" \
  -F "version=X.Y.Z" \
  -F "level=1" \
  http://192.168.137.1:5000/firmware
```

**Step 5: Device will auto-update**
- Next check cycle (within 5 minutes)
- Compares X.Y.Z > current version?
- If YES → Downloads and restarts
- If NO → Continues normally

---

## Semantic Versioning Guide

**Format:** `major.minor.patch`

### Major Version (X.0.0)
- Breaking changes
- Incompatible API changes
- Example: 1.0.0 → 2.0.0

### Minor Version (X.Y.0)
- New features
- Backward compatible
- Example: 1.0.0 → 1.1.0

### Patch Version (X.Y.Z)
- Bug fixes
- No new features
- Example: 1.1.0 → 1.1.1

---

## Common Scenarios

### Scenario: Patch Update (Bug Fix)
```
Current: 1.1.0
New: 1.1.1 (bug fix)

Upload: curl ... -F "version=1.1.1"
Compare: 1.1.1 > 1.1.0? YES
Device: Downloads and updates ✅
```

### Scenario: Minor Update (New Features)
```
Current: 1.1.0
New: 1.2.0 (new features)

Upload: curl ... -F "version=1.2.0"
Compare: 1.2.0 > 1.1.0? YES
Device: Downloads and updates ✅
```

### Scenario: Major Update (Breaking Change)
```
Current: 1.2.0
New: 2.0.0 (major change)

Upload: curl ... -F "version=2.0.0"
Compare: 2.0.0 > 1.2.0? YES
Device: Downloads and updates ✅
```

### Scenario: Downgrade Blocked
```
Current: 1.2.0
Server: 1.1.0 (admin mistake)

Device checks: 1.1.0 > 1.2.0? NO
Device: Ignores, keeps 1.2.0 ✅
```

---

## Monitor Device Updates

**Serial Console Output:**

**No Update:**
```
[FirmwareUpdater] Current version: 1.1.0
[FirmwareUpdater] ✅ Server version: 1.1.0 (update level: 2)
[FirmwareUpdater] ℹ️ Versions are identical (no update needed)
```

**Update Available:**
```
[FirmwareUpdater] Current version: 1.1.0
[FirmwareUpdater] ✅ Server version: 1.2.0 (update level: 1)
[FirmwareUpdater] ✅ Minor version higher on server
[FirmwareUpdater] ✅ Newer firmware available! (server: 1.2.0 > device: 1.1.0)
[FirmwareUpdater] 📥 Starting firmware download and flash...
[FirmwareUpdater] ✅ Firmware flashed successfully!
[FirmwareUpdater] 🔄 Rebooting device with new firmware...
```

**Error:**
```
[FirmwareUpdater] ❌ Failed to retrieve server version
[FirmwareUpdater] ❌ Update failed! Error: ...
```

---

## Check Interval

**Default:** 5 minutes (300,000 ms)

**To change:**
```cpp
// src/firmware_updater.cpp, line ~9
static const unsigned long checkInterval = 300000;

// For testing: 30 seconds
static const unsigned long checkInterval = 30000;

// For production: 1 hour
static const unsigned long checkInterval = 3600000;
```

---

## Header Format Details

**Sent by Cloud:**
```
X-Config-Version: "1.2.0_2"
                   └─ version_level format
                   └─ 1.2.0 = version
                   └─ 2 = update level
```

**Parsed by Device:**
```cpp
String versionInfo = "1.2.0_2";
int idx = versionInfo.indexOf('_');      // Position: 5
String version = versionInfo.substring(0, idx);      // "1.2.0"
int level = versionInfo.substring(idx+1).toInt();    // 2
```

---

## Verification Checklist

- [ ] Device version in main.cpp matches current build
- [ ] Cloud has newer version firmware uploaded
- [ ] Header parsing works (version_level format)
- [ ] Version comparison logic correct
- [ ] Device can download firmware binary
- [ ] Device can flash to memory
- [ ] Device restarts after update
- [ ] New firmware runs successfully

---

## Files Reference

| File | Purpose |
|------|---------|
| `include/firmware_updater.h` | Public API |
| `src/firmware_updater.cpp` | Version checking + OTA logic |
| `src/main.cpp` | Device version configuration |
| `cloud/app/routes/firmware.py` | Cloud endpoint implementation |

---

## Support Functions

### isVersionNewer(server, device)
Returns true if server version is newer
```cpp
bool result = isVersionNewer("1.2.0", "1.1.0");  // true
bool result = isVersionNewer("1.1.0", "1.2.0");  // false
```

### fetchServerVersion()
Fetches version from server header
```cpp
String ver = fetchServerVersion();  // "1.2.0"
```

### checkForUpdate()
Main update check function
```cpp
bool updated = checkForUpdate();  // true if update happened
```

### handle()
Periodic handler (call from main loop)
```cpp
FirmwareUpdater::handle();  // Checks every 5 minutes
```

---

## Summary

✅ Device checks every 5 minutes
✅ Compares versions semantically
✅ Only updates if server version is newer
✅ Prevents accidental downgrade
✅ Auto-restarts after successful update
✅ Handles errors gracefully
✅ Integrates with cloud firmware.py endpoint
