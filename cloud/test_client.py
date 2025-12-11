"""
Test client for sending compressed data to the server
"""

import requests


def test_compressed_upload():
    """Test uploading compressed binary data to the server"""
    
    # Example compressed data (from specification)
    compressed_hex = "07 E9 00 0A 00 13 00 13 00 1C 00 0E FF FF FF FF 00 32 FF FF FF FF FF FF FF FF FF FF 00 19 FF FF 00 00 00 00 06 00 00 00 00 00 00 00 00 00 06 00 00 00 00 00"
    
    # Convert hex string to bytes
    compressed_bytes = bytes.fromhex(compressed_hex)
    
    print("=" * 80)
    print("Testing Compressed Data Upload")
    print("=" * 80)
    print(f"Data size: {len(compressed_bytes)} bytes")
    print(f"Hex: {compressed_hex}")
    print()
    
    # Server URL (update as needed)
    url = "http://localhost:5000/data?regs=10"
    
    # Send POST request with binary data
    headers = {
        'Content-Type': 'application/octet-stream'
    }
    
    try:
        response = requests.post(url, data=compressed_bytes, headers=headers)
        
        print("Response Status:", response.status_code)
        print()
        
        if response.status_code == 201:
            print("✓ Success!")
            print()
            print("Response JSON:")
            import json
            print(json.dumps(response.json(), indent=2))
        else:
            print("✗ Error!")
            print(response.text)
            
    except requests.exceptions.ConnectionError:
        print("✗ Connection Error!")
        print("Make sure the server is running on http://localhost:5000")
    except Exception as e:
        print(f"✗ Error: {e}")


def test_json_upload():
    """Test the legacy JSON upload (for comparison)"""
    
    print("\n" + "=" * 80)
    print("Testing JSON Data Upload (Legacy)")
    print("=" * 80)
    
    url = "http://localhost:5000/data"
    
    payload = {
        "timestamp": "2025-10-19 19:28:14",
        "data": [100, 200, -1, 150, -1, 300, 250, -1, 180, 220]
    }
    
    try:
        response = requests.post(url, json=payload)
        
        print("Response Status:", response.status_code)
        print()
        
        if response.status_code == 201:
            print("✓ Success!")
            print()
            print("Response JSON:")
            import json
            print(json.dumps(response.json(), indent=2))
        else:
            print("✗ Error!")
            print(response.text)
            
    except requests.exceptions.ConnectionError:
        print("✗ Connection Error!")
        print("Make sure the server is running on http://localhost:5000")
    except Exception as e:
        print(f"✗ Error: {e}")


if __name__ == "__main__":
    print("\n")
    print("╔═══════════════════════════════════════════════════════════════╗")
    print("║       Compressed Data Upload Test Client                     ║")
    print("╚═══════════════════════════════════════════════════════════════╝")
    print()
    
    # Test compressed data upload
    test_compressed_upload()
    
    # Test JSON upload for comparison
    test_json_upload()
    
    print()
    print("=" * 80)
    print("Tests Complete")
    print("=" * 80)
