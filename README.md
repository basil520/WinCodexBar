# WinCodexBar

Windows tray app for tracking AI provider usage, implemented with C++/Qt and QML.

This repository is managed independently from the macOS CodexBar workspace that contains it.

## Requirements

- Windows 10 1809 or newer
- Qt 6.5 or newer
- CMake
- Visual Studio C++ build tools

## Build

```powershell
cmake -B build
cmake --build build --config Release
```

The release executable is generated at:

```text
build/Release/WinCodexBar.exe
```

## Test

```powershell
ctest --test-dir build -C Release --output-on-failure
```

E2E tests are opt-in:

```powershell
cmake -B build -DBUILD_E2E_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release -L e2e --output-on-failure
```

## Local Notes

The `docs/` directory is intentionally ignored by this repository. It can hold migration notes, E2E setup notes,
implementation reviews, and other working documents without adding them to WinCodexBar Git history.
