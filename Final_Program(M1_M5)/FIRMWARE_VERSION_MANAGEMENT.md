# Firmware Version Management

## How to Update Firmware Version

### Step 1: Update the FIRMWARE_VERSION Constant

**File:** `include/firmware_updater.h` (Line 11)

```cpp
// ============================================================
// FIRMWARE VERSION CONFIGURATION
// Change this value when building new firmware versions
// Format: "major.minor.patch" (e.g., "1.0.0", "1.2.3")
// ============================================================
#define FIRMWARE_VERSION "1.0.0"  // ← Change this
```

### Current Setup
- Default version in code: `FIRMWARE_VERSION = "1.0.0"`
- Automatically used in main.cpp: `FirmwareUpdater::begin("...", FIRMWARE_VERSION)`
- Device tracks version during runtime

---

## Update Workflow

### Scenario: You've Made Code Changes and Built v1.1.0

**Step 1: Update Version Constant**
```cpp
// include/firmware_updater.h
#define FIRMWARE_VERSION "1.1.0"  // Changed from "1.0.0"
```

**Step 2: Rebuild Firmware**
```bash
pio run -e nodemcuv2
# Generates: .pio/build/nodemcuv2/firmware.bin
```

**Step 3: Upload Binary to Cloud Server**
```bash
curl -X POST \
  -F "file=@.pio/build/nodemcuv2/firmware.bin" \
  -F "version=1.1.0" \
  -F "level=1" \
  http://192.168.137.1:5000/firmware
```

**Step 4: Device Auto-Updates**
- Next check cycle (within 5 minutes)
- Compares: 1.1.0 > 1.0.0? **YES**
- Downloads firmware binary
- Flashes to chip
- **Auto-restarts with new firmware**

**Step 5: Verify Update**
```
Serial Output:
╔════════════════════════════════════════════════════════════╗
║           FIRMWARE VERSION CHECK                           ║
╚════════════════════════════════════════════════════════════╝
[FirmwareUpdater] 📦 Device Version: 1.0.0
[FirmwareUpdater] ☁️  Server Version: 1.1.0
╠════════════════════════════════════════════════════════════╣
║ ✅ UPDATE AVAILABLE: 1.0.0 → 1.1.0                        ║
╚════════════════════════════════════════════════════════════╝
[FirmwareUpdater] 📥 Downloading firmware from server...

[Device flashes and restarts...]

╔════════════════════════════════════════════════════════════╗
║         FIRMWARE UPDATE SUCCESSFUL                         ║
╚════════════════════════════════════════════════════════════╝
[FirmwareUpdater] ✅ Firmware flashed to chip successfully!
[FirmwareUpdater] 📊 Version Updated:
[FirmwareUpdater]    OLD: 1.0.0
[FirmwareUpdater]    NEW: 1.1.0
[FirmwareUpdater] 💾 New version installed on chip
```

---

## Version Numbering Convention

### Semantic Versioning: major.minor.patch

| Type | Example | When to Increment |
|------|---------|-------------------|
| **Major** | **2**.0.0 | Breaking changes, major refactoring |
| **Minor** | 1.**1**.0 | New features, new functionality |
| **Patch** | 1.1.**3** | Bug fixes, minor improvements |

### Examples

```
Initial Release:        1.0.0
Bug Fix:               1.0.1
New Feature:           1.1.0
Another Bug Fix:       1.1.1
Major Refactor:        2.0.0
```

---

## Version Tracking

### How It Works

1. **Initialization** (src/main.cpp)
   ```cpp
   FirmwareUpdater::begin("http://...", FIRMWARE_VERSION);
   // Uses constant from firmware_updater.h
   // Example: FIRMWARE_VERSION = "1.0.0"
   ```

2. **Version Stored in Memory**
   ```cpp
   static String currentVersion = "1.0.0";  // Updated after successful flash
   ```

3. **On Every Check Cycle**
   ```
   Device Version (in memory): 1.0.0
   Server Version (from header): 1.1.0
   
   Compare: 1.1.0 > 1.0.0? YES
   → Download and flash firmware
   → Update currentVersion = "1.1.0"
   → Restart
   ```

4. **After Restart**
   ```cpp
   FirmwareUpdater::begin("...", FIRMWARE_VERSION);
   // FIRMWARE_VERSION is now defined as "1.1.0" (if you updated step 1)
   // Device continues with new version
   ```

---

## Key Points

✅ **Version Constant Location**: `include/firmware_updater.h` line 11
✅ **Easy to Update**: Just change one #define
✅ **Automatic Comparison**: Device checks if server version > device version
✅ **Auto-Install**: Downloads and installs if newer version available
✅ **Persistent**: Version updated in code for next boot

---

## Example Progression

### Building Multiple Versions

**Version 1.0.0** (Initial)
```cpp
#define FIRMWARE_VERSION "1.0.0"
// Build → Upload to devices
```

**Version 1.0.1** (Bug fix)
```cpp
#define FIRMWARE_VERSION "1.0.1"
// Build → Upload to cloud
// Devices detect: 1.0.1 > 1.0.0 → Auto-update
```

**Version 1.1.0** (New feature)
```cpp
#define FIRMWARE_VERSION "1.1.0"
// Build → Upload to cloud
// Devices detect: 1.1.0 > 1.0.1 → Auto-update
```

**Version 2.0.0** (Major change)
```cpp
#define FIRMWARE_VERSION "2.0.0"
// Build → Upload to cloud
// Devices detect: 2.0.0 > 1.1.0 → Auto-update
```

---

## Automatic Update Process

```
Device Boot
  ↓
Load FIRMWARE_VERSION ("1.0.0")
  ↓
Every 5 Minutes
  ├─ Fetch server version: "1.1.0"
  ├─ Compare: 1.1.0 > 1.0.0? YES
  ├─ Download firmware binary
  ├─ Flash to chip memory
  ├─ Update currentVersion = "1.1.0"
  └─ ESP.restart()
  
Device Boot with New Firmware
  ↓
Load FIRMWARE_VERSION (should be "1.1.0" if code was updated)
  ↓
Continue normal operation
```

---

## Important Notes

1. **Device must be online** (WiFi connected)
2. **Check interval**: 5 minutes (configurable in cpp file)
3. **Update is non-blocking**: Doesn't interrupt normal operation
4. **Auto-restart**: Device automatically reboots after update
5. **No downgrade**: Device prevents older versions from being installed

---

## Troubleshooting

### Device not updating?
```
Check:
1. Device version in code: #define FIRMWARE_VERSION "1.0.0"
2. Cloud has newer version: version="1.1.0"
3. Device WiFi connected
4. Serial console shows version comparison
```

### Device keeps updating?
```
Check:
1. You updated #define FIRMWARE_VERSION in code
2. Firmware binary was rebuilt after changing version
3. Serial shows correct version after restart
```

### Device shows old version after update?
```
Check:
1. Did you update #define FIRMWARE_VERSION in code?
2. Did you rebuild and upload to cloud?
3. Check serial console for actual version
```

---

## Summary

✅ Version defined as constant: `#define FIRMWARE_VERSION`
✅ Automatically used in initialization
✅ Compared with server version on every check
✅ If server > device: Auto-download and install
✅ Device restarts with new firmware
✅ Easy version management in one place
