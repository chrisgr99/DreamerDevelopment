#!/usr/bin/env python3
"""Splits the census into one file per maker, for whoever is writing help entries.

An entry has to be written against the real panel: what the maker called each control, and where
each one sits. Both are in the census the Dark module writes, which is one large file covering
every installed plugin. This cuts it into per-maker slices under research/help/census/.

NOT COMMITTED, like the rendered panels: it is derived, and the source is the census itself.
Re-run it after a fresh census, or when a plugin is installed or updated.

    python3 research/make_census_slices.py
"""
import collections
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, 'help', 'census')
RACK = os.path.expanduser('~/Library/Application Support/Rack2/DreamerDevelopment')


def rows(doc):
    m = doc['modules'] if isinstance(doc, dict) else doc
    return m if isinstance(m, list) else list(m.values())


def main():
    names_path = os.path.join(RACK, 'census.json')
    pos_path = os.path.join(RACK, 'census-positions.json')
    if not os.path.exists(names_path):
        print('no census at %s — run it from the Dark module first' % names_path)
        return 1
    names = json.load(open(names_path))
    pos = json.load(open(pos_path)) if os.path.exists(pos_path) else {'modules': []}

    bypos = {(r.get('plugin'), r.get('model')): r for r in rows(pos)}

    by = collections.defaultdict(dict)
    for r in rows(names):
        plugin = r.get('plugin') or r.get('model', '').split('-')[0]
        slug = r.get('model') or r.get('slug')
        if not plugin or not slug:
            continue
        entry = {
            'params':  [p.get('name') for p in r.get('params', [])],
            'inputs':  [p.get('name') for p in r.get('inputs', [])],
            'outputs': [p.get('name') for p in r.get('outputs', [])],
        }
        # WHERE EACH CONTROL SITS, because index order is not panel order. NYSTHI's Model277
        # numbers its output jacks from the bottom up, and an entry tagged from the numbering
        # alone was exactly reversed. y increases downwards.
        p = bypos.get((plugin, slug))
        if p:
            for kind, key in (('input', 'inputPos'), ('output', 'outputPos')):
                got = [(x['index'], x['cx'], x['cy'])
                       for x in p.get('ports', []) if x['kind'] == kind]
                entry[key] = {str(i): [cx, cy] for i, cx, cy in sorted(got)}
            got = [(x['index'], x.get('cx'), x.get('cy')) for x in p.get('params', [])]
            entry['paramPos'] = {str(i): [cx, cy] for i, cx, cy in sorted(got)}
        by[plugin][slug] = entry

    if not os.path.isdir(OUT):
        os.makedirs(OUT)
    for plugin, models in by.items():
        with open(os.path.join(OUT, plugin + '.json'), 'w') as f:
            json.dump({'plugin': plugin, 'models': models}, f, indent=1, ensure_ascii=False)
    print('%d makers, %d models -> %s'
          % (len(by), sum(len(v) for v in by.values()), OUT))
    return 0


if __name__ == '__main__':
    sys.exit(main())
