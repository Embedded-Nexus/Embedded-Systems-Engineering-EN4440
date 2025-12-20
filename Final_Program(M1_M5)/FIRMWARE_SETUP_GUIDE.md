# Firmware Update Setup - Quick Start

## Default Version

The default firmware version is defined in:
**`src/firmware_updater.cpp`** (line ~9)

```cpp
static String currentVersion = "1.0.0";  // ← Change here
```

**Example versions:**
- `1.0.0` - Initial release
- `1.1.0` - Minor update (new features)
- `1.2.0` - Minor update (bug fixes)
- `2.0.0` - Major update (breaking changes)

---

## How It Works

### Every Upload Cycle (30 seconds):

1. **Check firmware version**
   - Makes HEAD request to `/firmware` endpoint
   - Parses server version from header
   - Compares with device version

2. **If server version is newer:**
   - Downloads new firmware
   - Flashes to device memory
   - Auto-restarts device

3. **If no update needed:**
   - Continues with normal data operations
   - Compresses and uploads sensor data
   - Fetches configuration and commands

---

## Quick Configuration

### Step 1: Set Your Firmware Version
**File:** `src/firmware_updater.cpp`
```cpp
static String currentVersion = "1.0.0";
```

### Step 2: Build Firmware
```bash
cd e:\UoM\Sem07\Embedded\Repo\Embedded-Systems-Engineering-EN4440
pio run -e nodemcuv2
```

### Step 3: Upload to Cloud
```bash
curl -X POST \
  -F "file=@.pio/build/nodemcuv2/firmware.bin" \
  -F "version=1.0.0" \
  -F "level=1" \
  http://192.168.137.1:5000/firmware
```

### Step 4: Monitor Device
- Device checks every 30 seconds
- If new version found: downloads and restarts
- Watch serial console for update messages

---

## Version Comparison

**Semantic Versioning Format:** `major.minor.patch`

```
1.0.0 → 1.0.1: Patch update (1.0.1 > 1.0.0) ✅
1.0.0 → 1.1.0: Minor update (1.1.0 > 1.0.0) ✅
1.0.0 → 2.0.0: Major update (2.0.0 > 1.0.0) ✅
1.1.0 → 1.0.0: Downgrade prevented (1.0.0 < 1.1.0) ❌
1.2.0 → 1.2.0: Same version (1.2.0 = 1.2.0) ❌
```

---

## Upload Cycle Flow

```
Every 30 seconds:

Upload Cycle Starts
  ↓
[VERSION CHECK]
  ├─ Fetch server version
  ├─ Compare with device
  ├─ If newer: Download & Flash & Restart
  └─ If same/older: Continue
  
[DATA OPERATIONS]
  ├─ Compress sensor data
  ├─ Encrypt data
  ├─ Upload to cloud
  └─ Fetch config & commands

Sleep until next cycle...
```

---

## Serial Monitor Output

### No Update Needed:
```
[UploadManager] ⏫ Upload check triggered.
[FirmwareUpdater] 🔍 Checking for firmware updates
[FirmwareUpdater] Current version: 1.0.0
[FirmwareUpdater] 🌐 Fetching version info from: http://192.168.137.1:5000/firmware
[FirmwareUpdater] ✅ Server version: 1.0.0 (update level: 1)
[FirmwareUpdater] 📊 Version comparison:
[FirmwareUpdater]    Server:  1.0.0
[FirmwareUpdater]    Device:  1.0.0
[FirmwareUpdater] ℹ️ Versions are identical (no update needed)
```

### Update Available:
```
[UploadManager] ⏫ Upload check triggered.
[FirmwareUpdater] 🔍 Checking for firmware updates
[FirmwareUpdater] Current version: 1.0.0
[FirmwareUpdater] 🌐 Fetching version info from: http://192.168.137.1:5000/firmware
[FirmwareUpdater] ✅ Server version: 1.1.0 (update level: 1)
[FirmwareUpdater] 📊 Version comparison:
[FirmwareUpdater]    Server:  1.1.0
[FirmwareUpdater]    Device:  1.0.0
[FirmwareUpdater] ✅ Minor version higher on server
[FirmwareUpdater] ✅ Newer firmware available! (server: 1.1.0 > device: 1.0.0)
[FirmwareUpdater] 📥 Starting firmware download and flash...
[FirmwareUpdater] ✅ Firmware flashed successfully!
[FirmwareUpdater] Updating version from 1.0.0 to 1.1.0
[FirmwareUpdater] 🔄 Rebooting device with new firmware...
[Device restarts...]
```

---

## Testing Checklist

- [ ] Firmware version set in `src/firmware_updater.cpp`
- [ ] Firmware built with `pio run -e nodemcuv2`
- [ ] New firmware uploaded to cloud with newer version
- [ ] Device connected to WiFi
- [ ] Serial console open and monitoring
- [ ] Device checks version every 30 seconds
- [ ] Device downloads and updates when newer version found
- [ ] Device automatically restarts after update

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| Version not checking | Check if WiFi is connected |
| Update not starting | Verify cloud server has new firmware uploaded |
| Device stuck updating | Manually reset device and reupload old firmware |
| Version comparison wrong | Verify format is `major.minor.patch` (e.g., "1.0.0") |
| Header not parsed | Check cloud endpoint returns `X-Config-Version` header |

---

## Key Files

| File | Purpose |
|------|---------|
| `src/firmware_updater.cpp` | Default version variable (line ~9) |
| `src/upload_manager.cpp` | Firmware check integrated in upload cycle |
| `src/main.cpp` | Initialization |
| `cloud/app/routes/firmware.py` | Cloud endpoint |

---

## Summary

✅ Default version in code: `src/firmware_updater.cpp`
✅ Checks every 30 seconds (upload cycle)
✅ Semantic version comparison
✅ Auto-updates and restarts
✅ Integrated with data upload workflow
