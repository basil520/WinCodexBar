import re
ts = r'D:\WinCodexBar\translations\WinCodexBar_zh_CN.ts'
with open(ts, 'r', encoding='utf-8') as f:
    content = f.read()
unf = len(re.findall(r'type="unfinished"', content))
print(f'Unfinished: {unf}')

# Find new sources
ctxs = re.findall(r'<name>(.+?)</name>', content)
print(f'Contexts: {len(ctxs)}')

# Find sources with "Window" or "Monthly" or other new keywords
for m in re.finditer(r'<source>(.+?)</source>', content):
    src = m.group(1).strip()
    if 'Window' in src or 'Monthly' in src or '5-hour' in src:
        print(f'  Found: {src[:80]}')
