import re
ts = r'D:\WinCodexBar\translations\WinCodexBar_zh_CN.ts'
with open(ts, 'r', encoding='utf-8') as f:
    content = f.read()

# Replace "令牌" with "词元" and "token" with "词元" in translations only
replacements = [
    (r'<translation>令牌</translation>', '<translation>词元</translation>'),
    (r'<translation>tokens</translation>', '<translation>词元</translation>'),
    (r'<translation>Token 用量</translation>', '<translation>词元用量</translation>'),
    (r'<translation>Token 用量总览</translation>', '<translation>词元用量总览</translation>'),
    (r'<translation>Token 提供', '<translation>词元提供'),
    (r'<translation>token 来源</translation>', '<translation>词元来源</translation>'),
    (r'<translation>未启用 Token 提供商</translation>', '<translation>未启用词元提供商</translation>'),
    (r'<translation>未找到 Token 提供商</translation>', '<translation>未找到词元提供商</translation>'),
    (r'<translation>Tokens</translation>', '<translation>词元</translation>'),
]

for pattern, replacement in replacements:
    new_content, n = re.subn(pattern, replacement, content)
    if n > 0:
        content = new_content
        print(f'OK ({n}): {pattern[:60]}')

with open(ts, 'w', encoding='utf-8') as f:
    f.write(content)
print('Saved.')
