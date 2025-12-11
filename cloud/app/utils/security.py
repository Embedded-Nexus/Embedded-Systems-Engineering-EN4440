"""
Security layer for decrypting data from embedded devices
Mirrors the C++ security_layer.cpp implementation
"""
import struct
from datetime import datetime

# Constants (must match C++ security_layer.cpp)
NONCE_LEN = 12
TAG_LEN = 8
SEQ_LEN = 4

# Pre-Shared Key (must match C++ PSK[])
PSK = bytes([
    0x23, 0xAF, 0x77, 0x1D, 0x9B, 0x0F, 0xA5, 0x44,
    0xC1, 0xE9, 0x56, 0x72, 0xAA, 0xDE, 0x19, 0xBB
])

# Track last received sequence for replay protection
last_seq_received = 0


def rotl32(x, r):
    """Rotate left 32-bit integer"""
    return ((x << r) | (x >> (32 - r))) & 0xFFFFFFFF


def xorshift32(s):
    """XORshift32 PRNG"""
    s ^= (s << 13) & 0xFFFFFFFF
    s ^= s >> 17
    s ^= (s << 5) & 0xFFFFFFFF
    return s & 0xFFFFFFFF


def derive_seed(psk, nonce, seq):
    """Derive seed from PSK, nonce, and sequence number"""
    acc = 2166136261
    
    for b in psk:
        acc ^= b
        acc = (acc * 16777619) & 0xFFFFFFFF
    
    for b in nonce:
        acc ^= b
        acc = (acc * 16777619) & 0xFFFFFFFF
    
    acc ^= seq
    acc = (acc * 16777619) & 0xFFFFFFFF
    
    acc ^= rotl32(acc, 13)
    acc ^= acc >> 7
    acc ^= rotl32(acc, 17)
    
    if acc == 0:
        acc = 1
    
    return acc & 0xFFFFFFFF


def fnv1a64_keyed(key, header, data):
    """FNV-1a 64-bit hash"""
    OFFSET = 1469598103934665603
    PRIME = 1099511628211
    h = OFFSET
    
    for b in key:
        h ^= b
        h = (h * PRIME) & 0xFFFFFFFFFFFFFFFF
    
    for b in header:
        h ^= b
        h = (h * PRIME) & 0xFFFFFFFFFFFFFFFF
    
    for b in data:
        h ^= b
        h = (h * PRIME) & 0xFFFFFFFFFFFFFFFF
    
    return h & 0xFFFFFFFFFFFFFFFF


def xor_stream_transform(data, psk, nonce, seq):
    """XOR stream cipher transform"""
    s = derive_seed(psk, nonce, seq)
    output = bytearray()
    
    for i in range(len(data)):
        if (i & 3) == 0:
            s = xorshift32(s)
        
        ks = (s >> ((i & 3) * 8)) & 0xFF
        output.append(data[i] ^ ks)
    
    return bytes(output)


def build_header(nonce, seq):
    """Build header from nonce and sequence"""
    header = bytearray(nonce)
    header.extend(struct.pack('<I', seq))
    return bytes(header)


def decrypt_buffer(packet):
    """
    Decrypt encrypted packet
    Returns decrypted data or None if authentication fails
    """
    global last_seq_received
    
    # Validate minimum packet size
    min_size = SEQ_LEN + NONCE_LEN + TAG_LEN
    if len(packet) < min_size:
        print(f"[Security] Error: Packet too small. Got {len(packet)} bytes, need at least {min_size}")
        return None
    
    # Extract sequence number (little-endian)
    seq_in = struct.unpack('<I', packet[0:4])[0]
    
    # Extract nonce
    nonce_in = packet[SEQ_LEN:SEQ_LEN + NONCE_LEN]
    
    # Extract ciphertext
    cipher_len = len(packet) - (SEQ_LEN + NONCE_LEN + TAG_LEN)
    cipher_in = packet[SEQ_LEN + NONCE_LEN:SEQ_LEN + NONCE_LEN + cipher_len]
    
    # Extract tag (little-endian)
    tag_bytes = packet[SEQ_LEN + NONCE_LEN + cipher_len:SEQ_LEN + NONCE_LEN + cipher_len + TAG_LEN]
    tag_in = struct.unpack('<Q', tag_bytes)[0]
    
    # Build header and verify tag
    header = build_header(nonce_in, seq_in)
    expected_tag = fnv1a64_keyed(PSK, header, cipher_in)
    
    if expected_tag != tag_in:
        print(f"[Security] Error: Authentication failed. Expected {expected_tag:016X}, got {tag_in:016X}")
        return None
    
    # Check for replay attack
    if seq_in <= last_seq_received:
        print(f"[Security] Error: Replay attack detected. Seq {seq_in} <= last {last_seq_received}")
        return None
    
    # Decrypt
    plain = xor_stream_transform(cipher_in, PSK, nonce_in, seq_in)
    last_seq_received = seq_in
    
    print(f"[Security] ✅ Decryption successful (seq={seq_in}, len={len(plain)} bytes)")
    return plain


def hex_string_to_bytes(hex_str):
    """Convert hex string to bytes, handling whitespace and newlines"""
    # Remove all whitespace and newlines
    clean_hex = ''.join(hex_str.split())
    try:
        return bytes.fromhex(clean_hex)
    except ValueError as e:
        print(f"[Security] Error parsing hex string: {e}")
        return None
