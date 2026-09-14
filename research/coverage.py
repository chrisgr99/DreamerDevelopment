"""WHAT IS INSTALLED THAT NOBODY HAS WRITTEN ABOUT YET.

Every other tool here reads research/help and checks it against itself. This one starts from the
other end — the plugins actually installed — and says what is missing, which is the only question
that matters after adding a plugin from the library.

THREE KINDS OF GAP, AND THEY COST DIFFERENT AMOUNTS.

  A PLUGIN WITH NO FILE AT ALL is the expensive one: nobody has read its manual, so every module
  needs its lines written, its controls tagged, its jacks given families. That is the original
  pass, and it is hours per plugin rather than minutes.

  A MODULE WITH NO ENTRY inside a file that exists is the same work for one module — usually a
  maker who added modules in a version newer than the one that was read.

  A PORT WITH NO FIELDS is the cheap one: the prose is written and only the port properties are
  missing, which is the pass the agents are running now.

validate_help.py cannot see the first kind at all, because it walks the files that exist. That is
the hole this fills.

    python3 research/coverage.py          # the summary
    python3 research/coverage.py --models # every module that has no entry
"""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
HELP = os.path.join(HERE, 'help')
PLUGINS = os.path.expanduser('~/Library/Application Support/Rack2/plugins-mac-arm64')
# Rack's own Core is built into the application rather than installed as a plugin.
CORE = '/Applications/VCV Rack 2 Free.app/Contents/Resources/Core.json'


def installed():
    """Every model on this machine, as {plugin: [model slugs]}."""
    out = {}
    for name in sorted(os.listdir(PLUGINS)):
        path = os.path.join(PLUGINS, name, 'plugin.json')
        if not os.path.exists(path):
            continue
        with open(path) as f:
            doc = json.load(f)
        out[doc.get('slug', name)] = [m['slug'] for m in doc.get('modules', []) if m.get('slug')]
    if os.path.exists(CORE):
        with open(CORE) as f:
            doc = json.load(f)
        out[doc.get('slug', 'Core')] = [m['slug'] for m in doc.get('modules', []) if m.get('slug')]
    return out


def written():
    """What research/help covers, as {plugin: {model: (ports, ports with fields)}}."""
    out = {}
    for name in sorted(os.listdir(HELP)):
        if not name.endswith('.json'):
            continue
        with open(os.path.join(HELP, name)) as f:
            doc = json.load(f)
        mods = {}
        for model, entry in doc.get('modules', {}).items():
            if not isinstance(entry, dict):
                mods[model] = (0, 0)
                continue
            ports = len(entry.get('in') or {})
            done = len(((entry.get('props') or {}).get('in')) or {})
            mods[model] = (ports, done)
        out[doc['plugin']] = mods
    return out


def main():
    show_models = '--models' in sys.argv
    have, mine = installed(), written()

    no_file, no_entry, no_props = [], [], []
    for plugin, models in sorted(have.items()):
        if plugin not in mine:
            no_file.append((plugin, len(models)))
            continue
        for model in models:
            if model not in mine[plugin]:
                no_entry.append((plugin, model))
            else:
                ports, done = mine[plugin][model]
                if ports > done:
                    no_props.append((plugin, model, ports - done))

    print('installed: %d plugins, %d modules'
          % (len(have), sum(len(m) for m in have.values())))
    print()
    print('NO HELP FILE AT ALL — the full pass, hours per plugin')
    if no_file:
        for plugin, n in sorted(no_file, key=lambda x: -x[1]):
            print('   %-26s %4d modules' % (plugin, n))
        print('   %d plugins, %d modules' % (len(no_file), sum(n for _, n in no_file)))
    else:
        print('   none')
    print()
    print('NO ENTRY, in a file that exists — the same pass, one module at a time')
    if no_entry:
        by = {}
        for plugin, model in no_entry:
            by.setdefault(plugin, []).append(model)
        for plugin, models in sorted(by.items(), key=lambda x: -len(x[1])):
            print('   %-26s %4d: %s' % (plugin, len(models), ', '.join(sorted(models)[:6])))
        print('   %d modules' % len(no_entry))
    else:
        print('   none')
    print()
    print('PORTS WITH NO FIELDS — the cheap pass')
    if no_props:
        by = {}
        for plugin, model, n in no_props:
            by[plugin] = by.get(plugin, 0) + n
        for plugin, n in sorted(by.items(), key=lambda x: -x[1])[:12]:
            print('   %-26s %4d ports' % (plugin, n))
        print('   %d plugins, %d ports' % (len(by), sum(by.values())))
        if show_models:
            for plugin, model, n in sorted(no_props):
                print('      %s/%s  %d' % (plugin, model, n))
    else:
        print('   none')

    # An entry for something not installed is the other direction, and worth knowing after an
    # uninstall: the text is kept, but nobody can reach it.
    extra = [(p, m) for p, mods in mine.items() for m in mods
             if p in have and m not in have[p]]
    if extra:
        print('\nENTRIES FOR MODULES NOT INSTALLED: %d (kept, but unreachable)' % len(extra))


if __name__ == '__main__':
    main()
