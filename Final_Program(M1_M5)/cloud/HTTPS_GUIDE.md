# HTTPS/TLS Configuration Guide

## Problem

Your embedded device is trying to connect via **HTTPS**, but the server is running **HTTP** only, causing TLS handshake errors:

```
code 400, message Bad request version ('À\x13À')
\x16\x03\x01\x00... (TLS handshake data)
```

## Solutions

### Option 1: Use HTTP (Recommended for Development)

**On Embedded Device (ESP32/Arduino):**

```cpp
// Change HTTPS to HTTP
const char* serverUrl = "http://192.168.x.x:5000/data";

// Remove any SSL configuration
HTTPClient http;
http.begin(serverUrl);  // No need for WiFiClientSecure
```

**Pros:**
- Simple, no certificate management
- Faster connections
- Works immediately

**Cons:**
- No encryption (fine for local development)

---

### Option 2: Enable HTTPS on Server

#### Step 1: Generate SSL Certificates

**On Windows (PowerShell):**
```powershell
.\generate_ssl_certs.ps1
```

**Or manually with OpenSSL:**
```bash
openssl req -x509 -newkey rsa:4096 -nodes -out cert.pem -keyout key.pem -days 365 -subj "/CN=localhost"
```

This creates:
- `cert.pem` - SSL certificate
- `key.pem` - Private key

#### Step 2: Restart Server

The server will automatically detect the certificates and enable HTTPS:

```bash
python main.py
```

Output:
```
🔒 Starting server with HTTPS...
 * Running on https://0.0.0.0:5000
```

#### Step 3: Update Device to Handle Self-Signed Certificates

**ESP32/Arduino with Self-Signed Certificate:**

```cpp
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

WiFiClientSecure client;
HTTPClient http;

void setup() {
    // Option A: Disable certificate verification (INSECURE - development only)
    client.setInsecure();
    
    // Option B: Trust specific certificate (MORE SECURE)
    // const char* rootCACertificate = "-----BEGIN CERTIFICATE-----\n...";
    // client.setCACert(rootCACertificate);
    
    // Connect
    http.begin(client, "https://192.168.x.x:5000/data");
    http.addHeader("Content-Type", "application/octet-stream");
    
    int httpCode = http.POST(data, length);
}
```

**Option A: Insecure Mode (Quick Testing)**
```cpp
client.setInsecure();  // Skip certificate verification
```

**Option B: Trust Certificate (Better Security)**

1. Copy content of `cert.pem`
2. Add to your code:

```cpp
const char* rootCACertificate = 
"-----BEGIN CERTIFICATE-----\n"
"MIIFazCCA1OgAwIBAgIUXXXXXXXXXXXXXXXXXXXXXXXXXXX...\n"
"...\n"
"-----END CERTIFICATE-----\n";

client.setCACert(rootCACertificate);
```

---

## Current Server Configuration

The server now **auto-detects** SSL certificates:

- ✅ If `cert.pem` and `key.pem` exist → Runs with HTTPS
- ✅ If certificates missing → Runs with HTTP (shows warning)

---

## Troubleshooting

### Device: "SSL handshake failed"

**Cause:** Server using self-signed certificate

**Solution:**
```cpp
// Add this line
client.setInsecure();
```

### Device: "Connection refused"

**Cause:** Wrong protocol (HTTP vs HTTPS)

**Solution:**
- Check server startup message (HTTP or HTTPS?)
- Match device URL to server protocol

### Server: "Bad request version"

**Cause:** Device sending HTTPS to HTTP server

**Solution:**
- Change device to HTTP, OR
- Enable HTTPS on server (run `generate_ssl_certs.ps1`)

---

## Example: Complete ESP32 Code

### For HTTP Server:
```cpp
#include <HTTPClient.h>

void sendData() {
    HTTPClient http;
    http.begin("http://192.168.1.100:5000/data?regs=10");
    http.addHeader("Content-Type", "application/octet-stream");
    
    int httpCode = http.POST(compressedData, dataLength);
    
    if (httpCode == 201) {
        Serial.println("✓ Success!");
    }
    http.end();
}
```

### For HTTPS Server (Self-Signed):
```cpp
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

WiFiClientSecure client;

void sendData() {
    client.setInsecure();  // Accept self-signed cert
    
    HTTPClient http;
    http.begin(client, "https://192.168.1.100:5000/data?regs=10");
    http.addHeader("Content-Type", "application/octet-stream");
    
    int httpCode = http.POST(compressedData, dataLength);
    
    if (httpCode == 201) {
        Serial.println("✓ Success!");
    }
    http.end();
}
```

---

## Production Recommendations

### Development/Testing:
- Use **HTTP** (simpler, faster)
- Or HTTPS with `setInsecure()`

### Production Deployment:
- Use **proper SSL certificates** (Let's Encrypt, etc.)
- Implement certificate pinning
- Never use `setInsecure()` in production

---

## Quick Fix Commands

**Generate certificates:**
```powershell
.\generate_ssl_certs.ps1
```

**Start server:**
```bash
python main.py
```

**Test endpoint:**
```powershell
# HTTP
Invoke-WebRequest -Uri http://localhost:5000/data -Method GET

# HTTPS (ignore cert warnings)
Invoke-WebRequest -Uri https://localhost:5000/data -Method GET -SkipCertificateCheck
```

---

## Summary

| Scenario | Device Code | Server Setup |
|----------|-------------|--------------|
| **Development (Easy)** | `http://...` | No certificates needed |
| **Development (Secure)** | `https://...` + `setInsecure()` | Run `generate_ssl_certs.ps1` |
| **Production** | `https://...` + proper certs | Use Let's Encrypt or CA-signed certs |

**Recommendation for now:** Use HTTP on device → Immediate fix, no server changes needed!
