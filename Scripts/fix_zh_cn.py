import re
import os

ts = r'D:\WinCodexBar\translations\WinCodexBar_zh_CN.ts'
with open(ts, 'r', encoding='utf-8') as f:
    content = f.read()

fixes = [
    ("Accounts", "账户"),
    ("Adding...", "添加中..."),
    ("Add Account", "添加账户"),
    ("Waiting for Codex authorization...", "等待 Codex 授权..."),
    ("Open", "打开"),
    ("Cancel", "取消"),
    ("Select the active Codex account for usage tracking.", "选择用于用量追踪的活动 Codex 账户。"),
    ("System", "系统"),
    ("Active", "活动中"),
    ("No email", "无邮箱"),
    ("Use", "使用"),
    ("Set as active", "设为活动中"),
    ("Auth", "认证"),
    ("Re-authenticating...", "重新认证中..."),
    ("Re-authenticate", "重新认证"),
    ("Promote", "提升"),
    ("Promote to system account", "提升为系统账户"),
    ("Remove", "移除"),
    ("Removing...", "移除中..."),
    ("Remove account", "移除账户"),
    ("No accounts configured. Click 'Add Account' to add a Codex account.", "未配置账户。点击'添加账户'以添加 Codex 账户。"),
    ("Account Management", "账户管理"),
    ("Manage multiple Codex accounts. Switch between accounts to track usage separately.", "管理多个 Codex 账户。切换账户以分别追踪用量。"),
    ("Usage Projection", "用量预测"),
    ("Projected rate lanes and credits from the current active account.", "来自当前活动账户的预测速率通道和积分。"),
    ("Buy More Credits", "购买更多积分"),
    ("Supplemental Metrics", "补充指标"),
    ("Test Codex Connection", "测试 Codex 连接"),
    ("Run a single fetch attempt and record diagnostics.", "执行单次获取尝试并记录诊断信息。"),
    ("Last session source", "最近会话来源"),
    ("Recent Fetch Attempts", "最近获取尝试"),
    ("Test", "测试"),
    ("Token", "令牌"),
    ("Credit", "积分"),
    ("Credit Events", "积分事件"),
    ("Usage by Service", "按服务用量"),
    ("Purchase credits", "购买积分"),
    ("Codex Verbose Logging", "Codex 详细日志"),
    ("Web Dashboard Debug Dump", "Web Dashboard 调试转储"),
    ("Codex Diagnostics", "Codex 诊断"),
    ("Failed to create system tray icon. Please restart the app.", "创建系统托盘图标失败。请重启应用。"),
    ("Log detailed strategy-level diagnostics during Codex fetches.", "Codex 获取期间记录详细的策略级诊断。"),
    ("Save raw HTML from web dashboard fetches for troubleshooting.", "保存 Web 仪表盘获取的原始 HTML 用于故障排除。"),
    ("Usage Details", "用量详情"),
    ("Copy", "复制"),
    ("No accounts configured. Click &apos;Add Account&apos; to add a Codex account.", "未配置账户。点击添加账户以添加 Codex 账户。"),
    ("Credits", "积分"),
    ("Refresh", "刷新"),
    ("Today", "今日"),
    ("tokens", "tokens"),
    ("30 days", "30天"),
    ("Providers", "提供商"),
    ("Total", "总计"),
    ("Quota", "配额"),
]

count = 0
for src, trans in fixes:
    pattern = r'(<source>' + re.escape(src) + r'</source>\s*<translation type=\"unfinished\"></translation>)'
    replacement = '<source>' + src + '</source>\n        <translation>' + trans + '</translation>'
    new_content, n = re.subn(pattern, replacement, content)
    if n > 0:
        content = new_content
        count += n
        print(f'OK ({n}): {src[:60]}')

remaining = len(re.findall(r'type=\"unfinished\"', content))
print(f'\nTotal fixed: {count}, Remaining unfinished: {remaining}')

with open(ts, 'w', encoding='utf-8') as f:
    f.write(content)
print('Saved.')
