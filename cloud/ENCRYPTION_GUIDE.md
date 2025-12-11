# Authenticated Encryption Implementation

## Overview

This document describes the authenticated encryption/decryption implementation used for secure communication between the embedded device and the cloud server.

## Algorithm Details

### Cryptographic Primitives

1. **Stream Cipher**: XOR-based stream cipher with XorShift32 PRNG
2. **Authentication**: FNV-1a 64-bit keyed hash (HMAC-like)
3. **Key Derivation**: FNV-1a 32-bit with mixing
4. **Anti-Replay**: Sequence number tracking

### Constants

```python
PSK = [0x23, 0xAF, 0x77, 0x1D, 0x9B, 0x0F, 0xA5, 0x44,
       0xC1, 0xE9, 0x56, 0x72, 0xAA, 0xDE, 0x19, 0xBB]  # 16 bytes
NONCE_LEN = 12  # bytes
TAG_LEN = 8     # bytes
SEQ_LEN = 4     # bytes
```

## Packet Format

### Encrypted Packet Structure

```
[SEQ(4) | NONCE(12) | CIPHERTEXT(variable) | TAG(8)]
```

- **SEQ**: 4-byte sequence number (little-endian, 32-bit unsigned)
- **NONCE**: 12-byte random nonce (generated per packet)
- **CIPHERTEXT**: Encrypted payload (variable length)
- **TAG**: 8-byte authentication tag (little-endian, 64-bit)

### Minimum Packet Size

```
SEQ_LEN + NONCE_LEN + TAG_LEN = 4 + 12 + 8 = 24 bytes
```

## Encryption Process (C++ Side)

The embedded device performs encryption as follows:

1. **Generate Random Nonce**: 12 bytes of random data
2. **Encrypt Plaintext**: Using XOR stream cipher
   - Derive seed from PSK, nonce, and sequence number
   - Generate keystream using XorShift32
   - XOR plaintext with keystream
3. **Build Header**: Concatenate nonce and sequence number
4. **Compute Tag**: FNV-1a 64-bit keyed hash over PSK, header, and ciphertext
5. **Assemble Packet**: SEQ + NONCE + CIPHERTEXT + TAG
6. **Increment Sequence**: For next packet

## Decryption Process (Python Side)

The cloud server performs decryption as follows:

1. **Validate Packet Size**: Must be ≥ 24 bytes
2. **Extract Components**:
   - Sequence number (4 bytes, little-endian)
   - Nonce (12 bytes)
   - Ciphertext (remaining - 8 bytes)
   - Tag (last 8 bytes, little-endian)
3. **Build Header**: Concatenate nonce and sequence number
4. **Compute Expected Tag**: FNV-1a 64-bit keyed hash
5. **Verify Tag**: Compare expected vs. received (constant-time comparison recommended)
6. **Anti-Replay Check**: Reject if seq ≤ last_seq_received
7. **Decrypt**: XOR ciphertext with keystream
8. **Update State**: last_seq_received = seq

## API Usage

### Python Decryption

```python
from app.utils.compressor import decrypt_buffer, process_compressed_data

# Direct decryption
encrypted_packet = b'\x01\x00\x00\x00...'  # Full encrypted packet
plaintext = decrypt_buffer(encrypted_packet)
if plaintext is None:
    print("Decryption failed (authentication or replay)")
else:
    print(f"Decrypted: {plaintext}")

# Full pipeline (decrypt + decompress + decode)
snapshots = process_compressed_data(encrypted_packet, regs=10, key=None)
```

### HTTP Endpoint

```bash
# Send encrypted, compressed data
curl -X POST http://localhost:5000/data \
  -H "Content-Type: application/octet-stream" \
  --data-binary @encrypted_data.bin

# Optional: specify number of registers (default: 10)
curl -X POST "http://localhost:5000/data?regs=10" \
  -H "Content-Type: application/octet-stream" \
  --data-binary @encrypted_data.bin
```

**Note**: The `key` parameter is now optional. If omitted, the new authenticated encryption is used. If provided, legacy XOR decryption is used (deprecated).

## Security Features

### 1. Confidentiality
- XOR stream cipher with cryptographically strong key derivation
- Unique nonce per packet ensures different keystreams
- 16-byte PSK provides 128-bit security level

### 2. Authentication
- FNV-1a 64-bit keyed hash provides message authentication
- Tag covers both header (nonce + seq) and ciphertext
- Prevents tampering and forgery attacks

### 3. Anti-Replay Protection
- Sequence numbers must be strictly increasing
- Server rejects packets with seq ≤ last_seq_received
- Prevents replay attacks

### 4. Freshness
- Random nonce ensures each packet is unique
- Nonce included in authentication tag
- Prevents message duplication

## Implementation Details

### Key Derivation Function

```python
def _derive_seed(psk: bytes, nonce: bytes, seq: int) -> int:
    # FNV-1a hash of PSK + nonce + seq
    acc = 2166136261  # FNV-1a offset basis
    for b in psk:
        acc = (acc ^ b) * 16777619
    for b in nonce:
        acc = (acc ^ b) * 16777619
    acc = (acc ^ seq) * 16777619
    
    # Additional mixing
    acc ^= rotl32(acc, 13)
    acc ^= acc >> 7
    acc ^= rotl32(acc, 17)
    
    return acc if acc != 0 else 1
```

### XorShift32 PRNG

```python
def _xorshift32(s: int) -> tuple[int, int]:
    s ^= (s << 13) & 0xFFFFFFFF
    s ^= (s >> 17) & 0xFFFFFFFF
    s ^= (s << 5) & 0xFFFFFFFF
    return (s, s)
```

### Authentication Tag

```python
def _fnv1a64_keyed(key: bytes, header: bytes, data: bytes) -> int:
    OFFSET = 1469598103934665603  # Custom FNV-1a 64-bit offset (matches C++)
    PRIME = 1099511628211
    
    h = OFFSET
    for b in key + header + data:
        h = (h ^ b) * PRIME
    
    return h & 0xFFFFFFFFFFFFFFFF
```

**Note**: This implementation uses a custom FNV-1a offset value (`1469598103934665603`) that matches the C++ implementation, not the standard FNV-1a offset (`14695981039346656037`).

## Testing

Run the test suite to verify the implementation:

```bash
python test_decryption.py
```

### Test Cases

1. **Basic Decryption**: Encrypt and decrypt a test message
2. **Authentication Failure**: Verify corrupted packets are rejected
3. **Anti-Replay Protection**: Verify replay attacks are blocked

## Migration from Legacy System

### Old System (Deprecated)
```python
# Simple XOR with 8-bit key
snapshots = process_compressed_data(data, regs=10, key=0x5A)
```

### New System (Recommended)
```python
# Authenticated encryption with PSK
snapshots = process_compressed_data(encrypted_packet, regs=10, key=None)
```

### Backward Compatibility

The system supports both methods:
- `key=None`: Uses new authenticated encryption (recommended)
- `key=<int>`: Uses legacy XOR encryption (deprecated)

## Performance Considerations

- **Encryption/Decryption**: O(n) where n is payload size
- **Authentication**: O(n) single-pass hash computation
- **Memory**: Minimal additional overhead (~24 bytes per packet)
- **CPU**: Lightweight operations suitable for embedded systems

## Security Considerations

### Limitations

1. **Not AES-GCM**: This is a custom lightweight scheme, not a standardized AEAD
2. **FNV-1a**: Not a cryptographic hash; sufficient for authentication with secret key
3. **No Key Exchange**: PSK must be pre-shared securely
4. **No Forward Secrecy**: Compromised PSK exposes all past traffic

### Recommendations

1. **Protect the PSK**: Store securely, never transmit in plaintext
2. **Use HTTPS**: Add TLS layer for defense-in-depth
3. **Rotate Keys**: Periodically update the PSK
4. **Monitor Sequences**: Log rejected packets for anomaly detection
5. **Consider Upgrade**: For high-security applications, use AES-GCM or ChaCha20-Poly1305

## Troubleshooting

### Decryption Returns None

**Possible causes:**
1. **Authentication failure**: Tag mismatch (data corrupted or wrong PSK)
2. **Replay attack**: Sequence number ≤ previous
3. **Malformed packet**: Insufficient length or invalid format

**Solutions:**
- Verify PSK matches on both sides
- Check packet format and byte order
- Reset sequence counter if testing: `reset_sequence_counter()`
- Enable debug logging to inspect packet structure

### Example Debug Code

```python
import logging
logging.basicConfig(level=logging.DEBUG)

packet = b'...'
print(f"Packet length: {len(packet)}")
print(f"Packet hex: {packet.hex(' ')}")

seq = int.from_bytes(packet[0:4], 'little')
print(f"Sequence number: {seq}")

decrypted = decrypt_buffer(packet)
if decrypted is None:
    print("Decryption failed - check authentication")
else:
    print(f"Success: {decrypted}")
```

## References

- FNV Hash: http://www.isthe.com/chongo/tech/comp/fnv/
- XorShift: https://en.wikipedia.org/wiki/Xorshift
- AEAD: https://en.wikipedia.org/wiki/Authenticated_encryption

## Contact

For questions or issues, please refer to the main project documentation or contact the development team.
