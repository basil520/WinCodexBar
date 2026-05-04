import re

ts = r'D:\WinCodexBar\translations\WinCodexBar_zh_CN.ts'
with open(ts, 'r', encoding='utf-8') as f:
    content = f.read()

ctxs = re.findall(r'<name>(.+?)</name>', content)
print(f'Contexts: {len(ctxs)}')
unf = len(re.findall(r'type="unfinished"', content))
print(f'Unfinished: {unf}')
sources = re.findall(r'<source>(.+?)</source>', content)
print(f'Sources (some merged): {len(sources)}')

# Count individual message blocks
msgs = len(re.findall(r'<message>', content))
print(f'Total message blocks: {msgs}')
