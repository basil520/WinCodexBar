import re
ts = r'D:\WinCodexBar\translations\WinCodexBar_zh_CN.ts'
with open(ts, 'r', encoding='utf-8') as f:
    content = f.read()
for m in re.finditer(r'<source>(.+?)</source>\s*<translation type="unfinished"', content, re.DOTALL):
    ctx_start = content.rfind('<name>', 0, m.start())
    ctx_end = content.find('</name>', ctx_start)
    ctx = content[ctx_start+6:ctx_end]
    src = m.group(1).replace('\n', '\\n')
    print(f'  {ctx}: {src[:100]}')
