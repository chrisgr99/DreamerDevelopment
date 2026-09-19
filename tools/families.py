"""Writes src/PortFamilies.cpp from the help database, for Clarity's port colouring.

WHY THIS EXISTS AT ALL. Clarity colours a jack by what it carries — audio, cv, trigger, pitch —
and that judgement was made once, by hand, while the help text was written. It belongs to the
help database and it is edited there. This turns it into a table Clarity can compile against, so
that DreamerDevelopment needs neither the help plugin nor its data at runtime.

ONE SOURCE OF TRUTH IS A RULE ABOUT WHERE THINGS ARE EDITED, not about where they sit at build
time. Nobody hand-edits src/PortFamilies.cpp; it is regenerated, it says so at the top, and it
records which version of the database it came from. If a colour is wrong, it is wrong upstream.

    python3 tools/families.py <path to a DreamerHelp checkout>

The Makefile calls this automatically when a database file is newer than the table, so the
only thing a person has to remember is to pull the help repository.
"""
import json
import os
import subprocess
import sys

FAMILIES = ('audio', 'cv', 'trigger', 'pitch')


def version_of(repo):
    """What the table was generated from, recorded in the header so the question is answerable
    by opening the file. A checkout with no git metadata still generates; it just says so."""
    try:
        out = subprocess.run(['git', '-C', repo, 'describe', '--always', '--dirty'],
                             capture_output=True, text=True, timeout=10)
        if out.returncode == 0 and out.stdout.strip():
            return out.stdout.strip()
    except Exception:
        pass
    return 'unknown revision'


def main():
    if len(sys.argv) < 2:
        raise SystemExit('usage: families.py <path to a DreamerHelp checkout>')
    repo = os.path.abspath(os.path.expanduser(sys.argv[1]))
    src = os.path.join(repo, 'data', 'research')
    if not os.path.isdir(src):
        raise SystemExit('no data/research in %s — is that a DreamerHelp checkout?' % repo)

    # THE RESEARCH, NOT THE HELP. Each jack's family is kept with the facts behind the help, in
    # data/research/<Plugin>/<Module>.json, which DreamerHelp does not ship.
    rows = []
    ports = 0
    for plugin in sorted(os.listdir(src)):
        folder = os.path.join(src, plugin)
        if not os.path.isdir(folder):
            continue
        for name in sorted(os.listdir(folder)):
            if not name.endswith('.json') or name.startswith('.'):
                continue
            with open(os.path.join(folder, name), encoding='utf-8') as f:
                doc = json.load(f)
            model = doc.get('module') or name[:-5]
            facts = doc.get('facts') or {}
            ins, outs = [], []
            for kind, out in (('inputs', ins), ('outputs', outs)):
                m = {k: v['family'] for k, v in (facts.get(kind) or {}).items()
                     if isinstance(v, dict) and 'family' in v}
                if not m:
                    continue
                highest = max(int(k) for k in m)
                # -1 IS "NOBODY SAID", and it has to be storable: a row of jacks where the
                # third was never given a colour is not the same as a row that stops at two.
                out.extend([-1] * (highest + 1))
                for k, v in m.items():
                    out[int(k)] = FAMILIES.index(v) if v in FAMILIES else -1
            if ins or outs:
                rows.append((plugin, model, ins, outs))
                ports += sum(1 for v in ins + outs if v >= 0)

    # By plugin, then module. Clarity reads the table from start to finish, so the order is only
    # for a person reading it, and for a regeneration that changes nothing to diff as nothing.
    rows.sort(key=lambda r: (r[0], r[1]))

    def arr(name, vals):
        if not vals:
            return None, '0, NULL'
        return ('static const signed char %s[] = {%s};'
                % (name, ', '.join(str(v) for v in vals))), '%d, %s' % (len(vals), name)

    out = []
    out.append('/** GENERATED — do not edit.\n'
               '\n'
               '    Port families for Clarity\'s colouring, from the help database.\n'
               '    Source: DreamerHelp at %s\n'
               '    Regenerate: make families (or tools/families.py <checkout>)\n'
               '\n'
               '    THE DATABASE IS AUTHORITATIVE. If a colour here is wrong, fix it there and\n'
               '    regenerate; an edit made in this file will be overwritten without warning. */\n'
               % version_of(repo))
    out.append('#include "PortFamilies.hpp"')
    out.append('#include <cstring>')
    out.append('')
    decls, table = [], []
    for i, (plugin, model, ins, outs) in enumerate(rows):
        di, ri = arr('fi%d' % i, ins)
        do, ro = arr('fo%d' % i, outs)
        if di:
            decls.append(di)
        if do:
            decls.append(do)
        table.append('\t{"%s", "%s", %s, %s},' % (plugin, model, ri, ro))
    out.extend(decls)
    out.append('')
    out.append('const PortFamilyEntry PORT_FAMILIES[] = {')
    out.extend(table)
    out.append('};')
    out.append('const int PORT_FAMILY_COUNT = %d;' % len(rows))
    out.append('')

    dest = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                        'src', 'PortFamilies.cpp')
    with open(dest, 'w') as f:
        f.write('\n'.join(out))
    print('%d modules, %d ports coloured -> %s' % (len(rows), ports, dest))


if __name__ == '__main__':
    main()
