import re

ts = r'D:\WinCodexBar\translations\WinCodexBar_zh_CN.ts'
with open(ts, 'r', encoding='utf-8') as f:
    content = f.read()

fixes = [
    # ProviderLabels — 17 new
    ("Balance", "余额"),
    ("Prompts", "提示次数"),
    ("Window", "窗口"),
    ("Search", "搜索"),
    ("Recurring", "周期"),
    ("Promo", "促销"),
    ("Purchased", "已购"),
    ("Usage", "用量"),
    ("Pro", "专业版"),
    ("Flash", "闪电版"),
    ("Flash Lite", "闪电精简版"),
    ("Standard", "标准"),
    ("Claude", "Claude"),
    ("Gemini Pro", "Gemini Pro"),
    ("Gemini Flash", "Gemini Flash"),
    ("Requests", "请求"),
    ("Interval", "间隔"),

    # ProviderSettings — 4 new
    ("API key", "API 密钥"),
    ("API key (optional)", "API 密钥（可选）"),
    ("Manual cookie", "手动 Cookie"),

    # ProviderErrors — DeepSeek
    ("DeepSeek API key not configured.", "DeepSeek API 密钥未配置。"),
    ("Empty or invalid response from DeepSeek API", "DeepSeek API 响应为空或无效"),

    # MiniMax
    ("MiniMax API key not configured.", "MiniMax API 密钥未配置。"),
    ("Empty or invalid response from MiniMax API", "MiniMax API 响应为空或无效"),
    ("MiniMax API credentials are invalid or expired.", "MiniMax API 凭据无效或已过期。"),
    ("Empty response from MiniMax coding plan page", "MiniMax Coding Plan 页面响应为空"),
    ("MiniMax session expired. Please re-import your cookie.", "MiniMax 会话已过期，请重新导入 Cookie。"),
    ("Could not parse MiniMax coding plan data from page", "无法解析页面中的 MiniMax Coding Plan 数据"),
    ("No MiniMax cookie available. Log in to platform.minimax.io in your browser.", "无 MiniMax Cookie。请在浏览器中登录 platform.minimax.io。"),

    # Synthetic
    ("Empty or invalid response from Synthetic API", "Synthetic API 响应为空或无效"),
    ("Synthetic API key not configured.", "Synthetic API 密钥未配置。"),

    # Perplexity
    ("Empty or invalid response from Perplexity API", "Perplexity API 响应为空或无效"),
    ("No Perplexity cookie available. Log in to perplexity.ai in your browser or paste cookie manually.", "无 Perplexity Cookie。请在浏览器中登录 perplexity.ai 或手动粘贴 Cookie。"),

    # Amp
    ("Empty response from Amp", "Amp 响应为空"),
    ("Not signed in. Log in to ampcode.com in your browser.", "未登录。请在浏览器中登录 ampcode.com。"),
    ("Could not parse Amp usage data from page", "无法解析页面中的 Amp 用量数据"),
    ("Could not find quota in Amp usage data", "未在 Amp 用量数据中找到配额"),
    ("No Amp cookie available. Log in to ampcode.com in your browser.", "无 Amp Cookie。请在浏览器中登录 ampcode.com。"),

    # Augment
    ("auggie CLI not found in PATH.", "未在 PATH 中找到 auggie CLI。"),
    ("auggie account status timed out", "auggie account status 超时"),
    ("Empty output from auggie CLI", "auggie CLI 输出为空"),
    ("Empty response from Augment API", "Augment API 响应为空"),
    ("No Augment cookie available.", "无 Augment Cookie。"),

    # Gemini
    ("No quota buckets in Gemini response", "Gemini 响应中无配额桶"),
    ("Gemini OAuth credentials not found. Run 'gemini' CLI to authenticate.", "未找到 Gemini OAuth 凭据。请运行 'gemini' CLI 进行认证。"),
    ("Failed to refresh Gemini OAuth token.", "刷新 Gemini OAuth 令牌失败。"),
    ("Gemini API key mode does not support usage quotas. Use OAuth mode (run 'gemini' CLI to authenticate).", "Gemini API Key 模式不支持用量配额。请使用 OAuth 模式（运行 'gemini' CLI 认证）。"),

    # VertexAI
    ("Google ADC not found. Run 'gcloud auth application-default login'.", "未找到 Google ADC。请运行 'gcloud auth application-default login'。"),
    ("GCP project not found. Set GOOGLE_CLOUD_PROJECT or run 'gcloud config set project'.", "未找到 GCP 项目。请设置 GOOGLE_CLOUD_PROJECT 或运行 'gcloud config set project'。"),

    # JetBrains
    ("No AIAssistant quota data found in XML", "XML 中未找到 AI Assistant 配额数据"),
    ("No quotaInfo in JetBrains data", "JetBrains 数据中无 quotaInfo"),
    ("No JetBrains IDE installation found.", "未找到 JetBrains IDE 安装。"),

    # Factory
    ("Empty response from Factory API", "Factory API 响应为空"),
    ("No Factory cookie available. Log in to app.factory.ai in your browser.", "无 Factory Cookie。请在浏览器中登录 app.factory.ai。"),

    # Antigravity
    ("No response from Antigravity local service", "Antigravity 本地服务无响应"),
    ("Antigravity is not running.", "Antigravity 未在运行。"),

    # Warp
    ("Warp API key not configured.", "Warp API 密钥未配置。"),
    ("Empty response from Warp GraphQL API", "Warp GraphQL API 响应为空"),
    ("GraphQL error", "GraphQL 错误"),

    # Abacus
    ("No Abacus AI cookie available. Log in to apps.abacus.ai in your browser.", "无 Abacus AI Cookie。请在浏览器中登录 apps.abacus.ai。"),
    ("Empty response from Abacus AI API", "Abacus AI API 响应为空"),
    ("Abacus AI API error", "Abacus AI API 错误"),

    # QML "Balance" unfinished
    ("Balance", "余额"),
]

count = 0
for src, trans in fixes:
    pattern = r'(<source>' + re.escape(src) + r'</source>\s*<translation type=\"unfinished\"></translation>)'
    replacement = '<source>' + src + '</source>\n        <translation>' + trans + '</translation>'
    new_content, n = re.subn(pattern, replacement, content)
    if n > 0:
        content = new_content
        count += n
    else:
        # Try without type=unfinished (existing entries with empty translation)
        pattern2 = r'(<source>' + re.escape(src) + r'</source>\s*<translation></translation>)'
        new_content2, n2 = re.subn(pattern2, '<source>' + src + '</source>\n        <translation>' + trans + '</translation>', content)
        if n2 > 0:
            content = new_content2
            count += n2

remaining = len(re.findall(r'type="unfinished"', content))
print(f'Fixed: {count}, Remaining unfinished: {remaining}')

with open(ts, 'w', encoding='utf-8') as f:
    f.write(content)
print('Saved.')
