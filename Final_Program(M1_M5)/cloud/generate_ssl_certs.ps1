# Generate Self-Signed SSL Certificates for HTTPS

Write-Host "🔐 Generating Self-Signed SSL Certificates..." -ForegroundColor Cyan
Write-Host ""

# Check if OpenSSL is available
$opensslPath = Get-Command openssl -ErrorAction SilentlyContinue

if (-not $opensslPath) {
    Write-Host "❌ OpenSSL not found!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please install OpenSSL:" -ForegroundColor Yellow
    Write-Host "  1. Download from: https://slproweb.com/products/Win32OpenSSL.html" -ForegroundColor Yellow
    Write-Host "  2. Or install via Chocolatey: choco install openssl" -ForegroundColor Yellow
    Write-Host "  3. Or use Git Bash (comes with OpenSSL)" -ForegroundColor Yellow
    Write-Host ""
    exit 1
}

# Generate certificate
$certPath = Join-Path $PSScriptRoot "cert.pem"
$keyPath = Join-Path $PSScriptRoot "key.pem"

Write-Host "📝 Certificate will be saved to: $certPath" -ForegroundColor Gray
Write-Host "🔑 Private key will be saved to: $keyPath" -ForegroundColor Gray
Write-Host ""

# Generate self-signed certificate
& openssl req -x509 -newkey rsa:4096 -nodes -out $certPath -keyout $keyPath -days 365 -subj "/CN=localhost/O=Development/C=US"

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "✅ SSL Certificates generated successfully!" -ForegroundColor Green
    Write-Host ""
    Write-Host "📄 Files created:" -ForegroundColor Cyan
    Write-Host "  - cert.pem (Certificate)" -ForegroundColor Gray
    Write-Host "  - key.pem (Private Key)" -ForegroundColor Gray
    Write-Host ""
    Write-Host "🚀 You can now run the server with HTTPS support:" -ForegroundColor Green
    Write-Host "   python main.py" -ForegroundColor White
    Write-Host ""
    Write-Host "⚠️  Note: This is a self-signed certificate for development only!" -ForegroundColor Yellow
    Write-Host "   Your device may need to disable SSL verification or trust this certificate." -ForegroundColor Yellow
} else {
    Write-Host ""
    Write-Host "❌ Failed to generate certificates!" -ForegroundColor Red
}
