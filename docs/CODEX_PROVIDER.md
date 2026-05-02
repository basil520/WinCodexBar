# Codex Provider

Date: 2026-05-01

本文记录 WinCodexBar 当前 Codex provider 的连接链路、账号管理、已知边界和排查方式。

## 当前状态

Codex provider 已从“启动交互 TUI 后解析 `/status`”重构为“优先使用 Codex CLI app-server JSON-RPC，PTY `/status` 仅兜底”。Settings -> Providers -> Codex 的 Account Management 也已接入 app-managed `CODEX_HOME`，新增账号会使用独立 Codex home，不直接改写系统 `~/.codex`。

仍需补充真实首次登录的端到端验收、更多账号组合的 reconciliation 验证、credits/history 展示，以及 Web dashboard extras 与主用量结果的并行合并。

## 连接顺序

- `auto`: OAuth API -> `codex.cli.rpc` -> `codex.cli.pty` -> Web Dashboard。
- `cli`: `codex.cli.rpc` -> `codex.cli.pty`，不会尝试 OAuth 或 Web。
- `oauth`: 只使用 OAuth API。
- `web`: 只使用 Web Dashboard。

`codex.cli.rpc` 启动：

```powershell
codex -s read-only -a untrusted app-server --listen stdio://
```

RPC client 会按行读写 JSON-RPC，并调用：

- `initialize`
- `initialized`
- `account/read`
- `account/rateLimits/read`

`account/rateLimits/read` 会优先读取 `rateLimitsByLimitId.codex`，不存在时回退到 `rateLimits`。`primary`、`secondary` 映射为 `RateWindow`；非 unlimited credits 同步到 `ProviderFetchResult.credits` 和当前 UI 兼容字段 `UsageSnapshot.providerCost`。`account/read` 的 `chatgpt.email`、`planType` 用于填充身份信息。

RPC 失败时会给 UI 短错误并进入 fallback。原始 stdout/stderr、ANSI/TUI 控制序列和 JSON-RPC error body 只进入 debug details，避免连接失败卡片被长输出撑破。

### PTY 重试机制

`codex.cli.pty` 策略使用 ConPTY 启动 codex CLI，并支持自动重试：

- 使用持久化会话（`CodexPersistentCLISession`），避免每次重启进程。
- 首次尝试：200x60 终端尺寸，15秒超时。
- 支持 update prompt 检测和自动跳过（发送下箭头+回车）。
- 支持 cursor query 响应（`ESC[6n` → `ESC[1;1R`）。
- 会话按 binary path + environment 自动复用。

`ConPTYSession::start()` 支持自定义 `cols`/`rows` 参数，默认值为 120x30。

### Web Dashboard 缓存

Web Dashboard 策略支持 HTML 缓存：

- 缓存路径：`%AppData%/CodexBar/codex-dashboard-cache.json`。
- 缓存内容：`{accountEmail, html, updatedAt}`。
- 使用时机：authority evaluate 通过后保存；下次加载时先检查缓存。
- 失效条件：authority 拒绝、email 不匹配、手动清除。
- 安全权限：缓存文件仅所有者可读写（`QFile::ReadOwner | QFile::WriteOwner`）。

### Runtime 感知策略解析

`auto` source mode 根据运行时环境返回不同策略顺序：

- `isAppRuntime=true`（托盘应用）：OAuth → CLI RPC → CLI PTY → Web Dashboard。
- `isAppRuntime=false`（CLI 工具）：Web Dashboard → CLI RPC → CLI PTY。
- `.api` source mode：只使用 OAuth API。

### 账户存储版本迁移

`ManagedCodexAccountStore` 支持版本化 JSON 格式：

- 当前版本：2。
- JSON 格式：`{"version": 2, "accounts": [...]}`。
- v1 迁移：从 JWT 中提取 provider account ID。
- 向后兼容：如果 JSON 是数组（v1 无版本号），自动迁移。

### 安全文件权限

敏感文件使用仅所有者可读写权限：

- `auth.json`：`QFile::ReadOwner | QFile::WriteOwner`。
- `managed-codex-accounts.json`：`QFile::ReadOwner | QFile::WriteOwner`。
- `codex-dashboard-cache.json`：`QFile::ReadOwner | QFile::WriteOwner`。

### IdentityMatcher 邮箱交叉检查

`CodexIdentityMatcher::matches()` 支持邮箱交叉检查：

- 基础匹配：identity type 和 accountId/email 直接比较。
- 邮箱交叉检查：`providerAccount` 类型先检查 accountId，不匹配时检查 email。
- 用于账户 reconciliation 中匹配 managed account 和 live system account。

### Workspace 标签解析

`CodexSystemAccountObserver::loadSystemAccount()` 集成了 workspace 标签解析：

- 使用 `CodexOpenAIWorkspaceIdentityCache` 缓存 workspace 标签。
- 缓存未命中时调用 `CodexOpenAIWorkspaceResolver::resolve()` 从 API 获取。
- 结果缓存到本地文件，避免重复 API 调用。

### Web Dashboard 浏览器 Cookie 自动导入

`web` source mode 支持从已安装浏览器自动提取 `chatgpt.com` cookie：

- 优先级：手动 cookie header → 浏览器自动导入。
- 浏览器优先级：Edge → Chrome → Brave → Opera → Vivaldi → Firefox。
- 目标域名：`chatgpt.com`、`chat.openai.com`。
- 自动导入的 cookie 不持久化到磁盘，每次 fetch 时重新导入。
- `isAvailable()` 始终返回 `true`（自动导入 + 手动回退）。

### Dashboard Authority 验证

Web Dashboard 策略集成了原版的 Dashboard Authority 系统：

- 从 dashboard HTML 中提取 signed-in email。
- 使用 `CodexDashboardAuthorityContext::makeLiveWebInput()` 构建权威验证输入。
- 调用 `CodexDashboardAuthority::evaluate()` 进行 ownership 验证。
- `Attach` disposition：正常使用 dashboard 数据，附加 account email。
- `DisplayOnly` disposition：拒绝使用（当前实现为 failClosed）。
- `FailClosed` disposition：拒绝使用，返回详细错误信息。

错误详情包括：
- `WrongEmail`：期望邮箱 vs 实际邮箱。
- `MissingDashboardSignedInEmail`：dashboard 未暴露 signed-in email。
- 其他 policy 错误。

## 账号管理

Settings -> Providers -> Codex -> Account Management 支持 app-managed Codex 账号：

- Add Account 使用 `codex login --device-auth`。
- 每个 managed account 使用独立 `CODEX_HOME`，目录在应用数据目录下的 managed Codex homes 中。
- UI 显示 verification URL、user code、Open、Copy、Cancel 和状态。
- 登录成功后验证 scoped `auth.json`，优先从 id token/JWT 解析真实 email/account id。
- Set active 会更新 active account，后续 OAuth/RPC/PTY fetch context 都会注入对应 `CODEX_HOME`。
- 系统 `~/.codex` 或外部 `CODEX_HOME` 仍保留为 live system account。

QML 端通过 `UsageStore.codexAccountState` 订阅账号列表、active account、auth/removal 状态、auth 错误和 verification prompt；不再依赖无 notify 的直接方法调用。

## OAuth 凭据存储

`CodexOAuthCredentials` 结构体提供凭据的加载、解析和保存：

- `load(env)`：从 `auth.json` 加载凭据，支持 `CODEX_HOME` 环境变量。
- `parse(json)`：解析 JSON 凭据，支持 `OPENAI_API_KEY` 回退、snake_case/camelCase 双格式 key。
- `save(env)`：原子写入凭据到 `auth.json`，保留现有 JSON 字段（如 `last_refresh`）。
- `resolveAuthFilePath(env)`：集中解析 `auth.json` 路径。
- `needsRefresh()`：8天刷新阈值判断。

Token refresh 流程：
1. 检查 `needsRefresh()`（8天阈值）。
2. POST `https://auth.openai.com/oauth/token` with `refresh_token` grant。
3. 使用 `refreshed.save(env)` 原子写入新凭据。

## AUTHORIZATION 设备码问题

如果浏览器授权页显示类似“无法授权此设备（设备代码：AUTHORIZATION）”，通常说明旧构建把 CLI 输出中的 “device code authorization” 误当作真正 user code。

当前修复：

- device-auth parser 只接受明确的 `user code`、`code:`、`enter ... code` 形态。
- ANSI 输出会先清理控制序列再解析。
- `tst_CodexLoginRunner::ignoresAuthorizationTextBeforeRealDeviceCode` 覆盖该回归。

处理方式：更新到当前构建后重新点击 Add Account。旧设备码已经无效，应取消后重新生成。

## 常用排查命令

```powershell
codex --help
codex login --help
codex login status
codex app-server --help
codex app-server generate-json-schema --out build\codex-app-schema --experimental
```

使用指定账号目录检查 CLI 状态：

```powershell
$env:CODEX_HOME = "C:\Path\To\Managed\CodexHome"
codex login status
```

## 常见问题

| 现象 | 常见原因 | 处理 |
| --- | --- | --- |
| `Codex CLI RPC failed; falling back to /status.` | 当前 CLI app-server 不可用或启动失败 | 确认 `codex app-server --help` 可用；必要时更新 Codex CLI |
| `Codex CLI connection failed.` | OAuth/RPC/PTY/Web 都失败 | 在 details/debug log 查看短失败摘要；先确认 `codex login status` |
| `Could not parse codex CLI status output` | PTY fallback 返回交互 TUI 画面而不是可解析 status | 优先使用 RPC；更新 Codex CLI；原始 TUI 输出只应出现在 details/debug log |
| Add Account 显示 `AUTHORIZATION` | 旧 parser 误解析设备授权说明文字 | 更新到当前构建，取消旧流程后重新 Add Account |
| 切换 active account 后用量未变化 | 账号没有真正授权、active account 未更新，或远端返回相同套餐窗口 | 用 Test Connection 验证；检查 `CODEX_HOME` 对应 `codex login status` |

## 测试覆盖

相关默认测试目标：

- `tst_CodexProvider`: provider 顺序、RPC/PTY fallback、app-server 响应映射。
- `tst_CodexLoginRunner`: device-auth 输出解析、ANSI 清理、失败退出、`AUTHORIZATION` 回归、id token email 解析。
- `tst_FetchContext`: active managed account 注入 `CODEX_HOME`。
- `tst_ProviderPipeline`: owning strategy 生命周期与 source mode 过滤。

常用验证：

```powershell
cmake --build build --config Release --target tst_CodexProvider tst_CodexLoginRunner tst_FetchContext -- /m /v:minimal /clp:ErrorsOnly
ctest --test-dir build -C Release -R "tst_CodexProvider|tst_CodexLoginRunner|tst_FetchContext" --output-on-failure
cmake --build build --config Release --target WinCodexBar -- /m /v:minimal /clp:ErrorsOnly
ctest --test-dir build -C Release --output-on-failure
```
