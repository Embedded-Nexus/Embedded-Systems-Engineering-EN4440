# Firmware Version Checking - Updated Architecture

## How It Now Works

### Version Checking Integrated with Upload Cycle

```
Main Loop (every cycle):
  ↓
UploadManager::handle() (every 30 seconds)
  ├─ 🔄 FirmwareUpdater::handle() [← FIRST - Check firmware version]
  │  ├─ Fetch version from server
  │  ├─ Compare versions
  │  ├─ If newer: Download & Flash
  │  └─ If newer: Auto-restart
  │
  ├─ Compress and encrypt data
  ├─ Upload to cloud
  └─ Fetch config and commands

Next cycle in 30 seconds...
```

---

## Default Firmware Version

**Location:** `src/firmware_updater.cpp` (line ~7)

```cpp
static String currentVersion = "1.0.0";  // ← DEFAULT VERSION
```

**To Change Default Version:**
1. Edit `src/firmware_updater.cpp`
2. Change the `currentVersion` variable
3. Example: `static String currentVersion = "1.2.0";`
4. Rebuild and upload firmware

---

## How Version Checking Works

### 1. Upload Cycle Starts (Every 30 seconds)
```
UploadManager::handle() called
  ↓
Immediately calls: FirmwareUpdater::handle()
```

### 2. Version Check
```cpp
bool checkForUpdate() {
  // 1. Fetch server version from /firmware endpoint
  serverVersion = fetchServerVersion()  // Returns "1.2.0" from header "1.2.0_2"
  
  // 2. Compare versions
  if (isVersionNewer(serverVersion, currentVersion)) {
    // 3. Download firmware binary
    ESPhttpUpdate.update(...)
    
    // 4. Update internal version
    currentVersion = serverVersion
    
    // 5. Auto-restart device
    ESP.restart()
  }
}
```

### 3. Upload Continues (If No Update)
```
If no newer firmware:
  ├─ Continue with data compression
  ├─ Continue with data upload
  └─ Continue with commands
```

---

## Update Flow

```
Upload Cycle Triggered
  ↓
[Version Check] ← NEW
  ├─ Fetch: "1.2.0_2" from server header
  ├─ Parse: "1.2.0"
  ├─ Compare: 1.2.0 > 1.0.0 (current)?
  │
  ├─ YES → Download & Flash & Restart
  └─ NO → Continue normally
  
[Data Operations]
  ├─ Compress data
  ├─ Upload to cloud
  └─ Fetch commands

Next upload in 30 seconds...
```

---

## Key Changes Made

### 1. Removed Timer-Based Checking
- **Before**: Separate 5-minute timer
- **After**: Integrated with upload cycle (30 seconds)

### 2. Simplified Initialization
- **Before**: `FirmwareUpdater::begin(endpoint, version)`
- **After**: `FirmwareUpdater::begin(endpoint)`
- Default version defined in firmware_updater.cpp

### 3. Integrated with Upload Manager
- FirmwareUpdater::handle() now called from UploadManager::handle()
- Version check happens at start of each upload cycle

---

## Configuration

### Change Default Firmware Version

**File:** `src/firmware_updater.cpp`
```cpp
// Line ~7
static String currentVersion = "1.0.0";  // ← Change this

// Examples:
static String currentVersion = "1.0.0";  // v1.0.0
static String currentVersion = "1.1.0";  // v1.1.0
static String currentVersion = "1.2.3";  // v1.2.3
static String currentVersion = "2.0.0";  // v2.0.0
```

### Change Firmware Endpoint

**File:** `src/main.cpp`
```cpp
// In setup() function
FirmwareUpdater::begin("http://192.168.137.1:5000/firmware");
//                     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
//                     Change IP if needed
```

---

## Version Comparison Rules

### Format: `major.minor.patch`

**Semantic Versioning:**
```
1.0.0 → major=1, minor=0, patch=0
1.1.0 → major=1, minor=1, patch=0
1.2.3 → major=1, minor=2, patch=3
2.0.0 → major=2, minor=0, patch=0
```

**Comparison Logic:**
```
Compare: 1.2.0 > 1.1.0?
  1. Major: 1 == 1 (equal, continue)
  2. Minor: 2 > 1 (YES, return true)
  ↓
Update available ✅

Compare: 1.1.0 > 1.1.0?
  1. Major: 1 == 1 (equal, continue)
  2. Minor: 1 == 1 (equal, continue)
  3. Patch: 0 == 0 (equal)
  ↓
No update needed ❌

Compare: 1.0.5 > 1.1.0?
  1. Major: 1 == 1 (equal, continue)
  2. Minor: 0 < 1 (NO, return false)
  ↓
Downgrade prevented ❌
```

---

## Testing

### Step 1: Verify Default Version
Open `src/firmware_updater.cpp` and check line ~7:
```cpp
static String currentVersion = "1.0.0";
```

### Step 2: Upload Firmware with New Version
```bash
curl -X POST \
  -F "file=@firmware.bin" \
  -F "version=1.1.0" \
  -F "level=1" \
  http://192.168.137.1:5000/firmware
```

### Step 3: Monitor Device
- Device checks version every 30 seconds (at upload cycle)
- Serial output:
```
[UploadManager] ⏫ Upload check triggered.
[FirmwareUpdater] 🔍 Checking for firmware updates
[FirmwareUpdater] ✅ Server version: 1.1.0 (update level: 1)
[FirmwareUpdater] ✅ Minor version higher on server
[FirmwareUpdater] ✅ Newer firmware available! (server: 1.1.0 > device: 1.0.0)
[FirmwareUpdater] 📥 Starting firmware download and flash...
[FirmwareUpdater] ✅ Firmware flashed successfully!
[FirmwareUpdater] Updating version from 1.0.0 to 1.1.0
[FirmwareUpdater] 🔄 Rebooting device with new firmware...
[Device restarts with new firmware]
```

---

## Files Modified

| File | Change |
|------|--------|
| `src/firmware_updater.cpp` | Added static version variable, simplified begin() |
| `include/firmware_updater.h` | Simplified function signatures |
| `src/main.cpp` | Simplified initialization, removed separate call |
| `src/upload_manager.cpp` | Added FirmwareUpdater::handle() call at cycle start |

---

## Workflow Summary

**Before (Old):**
```
Main Loop
  ├─ PollingManager::handle() (10s)
  ├─ UploadManager::handle() (30s)
  ├─ FirmwareUpdater::handle() (5min) ← Separate timer
  └─ Sleep
```

**After (New):**
```
Main Loop
  ├─ PollingManager::handle() (10s)
  ├─ UploadManager::handle() (30s)
  │  └─ Includes: FirmwareUpdater::handle() ← Integrated
  └─ Sleep
```

---

## Benefits

✅ **Simpler**: Default version in code, no parameter passing
✅ **Efficient**: Version checked with upload cycle (30s instead of 5min)
✅ **Integrated**: Firmware check part of normal data operations
✅ **Non-blocking**: Doesn't interfere with other tasks
✅ **Immediate**: Updates as soon as newer version detected

---

## Summary

The firmware version checking system now:
- Uses a default version variable in `firmware_updater.cpp`
- Checks for new firmware at each upload cycle (30 seconds)
- Compares versions semantically
- Downloads and installs if server version is newer
- Auto-restarts device after successful update
- All integrated into the normal upload workflow
