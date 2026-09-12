#!/usr/bin/env python3
"""Where the maker told us something, did our own line use it?

Rack's tooltip is the maker's own name for a control, plus a second line where they wrote one.
That is often the most reliable description in existence — and an entry written from a manual can
miss it. This lists the controls where the maker said something substantial and our line shares
little of it, for a person to read.

It cannot judge meaning; it measures word overlap and sorts the least-overlapping first, which is
where a reviewer should start.

    python3 research/check_maker_text.py            # everything
    python3 research/check_maker_text.py Venom 40   # one maker, 40 rows
"""
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
HELP = os.path.join(HERE, 'help')
STOP = set('a an the of to in on at for with and or is are be by as from that this it its into '
           'per each every set sets setting control controls cv input output in out signal '
           'voltage volts knob jack port mode amount level value'.split())


def words(t):
    return {w for w in re.findall(r'[a-z0-9]+', t.lower()) if w not in STOP and len(w) > 2}


def main():
    only = sys.argv[1] if len(sys.argv) > 1 else None
    limit = int(sys.argv[2]) if len(sys.argv) > 2 else 25
    rows = []
    for name in sorted(os.listdir(HELP)):
        if not name.endswith('.json'):
            continue
        doc = json.load(open(os.path.join(HELP, name)))
        plugin = doc['plugin']
        if only and plugin.lower() != only.lower():
            continue
        cpath = os.path.join(HELP, 'census', plugin + '.json')
        if not os.path.exists(cpath):
            continue
        cen = json.load(open(cpath))['models']
        for model, entry in doc['modules'].items():
            if not isinstance(entry, dict) or model not in cen:
                continue
            lines = entry['lines']
            c = cen[model]
            for kind, namekey, desckey in (('param', 'params', 'paramDesc'),
                                           ('in', 'inputs', 'inputDesc'),
                                           ('out', 'outputs', 'outputDesc')):
                names = c.get(namekey) or []
                descs = c.get(desckey) or {}
                for idx, li in (entry.get(kind) or {}).items():
                    i = int(idx)
                    # THE DESCRIPTION ONLY, not the name.
                    #
                    # A name is a label — "Manual trigger", "Harmonic series" — and our house
                    # style forbids repeating it, so word overlap against a name is near zero
                    # even when our line is plainly better. The description is the second line of
                    # the tooltip, where a maker writes a FACT: what a jack is normalled to, what
                    # a switch does that its name does not say. Those are worth checking.
                    maker = (descs.get(idx) or '').strip()
                    mw = words(maker)
                    if len(maker) < 14 or len(mw) < 2:
                        continue
                    if not isinstance(li, int) or li >= len(lines):
                        continue
                    ours = words(lines[li])
                    shared = mw & ours
                    score = len(shared) / len(mw)
                    if score < 0.5:
                        rows.append((score, plugin, model, kind, idx, maker, lines[li]))
    rows.sort(key=lambda r: (r[0], r[1]))
    for score, plugin, model, kind, idx, maker, ours in rows[:limit]:
        print('%-16s %-22s %s %-3s  overlap %.0f%%' % (plugin, model, kind, idx, score * 100))
        print('   maker: %s' % maker[:150])
        print('   ours : %s' % ours[:150])
    print('\n%d controls where the maker said something our line may not have used'
          % len(rows))
    return 0


if __name__ == '__main__':
    sys.exit(main())
