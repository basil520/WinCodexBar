import re

ts = r'D:\WinCodexBar\translations\WinCodexBar_zh_CN.ts'
with open(ts, 'r', encoding='utf-8') as f:
    content = f.read()

fixes = [
    # Format strings and dynamic labels
    ("Balance unavailable for API calls", "API 余额不可用"),
    ("%1%2 — add credits at platform.deepseek.com", "%1%2 — 请前往 platform.deepseek.com 充值"),
    ("%1%2 (Paid: %1%3 / Granted: %1%4)", "%1%2（已购：%1%3 / 赠送：%1%4）"),
    ("Unlimited", "无限"),
    ("%1 / %2 requests", "%1 / %2 请求"),
    ("%1 bonus credits", "%1 奖励积分"),
    ("Promo credits", "促销积分"),
    ("Purchased credits", "已购积分"),
    ("compute points", "算力积分"),
    ("%1 / %2 prompts", "%1 / %2 提示"),
    ("%1 / %2 compute points", "%1 / %2 算力积分"),
    ("%1 remaining (%2/%3 used)", "剩余 %1（已用 %2/%3）"),
    ("%1/%2 requests", "%1/%2 请求"),
    ("Rate: %1/%2 per 5 hours", "速率：%1/%2 / 5 小时"),
    ("Credits: %1/%2", "积分：%1/%2"),
    ("api", "API"),
    ("web", "Web"),
    ("Paid", "已购"),
    ("Granted", "赠送"),
    ("requests", "请求"),
    ("bonus credits", "奖励积分"),
    ("%1 / %2", "%1 / %2"),
]

count = 0
for src, trans in fixes:
    pattern = r'(<source>' + re.escape(src) + r'</source>\s*<translation type=\"unfinished\">)\s*(</translation>)'
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
