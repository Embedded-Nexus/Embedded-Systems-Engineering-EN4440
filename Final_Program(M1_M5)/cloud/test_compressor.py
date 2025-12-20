"""
Test script for TimeSeriesCompressor decompression and decoding
"""

import sys
sys.path.insert(0, 'e:\\UoM\\Sem07\\Embedded\\Repo\\Embedded-Systems-Engineering-EN4440\\cloud')

from app.utils.compressor import decompress, decode_decompressed_data, process_compressed_data, xor_crypt


def test_decompression():
    """Test with the provided example data"""
    
    # Example compressed data from your specification
    compressed_hex = "07 E9 00 0A 00 13 00 13 00 1C 00 0E FF FF FF FF 00 32 FF FF FF FF FF FF FF FF FF FF 00 19 FF FF 00 00 00 00 06 00 00 00 00 00 00 00 00 00 06 00 00 00 00 00"
    
    # Convert hex string to bytes
    compressed_bytes = bytes.fromhex(compressed_hex)
    
    print("=" * 80)
    print("STEP 1: INPUT - COMPRESSED DATA")
    print("=" * 80)
    print(f"Hex: {compressed_hex}")
    print(f"Bytes: {list(compressed_bytes)}")
    print(f"Length: {len(compressed_bytes)} bytes")
    print()
    
    # Decompress with total regs (6 timestamp + 10 registers)
    regs = 10
    regs_total = 6 + regs
    decompressed = decompress(compressed_bytes, regs_total)
    
    print("=" * 80)
    print("STEP 2: DECOMPRESSION")
    print("=" * 80)
    print(f"Decompressed values ({len(decompressed)} values):")
    print(" ".join(str(x) for x in decompressed))
    print()
    
    # Basic sanity checks on decompressed sequence length and framing
    words_per_frame = 6 + regs
    assert len(decompressed) % words_per_frame == 0, "Decompressed length not multiple of frame size"
    print("✓ Decompression PASSED basic checks!")
    print()
    
    # Decode
    snapshots = decode_decompressed_data(decompressed, regs)
    
    print("=" * 80)
    print("STEP 3: DECODING")
    print("=" * 80)
    print(f"Total snapshots: {len(snapshots)}")
    print()
    
    expected_timestamps = [
        "2025-10-19T19:28:14",
        "2025-10-19T19:28:20",
        "2025-10-19T19:28:26",
    ]
    expected_registers = [
        [-1, -1, 50, -1, -1, -1, -1, -1, 25, -1],
        [-1, -1, 50, -1, -1, -1, -1, -1, 25, -1],
        [-1, -1, 50, -1, -1, -1, -1, -1, 25, -1],
    ]

    for i, snap in enumerate(snapshots, 1):
        print(f"Snapshot {i}:")
        print(f"  Timestamp: {snap['timestamp']}")
        print(f"  Registers:")
        for r, val in enumerate(snap['registers']):
            if val == -1:
                print(f"    R{r:02d} = (unread)")
            else:
                print(f"    R{r:02d} = {val}")
        print()

    # Validate decoded content
    assert len(snapshots) == 3, f"Expected 3 snapshots, got {len(snapshots)}"
    for idx, snap in enumerate(snapshots):
        assert snap['timestamp'] == expected_timestamps[idx], f"Timestamp mismatch at {idx}"
        assert snap['registers'] == expected_registers[idx], f"Registers mismatch at {idx}"
    
    # Test complete pipeline
    print("=" * 80)
    print("STEP 4: COMPLETE PIPELINE TEST")
    print("=" * 80)
    result = process_compressed_data(compressed_bytes, regs)
    print(f"Processed {len(result)} snapshots successfully")
    print()

    # Encrypted pipeline test (XOR encryption)
    print("=" * 80)
    print("STEP 5: ENCRYPTED PIPELINE TEST")
    print("=" * 80)
    key = 0x5A  # example key
    encrypted = xor_crypt(compressed_bytes, key)
    decrypted_result = process_compressed_data(encrypted, regs, key)
    assert decrypted_result == result, "Decryption + pipeline should reproduce original snapshots"
    print(f"Encrypted bytes processed and decoded correctly with key={key}")
    print()
    
    return True


if __name__ == "__main__":
    success = test_decompression()
    if success:
        print("✓ All tests PASSED!")
    else:
        print("✗ Some tests FAILED!")
