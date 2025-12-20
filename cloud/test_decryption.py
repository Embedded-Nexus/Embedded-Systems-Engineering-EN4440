"""
Test script for the new authenticated encryption/decryption algorithm.
This tests the Python implementation against the C++ algorithm.
"""

from app.utils.compressor import decrypt_buffer, PSK, _derive_seed, _xor_stream_transform, _fnv1a64_keyed, _build_header, reset_sequence_counter

def create_test_encrypted_packet():
    """
    Create a test encrypted packet manually to verify decryption.
    This simulates what the C++ encryption would produce.
    """
    # Test parameters
    plaintext = b"Hello, World! This is a test message."
    seq_number = 1
    nonce = bytes([0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C])
    
    # Encrypt the plaintext
    ciphertext = _xor_stream_transform(plaintext, PSK, nonce, seq_number)
    
    # Build header
    header = _build_header(nonce, seq_number)
    
    # Compute authentication tag
    tag = _fnv1a64_keyed(PSK, header, ciphertext)
    
    # Build packet: [SEQ(4) | NONCE(12) | CIPHERTEXT | TAG(8)]
    packet = bytearray()
    
    # Sequence number (little-endian)
    packet.append(seq_number & 0xFF)
    packet.append((seq_number >> 8) & 0xFF)
    packet.append((seq_number >> 16) & 0xFF)
    packet.append((seq_number >> 24) & 0xFF)
    
    # Nonce
    packet.extend(nonce)
    
    # Ciphertext
    packet.extend(ciphertext)
    
    # Tag (little-endian)
    for i in range(8):
        packet.append((tag >> (i * 8)) & 0xFF)
    
    return bytes(packet), plaintext

def test_decryption():
    """Test the decryption function."""
    print("=" * 60)
    print("Testing Authenticated Decryption")
    print("=" * 60)
    
    # Create test packet
    encrypted_packet, original_plaintext = create_test_encrypted_packet()
    
    print(f"\n1. Original plaintext: {original_plaintext}")
    print(f"   Length: {len(original_plaintext)} bytes")
    
    print(f"\n2. Encrypted packet (hex):")
    print(f"   {encrypted_packet.hex(' ').upper()}")
    print(f"   Length: {len(encrypted_packet)} bytes")
    
    # Decrypt
    decrypted = decrypt_buffer(encrypted_packet)
    
    if decrypted is None:
        print("\n❌ FAILED: Decryption returned None")
        return False
    
    print(f"\n3. Decrypted plaintext: {decrypted}")
    print(f"   Length: {len(decrypted)} bytes")
    
    # Verify
    if decrypted == original_plaintext:
        print("\n✅ SUCCESS: Decryption successful!")
        print("   Plaintext matches original")
        return True
    else:
        print("\n❌ FAILED: Decrypted plaintext does not match original")
        print(f"   Expected: {original_plaintext}")
        print(f"   Got:      {decrypted}")
        return False

def test_authentication_failure():
    """Test that authentication fails when tag is corrupted."""
    print("\n" + "=" * 60)
    print("Testing Authentication Failure (Corrupted Tag)")
    print("=" * 60)
    
    # Create valid packet
    encrypted_packet, _ = create_test_encrypted_packet()
    
    # Corrupt the last byte of the tag
    corrupted_packet = bytearray(encrypted_packet)
    corrupted_packet[-1] ^= 0xFF
    corrupted_packet = bytes(corrupted_packet)
    
    print(f"\n1. Corrupted packet (last byte modified)")
    
    # Try to decrypt
    decrypted = decrypt_buffer(corrupted_packet)
    
    if decrypted is None:
        print("\n✅ SUCCESS: Authentication correctly rejected corrupted packet")
        return True
    else:
        print("\n❌ FAILED: Authentication did not reject corrupted packet")
        return False

def test_replay_attack():
    """Test anti-replay protection."""
    print("\n" + "=" * 60)
    print("Testing Anti-Replay Protection")
    print("=" * 60)
    
    # Reset sequence counter for clean test
    reset_sequence_counter()
    
    # Create valid packet with sequence 1
    encrypted_packet, _ = create_test_encrypted_packet()
    
    print("\n1. First decryption (seq=1):")
    decrypted1 = decrypt_buffer(encrypted_packet)
    if decrypted1 is None:
        print("   ❌ FAILED: First decryption failed")
        return False
    print("   ✅ First decryption successful")
    
    print("\n2. Replay attempt (same seq=1):")
    decrypted2 = decrypt_buffer(encrypted_packet)
    if decrypted2 is None:
        print("   ✅ SUCCESS: Replay correctly rejected")
        return True
    else:
        print("   ❌ FAILED: Replay was not rejected")
        return False

def main():
    """Run all tests."""
    print("\n" + "=" * 60)
    print("AUTHENTICATED ENCRYPTION TEST SUITE")
    print("=" * 60)
    print(f"\nPSK: {PSK.hex(' ').upper()}")
    print(f"PSK Length: {len(PSK)} bytes")
    
    results = []
    
    # Run tests
    results.append(("Basic Decryption", test_decryption()))
    results.append(("Authentication Failure", test_authentication_failure()))
    results.append(("Anti-Replay Protection", test_replay_attack()))
    
    # Summary
    print("\n" + "=" * 60)
    print("TEST SUMMARY")
    print("=" * 60)
    
    for test_name, passed in results:
        status = "✅ PASS" if passed else "❌ FAIL"
        print(f"{status}: {test_name}")
    
    all_passed = all(result[1] for result in results)
    print("\n" + "=" * 60)
    if all_passed:
        print("🎉 ALL TESTS PASSED!")
    else:
        print("⚠️  SOME TESTS FAILED")
    print("=" * 60)

if __name__ == "__main__":
    main()
