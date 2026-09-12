"""Turn the port map into src/PortMap.cpp.

The map itself is research/nysthi-portmap.json, written by reading each module's panel with its
port indices drawn on. This only formats it. Run from the repository root:

    python3 research/make_portmap.py
"""
import json, os

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
FAM = {'audio': 'FAM_AUDIO', 'cv': 'FAM_CV', 'trigger': 'FAM_TRIGGER', 'pitch': 'FAM_PITCH'}


def main():
    m = json.load(open(os.path.join(HERE, 'nysthi-portmap.json')))
    src = open(os.path.join(ROOT, 'src', 'PortMap.cpp')).read()
    head = src.split('const Known KNOWN[] = {')[0]
    # THE TAIL IS COUNTED FROM THE HEAD, NOT FROM THE TOP OF THE FILE. Counting `};` from the
    # start meant the struct definitions above KNOWN were counted too, so a second run kept the
    # old ENTRIES array and wrote a file with two of everything. Split only what follows the
    # head: there the first `};` closes KNOWN and the second closes ENTRIES.
    body = src[len(head):]
    tail = body.split('};\n', 2)[-1]
    # '\n'.join puts the newline back, so a head that still ends in one grows a blank line on
    # every run. Trim it here and the file is byte-identical whenever the map has not changed.
    head = head.rstrip('\n')
    rows = []
    for model in sorted(m['modules']):
        e = m['modules'][model]
        entries = sorted(e['map'].items(),
                         key=lambda kv: (kv[0].split('/')[0], int(kv[0].split('/')[1])))
        rows.append((model, e['ports'], entries))
    out = [head, 'const Known KNOWN[] = {']
    for model, ports, _ in rows:
        out.append('\t{"%s", %d},' % (model, ports))
    out.append('};\n')
    out.append('const Entry ENTRIES[] = {')
    n = 0
    for model, _, entries in rows:
        for key, fam in entries:
            kind, idx = key.split('/')
            out.append('\t{"%s", %s, %s, %s},'
                       % (model, 'true' if kind == 'out' else 'false', idx, FAM[fam]))
            n += 1
    out.append('};\n')
    out.append(tail.lstrip('\n'))
    open(os.path.join(ROOT, 'src', 'PortMap.cpp'), 'w').write('\n'.join(out))
    print('%d entries across %d models' % (n, len(rows)))


main()
