param(
    [string]$SigningPassword = ""
)

$ErrorActionPreference = "Stop"

Write-Host "=== Signing WinCodexBar ===" -ForegroundColor Cyan

$buildDir = Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) "build"
$exePath = Join-Path $buildDir "Release" "WinCodexBar.exe"

if (-not (Test-Path $exePath)) {
    Write-Error "Build output not found: $exePath"
    exit 1
}

signtool verify /pa /all $exePath

if ($SigningPassword) {
    signtool sign /f code-signing.pfx /p $SigningPassword /td sha256 /tr http://timestamp.digicert.com $exePath
    Write-Host "Signed: $exePath" -ForegroundColor Green
} else {
    Write-Host "No signing password provided, skipping signing" -ForegroundColor Yellow
}
