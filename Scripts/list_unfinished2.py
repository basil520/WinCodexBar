import re

ts = r'D:\WinCodexBar\translations\WinCodexBar_zh_CN.ts'
with open(ts, 'r', encoding='utf-8') as f:
    content = f.read()

# Split by </context> then find messages with unfinished in each context
contexts = re.split(r'</context>', content)
for ctx_xml in contexts:
    name_match = re.search(r'<name>(.+?)</name>', ctx_xml)
    if not name_match:
        continue
    name = name_match.group(1)
    # Find all messages in this context
    msgs = re.finditer(r'<message>(.+?)</message>', ctx_xml, re.DOTALL)
    for m in msgs:
        msg_xml = m.group(1)
        src_match = re.search(r'<source>(.+?)</source>', msg_xml, re.DOTALL)
        if not src_match:
            continue
        src = src_match.group(1).strip()
        trans_match = re.search(r'<translation[^>]*>(.*?)</translation>', msg_xml, re.DOTALL)
        is_unfinished = 'type="unfinished"' in msg_xml
        trans = trans_match.group(1).strip() if trans_match else ''
        if is_unfinished or not trans:
            print(f'  {name}: {src[:80]}')
