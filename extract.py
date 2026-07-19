import json
import re

log_file = r'C:\Users\hakan\.gemini\antigravity-ide\brain\dbde4162-04d0-4478-b4e8-1568b9031e92\.system_generated\logs\transcript_full.jsonl'

diff_text = None
with open(log_file, 'r', encoding='utf-8') as f:
    for line in f:
        try:
            data = json.loads(line)
            if data.get('type') == 'TOOL_RESPONSE' and 'repowidget.cpp' in data.get('content', ''):
                diff_text = data['content']
        except:
            pass

if diff_text:
    in_block = False
    deleted_lines = []
    for line in diff_text.split('\n'):
        if line.startswith('@@ -214,259 +214,6 @@'):
            in_block = True
            continue
        if in_block:
            if line.startswith('[diff_block_end]'):
                in_block = False
            elif line.startswith('-'):
                deleted_lines.append(line[1:])
    
    with open('deleted_lines.txt', 'w', encoding='utf-8') as out:
        out.write('\n'.join(deleted_lines))
    print(f'Extracted {len(deleted_lines)} deleted lines.')
else:
    print('Diff text not found')
