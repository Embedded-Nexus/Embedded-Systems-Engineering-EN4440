"""
Test with actual encrypted data from the embedded device.
"""

from app.utils.compressor import decrypt_buffer, reset_sequence_counter

def test_actual_encrypted_data():
    """Test decryption with actual data from the device."""
    
    print("=" * 70)
    print("Testing Actual Encrypted Data from Device")
    print("=" * 70)
    
    # Expected compressed data (from debug output)
    expected_compressed_hex = "07E9000A001400140023001FFFFFFFFF0032FFFFFFFFFFFFFFFFFFFF0019FFFF000000000500000000000000000005000000000020000000000000000000003100000000070000000000"
    expected_compressed = bytes.fromhex(expected_compressed_hex)
    
    print(f"\n1. Expected Compressed Data (hex):")
    print(f"   {expected_compressed.hex(' ').upper()}")
    print(f"   Length: {len(expected_compressed)} bytes")
    
    # Encrypted data from device
    encrypted_hex = "180000009853C5347C4135F943017D2EFAA2F81AFE147B3DCC4EF87A931B859FF70AE75D0B3B28FAAF8F91AB22881C2961EB373C49D22EEA9F058E3CA1EB8B7B5C83A9793DD086713F9E21DBCC4327C5837187975F86524F14BB20AD6D4898B7E21F"
    encrypted_packet = bytes.fromhex(encrypted_hex)
    
    print(f"\n2. Encrypted Packet (hex):")
    print(f"   {encrypted_packet.hex(' ').upper()}")
    print(f"   Length: {len(encrypted_packet)} bytes")
    
    # Parse packet structure
    seq = int.from_bytes(encrypted_packet[0:4], 'little')
    nonce = encrypted_packet[4:16]
    ciphertext_len = len(encrypted_packet) - 24  # Total - (4 seq + 12 nonce + 8 tag)
    ciphertext = encrypted_packet[16:16+ciphertext_len]
    tag_bytes = encrypted_packet[-8:]
    tag = int.from_bytes(tag_bytes, 'little')
    
    print(f"\n3. Packet Structure:")
    print(f"   Sequence:    {seq}")
    print(f"   Nonce:       {nonce.hex(' ').upper()}")
    print(f"   Ciphertext:  {ciphertext.hex(' ').upper()}")
    print(f"   Ciphertext Length: {ciphertext_len} bytes")
    print(f"   Tag:         {tag:016X}")
    
    # Reset sequence counter for fresh test
    reset_sequence_counter()
    
    # Attempt decryption
    print(f"\n4. Attempting Decryption...")
    decrypted = decrypt_buffer(encrypted_packet)
    
    if decrypted is None:
        print("   ❌ FAILED: Decryption returned None")
        print("\n   Possible issues:")
        print("   - Authentication tag mismatch")
        print("   - Sequence number issue")
        print("   - PSK mismatch")
        return False
    
    print(f"   ✅ Decryption successful!")
    print(f"\n5. Decrypted Data (hex):")
    print(f"   {decrypted.hex(' ').upper()}")
    print(f"   Length: {len(decrypted)} bytes")
    
    # Compare with expected
    print(f"\n6. Verification:")
    if decrypted.hex().upper() == expected_compressed_hex.upper():
        print("   ✅ SUCCESS: Decrypted data MATCHES expected compressed data!")
        return True
    else:
        print("   ❌ MISMATCH: Decrypted data does NOT match expected")
        print(f"\n   Expected: {expected_compressed_hex}")
        print(f"   Got:      {decrypted.hex().upper()}")
        
        # Show byte-by-byte comparison
        print(f"\n   Byte-by-byte comparison:")
        min_len = min(len(expected_compressed), len(decrypted))
        for i in range(min_len):
            if expected_compressed[i] != decrypted[i]:
                print(f"   Position {i}: Expected {expected_compressed[i]:02X}, Got {decrypted[i]:02X}")
        
        if len(expected_compressed) != len(decrypted):
            print(f"   Length mismatch: Expected {len(expected_compressed)}, Got {len(decrypted)}")
        
        return False

def test_manual_decryption():
    """Manually verify the decryption algorithm step by step."""
    from app.utils.compressor import PSK, _derive_seed, _xor_stream_transform, _fnv1a64_keyed, _build_header
    
    print("\n" + "=" * 70)
    print("Manual Step-by-Step Decryption Verification")
    print("=" * 70)
    
    # Parse the encrypted packet
    encrypted_hex = "180000009853C5347C4135F943017D2EFAA2F81AFE147B3DCC4EF87A931B859FF70AE75D0B3B28FAAF8F91AB22881C2961EB373C49D22EEA9F058E3CA1EB8B7B5C83A9793DD086713F9E21DBCC4327C5837187975F86524F14BB20AD6D4898B7E21F"
    packet = bytes.fromhex(encrypted_hex)
    
    seq = int.from_bytes(packet[0:4], 'little')
    nonce = packet[4:16]
    ciphertext = packet[16:-8]
    tag_received = int.from_bytes(packet[-8:], 'little')
    
    print(f"\n1. Parsed Components:")
    print(f"   Sequence:  {seq} (0x{seq:08X})")
    print(f"   Nonce:     {nonce.hex(' ').upper()}")
    print(f"   Cipher Len: {len(ciphertext)} bytes")
    print(f"   Tag:       {tag_received:016X}")
    
    print(f"\n2. PSK (Pre-Shared Key):")
    print(f"   {PSK.hex(' ').upper()}")
    
    # Build header
    header = _build_header(nonce, seq)
    print(f"\n3. Header (Nonce + Seq):")
    print(f"   {header.hex(' ').upper()}")
    
    # Compute authentication tag
    tag_computed = _fnv1a64_keyed(PSK, header, ciphertext)
    print(f"\n4. Authentication Tag:")
    print(f"   Received:  {tag_received:016X}")
    print(f"   Computed:  {tag_computed:016X}")
    print(f"   Match:     {tag_received == tag_computed}")
    
    if tag_received != tag_computed:
        print("\n   ❌ Authentication FAILED - tags do not match!")
        return False
    
    print("\n   ✅ Authentication PASSED")
    
    # Decrypt ciphertext
    plaintext = _xor_stream_transform(ciphertext, PSK, nonce, seq)
    print(f"\n5. Decrypted Plaintext:")
    print(f"   {plaintext.hex(' ').upper()}")
    print(f"   Length: {len(plaintext)} bytes")
    
    return True

if __name__ == "__main__":
    print("\n" + "=" * 70)
    print("ACTUAL DATA DECRYPTION TEST")
    print("=" * 70)
    
    # Test 1: Full decryption
    result1 = test_actual_encrypted_data()
    
    # Test 2: Manual verification
    result2 = test_manual_decryption()
    
    # Summary
    print("\n" + "=" * 70)
    print("TEST RESULTS")
    print("=" * 70)
    print(f"Full Decryption Test:     {'✅ PASS' if result1 else '❌ FAIL'}")
    print(f"Manual Verification Test: {'✅ PASS' if result2 else '❌ FAIL'}")
    
    if result1 and result2:
        print("\n🎉 ALL TESTS PASSED! Decryption is working correctly.")
    else:
        print("\n⚠️  TESTS FAILED - Decryption algorithm needs adjustment.")
    print("=" * 70)
