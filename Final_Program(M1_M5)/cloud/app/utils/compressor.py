from typing import List, Iterable
from datetime import datetime
from typing import Optional

# -----------------------------
# Helpers
# -----------------------------

def get_u16_be(b: bytes, i: int) -> int:
    """Read uint16 (big-endian) at offset i."""
    return (b[i] << 8) | b[i + 1]

def put_u16_be(out: bytearray, v: int) -> None:
    """Append uint16 (big-endian)."""
    out.append((v >> 8) & 0xFF)
    out.append(v & 0xFF)

def clamp_s4(d: int) -> int:
    """Clamp delta to signed 4-bit range (-8..+7); return 127 if too large."""
    if d < -8 or d > 7:
        return 127
    return d

def pack_s4(s4: int) -> int:
    """Encode signed 4-bit integer (-8..+7) to unsigned nibble."""
    return s4 & 0xF

def unpack_s4(nib: int) -> int:
    """Decode unsigned nibble [0..15] to signed 4-bit (-8..+7)."""
    return nib - 16 if (nib & 0x8) else nib

# -----------------------------
# Codec
# -----------------------------

def compress(values: Iterable[int], regs: int) -> bytes:
    """Python port of TimeSeriesCompressor::compress."""
    vals = list(int(v) & 0xFFFF for v in values)
    out = bytearray()
    if regs <= 0 or not vals:
        return bytes(out)

    if len(vals) % regs != 0:
        raise ValueError("values length must be a multiple of regs")

    frames = len(vals) // regs

    # First frame (absolute)
    for j in range(regs):
        put_u16_be(out, vals[j])

    # Subsequent frames
    for f in range(1, frames):
        prev = vals[(f - 1) * regs : f * regs]
        curr = vals[f * regs : (f + 1) * regs]
        mask = 0
        nibbles = [0] * regs

        # Compute deltas
        for j in range(regs):
            diff = curr[j] - prev[j]
            s4 = clamp_s4(diff)
            if s4 == 127:
                mask |= (1 << j)
                nibbles[j] = 0
            else:
                nibbles[j] = pack_s4(s4)

        # Mask (LE)
        out.append(mask & 0xFF)
        out.append((mask >> 8) & 0xFF)

        # Packed nibbles
        for j in range(0, regs, 2):
            hi = nibbles[j] & 0xF
            lo = (nibbles[j + 1] & 0xF) if j + 1 < regs else 0
            out.append((hi << 4) | lo)

        # Absolute values for flagged regs
        for j in range(regs):
            if mask & (1 << j):
                put_u16_be(out, curr[j])

    return bytes(out)

def decompress(blob: bytes, regs: int) -> List[int]:
    """Python port of TimeSeriesCompressor::decompress."""
    out: List[int] = []
    if regs <= 0 or len(blob) < regs * 2:
        return out

    i = 0
    prev = [0] * regs

    # First frame (absolute)
    for j in range(regs):
        prev[j] = get_u16_be(blob, i)
        i += 2
        out.append(prev[j])

    # Subsequent frames
    while i < len(blob):
        need = 2 + ((regs + 1) // 2)
        if i + need > len(blob):
            break

        mask = blob[i] | (blob[i + 1] << 8)
        i += 2
        nb = (regs + 1) // 2
        packed = blob[i : i + nb]
        i += nb

        curr = prev[:]
        for j in range(regs):
            byte = packed[j // 2]
            nib = (byte >> 4) & 0xF if j % 2 == 0 else byte & 0xF
            if mask & (1 << j):
                if i + 1 >= len(blob):
                    return out
                curr[j] = get_u16_be(blob, i)
                i += 2
            else:
                curr[j] = (curr[j] + unpack_s4(nib)) & 0xFFFF

        out.extend(curr)
        prev = curr

    return out

# -----------------------------
# Decoding & Pipeline
# -----------------------------

def decode_decompressed_data(values: List[int], regs: int) -> List[dict]:
    """Decode a flat list of uint16 values into timestamped snapshots.

    Contract:
    - Input values are uint16 words laid out as frames of (6 + regs) words
      [year, month, day, hour, minute, second, reg0..reg{regs-1}]
    - 65535 indicates unread value and must be converted to -1 in output
    - regs > 0; frames with incomplete words are ignored

    Returns list of dicts: { 'timestamp': ISO8601 str, 'registers': List[int] }
    """
    if regs <= 0 or not values:
        return []

    frame_words = 6 + regs
    frame_count = len(values) // frame_words
    snapshots: List[dict] = []

    for f in range(frame_count):
        base = f * frame_words
        y, mo, d, h, mi, s = (
            int(values[base + 0]),
            int(values[base + 1]),
            int(values[base + 2]),
            int(values[base + 3]),
            int(values[base + 4]),
            int(values[base + 5]),
        )

        # Build ISO timestamp string safely (no validation beyond formatting)
        ts = f"{y:04d}-{mo:02d}-{d:02d}T{h:02d}:{mi:02d}:{s:02d}"

        regs_vals: List[int] = []
        for j in range(regs):
            v = int(values[base + 6 + j])
            regs_vals.append(-1 if v == 0xFFFF else v)

        snapshots.append({
            'timestamp': ts,
            'registers': regs_vals,
        })

    return snapshots


# -----------------------------
# Security Layer - Decryption
# -----------------------------

# Pre-shared key (PSK) - must match the encryption side
PSK = bytes([
    0x23, 0xAF, 0x77, 0x1D, 0x9B, 0x0F, 0xA5, 0x44,
    0xC1, 0xE9, 0x56, 0x72, 0xAA, 0xDE, 0x19, 0xBB
])

# Constants
NONCE_LEN = 12
TAG_LEN = 8
SEQ_LEN = 4

# Anti-replay state
last_seq_received = 0

def reset_sequence_counter():
    """Reset the anti-replay sequence counter. Use for testing or reinitialization."""
    global last_seq_received
    last_seq_received = 0

def _rotl32(x: int, r: int) -> int:
    """32-bit rotate left."""
    x = x & 0xFFFFFFFF
    return ((x << r) | (x >> (32 - r))) & 0xFFFFFFFF

def _xorshift32(s: int) -> tuple[int, int]:
    """XorShift32 PRNG. Returns (new_state, new_state)."""
    s = s & 0xFFFFFFFF
    s ^= (s << 13) & 0xFFFFFFFF
    s ^= (s >> 17) & 0xFFFFFFFF
    s ^= (s << 5) & 0xFFFFFFFF
    return (s & 0xFFFFFFFF, s & 0xFFFFFFFF)

def _derive_seed(psk: bytes, nonce: bytes, seq: int) -> int:
    """Derive a seed from PSK, nonce, and sequence number using FNV-1a."""
    acc = 2166136261  # FNV-1a 32-bit offset basis
    
    # Hash PSK
    for b in psk:
        acc ^= b
        acc = (acc * 16777619) & 0xFFFFFFFF
    
    # Hash nonce
    for b in nonce:
        acc ^= b
        acc = (acc * 16777619) & 0xFFFFFFFF
    
    # Hash sequence
    acc ^= seq
    acc = (acc * 16777619) & 0xFFFFFFFF
    
    # Final mixing
    acc ^= _rotl32(acc, 13)
    acc ^= (acc >> 7)
    acc ^= _rotl32(acc, 17)
    
    if acc == 0:
        acc = 1
    
    return acc & 0xFFFFFFFF

def _xor_stream_transform(data: bytes, psk: bytes, nonce: bytes, seq: int) -> bytes:
    """XOR stream cipher transformation."""
    s = _derive_seed(psk, nonce, seq)
    result = bytearray(len(data))
    
    for i in range(len(data)):
        if (i & 3) == 0:
            s, _ = _xorshift32(s)
        
        ks = (s >> ((i & 3) * 8)) & 0xFF
        result[i] = data[i] ^ ks
    
    return bytes(result)

def _fnv1a64_keyed(key: bytes, header: bytes, data: bytes) -> int:
    """FNV-1a 64-bit keyed hash for authentication."""
    OFFSET = 1469598103934665603  # Custom FNV-1a offset (matches C++ implementation)
    PRIME = 1099511628211
    
    h = OFFSET
    
    # Hash key
    for b in key:
        h ^= b
        h = (h * PRIME) & 0xFFFFFFFFFFFFFFFF
    
    # Hash header
    for b in header:
        h ^= b
        h = (h * PRIME) & 0xFFFFFFFFFFFFFFFF
    
    # Hash data
    for b in data:
        h ^= b
        h = (h * PRIME) & 0xFFFFFFFFFFFFFFFF
    
    return h & 0xFFFFFFFFFFFFFFFF

def _build_header(nonce: bytes, seq: int) -> bytes:
    """Build header from nonce and sequence number."""
    header = bytearray(NONCE_LEN + SEQ_LEN)
    header[0:NONCE_LEN] = nonce
    header[NONCE_LEN + 0] = seq & 0xFF
    header[NONCE_LEN + 1] = (seq >> 8) & 0xFF
    header[NONCE_LEN + 2] = (seq >> 16) & 0xFF
    header[NONCE_LEN + 3] = (seq >> 24) & 0xFF
    return bytes(header)

def decrypt_buffer(packet: bytes) -> Optional[bytes]:
    """Decrypt and authenticate a packet.
    
    Packet format: [SEQ(4) | NONCE(12) | CIPHERTEXT | TAG(8)]
    
    Returns decrypted plaintext on success, None on failure.
    """
    global last_seq_received
    
    # Validate minimum packet size
    min_size = SEQ_LEN + NONCE_LEN + TAG_LEN
    if len(packet) < min_size:
        return None
    
    # Extract sequence number (little-endian)
    seq_in = (packet[0] | 
              (packet[1] << 8) | 
              (packet[2] << 16) | 
              (packet[3] << 24))
    
    # Extract nonce
    nonce_in = packet[SEQ_LEN:SEQ_LEN + NONCE_LEN]
    
    # Calculate ciphertext length and extract
    cipher_len = len(packet) - (SEQ_LEN + NONCE_LEN + TAG_LEN)
    cipher_in = packet[SEQ_LEN + NONCE_LEN:SEQ_LEN + NONCE_LEN + cipher_len]
    
    # Extract tag (little-endian 64-bit)
    tag_ptr = SEQ_LEN + NONCE_LEN + cipher_len
    tag_in = 0
    for i in range(8):
        tag_in |= packet[tag_ptr + i] << (i * 8)
    
    # Build header and compute expected tag
    header = _build_header(nonce_in, seq_in)
    expected_tag = _fnv1a64_keyed(PSK, header, cipher_in)
    
    # Verify tag
    if expected_tag != tag_in:
        return None
    
    # Anti-replay check
    if seq_in <= last_seq_received:
        return None
    
    # Decrypt
    plain = _xor_stream_transform(cipher_in, PSK, nonce_in, seq_in)
    
    # Update last sequence
    last_seq_received = seq_in
    
    return plain

def xor_crypt(data: bytes, key: int) -> bytes:
    """Legacy XOR-encrypt/decrypt function - deprecated.
    
    Kept for backward compatibility but should not be used.
    Use decrypt_buffer() for proper authenticated decryption.
    """
    k = key & 0xFF
    return bytes((b ^ k) for b in data)


def process_compressed_data(blob: bytes, regs: int, key: Optional[int] = None) -> List[dict]:
    """Complete pipeline: decrypt, decompress bytes then decode into structured frames.

    Note: regs refers to the number of sensor registers (e.g., 10). The
    compressed stream contains 6 timestamp words + regs data words per frame,
    so we decompress with regs_total = 6 + regs, then decode with regs.
    
    Args:
        blob: Encrypted packet or compressed data
        regs: Number of sensor registers
        key: Legacy parameter (deprecated) - if None, uses new decryption; if provided, uses legacy XOR
    """
    # Choose decryption method
    if key is None:
        # Use new authenticated decryption
        decrypted = decrypt_buffer(blob)
        if decrypted is None:
            # Decryption failed (authentication error or replay attack)
            return []
        blob = decrypted
    else:
        # Legacy XOR decryption (deprecated but kept for backward compatibility)
        blob = xor_crypt(blob, key)

    regs_total = 6 + regs
    vals = decompress(blob, regs_total)
    return decode_decompressed_data(vals, regs)


def print_decoded_snapshots(snapshots: List[dict]) -> None:
    """Pretty-print decoded snapshots to stdout."""
    for idx, snap in enumerate(snapshots, 1):
        print(f"Snapshot {idx} @ {snap['timestamp']}")
        regs_vals = snap['registers']
        for r, val in enumerate(regs_vals):
            label = "(unread)" if val == -1 else str(val)
            print(f"  R{r:02d} = {label}")
        print()

# -----------------------------
# Test & Printing
# -----------------------------
if __name__ == "__main__":
    # Demo flow using 10 sensor registers (frame = 6 timestamp + 10 regs = 16 words)
    regs = 10  # sensor registers
    regs_total = 6 + regs
    raw = [
        2025, 10, 19, 19, 28, 14, 65535, 65535, 50, 65535, 65535, 65535, 65535, 65535, 25, 65535,
        2025, 10, 19, 19, 28, 20, 65535, 65535, 50, 65535, 65535, 65535, 65535, 65535, 25, 65535,
        2025, 10, 19, 19, 28, 26, 65535, 65535, 50, 65535, 65535, 65535, 65535, 65535, 25, 65535,
    ]

    blob = compress(raw, regs_total)
    values = decompress(blob, regs_total)
    snapshots = decode_decompressed_data(values, regs)

    print("Compressed bytes (hex):")
    print(blob.hex(" ").upper())
    print(f"\nCompressed length: {len(blob)} bytes\n")

    print("Decoded snapshots:")
    print_decoded_snapshots(snapshots)