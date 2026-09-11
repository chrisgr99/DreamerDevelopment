"""Compile what can be learned about NYSTHI's modules WITHOUT running Rack.

Three sources, none of them the ports themselves — because port names live in PortInfo, which
only exists once a module is instantiated inside Rack:

  1. plugin.json      — slug, name, description, tags.
  2. The panel SVG    — every text label, with its position resolved through the transform
                        chain and converted to panel pixels (Rack draws at 75 DPI).
  3. plugin.dylib     — the string literals belonging to each module. They are laid out per
                        translation unit and each module's block ends with its own panel file
                        name, which is what lets a block be attributed to a module at all.
  4. CHANGELOG.md     — NYSTHI's own documentation, written per module.

The strings are candidates for parameter and port names; which of the two they are cannot be
told from here, and that is the question the in-Rack census answers later.
"""
import json, os, re, subprocess, sys, collections

PLUGIN = sys.argv[1] if len(sys.argv) > 1 else 'NYSTHI'
ROOT = os.path.expanduser(
    '~/Library/Application Support/Rack2/plugins-mac-arm64/' + PLUGIN)
OUT = sys.argv[2] if len(sys.argv) > 2 else '.'
LIB = ('plugin.dylib' if os.path.exists(os.path.join(ROOT, 'plugin.dylib'))
       else 'plugin.so')

# ---- panel text ----------------------------------------------------------------------------

NUM = r'[-+]?[\d.eE+-]+'


def parse_transform(s):
    """The transform chain as a single 2x3 matrix, applied left to right."""
    m = [1., 0., 0., 1., 0., 0.]
    for name, args in re.findall(r'(\w+)\s*\(([^)]*)\)', s or ''):
        v = [float(x) for x in re.findall(NUM, args)]
        if name == 'translate':
            t = [1, 0, 0, 1, v[0], v[1] if len(v) > 1 else 0]
        elif name == 'scale':
            t = [v[0], 0, 0, v[1] if len(v) > 1 else v[0], 0, 0]
        elif name == 'matrix' and len(v) >= 6:
            t = v[:6]
        elif name == 'rotate':
            import math
            a = math.radians(v[0])
            t = [math.cos(a), math.sin(a), -math.sin(a), math.cos(a), 0, 0]
        else:
            continue
        m = [m[0]*t[0] + m[2]*t[1], m[1]*t[0] + m[3]*t[1],
             m[0]*t[2] + m[2]*t[3], m[1]*t[2] + m[3]*t[3],
             m[0]*t[4] + m[2]*t[5] + m[4], m[1]*t[4] + m[3]*t[5] + m[5]]
    return m


def apply(m, x, y):
    return (m[0]*x + m[2]*y + m[4], m[1]*x + m[3]*y + m[5])


def panel_text(path):
    """Every label on the panel, positioned in panel pixels."""
    import xml.etree.ElementTree as ET
    try:
        tree = ET.parse(path)
    except Exception as e:
        return None, [], str(e)
    root = tree.getroot()
    ns = '{http://www.w3.org/2000/svg}'
    w = root.get('width', '0').replace('px', '')
    h = root.get('height', '0').replace('px', '')
    vb = (root.get('viewBox') or '').split()
    try:
        w, h = float(w), float(h)
    except ValueError:
        w = h = 0.
    # SVG user units to panel pixels.
    sx = sy = 1.
    if len(vb) == 4 and float(vb[2]) and float(vb[3]):
        sx, sy = w / float(vb[2]), h / float(vb[3])
        ox, oy = float(vb[0]), float(vb[1])
    else:
        ox = oy = 0.

    labels = []

    def walk(node, m):
        m = mul(m, parse_transform(node.get('transform')))
        if node.tag == ns + 'text':
            # A text's own x/y, or the first tspan's.
            x = node.get('x')
            y = node.get('y')
            parts = []
            for sub in node.iter():
                if sub.tag == ns + 'tspan':
                    if x is None:
                        x, y = sub.get('x'), sub.get('y')
                if sub.text and sub.text.strip():
                    parts.append(sub.text.strip())
            txt = ' '.join(parts).strip()
            # NO x/y IS A POSITION TOO: Bogaudio places its text entirely by transform, so an
            # absent coordinate means the origin of whatever frame the element sits in.
            if txt and x is None:
                x = y = '0'
            if txt and x is not None and y is not None:
                try:
                    ux, uy = apply(m, float(x), float(y))
                    labels.append({'text': txt,
                                   'x': round((ux - ox) * sx, 1),
                                   'y': round((uy - oy) * sy, 1)})
                except ValueError:
                    pass
            return
        for child in node:
            walk(child, m)

    def mul(a, b):
        return [a[0]*b[0] + a[2]*b[1], a[1]*b[0] + a[3]*b[1],
                a[0]*b[2] + a[2]*b[3], a[1]*b[2] + a[3]*b[3],
                a[0]*b[4] + a[2]*b[5] + a[4], a[1]*b[4] + a[3]*b[5] + a[5]]

    walk(root, [1., 0., 0., 1., 0., 0.])
    labels.sort(key=lambda l: (l['y'], l['x']))
    return {'width': w, 'height': h, 'hp': round(w / 15.) if w else 0}, labels, None


# ---- strings from the binary ---------------------------------------------------------------

PANELS = set()


def string_blocks():
    """Each panel file name, and the plain-language strings that precede it.

    Strings from one translation unit sit together, and a module's block ends with the name of
    the file its panel is drawn from. Anything mangled, path-like or too short to be language
    is dropped, so what is left is names a person wrote.
    """
    raw = subprocess.run(['strings', '-n', '3', os.path.join(ROOT, LIB)],
                         capture_output=True, text=True).stdout.split('\n')
    blocks = {}
    pending = []
    for s in raw:
        s = s.strip()
        if not s:
            continue
        if s.lower().endswith('.svg'):
            # A PANEL, NOT A COMPONENT. Some plugins name their panel bare ("Nudger.svg"),
            # others by the path they load it from ("res/VCO.svg"). Either counts, as long as
            # a file of that name really is in res and it is not one of the shared control
            # graphics, which appear between modules and would otherwise split every block.
            base = s.rsplit('/', 1)[-1]
            if base.lower() in PANELS and 'componentlibrary' not in s.lower():
                blocks.setdefault(base, []).extend(pending)
                pending = []
            continue
        if re.match(r'^[0-9]*[A-Za-z_]+[A-Za-z0-9_]*E?$', s) and (
                re.match(r'^\d', s) or s.startswith(('_Z', 'ZN', 'N4', 'm_', 'fv_'))):
            continue                               # mangled or member name
        if len(s) < 3 or len(s) > 60:
            continue
        if not re.search(r'[A-Za-z]', s):
            continue
        if re.search(r'[{}<>@$^`|\\]', s):
            continue
        if s.count(' ') == 0 and (s.islower() or '_' in s) and not s.isalpha():
            continue
        pending.append(s)
    return blocks


# ---- NYSTHI's own documentation -------------------------------------------------------------

def changelog_sections():
    """Every '## HEADING' section, gathered by heading."""
    out = collections.defaultdict(list)
    path = os.path.join(ROOT, 'CHANGELOG.md')
    if not os.path.exists(path):
        return out
    head = None
    for line in open(path, encoding='utf-8', errors='ignore'):
        if line.startswith('## '):
            head = line[3:].strip()
            continue
        if head and line.strip():
            if line.startswith('#'):
                head = None
                continue
            out[head].append(line.rstrip())
    return out


# ---- putting it together ---------------------------------------------------------------------

def main():
    manifest = json.load(open(os.path.join(ROOT, 'plugin.json'), encoding='utf-8'))
    svgs = {f.lower(): f for f in os.listdir(os.path.join(ROOT, 'res')) if f.lower().endswith('.svg')}
    PANELS.update(svgs.keys())
    blocks = string_blocks()
    docs = changelog_sections()

    modules = []
    for m in manifest['modules']:
        slug = m['slug']
        name = m.get('name', slug)
        entry = {'slug': slug, 'name': name,
                 'description': m.get('description', ''), 'tags': m.get('tags', [])}

        # The panel: by exact file name first, then by the slug appearing in one.
        cand = None
        # A PANEL IS USUALLY NAMED AFTER THE SLUG, but not always: Bogaudio's slugs carry the
        # plugin's own name as a prefix its files do not ("Bogaudio-FMOp" is "FMOp.svg").
        bare = slug.split('-', 1)[1] if slug.lower().startswith(PLUGIN.lower() + '-') else slug
        for key in (slug + '.svg', name.lower() + '.svg', slug.lower() + '.svg',
                    bare + '.svg', bare.lower() + '.svg'):
            if key.lower() in svgs:
                cand = svgs[key.lower()]
                break
        if not cand:
            hits = [f for k, f in svgs.items()
                    if slug.lower() in k.replace('_', '').replace('-', '')]
            if len(hits) == 1:
                cand = hits[0]
        entry['panel'] = cand
        if cand:
            size, labels, err = panel_text(os.path.join(ROOT, 'res', cand))
            entry['panelSize'] = size
            # OFF THE PANEL IS NOT A POSITION. Inkscape files carry leftovers outside the
            # canvas, and a label at 1019 px on a 900 px panel is one of them. Kept, but
            # separated, so proximity matching is never done against something invisible.
            # POSITIONS ARE NOT TRUSTED. Every one of these files is an Inkscape document with
            # nested transforms, and spot checks against the rendered panel show the arithmetic
            # landing in the right order but the wrong place. The LABELS are sound; the
            # coordinates are a rough ordering, and the rendered PNG is what to look at when a
            # position matters.
            entry['panelText'] = labels
            entry['panelError'] = err
            entry['codeStrings'] = blocks.get(cand, [])
        else:
            entry['panelText'] = []
            entry['codeStrings'] = []

        # NYSTHI's documentation, matched on the module's name appearing in a heading.
        key = re.sub(r'[^A-Z0-9]', '', name.upper())
        entry['docs'] = []
        for head, lines in docs.items():
            hk = re.sub(r'[^A-Z0-9]', '', head.upper())
            if key and (key == hk or (len(key) > 4 and key in hk)):
                entry['docs'].extend(lines)
        modules.append(entry)

    json.dump({'plugin': PLUGIN, 'version': manifest['version'], 'modules': modules},
              open(os.path.join(OUT, PLUGIN.lower() + '-ports.json'), 'w', encoding='utf-8'),
              indent=1, ensure_ascii=False)

    # A readable companion.
    with open(os.path.join(OUT, PLUGIN.lower() + '-ports.md'), 'w', encoding='utf-8') as f:
        f.write('# %s %s — what can be read without running Rack\n\n'
                % (PLUGIN, manifest['version']))
        f.write('%d modules. For each: its manifest entry, the labels drawn on its panel with '
                'their positions in panel pixels, the strings its code carries (candidate '
                'parameter and port names), and whatever NYSTHI\'s own changelog says about '
                'it.\n\n' % len(modules))
        for e in modules:
            f.write('\n## %s\n\n' % e['name'])
            f.write('- slug `%s`' % e['slug'])
            if e.get('panelSize'):
                f.write(', %d HP' % e['panelSize']['hp'])
            f.write(', tags: %s\n' % (', '.join(e['tags']) or 'none'))
            if e['description']:
                f.write('- %s\n' % e['description'])
            f.write('- panel: %s\n' % (e['panel'] or 'NOT FOUND'))
            if e['panelText']:
                f.write('\nLabels on the panel, roughly in reading order. The coordinates are '
                        'indicative only — see the rendered panel for anything positional:\n\n')
                for l in e['panelText']:
                    f.write('  - `%s` at %.0f, %.0f\n' % (l['text'], l['x'], l['y']))
            else:
                f.write('\nNo live text on the panel — outlined, or unlabelled. '
                        'Read `panels/%s.png` to see it.\n'
                        % (e['panel'][:-4] if e['panel'] else '?'))

            if e['codeStrings']:
                f.write('\nStrings in its code:\n\n')
                for s in e['codeStrings']:
                    f.write('  - `%s`\n' % s)
            if e['docs']:
                f.write('\nFrom the changelog:\n\n')
                for d in e['docs'][:40]:
                    f.write('  %s\n' % d)
        f.write('\n')

    # A summary worth having in front of you.
    named = sum(1 for e in modules if e['panelText'])
    strings = sum(1 for e in modules if e['codeStrings'])
    documented = sum(1 for e in modules if e['docs'])
    nopanel = [e['slug'] for e in modules if not e['panel']]
    print('modules: %d' % len(modules))
    print('panel found: %d (missing: %s)' % (len(modules) - len(nopanel),
                                             ', '.join(nopanel) or 'none'))
    print('with live panel text: %d' % named)
    print('with code strings: %d' % strings)
    print('mentioned in the changelog: %d' % documented)


main()
