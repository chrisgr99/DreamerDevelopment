#!/usr/bin/env python3
"""What building the widget recovered: port names the census used to miss.

WHY THIS EXISTS. The census reads each port's name off the module, and for most makers that is
where it is set — in the module's constructor. Some call configInput and configOutput from the
WIDGET's constructor instead, guarded by `if (module)`, and those names were never recorded: the
census built its widgets with no module behind them. Rack itself shows those names perfectly
well, because in a rack the widget is built against a real module.

So the census now builds the widget against the module. This says what that was worth, which is
the question that was open before it was done. Take a copy of the old census first:

    cp ~/Library/Application\\ Support/Rack2/DreamerDevelopment/census.json /tmp/census-before.json
    # ... rewrite the census from the Dark module ...
    python3 research/census_recovered.py /tmp/census-before.json
"""
import collections
import json
import os
import sys

AFTER = os.path.expanduser(
    '~/Library/Application Support/Rack2/DreamerDevelopment/census.json')


def ports(path):
    """{(plugin, model, kind, index): name} for every port in a census."""
    doc = json.load(open(path))
    rows = doc['modules'] if isinstance(doc, dict) else doc
    out = {}
    for r in rows:
        for kind in ('inputs', 'outputs'):
            for i, p in enumerate(r.get(kind, [])):
                out[(r.get('plugin'), r.get('model'), kind, i)] = (p.get('name') or '').strip()
    return out


def main():
    before_path = sys.argv[1] if len(sys.argv) > 1 else '/tmp/census-before.json'
    if not os.path.exists(before_path):
        print('no snapshot at %s — take one before rewriting the census' % before_path)
        return 1
    before, after = ports(before_path), ports(AFTER)

    gained = collections.Counter()
    lost = collections.Counter()
    examples = []
    for key, name in after.items():
        was = before.get(key)
        if was is None:
            continue          # a model that was not in the old census
        if not was and name:
            gained[key[0]] += 1
            if len(examples) < 15:
                examples.append((key, name))
        elif was and not name:
            lost[key[0]] += 1

    blank_before = sum(1 for v in before.values() if not v)
    blank_after = sum(1 for v in after.values() if not v)
    print('ports unnamed before: %d' % blank_before)
    print('ports unnamed after:  %d' % blank_after)
    print('names recovered:      %d' % sum(gained.values()))
    if lost:
        # Nothing should ever go the other way. If something has, the widget pass is destroying
        # names rather than adding them, and that is a bug, not a result.
        print('\nNAMES LOST — this should never happen, investigate before trusting the census:')
        for k, v in lost.most_common():
            print('  %5d  %s' % (v, k))
    print('\nrecovered by plugin:')
    for k, v in gained.most_common(25):
        print('  %5d  %s' % (v, k))
    print('\na few of the recovered names:')
    for (plug, model, kind, i), name in examples:
        print('  %-22s %-22s %s%-3d %s' % (plug, model, kind[:-1], i, name))
    return 0


if __name__ == '__main__':
    sys.exit(main())
