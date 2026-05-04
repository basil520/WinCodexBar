import re
ts = r'D:\WinCodexBar\translations\WinCodexBar_zh_CN.ts'
with open(ts, 'r', encoding='utf-8') as f:
    content = f.read()

old = len(re.findall(r'type="unfinished"', content))
content = content.replace(' type="unfinished"', '')
new = len(re.findall(r'type="unfinished"', content))
print(f'Stripped: {old-new}, Remaining: {new}')

# Now check which have truly empty translation
empty = len(re.findall(r'<translation></translation>', content))
print(f'Truly empty: {empty}')

with open(ts, 'w', encoding='utf-8') as f:
    f.write(content)
print('Saved.')
