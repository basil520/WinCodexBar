# AGENTS.md — WinCodexBar

Compact repo guide for OpenCode. Omit anything an agent could guess from filenames.

## Build & Run

- **Requirements:** Windows 10 1809+, Qt 6.5+ (msvc2022_64 or msvc2019_64), CMake 3.21+, Visual Studio C++ Build Tools.
- **Configure:** `cmake -B build -DCMAKE_PREFIX_PATH=C:/Qt/6.5.3/msvc2022_64`
  - If Qt is missing, CMake warns but continues; the `WinCodexBar` executable target is **not created** and only the static `codexbar_core` library is available.
- **Build:** `cmake --build build --config Release`
- **Output:** `build/Release/WinCodexBar.exe`
- **Convenience script:** `Scripts/build.ps1` exists but hardcodes a local Qt path; prefer explicit `cmake` calls.
- **Auto-deploy:** `windeployqt` and `lrelease` (`.ts` → `.qm`) run automatically via CMake `POST_BUILD` / `add_custom_target`. No manual deployment step needed.

## Test

- **Unit tests (15 targets, default ON):** `ctest --test-dir build -C Release --output-on-failure`
- **E2E tests (5 targets, opt-in):** `cmake -B build -DBUILD_E2E_TESTS=ON` then `ctest --test-dir build -C Release -L e2e --output-on-failure`
  - E2E tests hit real accounts and require credentials in WinCred or env vars.
- On Windows, tests are wrapped with `PATH` and `QT_PLUGIN_PATH` injection via `add_wincodexbar_test()` / `add_e2e_test()` in the test CMake files.

## Architecture

- **Entry point:** `src/main.cpp` registers all 15 providers into `ProviderRegistry`, wires `UsageStore`/`SettingsStore`, and creates three `QQuickView` windows (tray popup, settings, usage details).
- **Library boundary:** `codexbar_core` (STATIC) contains all models, providers, network, and platform code. The `WinCodexBar` executable is a thin WIN32 launcher plus QML resource linking.
- **UI layer:** QML (`qml/TrayPanel.qml`, `qml/SettingsWindow.qml`, `qml/UsageWindow.qml`). `AppTheme` is exposed through both `qmldir` singleton and C++ root context.

## Provider Model

- All providers implement `IProvider` and return `IFetchStrategy*` vectors from `createStrategies()`.
- **Pipeline is sync:** `IFetchStrategy::fetchSync()` is the real path; the older `fetch()` is a compatibility wrapper. `ProviderPipeline` runs strategies sequentially and falls back on `shouldFallback`.
- **Source modes:** `auto`, `oauth`, `web`, `cli`, `api`. `ProviderPipeline::resolveStrategies()` filters by `ProviderSourceMode` from the fetch context.
- **New providers** must be manually instantiated and registered in `src/main.cpp`.

## Settings & Secrets

- **Global app settings** → Windows Registry (`HKEY_CURRENT_USER\Software\CodexBar`).
- **Provider settings** → `%AppData%/CodexBar/config.json` (non-secrets only).
- **Secrets** → Windows Credential Manager via `ProviderCredentialStore` facade. **Never write secrets to `config.json`.**
- Env vars override WinCred (used for CI/E2E).

## Debugging & QA

- **QML debug:** launch with `--show-settings --qml-log=<path>` to open Settings on startup and dump QML runtime state.
- **Browser override for testing:** set `CODEXBAR_CHROME_USER_DATA_DIR`, `CODEXBAR_EDGE_USER_DATA_DIR`, etc. to point `CookieImporter` / `BrowserDetection` at fixture profiles.
- **Single instance:** uses `CreateMutexW`; a second launch activates the existing window instead of spawning a process.

## Repo Quirks

- `/docs/` is listed in `.gitignore` (moved from the upstream macOS repo). Files under `docs/` may be untracked but contain valuable design context.
- `translations/` contains `.ts` source files; `.qm` binaries are generated into the build tree and copied to the output dir automatically.
- `resources/qml.qrc` already includes SVG icons for all 29 upstream providers, including the 14 not yet implemented.
