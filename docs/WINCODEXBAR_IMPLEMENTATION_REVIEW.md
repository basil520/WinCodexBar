# WinCodexBar 实现审查

Date: 2026-05-01

## 范围

本文比较当前 `WinCodexBar/` 目录下的 Qt/C++ Windows 实现与原版 Swift/macOS CodexBar。它记录的是当前源码状态、与原版差异、剩余未实现功能和优先级，不替代 `docs/WINDOWS_MIGRATION.md` 的设计/目标文档。

主要对照来源：

- `Sources/CodexBarCore/Providers/Providers.swift`
- `Sources/CodexBarCore/Providers/**`
- `Sources/CodexBar/**`
- `Sources/CodexBarCLI/**`
- `Sources/CodexBarWidget/**`
- `docs/WINDOWS_MIGRATION.md`
- `WinCodexBar/src/**`
- `WinCodexBar/qml/**`
- `WinCodexBar/tests/**`

## 执行摘要

2026-05-01 复审确认：当前 `WinCodexBar` 已不再是旧审查里描述的 8 provider MVP。源码在 `WinCodexBar/src/main.cpp` 注册了 15 个 provider：

- Codex
- Claude
- Cursor
- Copilot
- z.ai
- OpenRouter
- Kimi K2
- Kilo
- Kiro
- Mistral
- Ollama
- Kimi（普通，2026-04-30 新增）
- OpenCode（2026-04-30 新增）
- OpenCode Go（2026-04-30 新增）
- Alibaba（API/Web Cookie best-effort，2026-05-01 复审确认已注册）

原版 macOS CodexBar 的 `UsageProvider` 定义了 27 个 provider。因此当前 Windows 覆盖为 15/27，仍有 12 个 provider 完全缺失：

- Factory/Droid
- Gemini
- Antigravity
- MiniMax
- Vertex AI
- Augment
- JetBrains AI
- Amp
- Synthetic
- Warp
- Perplexity
- Abacus AI

功能差异复审结论：

- Provider 覆盖差异仍是最大缺口：Windows 端已覆盖 Codex、Claude、Cursor、Copilot、z.ai、OpenRouter、Kimi K2、Kilo、Kiro、Mistral、Ollama、Kimi、OpenCode、OpenCode Go、Alibaba；尚未覆盖 Factory/Droid、Gemini、Antigravity、MiniMax、Vertex AI、Augment、JetBrains AI、Amp、Synthetic、Warp、Perplexity、Abacus AI。
- 已覆盖 provider 多数仍是“窄路径”实现：Codex 已补齐 OAuth、CLI RPC、CLI PTY、Web fallback 和基础 managed account 设置链路；Claude 是 OAuth/Web cookie；Cursor 是 Web cookie；Kilo 是 API；Kiro 是非交互 CLI；Mistral/Ollama 是基础 Web cookie 形状；Alibaba 是 DashScope/API 与控制台 Cookie 的 best-effort 形状。原版对应 provider 通常还包含 richer descriptor、fallback/source planner、status probe、CLI/Web/credential prompt policy 或 provider-specific settings。
- 原版 merged icon 下的 provider switcher、Overview tab、Codex account switcher、token account switcher、OpenAI dashboard chart submenus、credits/usage breakdown/cost history hosted submenus，在 Windows 托盘 QML 面板中尚未功能对等。Windows 端已有 merged tray icon 与 provider 列表，但交互模型更简单。
- 原版 `codexbar` CLI、WidgetKit widget、Sparkle/appcast/release scripts、配置校验/JSON 输出、token account CLI 等生态能力，Windows 端当前仍缺 CLI executable、IPC/shared snapshot、updater、installer、signing/release automation。WidgetKit 属于 macOS 专属能力，不计为 Windows 缺陷。
- 默认 CTest 覆盖当前是 15 个测试目标；真实账号 E2E 是 `BUILD_E2E_TESTS=ON` 的 opt-in 目标（5 个），不属于默认构建。仍缺自动化 QML visual regression、真实托盘点击、真实 WinCred 交互、真实浏览器 profile 与更多 provider E2E。

2026-04-30 的 P0 基础设施实现后，旧文档中关于 cookie/browser、ConPTY、provider pipeline 和 fetch context 的阻塞性结论已经过期。当前 P0 基础能力已经落地并有 QtTest 覆盖：

- Windows browser profile detection：Edge、Chrome、Brave、Opera、Vivaldi、Firefox，并支持测试/调试用环境变量 override。
- Cookie import：Chromium SQLite cookie DB、Firefox `cookies.sqlite`、plaintext、legacy DPAPI、Chromium `v10/v11` AES-GCM；Chrome App-Bound/`v20` cookie 会安全跳过。
- ConPTY：真实 `CreatePseudoConsole` 路径、stdin/stdout pipe、reader thread、`waitForPattern()`、`terminate()`，并在 ConPTY 不可用或启动失败时 fallback 到 `QProcess`。
- Provider pipeline：主路径改为 `fetchSync(const ProviderFetchContext&)`，pipeline 不再嵌套等待 strategy `QFuture`。
- NetworkManager：新增同步 `getJsonSync`、`getStringSync`、`postJsonSync`、`postFormSync`，async wrapper 仅保留兼容。
- Fetch context：集中在 `UsageStore::buildFetchContextForProvider()` 注入 provider id、系统环境、settings snapshot、source mode、manual cookie、cookie source、network timeout 和 z.ai region 兼容映射。

2026-04-30 的 P1 基础闭环实现后，旧文档中关于 descriptor/source planner、API key UI、Copilot foreground login、status polling 和 merged tray icon 的 P1 阻塞性结论也已经过期。当前 P1 能力已经落地并通过 Release build、QtTest 和短启动 smoke 验证：

- ProviderDescriptor registry：provider 注册时自动生成 string-id descriptor，包含 display name、labels、dashboard/status URL、source modes 等 metadata。
- Provider settings UI：`SettingsWindow.qml` 改为 descriptor-driven，picker 使用 provider options，secret field 使用密码输入框，Dashboard/Status/Test Connection/Login action 从 QML 调用 `UsageStore`。
- Secret/credential：新增 `ProviderCredentialStore` facade，生产使用 Windows Credential Manager，测试可注入 in-memory backend；API key/manual cookie 通过 WinCred 管理，环境变量优先级高于 WinCred。
- Source planner：`ProviderPipeline::resolveStrategies()` 按 `ProviderSourceMode` 过滤 strategy；`ProviderPipeline::executeProvider()` 在 worker 线程内创建、执行并销毁 strategy，避免后台刷新/Test Connection 泄漏；unsupported mode 会产生明确错误。
- Copilot foreground login：新增 GitHub device-code 前台登录流程，显示 user code / verification URL，成功后写入 `com.codexbar.oauth.copilot` 并触发连接测试；后台刷新保持 non-interactive。
- Status polling / merged icon：新增 statuspage-style `/api/v2/status.json` polling，provider status model 暴露给 UI；merged tray icon 以所有 enabled providers 中最低 remaining 渲染，并在 tooltip 中显示最紧张 provider 与异常状态。

2026-04-30 后续修复了 Settings 窗口空白问题。根因是 Settings pane 未导入 `qml/components` 下的共享组件，且 Settings 使用的 `AppTheme` 主题对象缺少可靠 singleton/context 暴露；修复后 `SettingsWindow` 已通过 `--show-settings --qml-log=...` 运行时 smoke 验证，窗口状态为 `QQuickView::Ready`，实际截图确认 General 页可见，日志不再包含 SettingsWindow QML 加载错误。

2026-04-30 后续完成 Settings UI redesign：Settings 窗口改为 frameless 深色自绘顶栏，左侧固定导航和内容区改为紧凑工作台布局；Providers 页重做了 provider 列表、详情 header、usage/connection/settings 分区和 Test Connection inline 状态反馈。后续又修复了 provider switch 尺寸/颜色、provider 列表进度条挤压开关、状态 pill 偏移、Kimi K2 logo 资源等可见问题。当前 Settings 页的主流程体验已明显优于旧版“铺满式 widget layout”，但仍缺自动化 QML visual regression 测试。

2026-04-30 后续曾尝试接入 Windows 原生背景视觉层。实际验证结论是该路径依赖系统级视觉开关，强行打开会对其他软件产生显示副作用；相关源码、构建项、窗口初始化调用和 QML 半透明背景变量已经全部移除。当前 Settings 与 tray popup 回到普通深色实色窗口。

2026-05-01 后续补齐了 Codex Settings 账号管理主链路：Provider detail 内容区可滚动，Account Management 不再在默认 `900x640` 或最大化窗口中溢出；Add Account 改为前台 `codex login --device-auth` 设备码流程，使用独立 managed `CODEX_HOME`；active managed account 会注入 Codex OAuth/RPC/PTY fetch context。设备码解析也修复了把 `AUTHORIZATION` 当作 user code 的回归。更详细的连接、账号和排查说明见 `docs/CODEX_PROVIDER.md`。

这意味着 P0/P1 已从“阻塞基础设施与主流程 UI”转为“真实账号验证与 provider parity”。剩余主要工作集中在 P2-P3：Codex/Claude/Kiro 等核心 provider parity、缺失 provider、QML/tray 交互测试、真实 OAuth/WinCred/浏览器 profile 端到端测试、CLI/IPC/release 生态和 Windows 发布链路。

## 当前已实现功能

### 应用外壳

- Qt Widgets + Qt Quick/QML 应用入口。
- Windows Mutex 单实例保护（`CreateMutexW`），进程终止时自动释放，避免 `QSharedMemory` 僵尸锁问题。
- `QApplication::setQuitOnLastWindowClosed(false)` 常驻托盘。
- 应用退出时后台线程清理：`aboutToQuit` 中设置 `NetworkManager::setShuttingDown(true)` 和 `CostUsageScanner::setShuttingDown(true)`，取消线程池等待队列；`app.exec()` 返回后 `waitForDone(1500ms)`，超时使用 `ExitProcess()` 强制终止，防止托盘消失但进程残留。
- Win32 tray icon 与右键菜单，包含刷新、设置、关于、退出等基础动作。
- QML `TrayPanel.qml` 作为托盘弹窗。
- QML `SettingsWindow.qml` 作为设置窗口。
- `AppController` 连接 QML 与应用生命周期操作。
- 开机启动开关。
- `LanguageManager` 语言切换。

### Provider 运行时

- `IProvider` / `IFetchStrategy` / `ProviderPipeline` 基础接口。
- `IFetchStrategy::fetchSync()` 作为 pipeline 主路径；`fetch()` 作为兼容 wrapper。
- 已注册 provider 的 strategy 已迁移到同步 fetch 路径。
- `ProviderRegistry::registerProvider()` 会自动注册 string-id `ProviderDescriptor`，descriptor 已用于 display metadata、dashboard/status URL 和 source modes。
- `ProviderPipeline::execute()` 直接调用 `fetchSync()`，按 `shouldFallback` 决定是否继续尝试下一个 strategy；`executeProvider()` 负责 resolve + execute + cleanup 的 owning 路径。
- `ProviderPipeline::resolveStrategies()` 已根据 `ProviderSourceMode` 过滤 OAuth/Web/CLI/API strategy。
- `UsageStore` 提供 QML 可访问的 provider list、snapshot、错误、刷新状态。
- `UsageStore` 提供 QML 可访问的 provider descriptor、secret status、Test Connection、Copilot foreground login、provider status polling 状态。
- provider enabled 状态启动时从注册表读取；没有保存值时使用 provider `defaultEnabled()`。
- 自动刷新计时器根据 `SettingsStore::refreshFrequency()` 启停。
- 手动刷新会触发成本扫描和 provider 刷新。
- session quota transition notification 基础逻辑已接入。

### Codex Provider 连接路径

Codex provider 当前按原版 CodexBar 的主路径重构为 OAuth 优先、CLI RPC 次之、PTY `/status` 兜底：

- `auto` source mode 顺序：OAuth API → `codex.cli.rpc` → `codex.cli.pty` → Web Dashboard。
- `cli` source mode 顺序：`codex.cli.rpc` → `codex.cli.pty`，不会尝试 OAuth/Web。
- CLI RPC 启动命令：`codex -s read-only -a untrusted app-server --listen stdio://`。
- CLI RPC 发送 `initialize`、`initialized`、`account/rateLimits/read`、`account/read`；`rateLimitsByLimitId.codex` 优先，缺失时回退到历史 `rateLimits`。
- `primary`/`secondary` 映射为 `UsageSnapshot` 窗口；非 unlimited credits 同步到 `ProviderFetchResult.credits` 和现有 UI 兼容字段 `UsageSnapshot.providerCost`。
- Settings -> Providers -> Codex 的 Account Management 已使用可通知的 `UsageStore.codexAccountState`；Add Account 通过 `codex login --device-auth` 在独立 `CODEX_HOME` 下登录，active managed account 会注入 OAuth/RPC/PTY fetch context。
- device-auth 输出解析已收紧，避免把说明文本里的 `authorization` 误当成设备码；该回归由 `tst_CodexLoginRunner::ignoresAuthorizationTextBeforeRealDeviceCode` 覆盖。
- 原始 CLI stdout/stderr、TUI 控制序列和 JSON-RPC error body 只进入 debug log 或解析恢复路径；Settings 连接失败卡片显示短错误，避免原始 TUI 输出撑破布局。
- 常用排查命令：`codex --help`、`codex app-server --help`、`codex app-server generate-json-schema --out build\codex-app-schema --experimental`。
- 详细说明见 `docs/CODEX_PROVIDER.md`。

### OAuth 凭据存储

- `CodexOAuthCredentials` 结构体已从 `CodexUsageResponse.h` 中提取并增强，新增 `save()` 和 `resolveAuthFilePath()` 方法。
- `save()` 实现原子写入：读取现有 JSON、更新 `tokens` 字段、保留其他字段（如 `last_refresh`），写回磁盘。
- `resolveAuthFilePath()` 集中解析 `auth.json` 路径，支持 `CODEX_HOME` 环境变量。
- `CodexOAuthStrategy::attemptTokenRefresh()` 已改用 `refreshed.save(env)` 替代内联文件写入逻辑。
- 凭据加载支持 `OPENAI_API_KEY` 回退、snake_case/camelCase 双格式 key、8天刷新阈值。

### Dashboard Authority 上下文

- 新增 `CodexDashboardAuthorityContext` 类，移植原版 `CodexCLIDashboardAuthorityContext` 的核心功能。
- `makeLiveWebInput()` 从 `ProviderFetchContext` 获取当前 identity/email，构建 live web dashboard 权威验证输入。
- `makeCachedDashboardInput()` 根据 sourceLabel 判断是否信任 usage email（"codex-cli"/"oauth" 信任）。
- `attachmentEmail()` 从权威验证输入中提取最优 email（优先级：expectedScopedEmail > trustedCurrentUsageEmail > dashboardSignedInEmail）。
- `shouldTrustUsageEmail()` 判断 sourceLabel 是否应信任 usage email。
- `CodexDashboardAuthority::evaluate()` 已真正被 Web Dashboard 策略调用，根据 disposition 决定 attach/displayOnly/failClosed。
- `CodexDashboardDecisionReasonDetail` 结构体已携带 expected/actual/ambiguous email 数据，支持更详细的错误报告。

### Web Dashboard 浏览器 Cookie 自动导入

- `CodexWebDashboardStrategy::isAvailable()` 改为始终返回 `true`，不再要求手动 cookie header。
- `fetchSync()` 优先级：手动 cookie header → 浏览器自动导入。
- `importBrowserCookie()` 使用 `BrowserDetection::installedBrowsers()` + `CookieImporter::importCookies()` 自动提取 `chatgpt.com`/`chat.openai.com` cookie。
- 浏览器优先级：Edge → Chrome → Brave → Opera → Vivaldi → Firefox（与 `CookieImporter::importOrder()` 一致）。
- 自动导入的 cookie 不持久化到磁盘，每次 fetch 时重新导入（安全考虑）。
- `parseSignedInEmail()` 从 dashboard HTML 中提取 signed-in email，用于 authority 验证。

### Fetch Context 与网络

- `ProviderFetchContext` 已包含：
  - `providerId`
  - `ProviderSettingsSnapshot settings`
  - `allowInteractiveAuth`
  - `networkTimeoutMs`
  - `manualCookieHeader`
  - `cookieSource`
  - `sourceMode`
  - `environment`
- `UsageStore::buildFetchContextForProvider()` 集中构建 context。
- 支持 canonical settings key：`sourceMode`、`cookieSource`、`manualCookieHeader`、`networkTimeoutMs`。
- 兼容旧 key：`codexDataSource`、`claudeDataSource`、`cursorCookieSource`、`apiRegion`。
- z.ai `apiRegion` 继续映射到 `ZAI_API_REGION`，兼容现有 provider 逻辑。
- `NetworkManager` 提供同步 JSON/string/form/body 请求方法；旧 async 方法保留为 wrapper。

### Cookie / Browser / ConPTY

- `BrowserDetection` 已实现 Windows profile detection：
  - Edge
  - Chrome
  - Brave
  - Opera
  - Vivaldi
  - Firefox
- 支持 env override：
  - `CODEXBAR_CHROME_USER_DATA_DIR`
  - `CODEXBAR_EDGE_USER_DATA_DIR`
  - `CODEXBAR_BRAVE_USER_DATA_DIR`
  - `CODEXBAR_OPERA_USER_DATA_DIR`
  - `CODEXBAR_VIVALDI_USER_DATA_DIR`
  - `CODEXBAR_FIREFOX_PROFILES_DIR`
- `CookieImporter` 已实现：
  - Chromium `Network/Cookies` 和 legacy `Cookies`
  - Firefox `cookies.sqlite`
  - host/domain matching
  - plaintext cookie
  - legacy DPAPI cookie
  - Chromium `v10/v11` AES-GCM cookie
  - Chrome App-Bound/`v20` 安全跳过
- `ConPTYSession` 已实现真实 ConPTY 路径：
  - 动态探测 `CreatePseudoConsole` / `ClosePseudoConsole`
  - pseudo console 创建（支持自定义 cols/rows 参数，默认 120x30）
  - process attribute 注入
  - stdin 写入
  - stdout reader thread
  - buffered `readOutput()`
  - `waitForPattern()`
  - `terminate()`
  - ConPTY 启动失败时 fallback 到 `QProcess`
- ConPTY 子进程启动时使用 `STARTF_USESTDHANDLES` 并置空 std handles，避免 console 子进程复制父进程真实 std handles 而绕过 PTY。

### UI 展示

- 托盘弹窗显示成本区块、provider 卡片、进度条、用量百分比、reset 文案、错误文案。
- `UsageCard.qml` 和 `ProgressMeter.qml` 提供基础卡片/进度展示。
- `PlanUtilizationChart.qml` 显示 Codex/Claude 这类 provider 的历史利用率。
- `TrayPanel.qml` 已能显示 provider cost、z.ai 细节、OpenRouter 细节和本地成本扫描结果。
- 设置窗口包含 General、Providers、Display、Advanced、About、Debug 等页面。
- Settings 窗口已改为 frameless 深色自绘顶栏、紧凑左侧导航和左上对齐内容区；General/Display/Advanced/About/Debug 已统一为页面标题、说明和紧凑 section rows。
- Providers 设置页已重做为紧凑 provider 列表 + 详情工作台结构；provider 列表 switch 使用深色小尺寸 `SettingsSwitch`，进度条不会再挤开右侧开关。
- Providers 设置页已改为 descriptor-driven：支持动态 picker options、secret/password field、WinCred 状态、Clear、Dashboard、Status、Test Connection 和 Copilot Login。
- Provider 详情页包含 hero header、状态 pill、enabled switch、Dashboard/Status/Refresh 快捷操作；Test Connection 已有 testing/succeeded/failed inline feedback、details 展开、Copy 和 Retry。
- Copilot Login UI 会显示 GitHub device user code、verification URL，并提供 Open、Copy Code、Cancel。
- Settings 空白问题已修复：pane 层已补齐 `../components` import，Controls 子组件已补齐 `QtQuick.Controls` import，`AppTheme` 已通过 `qmldir` singleton 和 C++ root context 双路径暴露；`--show-settings --qml-log=...` 可用于启动时直接打开设置窗口并记录 QML runtime 状态。
- Windows 原生背景视觉层试验已回退：当前源码不再包含相关 native helper、窗口调用或 QML glass 变量。

### 设置与存储

- General/Display/Advanced 若干设置通过 `QSettings` 持久化。
- provider enabled 状态写入 Windows 注册表。
- provider settings 与 provider order 写入 `%AppData%/CodexBar/config.json`。
- `WindowsCredentialStore` 对 Windows Credential Manager 提供 read/write/remove/exists 简单封装。
- `ProviderCredentialStore` facade 封装生产 WinCred backend 和测试 in-memory backend。
- 若干 provider 可读取环境变量或 Windows Credential Manager 中的 API key/token。
- z.ai、OpenRouter、Kimi K2、Kilo、Alibaba API key，以及 Claude/Cursor/Mistral/Ollama/Alibaba manual cookie header，可通过 settings UI 写入/清除 WinCred；敏感值不写入 `config.json`。
- 非敏感 provider settings 现在由 `SettingsStore::setProviderSetting()` 即时持久化。
- `SettingsStore` 已补齐 `checkForUpdates` 和 `resetToDefaults()`；`UsageStore` 已补齐 QML-facing `setProviderSetting()`、`refreshAll()`、`clearCache()`，避免 Settings/Debug 页面调用缺失接口。

### 本地成本与历史

- `CostUsageScanner` 已能扫描 Codex、Claude、Pi 等本地 session/transcript 数据。
- 成本扫描包含基础价格映射、fork/duplicate 处理、daily 汇总与 30 天成本。
- `UsageHistoryStore` 能记录 provider sample，并供 `PlanUtilizationChart` 展示。
- `UsageStore::setCostUsageEnabled(true)` 在启动时开启成本扫描。

### 测试

当前 WinCodexBar 默认 CTest/QtTest 覆盖 15 个目标：

- `tst_RateWindow`
- `tst_UsagePace`
- `tst_Localization`
- `tst_TextParser`
- `tst_SingleInstanceGuard`
- `tst_ZaiProvider`
- `tst_CostUsageScanner`
- `tst_ProviderPipeline`
- `tst_CodexProvider`
- `tst_CodexLoginRunner`
- `tst_FetchContext`
- `tst_CookieImporter`
- `tst_ConPTYSession`
- `tst_PerProviderCostUsage`
- `opencode_debug_test`

真实账号 E2E 测试是 opt-in（`BUILD_E2E_TESTS=ON`）：

- `tst_E2E_Codex`
- `tst_E2E_Copilot`
- `tst_E2E_Kimi`
- `tst_E2E_OpenCode`
- `tst_E2E_OpenCodeGo`

新增 P0 测试覆盖了 pipeline fallback、fetch context 注入、cookie/profile fixture、真实 ConPTY console handles 和交互写入/读取。

新增 P1 测试覆盖了 descriptor string id 注册、source mode strategy filtering、ProviderCredentialStore in-memory backend、WinCred/manual cookie fetch context 注入、env-var secret 优先级和 provider secret status。

新增 QML runtime smoke 验证覆盖 SettingsWindow 启动加载、主题对象解析、pane/component import 链路和 OpenRouter 空 usage guard；目前仍是手动/脚本验证，尚未纳入自动 QtTest。

## Provider 状态表

| Provider | 当前可用程度 | 当前依赖 / 数据源 | 主要剩余缺口 | 与原版差异 |
| --- | --- | --- | --- | --- |
| Codex | 部分可用；OAuth 路径已有历史 E2E 记录，CLI RPC 优先，Settings managed account 主链路已实现 | `~/.codex/auth.json` / `CODEX_HOME` OAuth token；ChatGPT backend usage API；source mode 支持 `auto/oauth/cli/web/api`；CLI 优先使用 `codex -s read-only -a untrusted app-server --listen stdio://` JSON-RPC，失败后再用 ConPTY `/status` fallback（支持重试机制，首次 200x60，失败后 220x70）；CLI PTY 使用持久化会话避免重启开销；Web Dashboard 支持浏览器 Cookie 自动导入 + Dashboard Authority 验证 + HTML 缓存；Account Management 可创建独立 managed `CODEX_HOME`，active account 注入 OAuth/RPC/PTY fetch context；账户存储支持版本迁移和安全文件权限；Workspace 标签通过 API 解析并缓存；IdentityMatcher 支持邮箱交叉检查 | 仍需真实 Add Account/OAuth 首次登录 E2E、code review remaining、credits history、Web dashboard extras 并行合并；多账号 reconciliation 需要更多真实账号验证和持久化 polish | 原版有 OAuth、CLI RPC/PTY、Web dashboard、workspace identity cache、managed account observer |
| Claude | 部分可用 | Claude OAuth credentials 或 Claude Web cookie；支持 browser/manual cookie；source mode 支持 `auto/oauth/web` | 缺 CLI、Keychain prompt/delegated refresh policy、web extras 完整 parity；真实账号 Web/OAuth 组合仍需继续验证 | 原版支持 `.auto/.web/.cli/.oauth` source planner，Keychain/CLI/Web 多路径更完整 |
| Cursor | 部分可用但仍需验证 | Cursor Web cookie；usage-summary/auth/subscription API；支持 browser/manual cookie；settings UI 已支持 cookie source 与 manual cookie | 缺 Safari/WebKit fallback；真实账号端到端仍需验证 | 原版有 browser cookie order、CursorRequestUsage、CursorStatusProbe 和更完整 web session 处理 |
| Copilot | 首次登录流程已具备 UI；OAuth 路径已有历史 E2E 记录 | WinCred `com.codexbar.oauth.copilot`；GitHub device flow foreground UI；Copilot internal API | 缺 token refresh/错误模型 polish、device flow 失败/取消场景测试 | 原版有更完整的 `CopilotDeviceFlow` 状态模型和集成体验 |
| z.ai | API key 配好后可用 | `Z_AI_API_KEY` 或 WinCred `com.codexbar.apikey.zai`；settings UI 可写入/清除 API key；region settings 映射；Test Connection | 缺真实账号端到端验证、错误模型 polish | 原版有 `ZaiSettingsReader`、API region、descriptor metadata 和统一 fetch context |
| OpenRouter | API key 配好后可用 | `OPENROUTER_API_KEY` 或 WinCred `com.codexbar.apikey.openrouter`；settings UI 可写入/清除 API key；credits/key API；Test Connection | 缺真实账号端到端验证、credential UX polish | 原版 descriptor、settings reader、credits metadata 更完整 |
| Kimi K2 | API key 配好后部分可用 | `KIMI_K2_API_KEY` / `KIMI_API_KEY` / `KIMI_KEY` 或 WinCred `com.codexbar.apikey.kimik2`；settings UI 可写入/清除 API key；Test Connection | 只覆盖 Kimi K2 credit endpoint；普通 Kimi 已由独立 `kimi` provider 覆盖；真实账号端到端仍需验证 | 原版 Kimi K2 和 Kimi 是两个 provider |
| Kilo | API key 配好后部分可用 | `KILO_API_KEY` 或 WinCred `com.codexbar.apikey.kilo`；settings UI 可写入/清除 API key；Kilo trpc endpoint；Test Connection | 缺 CLI fallback；真实账号端到端仍需验证 | 原版有 `KiloSettingsReader`、data source 和 usage fetcher |
| Kiro | 部分可用 | `QProcess` 运行 `kiro-cli chat --no-interactive /usage`，文本解析 | 尚未接入新 ConPTY；CLI discovery、交互 prompt、超时/错误处理不完整 | 原版有 `KiroStatusProbe`、descriptor 和更统一 CLI 配置 |
| Kimi | Web Cookie 路径已新增，端到端已验证 ✅ | `kimi-auth` JWT token；POST `GetUsages`；JWT payload 提取 session/device/traffic ID；支持 manual cookie / browser import | 错误模型 polish | 原版有 `KimiCookieImporter`、`KimiUsageFetcher`、descriptor |
| OpenCode | Web Cookie 路径已新增；E2E harness 覆盖无 subscription QSKIP | `auth`/`__Host-auth` cookie；两步 API（workspace ID → subscription）；JSON + 文本双路径解析；支持 manual cookie / browser import；无 subscription 时正确 QSKIP | 仍需有 subscription workspace 的真实账号 PASS 记录、错误模型 polish | 原版有 `OpenCodeCookieImporter`、`OpenCodeUsageFetcher`、descriptor |
| OpenCode Go | Web Cookie 路径已新增；有 opt-in E2E 目标 | `auth`/`__Host-auth` cookie；workspace ID → Go 页面抓取；JSON + regex 双路径解析；支持 manual cookie / browser import / workspace ID override；5-hour/weekly/monthly 三窗口 | 仍需真实账号/subscription 端到端记录、错误模型 polish | 原版有 `OpenCodeGoUsageFetcher`、`OpenCodeGoUsageSnapshot`、descriptor |
| Mistral | 有基础 Web fetch 形状，端到端未验证 | Web cookie；admin billing API；支持 browser/manual cookie；settings UI 可写入/清除 manual cookie | 缺真实账号验证、错误模型、cookie source 体验 | 原版有 `MistralCookieImporter`、usage fetcher、models/errors |
| Ollama | 有基础 Web fetch 形状，端到端未验证 | Web cookie；ollama.com settings/usage HTML/API；支持 browser/manual cookie；settings UI 可写入/清除 manual cookie | 缺真实账号验证，HTML/API 解析路径仍脆弱 | 原版有 Ollama usage fetcher/parser/snapshot 和 descriptor |
| Alibaba | API/Web Cookie best-effort 路径已注册 | `ALIBABA_API_KEY` 或 WinCred `com.codexbar.apikey.alibaba`；manual cookie / browser cookie import；`apiRegion` 支持 International/China；source mode 支持 `auto/api/web` | 真实账号端到端未验证；DashScope quota/user endpoint 和控制台登录探测仍是通用 best-effort；错误模型、本地证书/网络错误处理仍需 polish | 原版有 `AlibabaCodingPlanCookieImporter`、更细的 Coding Plan usage fetcher 和 provider-specific settings |

## 完全缺失的 12 个 provider

| Provider | 原版主要数据源 / 实现 | Windows 需要补的适配层 |
| --- | --- | --- |
| Factory/Droid | local storage importer、Factory status probe | Windows local storage/browser profile detection、status probe、cookie/local storage 解密 |
| Gemini | Gemini descriptor、status probe、CLI/OAuth 相关路径 | Google/Gemini OAuth or CLI credentials、status polling、source planner |
| Antigravity | 本地运行实例 status probe，端口/CSRF/本地 API 探测 | Windows 进程/端口发现、CSRF 提取、本地 API 探测、错误映射 |
| MiniMax | API region、API settings、cookie/local storage/manual auth modes | API key UI、region settings、Cookie/LocalStorage importer、manual cookie/token 模式 |
| Vertex AI | Vertex AI OAuth credentials、token refresher、usage fetcher | Google OAuth credential store、token refresh、project/billing settings |
| Augment | `auggie` CLI probe、web status probe、session keepalive | `auggie` CLI discovery、ConPTY strategy、cookie import、session keepalive、manual refresh action |
| JetBrains AI | JetBrains IDE detector、local XML/status probe | Windows JetBrains config path detection、IDE state probe、XML/local settings parsing |
| Amp | Web cookie + `AmpUsageFetcher` | provider class、cookie/manual setting、descriptor、usage parser |
| Synthetic | API stats/settings reader | API key/config UI、usage stats fetcher、snapshot mapping |
| Warp | settings reader、usage fetcher | Warp account/token/settings discovery、API fetcher、provider settings |
| Perplexity | cookie header/importer、settings reader、usage fetcher | provider class、browser cookie/manual cookie、API errors/model mapping |
| Abacus AI | cookie importer、多 session usage fetcher、admin dashboard | provider class、多 session selection、dashboard cookies、usage model mapping |

## 未完成 / 部分实现功能优先级矩阵

P 级表示“修复/实现优先级”，不是 bug 严重性或发布阻断标签。P0/P1 基础闭环当前已完成，剩余主要从真实账号验证、P2 provider parity 和 P3 生态工作继续推进。

- `P0`：阻塞核心 provider 可用性、会造成大量 provider 不可用，或让刷新架构不可靠。
- `P1`：用户可见的核心功能缺口，影响主流程完成，但不一定阻塞所有 provider。
- `P2`：原版 parity、体验完整度、维护性或测试可靠性问题。
- `P3`：低优先级 backlog、发布生态、非首批 Windows parity 项。

### P0 基础设施：已完成，需后续接入与真实账号验证

| 功能 | 当前状态 | 后续注意事项 |
| --- | --- | --- |
| `CookieImporter` / `BrowserDetection` | 已实现 Windows browser detection、Chromium/Firefox cookie DB 读取、DPAPI、`v10/v11` AES-GCM、manual cookie context | Chrome App-Bound/`v20` 仍按设计跳过；需要真实浏览器 profile 和真实账号 provider 端到端验证 |
| `ConPTYSession` | 已实现真实 ConPTY 优先、`QProcess` fallback、stdin/stdout、pattern wait、terminate；测试断言子进程看到非 redirected console handles | Codex 已接入 ConPTY strategy（自动处理交互 prompt）；Claude/Kiro/Gemini/Augment 等 provider 仍需逐步接入 |
| `ProviderPipeline` 并发模型 | pipeline 主路径已同步化，strategy 直接 `fetchSync()`；NetworkManager 提供 sync 方法 | 仍需补更完整 timeout/cancellation 语义和压力测试 |
| `ProviderFetchContext` 通用注入 | 已集中注入 provider id、settings snapshot、source/manual cookie、manual cookie header、timeout、系统 env、z.ai region 映射；secret resolver / WinCred facade / Copilot foreground auth intent 已接入 | 仍需真实账号端到端验证、prompt policy 和 provider-specific 错误模型 polish |

### P1 当前 provider 主流程：基础闭环已完成，剩余以真实账号验证为主

| 功能 / Provider | 当前状态 | P1 理由 |
| --- | --- | --- |
| ProviderDescriptor registry | 已改为 string-id descriptor；`registerProvider()` 自动注册 metadata、dashboard/status URL、source modes，并由 QML/策略消费 | 后续可继续扩展 richer descriptor metadata，但 P1 阻塞已解除 |
| API key / token UI 与 WinCred 写入 | 已通过 descriptor-driven settings UI 支持 secret field、WinCred 写入/清除、env-var 状态、Test Connection；敏感值不写入 `config.json` | 仍需真实账号端到端验证和错误提示 polish |
| Copilot foreground login | 已新增 foreground GitHub device-code UI，显示 user code / verification URL，支持 open/copy/cancel，成功后写入 WinCred；后台 refresh 保持 non-interactive | 仍需真实账号端到端验证、错误状态 polish |
| Codex | OAuth/API、CLI RPC、CLI PTY fallback、Web Dashboard 四路径已实现；Settings Account Management 已支持 foreground device-auth、managed `CODEX_HOME`、active account 注入 fetch context | 真实 Add Account/OAuth 首次登录、更多多账号 reconciliation、Web dashboard extras 并行合并等仍需端到端验证 |
| Claude | OAuth/Web 路径和 `auto/oauth/web` source planner 已有基础 | 缺 CLI/Credential prompt/delegated refresh parity；真实账号端到端仍需验证 |
| Cursor | Web API、cookie source/manual cookie UX 和 statuspage polling 已有基础 | 缺 Safari/WebKit fallback；真实账号端到端仍需验证 |
| Mistral / Ollama | Web provider 已注册，manual cookie secret UI 已可用 | 缺真实账号验证、错误模型和解析健壮性 |
| z.ai / OpenRouter / Kimi K2 / Kilo / Alibaba | API fetch、secret UI、WinCred 管理、Test Connection 和 `api` source mode 已有基础 | 仍需真实账号端到端验证和 provider-specific error polish；Alibaba 的 API/Web quota 解析仍是 best-effort |
| Status polling / incident indicator | 已新增 statuspage-style `/api/v2/status.json` polling、provider status model 和 QML 状态展示 | Google Workspace-specific status 留给 Gemini/Vertex；incident badge polish 仍可继续 |
| Merge Icons | 已按所有 enabled providers 中最低 primary/secondary remaining 渲染 merged icon，tooltip 显示最紧张 provider 和异常状态 | 图标视觉 polish、tray 真实交互测试仍需补充 |

### P2 parity、体验和维护性

| 功能 / Provider | 当前状态 | P2 理由 |
| --- | --- | --- |
| Kiro ConPTY 接入 | 当前仍使用 `QProcess` 非交互 `/usage` 路径 | 新 ConPTY 平台层已可用，但 Kiro 还未接入交互/PTY strategy |
| Provider settings 保存语义 | 非敏感 provider settings 已即时持久化到 `%AppData%/CodexBar/config.json`；secret 走 WinCred | 仍需补 settings binding / persistence smoke 测试 |
| 启动立即刷新 provider | 启动时开启成本扫描和自动刷新 timer，但不一定立即刷新 provider | refresh frequency 较长时托盘 provider 数据可能长时间为空 |
| `CostUsageScanner` parity | 已能扫描多类本地 session/transcript | 与原版 Swift scanner 的增量缓存、健壮性和完整 pricing/source 覆盖仍有差距 |
| QML Display 设置消费 | 多个 display toggle 已存储 | `usageBarsShowUsed`、`resetTimesShowAbsolute`、`showOptionalCreditsAndExtraUsage` 等未全部影响 UI |
| provider ordering / drag persistence | order 存储雏形存在 | settings/UI 体验与原版 provider 顺序管理仍不完整 |
| Windows 原生背景视觉试验 | 已从源码中移除，Settings/tray 回到普通深色实色窗口 | 后续如果重新尝试，必须提供应用内开关并避免要求用户修改系统全局视觉设置 |
| 测试覆盖 | 默认 CTest 当前覆盖 15 个目标；E2E 目标 5 个，需 `BUILD_E2E_TESTS=ON`；SettingsWindow 有 `--show-settings --qml-log=...` runtime smoke | 仍缺自动化 QML 渲染测试、真实网络、真实 OAuth 首次登录、WinCred 交互、tray interaction、真实浏览器 profile、更多 provider 端到端 |

### P2/P3 缺失 provider

| 优先级 | Provider | 归属理由 |
| --- | --- | --- |
| P2 | Gemini | 原版核心 provider，依赖 OAuth/CLI/status 基础设施 |
| P2 | Antigravity | 需要本地进程/端口/CSRF/status probe，和本地探测/ConPTY 相关 |
| P2 | Vertex AI | 与 Google OAuth/token refresh/provider settings 相关，可复用 Gemini 能力 |
| P2 | Augment | 原版有 CLI + Web 双路径和 session keepalive，依赖 ConPTY/CookieImporter |
| P2 | JetBrains AI | 需要 Windows IDE/config path 探测，属于本地 parity 能力 |
| P2 | MiniMax | API/cookie/local storage/manual auth 多模式，依赖 settings、CookieImporter、secret UI |
| P2 | Perplexity | Web cookie/manual header provider，依赖 CookieImporter 与 settings UX |
| P2 | ~~OpenCode~~ ✅ 已完成 | Web cookie support、usage fetcher、CookieImporter/manual cookie |
| P2 | ~~OpenCode Go~~ ✅ 已完成 | Web cookie support、Go page fetcher、CookieImporter/manual cookie/workspace ID override |
| P2 | Factory/Droid | 需要 local storage importer 和 status probe，依赖 browser/local storage 适配 |
| P2 | Amp | Web cookie provider，依赖 CookieImporter/manual cookie |
| P3 | Synthetic | API stats/settings provider，用户主流程优先级低于核心 provider |
| P3 | Warp | 依赖 Warp account/settings discovery，可作为后续 backlog |
| P3 | Abacus AI | 多 session cookie/admin dashboard provider，依赖 CookieImporter 和 session UX 成熟后推进 |

### P3 应用生态与非首批 parity

| 功能 | 当前状态 | P3 理由 |
| --- | --- | --- |
| CLI executable | Windows 端尚无 `codexbar` 等价 CLI | 不阻塞托盘 app 主流程，可在 provider runtime 稳定后推进 |
| named pipe IPC / shared snapshot file | 尚未实现 | 属于跨进程/集成生态能力 |
| updater / installer / signing / release automation | 尚未形成完整 Windows 发布链路 | 发布体验重要，但优先级低于 provider 可用性 |
| Windows polish / dashboard links / 更多通知细节 | 部分 UI/菜单能力存在 | 属于体验补齐，不应早于核心 provider 和平台适配层 |

### N/A 非目标项

下列项目由 `docs/WINDOWS_MIGRATION.md` 标记为 Windows 非目标或替代实现方向，不分配 P 级，除非产品需求变化：

- WidgetKit
- Confetti/Vortex celebration animation
- DisplayLink animation
- NSMenu custom view embedding
- SF Symbols

## 与原版 macOS CodexBar 的关键差异

### ProviderDescriptor registry

原版 provider 通过 `@ProviderDescriptorRegistration` / `@ProviderDescriptorDefinition` 宏注册 descriptor。descriptor 包含 metadata、icon、source modes、fetch plan、CLI config、dashboard URL、status page URL、browser cookie order 等。UI 和 fetch pipeline 都围绕 descriptor 读取元数据。

WinCodexBar 已将 `ProviderDescriptor` 调整为 string-id 驱动，`ProviderRegistry::registerProvider()` 会从 provider 实例自动生成 descriptor，并向 QML 暴露 display metadata、dashboard URL、status URL 和 source modes。与原版相比，Windows descriptor 仍比 Swift 宏体系轻量，browser cookie order、CLI config、status product metadata 等 richer metadata 还未完全补齐。

### Source planner

原版每个 provider 的 source mode 和 strategy ordering 由 descriptor/fetch plan/source planner 管理。例如 Claude 有 `.auto/.web/.cli/.oauth`，Codex 有 OAuth、CLI、Web dashboard、managed account 等组合逻辑。

WinCodexBar 当前已有 `ProviderSourceMode`、fetch context 注入和 pipeline source filtering。`auto` 保持 provider 自身 strategy 顺序，`oauth/web/cli/api` 会过滤到对应 strategy kind；当前实现模式覆盖 Codex `auto/oauth/cli/web`、Claude `auto/oauth/web`、Cursor `auto/web`、Kiro `cli`、z.ai/OpenRouter/Kimi K2/Kilo `api`、Mistral/Ollama `web`、Alibaba `auto/api/web`。与原版相比，策略排序、fallback 条件和 foreground/background prompt policy 仍更分散。

### Credentials 与 prompt policy

原版使用 macOS Keychain、安全 CLI reader、prompt cooldown、delegated refresh policy、foreground/background prompt 区分等机制。Claude OAuth 相关策略尤其完整。

WinCodexBar 目前有 `WindowsCredentialStore` 简单 CRUD、`ProviderCredentialStore` facade 和 settings secret UI。API key/manual cookie 可从 UI 写入/清除 WinCred，环境变量优先于 WinCred，敏感值不写入 `config.json`。Copilot 已有 foreground GitHub device-code login，后台 refresh 不会静默发起 device flow。与原版相比，prompt cooldown、delegated refresh policy 和 Claude OAuth prompt policy 仍未完整实现。

### UI 与设置

原版设置窗口围绕 provider descriptor 和 settings snapshot 展开，有 provider-specific 控件、source mode、manual cookie、API key、prompt policy、status checks、dashboard links。

WinCodexBar 设置窗口已有页面结构，并且 Providers 页已改为 descriptor-driven。Picker 使用 provider-provided options，不再硬编码 z.ai region；secret field 使用密码输入框并显示 env/WinCred 状态；provider 详情提供 Dashboard、Status、Test Connection 和 Copilot Login。Display 中的 `statusChecksEnabled` 已驱动 status polling，`mergeIcons` 已驱动 merged tray icon；`usageBarsShowUsed`、`resetTimesShowAbsolute`、`showOptionalCreditsAndExtraUsage` 等显示设置仍未全部消费。

### Tray icon 与状态

原版有更完整的 menu bar icon 状态、combined icon、provider ordering、status polling、incident badge、dashboard links 和 provider silo 约束。

WinCodexBar 的 `mergeIcons` 设置已接入 `StatusItemController::applyMergedIcon()`，当前按所有 enabled providers 中最低 primary/secondary remaining 渲染合并图标，并在 tooltip 中显示最紧张 provider 与 outage/degraded 状态。status checks 已支持 statuspage-style `/api/v2/status.json` polling；Google Workspace-specific status 和更完整 incident badge 仍未实现。原版 merged menu 中的 provider switcher、Overview tab、Codex account switcher、token account switcher、usage/credits/cost hosted submenus 尚未在 QML tray panel 中实现对等体验。

### CLI、Widget、Updater、Installer、IPC

原版 SwiftPM 包含 app、`CodexBarCLI`、`CodexBarWidget`、Swift macros、辅助 Claude 工具、Sparkle update/appcast/release scripts。

WinCodexBar 当前主要是桌面 app。缺少 Windows CLI executable、named pipe IPC、shared snapshot file、updater、installer、signing/release automation。WidgetKit 是 macOS 专属能力，不应算作 Windows 缺陷。

## 推荐推进顺序

1. P1 收尾验证：
   - 回归验证 Copilot foreground login 的首次登录、取消、失败和 token refresh 场景。
   - 真实账号验证 z.ai / OpenRouter / Kimi K2 / Kilo / Alibaba API key UX 与 Test Connection。
   - 真实账号验证 Claude/Cursor/Mistral/Ollama manual cookie 与 browser cookie 路径。
   - 为 OpenCode/OpenCode Go 补有 subscription workspace 的真实账号 PASS 记录。
   - 将现有 SettingsWindow runtime smoke 固化为自动化测试，并继续补 WinCred interaction、tray interaction 测试。
2. P1/P2 provider parity：
   - Codex Web dashboard extras/managed account。
   - Claude CLI/source planner/delegated refresh。
   - Kiro ConPTY 接入。
   - Gemini/Vertex。
3. P1/P2 托盘状态 polish：
   - incident badge。
   - provider dashboard/status links 的真实交互验证。
   - notifications polish。
4. P2 视觉与 Settings polish：
   - 将 SettingsWindow runtime smoke 固化为自动化 QML 测试。
   - 继续打磨普通深色实色窗口下的 Settings/tray 视觉一致性。
   - 后续任何系统级视觉试验都必须可关闭，且不得依赖用户修改系统全局视觉设置。
5. P2/P3 provider backlog：
   - MiniMax、Perplexity、Amp。
   - Augment、Antigravity、JetBrains、Factory/Droid。
   - Synthetic、Warp、Abacus AI。
   - ~~Kimi~~ ✅ 已完成（Web Cookie 路径，2026-04-30）。
   - ~~OpenCode~~ ✅ 已完成（Web Cookie 路径，2026-04-30）。
   - ~~OpenCode Go~~ ✅ 已完成（Web Cookie 路径，2026-04-30）。
   - ~~Alibaba~~ ✅ 已注册（API/Web Cookie best-effort 路径，2026-05-01 复审确认）。
6. P3 应用生态：
   - CLI executable。
   - named pipe IPC。
   - shared snapshots。
   - updater/installer/release pipeline。
7. 扩大测试：
   - fake network/provider parser fixtures。
   - real browser profile fixtures。
   - OAuth/device flow UI tests。
   - provider end-to-end opt-in tests。

## 验证记录

2026-05-01 功能差异复审后记录：

```powershell
ctest --test-dir cmake-build-codex-release -C Release --output-on-failure
```

执行目录：`D:\CodexBar\WinCodexBar`

结果：12/12 通过。该历史命令记录的是当时的默认测试集；当前默认 CTest 已扩展到 15 个目标。真实账号 E2E 仍需 `BUILD_E2E_TESTS=ON` 单独配置。

2026-04-30 P0/P1/P2 实现后记录的验证命令：

```powershell
ctest --test-dir build -C Release --output-on-failure
```

执行目录：`D:\CodexBar\WinCodexBar`

结果：15/15 通过（10 单元测试 + 5 E2E 测试）。新增 Kimi provider、OpenCode provider、OpenCode Go provider 和 E2E 基础设施。

```powershell
# 单独运行各 E2E 测试（在已登录账号的环境中）
$env:CODEXBAR_E2E_OPENCODE_COOKIE = "auth=..."
& 'D:\CodexBar\WinCodexBar\build\tests\e2e\Release\tst_E2E_Codex.exe' -v2
& 'D:\CodexBar\WinCodexBar\build\tests\e2e\Release\tst_E2E_Copilot.exe' -v2
& 'D:\CodexBar\WinCodexBar\build\tests\e2e\Release\tst_E2E_Kimi.exe' -v2
& 'D:\CodexBar\WinCodexBar\build\tests\e2e\Release\tst_E2E_OpenCode.exe' -v2
```

执行目录：`D:\CodexBar\WinCodexBar`

结果：
- tst_E2E_Codex: PASS（OAuth 真实账号验证通过）
- tst_E2E_Copilot: PASS（OAuth 真实账号验证通过）
- tst_E2E_Kimi: PASS（Web Cookie 真实账号验证通过）
- tst_E2E_OpenCode: SKIP（workspace 无 subscription 数据，预期行为）

OpenCode 在无 subscription 时正确 QSKIP，不报错；代码框架已验证。当 workspace 有 subscription 时会自动解析并 PASS。

```powershell
cmake --build build --config Release --target WinCodexBar -- /m /v:minimal /clp:ErrorsOnly
```

执行目录：`D:\CodexBar\WinCodexBar`

结果：通过。验证时曾发现 `WinCodexBar.exe` 正在运行并锁住输出文件，停止进程后构建通过。

2026-04-30 P1 基础闭环实现后记录的验证命令：

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\build.ps1 -BuildType Release
```

执行目录：`D:\CodexBar\WinCodexBar`

结果：通过。`windeployqt` 仍输出 `does not seem to be a Qt executable` warning，但 Release executable、测试二进制和翻译文件均生成成功。

```powershell
ctest --test-dir build -C Release --output-on-failure
```

执行目录：`D:\CodexBar\WinCodexBar`

结果：10/10 通过。

```powershell
$p = Start-Process -FilePath 'D:\CodexBar\WinCodexBar\build\Release\WinCodexBar.exe' -RedirectStandardOutput 'D:\CodexBar\WinCodexBar\build\codex_run_stdout.txt' -RedirectStandardError 'D:\CodexBar\WinCodexBar\build\codex_run_stderr.txt' -PassThru -WindowStyle Hidden
Start-Sleep -Seconds 3
if (!$p.HasExited) { Stop-Process -Id $p.Id }
Get-Content -Raw 'D:\CodexBar\WinCodexBar\build\codex_run_stderr.txt'
Get-Content -Raw 'D:\CodexBar\WinCodexBar\build\codex_run_stdout.txt'
```

执行目录：`D:\CodexBar`

结果：短启动通过，stdout/stderr 无 QML 启动错误输出。

2026-04-30 Settings 空白修复后记录的 QML runtime smoke：

```powershell
$log = 'D:\CodexBar\WinCodexBar\build\qml_settings.log'
$p = Start-Process -FilePath 'D:\CodexBar\WinCodexBar\build\Release\WinCodexBar.exe' -ArgumentList "--show-settings --qml-log=$log" -PassThru
Start-Sleep -Seconds 4
if (!$p.HasExited) { Stop-Process -Id $p.Id }
Get-Content -Raw $log
```

执行目录：`D:\CodexBar\WinCodexBar`

结果：通过。`SettingsWindow` 状态为 `QQuickView::Ready`，root item 尺寸为 `720x560`，General 页可见；QML 日志不再包含 `SettingsWindow.qml`、`GeneralPane.qml`、`SettingsToggleRow is not a type` 或 `AppTheme` 相关加载错误。另通过桌面截图确认 Settings 页不再空白。

2026-04-30 Settings redesign、provider UI 修复、Kimi K2 logo 更新与系统背景视觉试验回退后记录：

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\build.ps1 -BuildType Release
```

执行目录：`D:\CodexBar\WinCodexBar`

结果：通过。`windeployqt` 仍输出 `does not seem to be a Qt executable` warning，但 Release executable 正常生成。

```powershell
ctest --test-dir build -C Release --output-on-failure
```

执行目录：`D:\CodexBar\WinCodexBar`

结果：15/15 通过。

```powershell
$log = 'D:\CodexBar\WinCodexBar\build\qml_settings.log'
$args = '--show-settings --qml-log=' + $log
$p = Start-Process -FilePath 'D:\CodexBar\WinCodexBar\build\Release\WinCodexBar.exe' -ArgumentList $args -PassThru
Start-Sleep -Seconds 8
if (!$p.HasExited) { Stop-Process -Id $p.Id }
Get-Content -Raw $log
```

执行目录：`D:\CodexBar\WinCodexBar`

结果：通过。`SettingsWindow` 状态为 `QQuickView::Ready`，root item 尺寸为 `900x640`，QML 日志不包含 SettingsWindow 加载错误。该 smoke 同时覆盖 frameless titlebar、Settings redesigned layout 和 provider pane 基础加载路径。

2026-05-01 Codex CLI RPC、Account Management、device-auth 解析修复后记录：

```powershell
cmake --build build --config Release --target tst_CodexLoginRunner -- /m /v:minimal /clp:ErrorsOnly
ctest --test-dir build -C Release -R tst_CodexLoginRunner --output-on-failure
cmake --build build --config Release --target WinCodexBar -- /m /v:minimal /clp:ErrorsOnly
ctest --test-dir build -C Release --output-on-failure
```

执行目录：`D:\CodexBar\WinCodexBar`

结果：通过。`tst_CodexLoginRunner` 覆盖 device-auth 标准输出、ANSI 输出、缺失 code、失败退出、id token email 解析，以及 `AUTHORIZATION` 被误当作设备码的回归；完整默认 CTest 为 15/15 通过。验证中曾遇到运行中的 `WinCodexBar.exe` 锁住 Release 输出文件，停止进程后构建通过。

```powershell
$log = 'D:\CodexBar\WinCodexBar\build\qml_settings.log'
$args = '--show-settings --qml-log=' + $log
$p = Start-Process -FilePath 'D:\CodexBar\WinCodexBar\build\Release\WinCodexBar.exe' -ArgumentList $args -PassThru
Start-Sleep -Seconds 8
if (!$p.HasExited) { Stop-Process -Id $p.Id }
Get-Content -Raw $log
```

执行目录：`D:\CodexBar\WinCodexBar`

结果：通过。`SettingsWindow` 状态为 `QQuickView::Ready`，root item 尺寸为 `900x640`，用于确认 Providers/Codex 详情页加载路径仍正常。Codex Account Management 的真实首次 Add Account/OAuth 授权仍需手工账号验证。

源码检查结果：系统原生背景视觉层相关 helper、窗口调用、构建项和 QML glass 变量已从 `WinCodexBar/` 源码中移除。

当前测试覆盖不足之处：

- QML runtime 真实渲染目前只有手动/脚本 smoke，尚未纳入自动 QtTest。
- 默认构建运行 15 个 CTest 目标；真实账号 E2E 需要显式 `BUILD_E2E_TESTS=ON` 和账号环境变量。
- 未覆盖真实网络 API。
- 未覆盖真实 Chrome App-Bound cookie profile。
- 未覆盖真实 OAuth 首次登录。
- 未覆盖真实 Windows Credential Manager 交互式 UX。
- 未覆盖 provider pipeline 饱和/取消压力。
- 未覆盖托盘真实点击/菜单交互。
- 未覆盖真实账号 provider end-to-end。

## 非目标项

`docs/WINDOWS_MIGRATION.md` 明确把部分 macOS 专属能力列为 Windows 非目标或替代实现方向。除非产品需求变化，下列项目不应计入 WinCodexBar 缺陷：

- WidgetKit 桌面小组件。
- Confetti/Vortex celebration animation。
- DisplayLink animation，Windows 可用 timer-driven updates 替代。
- NSMenu custom view embedding，Windows 已由 QML popup panel 替代。
- SF Symbols，Windows 可由 SVG/PNG/Qt icon assets 替代。
