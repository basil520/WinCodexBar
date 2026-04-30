#Requires -Version 5.1
param(
    [Parameter(Mandatory=$true)]
    [string]$Token
)

$ErrorActionPreference = "Stop"

$uri = "https://www.kimi.com/apiv2/kimi.gateway.billing.v1.BillingService/GetUsages"

$headers = @{
    "Content-Type" = "application/json"
    "Authorization" = "Bearer $Token"
    "Cookie" = "kimi-auth=$Token"
    "Origin" = "https://www.kimi.com"
    "Referer" = "https://www.kimi.com/code/console"
    "Accept" = "*/*"
    "Accept-Language" = "en-US,en;q=0.9"
    "User-Agent" = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/143.0.0.0 Safari/537.36"
    "connect-protocol-version" = "1"
    "x-language" = "en-US"
    "x-msh-platform" = "web"
    "r-timezone" = "China Standard Time"
}

# Try to decode JWT payload for session headers
try {
    $parts = $Token -split '\.'
    if ($parts.Length -eq 3) {
        $payload = $parts[1].Replace('-','+').Replace('_','/')
        while ($payload.Length % 4 -ne 0) { $payload += '=' }
        $json = [System.Text.Encoding]::UTF8.GetString([Convert]::FromBase64String($payload)) | ConvertFrom-Json
        if ($json.device_id) { $headers["x-msh-device-id"] = $json.device_id }
        if ($json.ssid) { $headers["x-msh-session-id"] = $json.ssid }
        if ($json.sub) { $headers["x-traffic-id"] = $json.sub }
        Write-Host "JWT decoded: device_id=$($json.device_id), ssid=$($json.ssid), sub=$($json.sub)"
    }
} catch {
    Write-Host "JWT decode failed (non-fatal): $_"
}

$body = @{ scope = @("FEATURE_CODING") } | ConvertTo-Json -Compress

Write-Host "Sending request to $uri ..."
try {
    $response = Invoke-WebRequest -Uri $uri -Method POST -Headers $headers -Body $body -TimeoutSec 30
    Write-Host "Status: $($response.StatusCode)"
    Write-Host "Response: $($response.Content)"
} catch {
    Write-Host "Error: $($_.Exception.Message)"
    if ($_.Exception.Response) {
        $status = $_.Exception.Response.StatusCode.value__
        $reader = New-Object System.IO.StreamReader($_.Exception.Response.GetResponseStream())
        $reader.BaseStream.Position = 0
        $reader.DiscardBufferedData()
        $errorBody = $reader.ReadToEnd()
        Write-Host "HTTP $status Response: $errorBody"
    }
}
