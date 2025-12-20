"""
Debug the tag computation to find the discrepancy.
"""

from app.utils.compressor import PSK

def debug_fnv1a64():
    """Test different implementations of FNV-1a 64-bit."""
    
    # Test data from device
    encrypted_hex = "180000009853C5347C4135F943017D2EFAA2F81AFE147B3DCC4EF87A931B859FF70AE75D0B3B28FAAF8F91AB22881C2961EB373C49D22EEA9F058E3CA1EB8B7B5C83A9793DD086713F9E21DBCC4327C5837187975F86524F14BB20AD6D4898B7E21F"
    packet = bytes.fromhex(encrypted_hex)
    
    seq = int.from_bytes(packet[0:4], 'little')
    nonce = packet[4:16]
    ciphertext = packet[16:-8]
    tag_received = int.from_bytes(packet[-8:], 'little')
    
    # Build header: nonce + seq
    header = bytearray(16)
    header[0:12] = nonce
    header[12] = seq & 0xFF
    header[13] = (seq >> 8) & 0xFF
    header[14] = (seq >> 16) & 0xFF
    header[15] = (seq >> 24) & 0xFF
    
    print("=" * 70)
    print("FNV-1a 64-bit Tag Computation Debug")
    print("=" * 70)
    
    print(f"\nInputs:")
    print(f"  PSK:        {PSK.hex(' ').upper()}")
    print(f"  Nonce:      {nonce.hex(' ').upper()}")
    print(f"  Sequence:   {seq} (0x{seq:08X})")
    print(f"  Header:     {header.hex(' ').upper()}")
    print(f"  Ciphertext: {ciphertext.hex(' ').upper()}")
    print(f"  Tag (rcvd): {tag_received:016X}")
    
    # Implementation 1: Current implementation
    OFFSET = 14695981039346656037
    PRIME = 1099511628211
    
    h = OFFSET
    for b in PSK:
        h ^= b
        h = (h * PRIME) & 0xFFFFFFFFFFFFFFFF
    for b in header:
        h ^= b
        h = (h * PRIME) & 0xFFFFFFFFFFFFFFFF
    for b in ciphertext:
        h ^= b
        h = (h * PRIME) & 0xFFFFFFFFFFFFFFFF
    
    print(f"\n  Tag (comp): {h:016X}")
    print(f"  Match:      {h == tag_received}")
    
    # Try different orders or methods
    print(f"\nTrying alternative: FNV-1a with different constant")
    
    # Check if the C++ uses different FNV constants
    OFFSET_ALT = 0xcbf29ce484222325  # Alternate FNV offset
    PRIME_ALT = 0x100000001b3  # Alternate FNV prime
    
    h2 = OFFSET_ALT
    for b in PSK:
        h2 ^= b
        h2 = (h2 * PRIME_ALT) & 0xFFFFFFFFFFFFFFFF
    for b in header:
        h2 ^= b
        h2 = (h2 * PRIME_ALT) & 0xFFFFFFFFFFFFFFFF
    for b in ciphertext:
        h2 ^= b
        h2 = (h2 * PRIME_ALT) & 0xFFFFFFFFFFFFFFFF
    
    print(f"  Tag (alt):  {h2:016X}")
    print(f"  Match:      {h2 == tag_received}")
    
    # Check the actual values used
    print(f"\nConstants verification:")
    print(f"  OFFSET (decimal): {OFFSET}")
    print(f"  OFFSET (hex):     0x{OFFSET:016X}")
    print(f"  PRIME (decimal):  {PRIME}")
    print(f"  PRIME (hex):      0x{PRIME:016X}")
    
    print(f"\n  Expected constants from C++:")
    print(f"  OFFSET: 1469598103934665603 (0x{1469598103934665603:016X})")
    print(f"  PRIME:  1099511628211 (0x{1099511628211:016X})")
    
    # Verify the constants match
    if OFFSET == 1469598103934665603 and PRIME == 1099511628211:
        print(f"  ✅ Constants match C++ code")
    else:
        print(f"  ❌ Constants don't match!")

if __name__ == "__main__":
    debug_fnv1a64()
