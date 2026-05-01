# WinCodexBar End-to-End (E2E) Testing

> **Status**: P1 infrastructure implemented. Real-account verification covers Codex, Copilot, Kimi, OpenCode, and OpenCode Go.

## Overview

E2E tests verify that each provider can successfully fetch real usage data from production APIs using **real user credentials**. They are:

- **Opt-in**: `BUILD_E2E_TESTS` defaults to `OFF`
- **Isolated**: No writes to `%AppData%`, registry, or user `~/.codex/auth.json`
- **Safe**: Missing credentials cause `QSKIP`, not failure
- **Non-blocking in CI**: Labeled `e2e`; standard `ctest` skips them

## Build

```powershell
cd D:\CodexBar\WinCodexBar
cmake -B build -DBUILD_E2E_TESTS=ON
cmake --build build --config Release
```

## Run

```powershell
# Run only e2e tests
ctest --test-dir build -C Release -L e2e --output-on-failure

# Or use the helper script
.\Scripts\run_e2e_tests.ps1
```

## Credential Setup

### Codex (OAuth)

Requires a valid `~/.codex/auth.json` from the Codex CLI.

```powershell
# The test auto-discovers this file. To use a custom location:
$env:CODEX_HOME = "C:\Some\Path"
```

> **Safety**: The test copies your real `auth.json` into a temporary directory and sets `CODEX_HOME` to that temp dir, so token refresh writes do not mutate your original file.

Settings -> Providers -> Codex -> Add Account uses `codex login --device-auth` with an app-managed `CODEX_HOME`. That foreground device-code flow is covered by unit parsing tests, but a first-time real OAuth authorization still needs manual validation on a machine with a browser and real account.

### Copilot (OAuth)

Two options:

1. **Environment variable** (preferred for testing):
   ```powershell
   $env:CODEXBAR_E2E_COPILOT_TOKEN = "gho_xxxxxxxx"
   ```
   The test injects this token into an in-memory credential backend, avoiding any Windows Credential Manager interaction.

2. **Windows Credential Manager**:
   Store a valid token under target `com.codexbar.oauth.copilot`. The test will read it directly.

### Kimi (Web Cookie)

Requires a `kimi-auth` cookie from `https://www.kimi.com/code/console`.

**How to obtain**:
1. Open Chrome → `https://www.kimi.com/code/console`
2. F12 → Application → Cookies → `www.kimi.com`
3. Copy the value of `kimi-auth`
4. Set environment variable:
   ```powershell
   $env:CODEXBAR_E2E_KIMI_COOKIE = "kimi-auth=YOUR_TOKEN_HERE"
   ```

### OpenCode (Web Cookie)

Requires an `auth` or `__Host-auth` cookie from `https://opencode.ai`.

**How to obtain**:
1. Open Chrome → `https://opencode.ai`
2. F12 → Application → Cookies → `opencode.ai`
3. Copy the value of `auth` or `__Host-auth`
4. Set environment variable:
   ```powershell
   $env:CODEXBAR_E2E_OPENCODE_COOKIE = "auth=YOUR_TOKEN_HERE"
   ```

> **Note**: OpenCode E2E will QSKIP if the workspace has no Web subscription data. This is expected behavior.

### OpenCode Go (Web Cookie)

Requires the same cookie as base OpenCode (`auth` or `__Host-auth` from `https://opencode.ai`).

**How to obtain**:
1. Same cookie as OpenCode (see above)
2. Set environment variable:
   ```powershell
   $env:CODEXBAR_E2E_OPENCODEGO_COOKIE = "auth=YOUR_TOKEN_HERE"
   ```
   Or reuse the OpenCode cookie:
   ```powershell
   $env:CODEXBAR_E2E_OPENCODEGO_COOKIE = $env:CODEXBAR_E2E_OPENCODE_COOKIE
   ```

> **Note**: OpenCode Go fetches usage from the Go workspace page (`/workspace/<id>/go`) and supports 5-hour, weekly, and optional monthly usage windows.

## Privacy & Security

- **Never commit tokens** to git.
- **Log masking**: E2E test output automatically masks `Authorization`, `Cookie`, and JSON token fields.
- **No system mutation**: Tests do not write to registry, `%AppData%`, or browser profiles.

## Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| `QSKIP: Codex auth.json not available` | Missing `~/.codex/auth.json` | Run `codex login` or set `CODEX_HOME` |
| `QSKIP: Copilot token not available` | Missing env var or WinCred | Set `CODEXBAR_E2E_COPILOT_TOKEN` |
| `Codex fetch failed: API error` | Token expired | Run `codex login` to refresh |
| Codex Add Account shows device code `AUTHORIZATION` | Old build parsed the phrase "device code authorization" as the code | Update to the current build, cancel the old flow, and start Add Account again |
| `Copilot fetch failed: 401` | Token expired or revoked | Generate a new token |
| `OpenCode fetch failed: No usage data` | Workspace has no subscription | QSKIP is expected; upgrade to paid plan or check workspace settings |
| `OpenCode Go fetch failed: No usage data` | Workspace has no Go subscription | QSKIP is expected; ensure Go workspace has active subscription |
