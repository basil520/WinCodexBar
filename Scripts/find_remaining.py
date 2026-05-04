import re
ts = r'D:\WinCodexBar\translations\WinCodexBar_zh_CN.ts'
with open(ts, 'r', encoding='utf-8') as f:
    content = f.read()

blocks = content.split('</context>')
for block in blocks:
    nm = re.search(r'<name>(.+?)</name>', block)
    name = nm.group(1) if nm else '?'
    for m in re.finditer(r'<source>(.+?)</source>\s*<translation type="unfinished"></translation>', block, re.DOTALL):
        src = m.group(1).strip()
        print(f'{name}: {src[:120]}')
