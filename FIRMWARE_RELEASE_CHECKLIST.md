# Firmware Version Management - Quick Checklist

## Current Implementation Status

✅ **FIRMWARE_VERSION Constant**
- Location: `include/firmware_updater.h` line 11
- Format: `#define FIRMWARE_VERSION "1.0.0"`
- Easy to update when building new versions

✅ **Automatic Initialization**
- `FirmwareUpdater::begin("...", FIRMWARE_VERSION)`
- Used automatically in `src/main.cpp`
- No manual version entry needed

✅ **Version Checking on Every Cycle**
- Checks every 5 minutes
- Compares device version with server version
- Only updates if server version > device version

✅ **Automatic Installation**
- Downloads firmware binary if newer available
- Flashes directly to ESP8266 chip
- Installs to device memory automatically

✅ **Auto-Restart**
- Device automatically restarts after successful update
- Boots with new firmware version
- All systems re-initialize

---

## Releasing a New Firmware Version

### Before You Start
- [ ] Make all code changes
- [ ] Test locally if possible
- [ ] Decide on new version number

### Release Process

**Step 1: Update Version Constant**
```cpp
// include/firmware_updater.h (line 11)
#define FIRMWARE_VERSION "1.0.0"  // Change to new version
```

**Step 2: Rebuild Firmware**
```bash
pio run -e nodemcuv2
# Output: .pio/build/nodemcuv2/firmware.bin
```

**Step 3: Upload to Cloud**
```bash
# Windows
curl -X POST ^
  -F "file=@.pio/build/nodemcuv2/firmware.bin" ^
  -F "version=1.0.0" ^
  -F "level=1" ^
  http://192.168.137.1:5000/firmware

# Linux/Mac
curl -X POST \
  -F "file=@.pio/build/nodemcuv2/firmware.bin" \
  -F "version=1.0.0" \
  -F "level=1" \
  http://192.168.137.1:5000/firmware
```

**Step 4: Monitor Device**
- Device checks every 5 minutes
- Serial console shows version comparison
- Automatic download and install
- Device restarts with new firmware

---

## Example Version Release Sequence

### Release 1: Initial v1.0.0
```
#define FIRMWARE_VERSION "1.0.0"
pio run -e nodemcuv2
curl ... -F "version=1.0.0"
Device boots with 1.0.0
```

### Release 2: Bug Fix v1.0.1
```
#define FIRMWARE_VERSION "1.0.1"
pio run -e nodemcuv2
curl ... -F "version=1.0.1"
Device detects: 1.0.1 > 1.0.0 ✅ Auto-updates
```

### Release 3: New Feature v1.1.0
```
#define FIRMWARE_VERSION "1.1.0"
pio run -e nodemcuv2
curl ... -F "version=1.1.0"
Device detects: 1.1.0 > 1.0.1 ✅ Auto-updates
```

### Release 4: Major Update v2.0.0
```
#define FIRMWARE_VERSION "2.0.0"
pio run -e nodemcuv2
curl ... -F "version=2.0.0"
Device detects: 2.0.0 > 1.1.0 ✅ Auto-updates
```

---

## What Happens After Update

### Device Successfully Updates
```
1. Old firmware: 1.0.0
2. Server has: 1.1.0
3. Device downloads 1.1.0
4. Device flashes 1.1.0 to chip
5. Device updates internal version: 1.1.0
6. Device restarts
7. New firmware boots
8. Next check shows: "No newer version available"
```

### What If You Don't Update Code Constant?
```
Scenario: Device has 1.1.0, but code still says FIRMWARE_VERSION "1.0.0"

After restart:
FirmwareUpdater::begin("...", "1.0.0");  // Code not updated!
Device thinks it's 1.0.0
Next check: "Server: 1.1.0, Device: 1.0.0" → Update again!

Solution: Always update #define FIRMWARE_VERSION before rebuild
```

---

## Serial Console Output Examples

### Initialization
```
╔════════════════════════════════════════════════════════════╗
║         FIRMWARE UPDATER INITIALIZATION                    ║
╚════════════════════════════════════════════════════════════╝
[FirmwareUpdater] 📌 Endpoint: http://192.168.137.1:5000/firmware
[FirmwareUpdater] 📦 Current Version: 1.0.0
[FirmwareUpdater] ⏱️  Check Interval: 5 minutes
[FirmwareUpdater] ✅ Firmware updater ready
```

### Version Check (No Update)
```
╔════════════════════════════════════════════════════════════╗
║           FIRMWARE VERSION CHECK                           ║
╚════════════════════════════════════════════════════════════╝
[FirmwareUpdater] 🌐 Endpoint: http://192.168.137.1:5000/firmware
[FirmwareUpdater] 📦 Device Version: 1.0.0
[FirmwareUpdater] ☁️  Server Version: 1.0.0
╠════════════════════════════════════════════════════════════╣
║ ℹ️  NO NEWER VERSION - Device is up-to-date                ║
╚════════════════════════════════════════════════════════════╝
```

### Version Check (Update Available)
```
╔════════════════════════════════════════════════════════════╗
║           FIRMWARE VERSION CHECK                           ║
╚════════════════════════════════════════════════════════════╝
[FirmwareUpdater] 🌐 Endpoint: http://192.168.137.1:5000/firmware
[FirmwareUpdater] 📦 Device Version: 1.0.0
[FirmwareUpdater] ☁️  Server Version: 1.1.0
╠════════════════════════════════════════════════════════════╣
║ ✅ UPDATE AVAILABLE: 1.0.0 → 1.1.0                        ║
╚════════════════════════════════════════════════════════════╝
[FirmwareUpdater] 📥 Downloading firmware from server...
```

### Update Success
```
╔════════════════════════════════════════════════════════════╗
║         FIRMWARE UPDATE SUCCESSFUL                         ║
╚════════════════════════════════════════════════════════════╝
[FirmwareUpdater] ✅ Firmware flashed to chip successfully!
[FirmwareUpdater] 📊 Version Updated:
[FirmwareUpdater]    OLD: 1.0.0
[FirmwareUpdater]    NEW: 1.1.0
[FirmwareUpdater] 💾 New version installed on chip
[FirmwareUpdater] 🔄 Rebooting device with new firmware...
```

---

## Important Files

| File | Purpose |
|------|---------|
| `include/firmware_updater.h` | **CHANGE THIS**: Version constant (#define) |
| `src/main.cpp` | Uses FIRMWARE_VERSION automatically |
| `src/firmware_updater.cpp` | Update logic (don't modify) |
| `cloud/app/routes/firmware.py` | Cloud endpoint (don't modify) |

---

## Troubleshooting

### Device not updating
```
Check:
☐ Is FIRMWARE_VERSION updated in firmware_updater.h?
☐ Did you rebuild after changing version?
☐ Is new firmware uploaded to cloud server?
☐ Is device WiFi connected?
☐ Check serial console for version comparison
```

### Device keeps re-updating
```
Check:
☐ Did you update #define FIRMWARE_VERSION after update?
☐ Binary should be rebuilt with new version number
☐ Version in cloud should match #define FIRMWARE_VERSION
```

### Device shows wrong version
```
Check:
☐ Serial console: what does it show as "Current Version"?
☐ Check #define FIRMWARE_VERSION in firmware_updater.h
☐ Was firmware rebuilt after changing version?
☐ Device may still have old firmware (wait for update)
```

---

## Production Checklist

Before deploying devices:

- [ ] Set correct FIRMWARE_VERSION in firmware_updater.h
- [ ] Build firmware: `pio run -e nodemcuv2`
- [ ] Upload to cloud with matching version number
- [ ] Test on one device first
- [ ] Monitor serial console for version check
- [ ] Verify device updates successfully
- [ ] Confirm device restarts with new firmware
- [ ] Check serial shows "No newer version" on next check

---

## Summary

✅ Version is a #define constant in one location
✅ Automatically used for initialization
✅ Device compares with server every 5 minutes
✅ Auto-downloads if server version is newer
✅ Auto-installs to device memory
✅ Auto-restarts after successful update
✅ Easy to release new versions - just update constant and rebuild
