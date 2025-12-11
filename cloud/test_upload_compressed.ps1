# Test Compressed Data Upload

# This script demonstrates how to send compressed binary data to the endpoint

# Example compressed data (hex format)
$compressedHex = "07E9000A00130013001C000EFFFFFFFF0032FFFFFFFFFFFFFFFFFFFF0019FFFF00000000060000000000000000000600000000"

# Convert hex to bytes and save to temporary file
$bytes = [byte[]]::new($compressedHex.Length / 2)
for ($i = 0; $i -lt $compressedHex.Length; $i += 2) {
    $bytes[$i/2] = [Convert]::ToByte($compressedHex.Substring($i, 2), 16)
}

$tempFile = [System.IO.Path]::GetTempFileName()
[System.IO.File]::WriteAllBytes($tempFile, $bytes)

Write-Host "Sending compressed data to server..."
Write-Host "File size: $($bytes.Length) bytes"
Write-Host ""

# Send to server (update URL as needed)
$url = "http://localhost:5000/data?regs=10"

try {
    $response = Invoke-WebRequest -Uri $url -Method POST -ContentType "application/octet-stream" -InFile $tempFile
    
    Write-Host "✓ Success!"
    Write-Host "Status: $($response.StatusCode)"
    Write-Host ""
    Write-Host "Response:"
    $response.Content | ConvertFrom-Json | ConvertTo-Json -Depth 10
} catch {
    Write-Host "✗ Error!"
    Write-Host $_.Exception.Message
    if ($_.Exception.Response) {
        $reader = [System.IO.StreamReader]::new($_.Exception.Response.GetResponseStream())
        $reader.ReadToEnd()
    }
} finally {
    # Cleanup
    Remove-Item $tempFile -ErrorAction SilentlyContinue
}
