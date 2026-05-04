import re

ts = r'D:\WinCodexBar\translations\WinCodexBar_zh_CN.ts'
with open(ts, 'r', encoding='utf-8') as f:
    content = f.read()

# Dictionary: actual .ts source text → Chinese translation
# Key: EXACT source text from .ts (with &apos; etc.)
translations = {
    # Gemini (with &apos;)
    "Gemini OAuth credentials not found. Run &apos;gemini&apos; CLI to authenticate.": "未找到 Gemini OAuth 凭据。请运行 'gemini' CLI 进行认证。",
    "Gemini API key mode does not support usage quotas. Use OAuth mode (run &apos;gemini&apos; CLI to authenticate).": "Gemini API Key 模式不支持用量配额。请使用 OAuth 模式（运行 'gemini' CLI 认证）。",
    "Google ADC not found. Run &apos;gcloud auth application-default login&apos;.": "未找到 Google ADC。请运行 'gcloud auth application-default login'。",
    "GCP project not found. Set GOOGLE_CLOUD_PROJECT or run &apos;gcloud config set project&apos;.": "未找到 GCP 项目。请设置 GOOGLE_CLOUD_PROJECT 或运行 'gcloud config set project'。",

    # Codex with backticks
    "Codex CLI update needed. Run `npm i -g @openai/codex` to continue.": "需更新 Codex CLI。请运行 npm i -g @openai/codex 继续。",
    "Refresh token expired. Please run `codex` to log in again.": "刷新令牌已过期。请运行 codex 重新登录。",
    "Refresh token was revoked. Please run `codex` to log in again.": "刷新令牌已被撤销。请运行 codex 重新登录。",
    "Refresh token was already used. Please run `codex` to log in again.": "刷新令牌已被使用。请运行 codex 重新登录。",
    "codex CLI not found in PATH. Install via `npm i -g @openai/codex`": "PATH 中未找到 codex CLI。请通过 npm i -g @openai/codex 安装。",

    # Others
    "Codex CLI RPC returned no rate limit data.": "Codex CLI RPC 未返回速率限制数据。",
    "Codex CLI RPC returned no rate limit windows.": "Codex CLI RPC 未返回速率限制窗口。",
    "ConPTY is not available on this Windows version (requires Windows 10 1809+). codex CLI needs a terminal.": "此 Windows 版本不支持 ConPTY（需要 Windows 10 1809+）。codex CLI 需要终端。",
    "codex CLI still reports 'stdin is not a terminal' even with ConPTY.": "codex CLI 仍报告 stdin 不是终端，即使已启用 ConPTY。",
    "Codex data not available yet; will retry shortly.": "Codex 数据暂不可用，稍后将重试。",
    "Could not parse codex CLI status output. The CLI returned interactive screen output instead of usage data.": "无法解析 codex CLI 状态输出。CLI 返回了交互式屏幕输出而非用量数据。",
    "Empty response from web dashboard (HTTP request failed or returned empty)": "Web Dashboard 响应为空（HTTP 请求失败或返回空内容）",
    "Could not parse usage data from web dashboard.": "无法解析 Web Dashboard 用量数据。",
    "Failed to start ConPTY session for codex CLI": "无法启动 codex CLI 的 ConPTY 会话",
    "codex CLI exited before we could send /status": "codex CLI 在发送 /status 前已退出",
    "Could not parse codex CLI status output.": "无法解析 codex CLI 状态输出。",
    "Failed to start persistent CLI session": "无法启动持久 CLI 会话",
    "CLI session exited unexpectedly": "CLI 会话意外退出",
    "Timed out waiting for status output": "等待状态输出超时",
    "Empty response from token refresh endpoint": "令牌刷新端点响应为空",
    "Token refresh failed": "令牌刷新失败",
    "Invalid refresh response: missing access_token": "无效的刷新响应：缺少 access_token",
    "RPC client not started": "RPC 客户端未启动",
    "No credits data in OAuth response": "OAuth 响应中无积分数据",
    "Codex CLI RPC failed to start": "Codex CLI RPC 启动失败",
    "No credits in RPC response": "RPC 响应中无积分",
    "No credits parsed from CLI status output": "CLI 状态输出中未解析到积分",

    # Kimi
    "Kimi auth token is missing. Please add your JWT token from the Kimi console.": "Kimi 认证令牌缺失。请从 Kimi 控制台添加 JWT 令牌。",
    "Kimi auth token is invalid or expired. Please refresh your token.": "Kimi 认证令牌无效或已过期。请刷新令牌。",
    "Invalid request": "无效请求",
    "Kimi network error": "Kimi 网络错误",
    "Kimi API error": "Kimi API 错误",
    "Failed to parse Kimi usage data": "无法解析 Kimi 用量数据",
    "Unknown Kimi error": "未知 Kimi 错误",

    # OpenCode Go
    "No OpenCode Go auth cookie found in browser cookies or manual input.": "未找到 OpenCode Go 认证 Cookie。",
    "Failed to fetch OpenCode workspace ID.": "获取 OpenCode 工作区 ID 失败。",
    "OpenCode Go workspace page returned no usage data.": "OpenCode Go 工作区页面未返回用量数据。",
    "No usage data found in OpenCode Go response.": "OpenCode Go 响应中未找到用量数据。",

    # Pipeline/other
    "Application shutting down": "应用正在关闭",
    "No available fetch strategy": "无可用获取策略",
    "test override returned no credits": "测试覆盖未返回积分",
    "empty or invalid response from Alibaba web": "Alibaba Web 响应为空或无效",
    "Quota information unavailable": "配额信息不可用",
    "API authenticated": "API 已认证",

    # QML
    "Code Review": "Code Review",
    "Metric #": "指标 #",
    "Credits": "积分",
    "CodexBar": "CodexBar",
}

count = 0
for src, trans in translations.items():
    # Replace exact source text with translation
    pattern = r'(' + re.escape(src) + r'</source>\s*<translation type="unfinished">)\s*(</translation>)'
    replacement = r'\1' + trans + r'\2'
    new_content, n = re.subn(pattern, replacement, content)
    if n > 0:
        content = new_content
        count += n

remaining = len(re.findall(r'type="unfinished"', content))
print(f'Fixed: {count}, Remaining unfinished: {remaining}')

with open(ts, 'w', encoding='utf-8') as f:
    f.write(content)
print('Saved.')
