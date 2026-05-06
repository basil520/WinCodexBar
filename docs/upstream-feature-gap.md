# WinCodexBar 与上游 CodexBar 功能实现差异

本文基于本机两个仓库的当前 checkout：Windows 移植仓库 `D:\WinCodexBar`，上游 macOS 仓库 `D:\CodexBar`。结论只描述当前代码里能确认的实现状态，不推断未提交或外部发布版本。

## 总览

WinCodexBar 已经不是一个“只移植少数 provider”的壳子。它有独立的 Qt/QML 桌面应用、Windows 托盘、ProviderPipeline、Windows Credential Manager、ConPTY、CookieImporter、Codex 多账户、通用 token account、状态轮询、CLI、成本扫描、单元测试和 E2E 测试。Provider 覆盖面已经补齐到上游当前 `UsageProvider` 的 29 个：WinCodexBar 已注册 29 个 provider。

主要未实现项集中在四类：

- Windsurf v1 已补齐，但自动 Chromium localStorage session 导入仍未实现；当前 Web source 只支持手动 session bundle。
- macOS 平台能力：WidgetKit、小组件、Sparkle 更新、全局快捷键、多个独立菜单栏 status item、部分 SwiftUI 菜单 hosted views。
- 上游 CLI/发布链路：`cache clear` 子命令、CLI tarball/Homebrew/安装脚本、appcast 自动更新链路。
- 若干辅助能力未完全等价：Google Workspace 状态源、link-only 状态入口、上游专用诊断 helper、部分菜单级图表/历史视图。

## Provider 覆盖矩阵

| Provider | 上游 CodexBar | WinCodexBar | 差异 |
| --- | --- | --- | --- |
| Codex | 已实现 | 已实现 | Win 版有 OAuth、CLI/RPC、OpenAI dashboard、credits、多账户与 ConPTY 适配；总体是重点移植项。 |
| Claude | 已实现 | 已实现 | Win 版有 OAuth/Web/CLI 路径与本地成本扫描；缺少上游 `CodexBarClaudeWatchdog` 独立 helper。 |
| Cursor | 已实现 | 已实现 | Web cookie 路径已移植；WebKit session fallback 属 macOS 上游实现，Win 版用 Windows browser import。 |
| OpenCode / OpenCode Go | 已实现 | 已实现 | 两者均已移植；Win 版还将 OpenCode DB 纳入成本扫描计划。 |
| Alibaba / Factory / Gemini / Antigravity / Copilot | 已实现 | 已实现 | 使用 Windows 版本的 cookie、CLI、OAuth 或本地探测路径；Gemini/Antigravity 的 Google Workspace 状态轮询未完全等价。 |
| z.ai / MiniMax / Kimi / Kimi K2 / Kilo / Kiro | 已实现 | 已实现 | API、web、CLI 路径均有对应实现。 |
| Vertex AI / Augment / JetBrains / Amp / Ollama | 已实现 | 已实现 | 已移植主要 usage fetch；平台路径和凭据来源按 Windows 调整。 |
| Synthetic / Warp / OpenRouter / Perplexity / Abacus / Mistral / DeepSeek / Codebuff | 已实现 | 已实现 | 已注册并有 provider 文件、设置和测试覆盖的一部分。 |
| Windsurf | 已实现 | 已实现 v1 | 已注册并有图标、设置和测试。本地 SQLite cache 与手动 Web API session bundle 已实现；仍缺上游的 Chromium localStorage 自动导入。 |

证据位置：

- Win provider 注册集中在 [ProviderBootstrap.cpp](D:/WinCodexBar/src/providers/ProviderBootstrap.cpp:36)，当前已包含 `windsurf`。
- 上游 provider 枚举包含 `windsurf`，见 [Providers.swift](D:/CodexBar/Sources/CodexBarCore/Providers/Providers.swift:5)。
- 上游 Windsurf descriptor 定义了 `.auto/.web/.cli` 和 `WindsurfWebFetchStrategy`、`WindsurfLocalFetchStrategy`，见 [WindsurfProviderDescriptor.swift](D:/CodexBar/Sources/CodexBarCore/Providers/Windsurf/WindsurfProviderDescriptor.swift:1)。
- Win 版已包含 `resources/icons/ProviderIcon-windsurf.svg` 并在 `resources/qml.qrc` 中注册。

## Provider 管线与数据源差异

上游使用 Swift 的 `ProviderFetchPlan`/`ProviderFetchStrategy` 异步管线，provider descriptor 通过宏注册；WinCodexBar 使用 C++ `IProvider::createStrategies()` 返回 `IFetchStrategy*`，由同步 `fetchSync()` 顺序执行，并按 `ProviderSourceMode` 过滤。功能上相当接近，但实现差异会影响扩展方式：

- 上游新 provider 只要加 `UsageProvider`、descriptor、implementation，并由宏/registry 接入；Win 版必须手动新增 provider 类、加入 `CMakeLists.txt`、加入 `ProviderBootstrap::registerAllProviders()`、资源和 QML 设置。
- 上游 `ProviderFetchKind` 有 `.localProbe`；Win 版当前 source mode 过滤只公开 `auto/oauth/web/cli/api`，本地探测类 provider 通过内部 strategy kind 运行，但设置层没有独立 local source mode。Windsurf 为保持上游 `auto/web/cli` 命名，暂时把本地 SQLite 策略标为 `ProviderFetchKind::CLI`，`sourceLabel` 仍显示 `local`。
- Win 版 pipeline 是同步执行策略，UI 侧用线程池包裹；上游是 async/await。对 provider 行为本身影响不大，但上游异步取消、actor 隔离和 Swift strict concurrency 的安全网在 Win 版不存在。

## UI 与托盘差异

WinCodexBar 的 UI 是 Qt/QML 三窗口：托盘弹窗、设置窗口、Usage Details。上游是 SwiftUI + AppKit 菜单栏应用，菜单里有多种 hosted submenu 和多个 `NSStatusItem` 行为。

当前 Win 版已实现：

- QML 托盘用量面板、provider 卡片、刷新/设置/退出。
- 设置页：General、Providers、Display、Advanced、About，Debug 按开关显示。
- Provider 启用/排序、设置字段、敏感字段、连接测试、登录流程。
- Display 中有 Merge Icons、使用量显示方向、绝对重置时间、额外 credits/usage。
- Usage Details 窗口和 token cost 概览。

仍未完全等价：

- 上游“Merge Icons 关闭时每个 provider 一个菜单栏图标”的体验没有等价实现。Win 版 Windows 托盘始终只有一个 tray icon；关闭 Merge Icons 时只是切换当前选中的 provider icon。
- 上游 merged menu 的 provider switcher、overview tab provider 选择、highest usage 自动选择没有完整移植。Win 版有 `mergeIcons` 设置，但没有上游 `menuBarShowsHighestUsage`、`mergedOverviewSelectedProviders` 这组选择逻辑。
- 上游菜单中的 credits history、cost history、usage breakdown、storage breakdown 等 SwiftUI hosted charts 只部分映射到 QML。Win 版托盘中有 token usage、daily bars、Codex dashboard details、plan utilization chart，但不是完整的上游菜单视图集合。
- 上游全局快捷键打开菜单使用 `KeyboardShortcuts`；Win 版没有全局 hotkey 设置。
- 上游 weekly reset confetti 使用 `ScreenConfettiOverlayController`；Win 版没有等价动效。

## 状态检查差异

WinCodexBar 已有状态轮询开关和 `UsageStore::refreshProviderStatuses()`，但当前只处理 `statusPageURL` 并拼接 `/api/v2/status.json`。这意味着：

- OpenAI/Codex、Claude、Cursor、Factory、Copilot 这类 Statuspage 源可以工作。
- 上游对 Gemini/Antigravity 使用 Google Workspace incidents feed；Win 版 provider metadata 里虽然有 `statusWorkspaceProductID`，但 `refreshProviderStatuses()` 没有读取 Google feed，因此这两者的状态检查缺口仍在。
- 上游支持 `statusLinkURL` 这种“只打开状态页，不自动轮询”的入口；Win 版 descriptor 暴露了字段，但轮询和菜单动作没有完整使用它。

相关代码：

- Win 状态轮询实现见 [UsageStore.cpp](D:/WinCodexBar/src/app/UsageStore.cpp:2096)。
- 上游状态设计见 [status.md](D:/CodexBar/docs/status.md:1)。

## 设置、凭据与账户

这里是“实现不同”，不是简单缺失：

- 上游非敏感 provider 配置和 token accounts 写入 `~/.codexbar/config.json`，Keychain 用于 cookie/cache/OAuth 等凭据。
- WinCodexBar 全局设置进 Windows Registry，provider 非敏感设置进 `%AppData%/CodexBar/config.json`，敏感字段通过 `ProviderCredentialStore`/Windows Credential Manager，环境变量仍可覆盖。
- WinCodexBar 已实现 Codex managed accounts 和通用 token accounts；这部分并不是缺口。

需要注意的缺口：

- 上游有 Keychain prompt coordination、Keychain migration、macOS Full Disk Access 引导等平台能力；Win 版没有也不需要等价实现，但对“从 macOS 文档直接照搬到 Windows”的用户体验不成立。
- 上游 provider 设置 UI 有部分 SwiftUI picker/field 的动态说明和 trailing text；Win 版通过统一 QML descriptor 渲染，个别 provider 的细粒度说明可能比上游少。

## CLI 差异

WinCodexBar 有 `usage`、`cost`、`config` 三类 CLI 命令，并支持 text/json、provider/source 过滤和配置 validate/dump。

未实现或不等价：

- 上游 CLI 有 `cache clear`，Win 版 `CLIEntry::isCliCommand()` 只接受 `usage/cost/config`。
- 上游没有命令时默认进入 `usage`；Win 版没有 positional command 会显示 help 并返回错误。
- 上游 CLI 发布包含 macOS/Linux tarballs、Homebrew formula、`bin/install-codexbar-cli.sh`；Win 版当前是主 exe 内置 CLI 子命令，没有独立 CLI 安装/发布链路。
- 上游 CLI 使用 Commander，有更完整的统一参数解析和错误输出结构；Win 版用 `QCommandLineParser`，功能面较窄。

证据：

- 上游命令列表见 [CLIEntry.swift](D:/CodexBar/Sources/CodexBarCLI/CLIEntry.swift:35)。
- Win 命令入口见 [CLIEntry.cpp](D:/WinCodexBar/src/cli/CLIEntry.cpp:10)。

## 更新、发布与安装

上游完整实现了 Sparkle 更新：

- `SparkleUpdaterController`、稳定/测试 channel、appcast、自动检查和手动检查。
- 发布脚本、签名、公证、Homebrew、appcast 校验。

WinCodexBar 当前有 `checkForUpdates` 设置项，但代码里只持久化这个偏好，没有发现实际更新检查、下载、安装、release feed 或 update channel 实现。因此“检查更新”目前应视为 UI/设置占位。

Win 版已有：

- CMake 构建。
- Qt `windeployqt` 自动部署。
- `.ts` 到 `.qm` 的翻译编译。
- `Scripts/sign.ps1` 这样的本地签名脚本。

未实现：

- Windows 安装器/MSIX/MSI/winget/Homebrew 等正式分发链路。
- 自动更新框架。
- 版本 feed、增量/全量更新校验、update channel。

## Widget 与系统集成

上游有 `Sources/CodexBarWidget` WidgetKit 扩展，提供 switcher、usage、history、compact metric 四类 widget。WinCodexBar 有 `WidgetSnapshot` 模型，但没有 Windows Widgets 扩展、没有写共享 widget snapshot 的系统集成，也没有 Windows 小组件 manifest/package。

未实现：

- WidgetKit 对应能力。
- Windows Widgets 或 Live Tile 等替代实现。
- provider picker 小组件配置。
- widget snapshot 发布到系统组件。

上游文档见 [widgets.md](D:/CodexBar/docs/widgets.md:1)。

## 辅助进程与诊断工具

上游有多个独立 target：

- `CodexBarClaudeWatchdog`：辅助 Claude CLI PTY 稳定性。
- `CodexBarClaudeWebProbe`：Claude web fetch 诊断。
- `CodexBarMacros` / `CodexBarMacroSupport`：provider 注册宏。

WinCodexBar 没有这些 target。部分能力被 Windows 原生实现替代，例如 ConPTY 和 QML debug log；但上游的独立 Claude helper/probe 与宏化注册没有等价实现。

## 测试覆盖差异

WinCodexBar 当前测试已经不少：`tests` 下约 38 个单元测试目标，`tests/e2e` 下 5 个真实账号 E2E。上游 `Tests/CodexBarTests` 当前约 240 个 Swift 测试文件，覆盖面明显更广，尤其是：

- Sparkle/update channel、WidgetKit、KeyboardShortcuts、macOS Keychain/权限。
- 多 provider 的 parser characterization、menu descriptor、menu card、UI smoke。
- Provider status、browser cookie order、storage footprint、config migration。
- Windsurf 自动 browser localStorage session 导入。

因此 Win 版缺口不只是功能本身，也包括这些功能对应的回归测试。

## 建议实现顺序

1. **补 Windsurf 自动 localStorage 导入**：当前 provider v1 已可用，剩余是从 Chromium `Local Storage/leveldb` 自动读取 Devin session。
2. **补状态源兼容**：实现 Google Workspace incidents feed；让 `statusLinkURL` 在 Provider 设置/托盘菜单里可打开。
3. **清理更新 UI**：要么实现 Windows 自动更新/检查更新，要么暂时隐藏 `Check for Updates`，避免给用户错误预期。
4. **补 CLI cache clear**：实现 cookie/cost/dashboard cache 清理，并与上游 CLI 行为对齐。
5. **增强托盘行为**：决定 Windows 上是否真的需要多 tray icon；如果不做，也应在文档中说明这是 Windows 产品形态差异。
6. **Widget/发布链路**：这是大块平台工程，建议晚于 provider parity 和状态/更新基础能力。

## 快速清单

- [x] Windsurf provider v1：本地 SQLite + 手动 Web API session bundle。
- [x] Windsurf icon/resource/QML 设置/测试。
- [ ] Windsurf Chromium localStorage 自动 session 导入。
- [ ] Google Workspace provider status polling。
- [ ] `statusLinkURL` 菜单入口。
- [ ] `codexbar cache clear` 等价命令。
- [ ] 实际 Windows 更新检查/安装，或隐藏更新开关。
- [ ] 独立 CLI 发布/安装路径。
- [ ] Widget/Windows 小组件。
- [ ] 全局快捷键。
- [ ] 多 provider 独立托盘图标，或明确不做。
- [ ] 上游菜单中的完整 history/breakdown hosted views。
- [ ] Claude watchdog/web probe 等价诊断能力。
- [ ] 更完整的 provider parity 测试。

## 详细修复方案

下面按“先补用户可见缺口，再补平台工程”的顺序展开。每项都给出建议改动点、实现路线和验收方式。

### 1. Windsurf provider

**当前状态**

WinCodexBar 已实现 Windsurf provider v1，核心文件和接入点如下：

- `src/providers/windsurf/WindsurfProvider.h`
- `src/providers/windsurf/WindsurfProvider.cpp`
- `tests/tst_WindsurfProvider.cpp`
- `resources/icons/ProviderIcon-windsurf.svg`
- `resources/qml.qrc`
- `src/providers/ProviderBootstrap.cpp`
- `CMakeLists.txt`

已实现能力：

- Metadata：`id = "windsurf"`、`displayName = "Windsurf"`、`sessionLabel = "Daily"`、`weeklyLabel = "Weekly"`、`dashboardURL = "https://windsurf.com/subscription/usage"`、`brandColor = "#34E8BB"`。
- Source modes：`auto/web/cli`。
- 本地策略：读取 `%APPDATA%/Windsurf/User/globalStorage/state.vscdb`，并额外尝试 `%LOCALAPPDATA%/Windsurf/User/globalStorage/state.vscdb`；支持 `CODEXBAR_WINDSURF_STATE_DB` 覆盖。
- SQLite 查询：`SELECT value FROM ItemTable WHERE key = 'windsurf.settings.cachedPlanInfo' LIMIT 1`。
- SQLite value 解码：支持 TEXT、UTF-8 BLOB、UTF-16LE BLOB。
- 使用量映射：优先用 `quotaUsage.dailyRemainingPercent` / `weeklyRemainingPercent` 映射每日和每周窗口；缺失时回退到 messages / flowActions 的 used / total。
- Web 策略：支持手动 Windsurf session JSON/key-value bundle，调用 `GetPlanStatus` protobuf endpoint。
- Protobuf：已移植最小 request encode 和 response decode，解析 plan name、plan end、daily/weekly quota/reset。
- 凭据：手动 session 存入 Windows Credential Manager，credential target 为 `com.codexbar.session.windsurf`，环境变量覆盖为 `CODEXBAR_WINDSURF_SESSION`。
- UI/资源：图标已加入 QRC，托盘品牌色 map 已补 `windsurf`。
- 测试：`tst_WindsurfProvider` 覆盖 metadata、cached plan JSON、usage fallback、SQLite BLOB、manual session parse、protobuf codec、source mode filtering、Web 配置错误；`tst_ProviderBootstrap` 覆盖注册；`tst_FetchContext` 覆盖 `cookieSource=off` 默认值。

**剩余差异**

上游 Windsurf Web 策略可以从 Chromium localStorage 自动导入 Devin session。WinCodexBar v1 暂未实现该能力，因为现有 Windows 侧只有 `BrowserDetection` / `CookieImporter`，没有读取 Chromium `Local Storage/leveldb` 的 importer。

当前行为：

- `auto`：如果用户配置了手动 session bundle，则先尝试 Web；否则尝试本地 SQLite cache。
- `web`：只运行 Web；`cookieSource=off` 或没有手动 session 时返回明确配置错误。
- `cli`：运行本地 SQLite cache。为保持上游 source mode 命名，Win 版本地策略的 `kind()` 暂用 `ProviderFetchKind::CLI`，但 `sourceLabel` 仍是 `local`。

**后续措施**

1. 新增 `BrowserStorageImporter` 或等价 helper，复用 `BrowserDetection` 的 Chromium profile 枚举。
2. 读取 Chrome/Edge/Brave/Vivaldi 等 profile 下的 `Local Storage/leveldb`。
3. 按 origin `https://windsurf.com` 提取以下 key：
   - `devin_session_token`
   - `devin_auth1_token`
   - `devin_account_id`
   - `devin_primary_org_id`
4. 支持 localStorage 值的 JSON string wrapper 解码、去引号和空白裁剪。
5. 对同一个 `devin_session_token` 去重。
6. 自动导入顺序对齐上游：优先 Chrome，再 fallback 到其他 Chromium 浏览器。
7. 只对自动导入 session 做 400/401/403 后尝试下一个 profile；手动 session 仍应直接报错，不 fallback。

**已验证**

```powershell
cmake --build build --config Release --target tst_WindsurfProvider -- /m /v:minimal /clp:ErrorsOnly
ctest --test-dir build -C Release -R Windsurf --output-on-failure
cmake --build build --config Release --target tst_FetchContext -- /m /v:minimal /clp:ErrorsOnly
ctest --test-dir build -C Release -R FetchContext --output-on-failure
ctest --test-dir build -C Release -R ProviderBootstrap --output-on-failure
cmake --build build --config Release --target WinCodexBar -- /m /v:minimal /clp:ErrorsOnly
```

### 2. Provider 状态检查

**当前差异**

WinCodexBar 已有 `statusPageURL` 轮询，但只支持 Statuspage.io 的 `/api/v2/status.json`。上游还支持：

- Google Workspace incidents feed，用于 Gemini / Antigravity。
- `statusLinkURL`，用于只提供状态页链接、不自动轮询的 provider。

Win 版虽然把 `statusLinkURL`、`statusWorkspaceProductID` 放进 descriptor map，但 `UsageStore::refreshProviderStatuses()` 没消费 `statusWorkspaceProductID`，QML `ProviderDetailView` 的 Status 按钮也只看 `statusPageURL`。

**建议实现**

1. 抽一个状态 helper，避免 `UsageStore.cpp` 继续膨胀：
   - `src/providers/shared/ProviderStatusFetcher.h`
   - `src/providers/shared/ProviderStatusFetcher.cpp`
2. 保留现有 Statuspage fetch：
   - 输入 `statusPageURL`
   - 输出 `{ state, description, updatedAt }`
   - `none -> ok`，`minor/maintenance -> degraded`，`major/critical -> outage`。
3. 新增 Google Workspace fetch：
   - GET `https://www.google.com/appsstatus/dashboard/incidents.json`
   - 过滤 `end == null`
   - 匹配 `currently_affected_products[].id`，没有该字段时退回 `affected_products[].id`
   - 按上游规则映射：`SERVICE_INFORMATION -> degraded`，`SERVICE_DISRUPTION -> outage` 或 degraded/major，`SERVICE_OUTAGE -> outage`，`AVAILABLE -> ok`
   - summary 从 `most_recent_update.text` 中去掉 `**Summary**`、Markdown link、列表前缀。
4. 修改 `UsageStore::refreshProviderStatuses()`：
   - 如果有 `statusPageURL`，走 Statuspage。
   - 否则如果有 `statusWorkspaceProductID`，走 Workspace feed。
   - 只有 `statusLinkURL` 的 provider 不轮询，但 UI 显示链接按钮。
5. 修改 QML：
   - `qml/components/ProviderDetailView.qml` 中 Status 按钮改成 `visible: descriptor.statusPageURL || descriptor.statusLinkURL`。
   - 点击 URL 使用 `descriptor.statusLinkURL || descriptor.statusPageURL`。

**验收**

- 端到端：Gemini/Antigravity 设置页 Status 不再长期 Unknown。
- 单测：移植上游 `GoogleWorkspaceStatusTests` 和 `ProviderMetadataStatusLinkTests` 的核心 fixture。
- 加 Win 版测试：`tst_ProviderStatusFetcher.cpp`。

### 3. CLI `cache clear`

**当前差异**

上游 CLI 有 `cache clear --cookies|--cost|--all [--provider]`。WinCodexBar CLI 只有 `usage`、`cost`、`config`。Win 版已有 `UsageStore::clearCache()`，但它只清内存 snapshot/credential cache，不是上游 CLI 语义；成本扫描缓存由 `CostUsageCache` 管，浏览器 cookie import 还有 `CookieImporter` 内存 cache。

**建议实现**

1. 新增：
   - `src/cli/CLICacheCommand.h`
   - `src/cli/CLICacheCommand.cpp`
2. 修改：
   - `CLIEntry::isCliCommand()` 增加 `cache`
   - `CLIEntry::run()` 增加 `cache` 分支
   - `CMakeLists.txt` 加 CLI cache 文件
3. 参数对齐上游：
   - `cache clear --cookies`
   - `cache clear --cost`
   - `cache clear --all`
   - `--provider <id>` 只作用于 cookie/secret cache，不作用于 cost cache
   - `--format text|json`、`--pretty`
4. 清理对象：
   - cost：`CostUsageCache::instance().load(); clear(); save();`
   - cookie import 内存：给 `CookieImporter` 增加 `clearCache()` 和可选 `clearCache(domains/provider)`。
   - 手动 cookie secret：遍历所有 provider `settingsDescriptors()`，只移除 key/credentialTarget 明确属于 cookie/session 的 descriptor，避免误删 API key。
   - Codex dashboard cache：可选纳入 `--all`，调用 `CodexDashboardCache::clear()`。

**验收**

- `WinCodexBar.exe cache clear --cost`
- `WinCodexBar.exe cache clear --cookies --provider perplexity`
- `WinCodexBar.exe cache clear --all --format json --pretty`
- 新增 `tst_CLI` 用例覆盖未知 provider、provider+cost 冲突、json 输出。

### 4. 更新检查

**当前差异**

WinCodexBar 有 `SettingsStore.checkForUpdates` 和设置页开关，但没有 updater。上游 Sparkle 是完整链路：feed、channel、下载、重启安装、About UI。

**建议实现**

短期二选一：

- **产品诚实方案（推荐先做）**：隐藏或禁用 `Check for Updates`，说明 Windows 版暂未提供自动更新。
- **轻量检查方案**：只检查 GitHub Releases 最新版本，不自更新；有新版本时打开 release 页面。

完整方案：

1. 新增 `src/app/UpdateChecker.{h,cpp}`。
2. 用 `NetworkManager` 请求 GitHub Releases API 或项目自有 `latest.json`。
3. 版本比较使用语义版本解析，不做字符串比较。
4. SettingsStore 增加 `updateChannel` 后再考虑 stable/beta。
5. AboutPane 增加 “Check Now” 和当前版本。
6. 真正自更新需要 Windows installer/MSIX/MSI 或外部 updater helper；主进程不能安全覆盖自身 exe。

**验收**

- 关闭开关时不发网络请求。
- 开启开关时最多每日后台检查一次。
- 手动检查可显示 “已是最新/发现新版本/检查失败”。
- 若还没有 installer，不要自动下载并替换 exe。

### 5. Widget / WidgetSnapshot

**当前差异**

WinCodexBar 的 `WidgetSnapshot.h` 明确写着 “currently unused”。上游 `WidgetSnapshotStore` 会把 JSON 写到 app-group container，WidgetKit 扩展读取并渲染四类 widget。Win 版没有 Windows Widgets provider、没有 snapshot 写入流程，也没有系统注册包。

**建议实现**

建议分阶段，而不是直接追 WidgetKit parity：

1. 先决定是否做 Windows Widgets。第三方 Windows Widgets 通常需要打包和系统集成，工作量明显大于普通 Qt 功能。
2. 如果暂不做：
   - 保留 `WidgetSnapshot` 但在差异文档和代码注释中标注“预留”。
   - 不在 README 宣传 widgets。
3. 如果要做：
   - 先让 `UsageStore` 在刷新完成后生成 `%LOCALAPPDATA%/CodexBar/widget-snapshot.json`。
   - 补 `WidgetSnapshot` 到 JSON 的序列化，字段对齐上游：`usageRows`、`tokenUsage`、`codeReviewRemainingPercent`。
   - 再做 Windows Widget provider 或小型 companion app 读取该 JSON。

**验收**

- 没有 widget 系统实现前，仓库里不应出现“Windows Widgets 已支持”的用户文案。
- 如果实现 snapshot，增加 JSON golden test。

### 6. 托盘与 Display 设置

**当前差异**

上游在 Merge Icons 关闭时可以为每个 provider 创建独立菜单栏图标；WinCodexBar 始终只有一个 Windows tray icon。上游 Display 还有 `Switcher shows icons`、`Show most-used provider`、`Menu bar shows percent`、`Display mode`、`Overview tab providers`、`Show all token accounts in menu` 等设置，Win 版只移植了其中一部分。

**建议实现**

先做产品决策：

- 如果 Windows 版坚持单 tray icon：把 “Merge Icons” 文案改成更准确的 “Combined tray icon / Combined mode”，并在文档说明 Windows 版不支持多托盘图标。
- 如果追上游行为：`StatusItemController` 需要管理多个 `NOTIFYICONDATAW`，每个 provider 一个 `uID`、tooltip、右键菜单和点击行为。

多图标实现路线：

1. `StatusItemController` 增加 `QHash<QString, NOTIFYICONDATAW>` 或轻量 wrapper。
2. provider enabled/order 变化时增删 tray icon。
3. 每个 icon 点击可以打开同一个 QML tray panel，但预选对应 provider。
4. `TrayIconRenderer` 接收 provider icon style/brand color，不再全是 Default。
5. Explorer 重启时重建所有 icon。

Display 设置补齐路线：

1. `SettingsStore` 增加缺失属性。
2. `DisplayPane.qml` 增加对应开关/选择器。
3. `StatusItemController::applyMergedIcon()` 增加 highest-usage 选择逻辑。
4. QML tray panel 增加 overview provider selection 时，复用 provider order 和 enabled providers。

**验收**

- 单图标路线：UI 文案不再暗示多图标。
- 多图标路线：启用 3 个 provider 时 Windows tray 有 3 个可独立 hover/click 的 icon，Explorer 重启后恢复。

### 7. Codex dashboard / consumer projection

**当前差异**

Win 版已经有 `OpenAIDashboardFetcher`、`CodexDashboardCache` 和 QML 中的部分 dashboard 展示，但 `CodexConsumerProjection.cpp` 仍有 TODO：`hasUsageBreakdown`、`hasCreditsHistory` 固定为 false，`resolveDashboardVisibility()` 当前也返回 Hidden。因此上游 Codex dashboard 的消费投影还没完全接上。

**建议实现**

1. 扩展 `UsageStore::codexConsumerProjectionData()` 的 context，把 `m_dashboardData["codex"]` 转成 projection 能理解的数据。
2. `CodexConsumerProjection::Context` 增加 dashboard snapshot 或简化后的 dashboard fields。
3. `resolveDashboardVisibility()` 按上游规则判断 Attached/Hidden。
4. `hasUsageBreakdown`、`hasCreditsHistory` 根据 dashboard data 设置。
5. QML 中已展示的 `Credit Events`、`Usage by Service` 可以保留，但最好和 projection 的可见性一致。

**验收**

- Codex dashboard 成功抓取后，projection 中 `hasUsageBreakdown/hasCreditsHistory` 为 true。
- 无 dashboard 或登录失效时，UI 不显示空区块，只显示清晰错误。
- 加 `tst_CodexConsumerProjection` 覆盖。

### 8. Claude helper / diagnostics

**当前差异**

上游有 `CodexBarClaudeWatchdog` 和 `CodexBarClaudeWebProbe`。Win 版已使用 ConPTY，能覆盖很多 Windows CLI 场景，但缺少独立 watchdog/probe 这种“诊断/隔离”能力。

**建议实现**

短期：

- 在 DebugPane 或 CLI 增加 `diagnose claude`，输出 OAuth、web cookie、CLI PTY 三条路径的可用性和最近错误。

中期：

- 如果 Claude CLI PTY 在 Windows 上仍有卡死/孤儿进程问题，新增一个小 helper exe，例如 `WinCodexBarClaudeProbe.exe`，由主进程启动并设置严格 timeout。
- helper 和主进程用 stdout JSON 通信，避免共享 Qt UI 线程状态。

**验收**

- 诊断命令不会弹 UI，不会泄露 token。
- 超时后 helper 子进程被清理。

### 9. 测试补齐策略

**当前差异**

Win 版测试数量已经不少，但上游测试覆盖的是更细的 provider parser、menu descriptor、widget、status、config migration、update channel 等。新增功能时建议按缺口补，而不是机械移植全部 Swift 测试。

**建议优先补的测试**

1. `tst_WindsurfProvider`：已补 v1 provider、SQLite、manual session、protobuf、source mode 覆盖；后续还需补自动 localStorage importer 测试。
2. `tst_ProviderStatusFetcher`
3. `tst_CLICacheCommand`
4. `tst_UpdateChecker`（如果实现轻量检查）
5. `tst_CodexConsumerProjection`
6. `tst_TrayIconMode` 或拆到可测试的 selection helper，避免直接测 Windows shell icon。

**通用验收命令**

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

涉及真实账号的 provider 不应放进默认单元测试；继续放在 `tests/e2e` 并维持 `BUILD_E2E_TESTS=ON` opt-in。
