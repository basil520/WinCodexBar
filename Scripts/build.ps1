param(
    [string]$BuildType = "Release",
    [string]$QtPath = "D:\QT_5152\6.5.2\msvc2019_64",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)

if ($QtPath) {
    $env:CMAKE_PREFIX_PATH = $QtPath
}

Write-Host "=== Configuring WinCodexBar ($BuildType) ===" -ForegroundColor Cyan

$buildDir = Join-Path $projectRoot "build"
if ($Clean -and (Test-Path $buildDir)) {
    Remove-Item -Recurse -Force $buildDir
}
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
}

Push-Location $buildDir

try {
    if (-not (Test-Path "CMakeCache.txt")) {
        $cmakeArgs = @("..")
        if ($QtPath) {
            $cmakeArgs += "-DCMAKE_PREFIX_PATH=$QtPath"
        }
        cmake @cmakeArgs
        if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }
    }

    cmake --build . --config $BuildType --parallel
    if ($LASTEXITCODE -ne 0) { throw "CMake build failed" }

    $exeDir = Join-Path $buildDir $BuildType
    $exePath = Join-Path $exeDir "WinCodexBar.exe"
    if (Test-Path $exePath) {
        Write-Host "`n=== Deploying Qt DLLs ===" -ForegroundColor Cyan
        $qmlDir = Join-Path (Split-Path -Parent $buildDir) "qml"
        $qtBinDir = if ($QtPath) { Join-Path $QtPath "bin" } else { "" }
        if ($qtBinDir -and (Test-Path $qtBinDir)) {
            $env:PATH = "$qtBinDir;$env:PATH"
        }
        windeployqt --qmldir $qmlDir $exePath
        if ($LASTEXITCODE -ne 0) { Write-Warning "windeployqt completed with warnings" }
    }

    Write-Host "`n=== Build succeeded! ===" -ForegroundColor Green
    Write-Host "Output: $exePath" -ForegroundColor Green
}
finally {
    Pop-Location
}
