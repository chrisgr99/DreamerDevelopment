"""THE PART OF AN ENTRY THAT IS ALREADY KNOWN, WRITTEN DOWN BEFORE ANYBODY STARTS.

Adding 269 plugins means 269 files whose skeleton is identical and whose contents are not. The
skeleton can be built from what is already on disk — the installed manifest and the census — and
every minute an agent spends assembling structure is a minute not spent reading source, plus a
chance to get a port index wrong.

So this writes two things per plugin.

THE ENTRY ITSELF, research/help/<Plugin>.json, with the plugin slug, the source URL and version
taken from the installed manifest, today's date, and one stub per model carrying the maker's own
one-line description as its first line. Marked `partial` so the validator does not yet demand
full coverage. An agent rewrites the lines, adds the tag maps, the families and the props, and
removes the flag when the plugin is done.

AND A WORKSHEET, research/help/scaffold/<Plugin>.md, which is never shipped and never validated.
It says what the census knows and an agent would otherwise have to work out: how many controls
each model has, which ports the maker never named — those are the ones the help exists for — and
which have no widget at all, because a port with no position can never be clicked and gets no
line. Those two facts caused most of the corrections in the first library.

    python3 research/scaffold.py            # every installed plugin with no entry
    python3 research/scaffold.py Befaco     # one of them, by slug prefix
    python3 research/scaffold.py --force    # rewrite skeletons that already exist
"""
import datetime
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
HELP = os.path.join(HERE, 'help')
CENSUS = os.path.join(HELP, 'census')
SHEETS = os.path.join(HELP, 'scaffold')
PLUGINS = os.path.expanduser('~/Library/Application Support/Rack2/plugins-mac-arm64')
CORE = '/Applications/VCV Rack 2 Free.app/Contents/Resources/Core.json'


def manifests():
    out = {}
    for name in sorted(os.listdir(PLUGINS)):
        path = os.path.join(PLUGINS, name, 'plugin.json')
        if os.path.exists(path):
            with open(path) as f:
                doc = json.load(f)
            out[doc.get('slug', name)] = doc
    if os.path.exists(CORE):
        with open(CORE) as f:
            doc = json.load(f)
        out[doc.get('slug', 'Core')] = doc
    return out


def census_of(slug):
    path = os.path.join(CENSUS, slug + '.json')
    if not os.path.exists(path):
        return {}
    with open(path) as f:
        return json.load(f).get('models', {})


def worksheet(slug, doc, models):
    """What the census already settles, as prose an agent reads once."""
    lines = ['# %s — what the census already says' % slug, '']
    lines.append('Installed version %s. Source: %s'
                 % (doc.get('version', '?'), doc.get('sourceUrl') or 'NOT PUBLISHED — the binary '
                    'and the maker\'s manuals are what there is'))
    lines.append('')
    lines.append('%d modules. For each: how many controls, then anything the census flags.'
                 % len(doc.get('modules', [])))
    lines.append('')
    unnamed_total = 0
    hidden_total = 0
    for m in doc.get('modules', []):
        slug_m = m.get('slug')
        c = models.get(slug_m, {})
        ins = c.get('inputs') or []
        outs = c.get('outputs') or []
        pars = c.get('params') or []
        ipos = c.get('inputPos') or {}
        unnamed = [i for i, n in enumerate(ins) if not (n or '').strip()]
        hidden = [i for i in range(len(ins)) if str(i) not in ipos]
        unnamed_total += len(unnamed)
        hidden_total += len(hidden)
        bits = ['%d in, %d out, %d controls' % (len(ins), len(outs), len(pars))]
        if not c:
            bits.append('**NOT IN THE CENSUS** — skipped or failed to build')
        if unnamed:
            bits.append('**%d inputs the maker never named**: %s'
                        % (len(unnamed), ', '.join(str(i) for i in unnamed[:12])))
        if hidden:
            bits.append('**%d inputs with no jack on the panel** (no line, no props): %s'
                        % (len(hidden), ', '.join(str(i) for i in hidden[:12])))
        tags = ', '.join(m.get('tags') or []) or 'none'
        lines.append('## %s — %s' % (slug_m, m.get('name') or slug_m))
        lines.append('')
        lines.append("Maker's own summary: %s" % (m.get('description') or '(none)'))
        lines.append('')
        lines.append('Tags: %s' % tags)
        lines.append('')
        lines.append(' · '.join(bits))
        lines.append('')
    lines.insert(4, 'Across the plugin: %d unnamed inputs, %d inputs with no panel jack.'
                 % (unnamed_total, hidden_total))
    lines.insert(5, '')
    return '\n'.join(lines) + '\n'


def skeleton(slug, doc):
    mods = {}
    for m in doc.get('modules', []):
        # A PLACEHOLDER, NOT THE MAKER'S WORDS. Seeding the first line with the browser
        # description was tried and withdrawn: those are written to sell, so 122 style errors
        # landed in the tree at once and "the validator is clean" stopped meaning anything. The
        # maker's summary is in the worksheet instead, where an agent reads it and rewrites it.
        mods[m['slug']] = {'lines': ['TODO: what this module is']}
    return {
        'plugin': slug,
        # A SOURCE IS REQUIRED, AND "there is none" IS A SOURCE. Thirty-three of these plugins
        # publish no code; saying so is the honest entry, and it tells the next agent what it is
        # dealing with before it goes looking.
        'source': (doc.get('sourceUrl') or doc.get('manualUrl')
                   or 'no published source; read from the installed %s build'
                   % doc.get('version', '?')),
        'read': datetime.date.today().isoformat(),
        'partial': True,
        'modules': mods,
    }


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    force = '--force' in sys.argv
    only = args[0] if args else None

    os.makedirs(SHEETS, exist_ok=True)
    made = kept = 0
    for slug, doc in sorted(manifests().items()):
        if only and not slug.lower().startswith(only.lower()):
            continue
        entry = os.path.join(HELP, slug + '.json')
        if os.path.exists(entry):
            # --force REFRESHES A SKELETON, AND NEVER TOUCHES REAL WORK.
            #
            # On 14 September it did: pointed at the whole tree it overwrote 107 finished entries
            # with TODO stubs, and four waves of port fields that had not yet been committed went
            # with them. A flag whose worst case is "lose a day" is the wrong flag.
            #
            # So the file itself decides. An entry nobody has written yet is all placeholders and
            # can be rebuilt freely; one with a single real line is somebody's work and is left
            # alone whatever the flag says.
            written = False
            try:
                with open(entry) as f:
                    old = json.load(f)
                for e in (old.get('modules') or {}).values():
                    lines = e.get('lines') if isinstance(e, dict) else e
                    if any(not str(l).startswith('TODO') for l in (lines or [])):
                        written = True
                        break
            except Exception:
                written = True
            if written or not force:
                kept += 1
                continue
        models = census_of(slug)
        with open(entry, 'w') as f:
            json.dump(skeleton(slug, doc), f, ensure_ascii=False, indent=1)
            f.write('\n')
        with open(os.path.join(SHEETS, slug + '.md'), 'w') as f:
            f.write(worksheet(slug, doc, models))
        made += 1
    print('%d skeletons written, %d existing entries left alone' % (made, kept))
    print('worksheets in research/help/scaffold/')


if __name__ == '__main__':
    main()
