# Quick Fix: TLS Handshake Error

## The Problem
```
code 400, message Bad request version ('À\x13À')
\x16\x03\x01... (TLS handshake)
```

Your **ESP32 is sending HTTPS** requests, but the **server expects HTTP**.

---

## ⚡ Quick Fix (Recommended)

### Change Device to HTTP

**In your ESP32 code, find:**
```cpp
const char* serverUrl = "https://192.168.x.x:5000/data";
```

**Change to:**
```cpp
const char* serverUrl = "http://192.168.x.x:5000/data";
```

**✅ That's it! Redeploy and test.**

---

## 🔒 Alternative: Enable HTTPS on Server

### Step 1: Generate Certificates
```powershell
.\generate_ssl_certs.ps1
```

### Step 2: Restart Server
```bash
python main.py
```
Output should show: `🔒 Starting server with HTTPS...`

### Step 3: Update ESP32 to Accept Self-Signed Cert
```cpp
#include <WiFiClientSecure.h>

WiFiClientSecure client;
client.setInsecure();  // Accept self-signed cert

HTTPClient http;
http.begin(client, "https://192.168.x.x:5000/data");
// ... rest of code
```

---

## 📝 Summary

| Fix | Difficulty | Security | Speed |
|-----|-----------|----------|-------|
| **Use HTTP** | ⭐ Easy | Low (local only) | Fast |
| **Use HTTPS** | ⭐⭐ Medium | Medium (self-signed) | Slower |

**For local development:** Just use HTTP!

See `HTTPS_GUIDE.md` for detailed instructions.
