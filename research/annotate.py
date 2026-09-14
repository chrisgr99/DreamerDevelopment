"""Draw each port's index onto the panel it belongs to.

THE WHOLE DIFFICULTY OF READING A PANEL is knowing which hole is port 3, and that is the one
thing a picture does not say. The scan knows where every jack sits; the panel says what each one
is for. Putting the first on top of the second turns a guessing exercise into a reading one.

Positions come from census-positions.json and are in the module's own pixels. A panel SVG is
drawn in user units, which the viewBox maps to those pixels — so the annotation is wrapped in
the inverse of that mapping and lands exactly on the jack.

  python3 annotate.py <Plugin> <model slug> [out.png]
  python3 annotate.py NYSTHI Model277

ANY PLUGIN, NOT JUST THE ONE IT WAS WRITTEN FOR. This began as a NYSTHI tool and stayed one, so
three agents in a row rewrote it from scratch for their own maker before noticing. The plugin is
now an argument, the panel is found from the installed manifest rather than a NYSTHI-only scan
file, and nobody need write it again.
"""
import json, os, re, subprocess, sys

PLUGINS = os.path.expanduser('~/Library/Application Support/Rack2/plugins-mac-arm64')
HERE = os.path.dirname(os.path.abspath(__file__))
POS = os.path.expanduser('~/Library/Application Support/Rack2/DreamerDevelopment/census-positions.json')
NAMES = os.path.expanduser('~/Library/Application Support/Rack2/DreamerDevelopment/census.json')


def panel_for(plugin, model):
    """The panel file this model draws.

    A maker names the file after the model often enough to try that first; where they do not, the
    only general answer is to look at every SVG in res/ and take the one whose name matches most
    closely. That is a guess, and it says so — pass the filename yourself when it guesses wrong."""
    res = os.path.join(PLUGINS, plugin, 'res')
    if not os.path.isdir(res):
        return None
    svgs = []
    for dirpath, _, names in os.walk(res):
        for n in names:
            if n.lower().endswith('.svg'):
                svgs.append(os.path.relpath(os.path.join(dirpath, n), res))
    want = model.lower()
    for cand in svgs:
        if os.path.splitext(os.path.basename(cand))[0].lower() == want:
            return cand
    near = [c for c in svgs if want in os.path.basename(c).lower()]
    if near:
        return sorted(near, key=len)[0]
    return None


def annotate(plugin, model, out=None, scale=2.0, panel=None):
    pos = json.load(open(POS, encoding='utf-8'))
    entry = next((m for m in pos['modules']
                  if m['model'] == model and m.get('plugin') == plugin), None)
    if not entry:
        raise SystemExit('no positions for %s/%s' % (plugin, model))
    panels = panel if isinstance(panel, list) else ([panel] if panel else [])
    panels = panels or ([panel_for(plugin, model)] if panel_for(plugin, model) else [])
    if not panels:
        raise SystemExit('no panel found for %s/%s — name the file yourself:\n'
                         '  annotate.py %s %s TheFile.svg'
                         % (plugin, model, plugin, model))

    svg = open(os.path.join(PLUGINS, plugin, 'res', panels[0]),
               encoding='utf-8', errors='ignore').read()
    head = re.search(r'<svg\b[^>]*>', svg, re.S).group(0)

    # SOME MAKERS SPLIT A PANEL ACROSS TWO FILES — Leviathan draws <name>.panel.svg and then
    # <name>.labels.svg over it, so either one alone is unreadable: the first has no lettering and
    # the second has no artwork. Name them both and they are drawn in the order given, which is
    # what the module itself does.
    overlays = []
    for extra in panels[1:]:
        more = open(os.path.join(PLUGINS, plugin, 'res', extra),
                    encoding='utf-8', errors='ignore').read()
        body = re.sub(r'^.*?<svg\b[^>]*>', '', more, count=1, flags=re.S)
        overlays.append(re.sub(r'</svg>\s*$', '', body, count=1, flags=re.S))

    # THE PANEL'S OWN width ATTRIBUTE IS NOT A PIXEL COUNT. Inkscape writes it in millimetres on
    # most hand-drawn panels, and reading 128.5mm as 128.5 pixels put every marker about three
    # times too far out and rendered the picture a third of its size — two agents hit it on the
    # same afternoon. The scan already knows the module's width in the pixels the positions are
    # measured in, so that is the denominator, whatever unit the file declares.
    w = float(entry.get('width') or 0.0)
    if not w:
        wm = re.search(r'\bwidth\s*=\s*"([\d.]+)\s*(mm|cm|in|pt|pc)?', head)
        if wm:
            # Rack draws panels at 75 DPI; that is what turns a physical unit into its pixels.
            per = {None: 1.0, '': 1.0, 'mm': 75 / 25.4, 'cm': 750 / 25.4, 'in': 75.0,
                   'pt': 75 / 72.0, 'pc': 75 / 6.0}[wm.group(2)]
            w = float(wm.group(1)) * per
    vb = re.search(r'viewBox="([^"]+)"', head)
    sx = 1.0
    ox = oy = 0.0
    if vb:
        parts = [float(v) for v in vb.group(1).split()]
        if not w:
            w = parts[2]
        sx = parts[2] / w
        ox, oy = parts[0], parts[1]
    if not w:
        w = 300.0

    marks = ['<g transform="translate(%g,%g) scale(%g)" font-family="sans-serif">' % (ox, oy, sx)]
    for p in entry['ports']:
        inp = p['kind'] == 'input'
        # Inputs ringed, outputs filled: the same distinction Clarity draws, so the picture
        # reads the way the rack does.
        marks.append(
            '<circle cx="%g" cy="%g" r="11" fill="%s" fill-opacity="0.85" stroke="#000" '
            'stroke-width="1"/>' % (p['cx'], p['cy'], '#ffd24a' if inp else '#4ad2ff'))
        marks.append(
            '<text x="%g" y="%g" font-size="11" text-anchor="middle" fill="#000">%d</text>'
            % (p['cx'], p['cy'] + 4, p['index']))
    # PARAMETERS WERE COMPUTED AND NEVER DRAWN. The dossier listed every knob's index and position
    # while the picture showed only jacks, so working out which knob was param 7 meant counting
    # down a column by hand — and an agent rewrote the tool locally rather than do that. Knobs are
    # the controls most often out of enum order, so they are the ones that most need the picture.
    for q in entry.get('params', []):
        cx = q.get('cx', q.get('x'))
        cy = q.get('cy', q.get('y'))
        if cx is None or cy is None:
            continue
        marks.append(
            '<circle cx="%g" cy="%g" r="9" fill="#e8e8e8" fill-opacity="0.9" stroke="#000" '
            'stroke-width="1"/>' % (cx, cy))
        marks.append(
            '<text x="%g" y="%g" font-size="10" text-anchor="middle" fill="#000">%d</text>'
            % (cx, cy + 3.5, q['index']))
    marks.append('</g>')

    stamped = svg.replace('</svg>', '\n'.join(overlays + marks) + '\n</svg>')
    tmp = '/tmp/annotate-%s.svg' % model
    open(tmp, 'w', encoding='utf-8').write(stamped)
    out = out or os.path.join('/tmp', 'panel-%s.png' % model)
    subprocess.run(['rsvg-convert', '-w', str(int(w * scale)), tmp, '-o', out], check=True)
    return out, entry


def dossier(plugin, model):
    """Everything known about this module in one block, to read beside the picture."""
    pos = json.load(open(POS, encoding='utf-8'))
    nam = json.load(open(NAMES, encoding='utf-8'))
    p = next((m for m in pos['modules']
              if m['model'] == model and m.get('plugin') == plugin), None)
    n = next((m for m in nam['modules']
              if m['model'] == model and m['plugin'] == plugin), None)
    lines = ['== %s  (%s)  %.0f x %.0f px' % (model, p['name'], p['width'], p['height'])]
    # EARLY RECORDS CARRY ONLY A CORNER. The positions file holds two shapes; sort and print
    # whichever this one has, rather than dying on the older ones.
    def cx(q):
        return q.get('cx', q.get('x', 0.0))

    def cy(q):
        return q.get('cy', q.get('y', 0.0))
    ins = sorted([q for q in p['ports'] if q['kind'] == 'input'], key=lambda q: (cy(q), cx(q)))
    outs = sorted([q for q in p['ports'] if q['kind'] == 'output'], key=lambda q: (cy(q), cx(q)))
    for kind, group in (('in', ins), ('out', outs)):
        for q in group:
            name = ''
            if n:
                src = n['inputs'] if kind == 'in' else n['outputs']
                hit = next((x for x in src if x['index'] == q['index']), None)
                name = (hit['name'] if hit else '') or ''
            lines.append('  %-3s %2d at %5.0f,%5.0f  %s' % (kind, q['index'], cx(q), cy(q), name))
    if n and n['params']:
        lines.append('  params:')
        for q in sorted(p['params'], key=lambda z: (cy(z), cx(z))):
            hit = next((x for x in n['params'] if x['index'] == q['index']), None)
            lines.append('    %2d at %5.0f,%5.0f  %s' % (q['index'], cx(q), cy(q),
                                                          (hit['name'] if hit else '') or ''))
    return '\n'.join(lines)


if __name__ == '__main__':
    if len(sys.argv) < 3:
        raise SystemExit('usage: annotate.py <Plugin> <model slug> [panel.svg] [out.png]')
    plugin, model = sys.argv[1], sys.argv[2]
    # THE GUESS FAILS WHENEVER A MAKER NAMES THE PANEL AFTER SOMETHING OTHER THAN THE MODEL —
    # InputGenie draws CVGenie.svg, xen-qnt draws XenQnt.svg. The old error told the reader to
    # pass the filename and gave them nowhere to pass it: the third argument was the output path.
    # Anything ending in .svg is now taken as the panel, so the advice and the program agree.
    panels = []
    out = None
    for a in sys.argv[3:]:
        if a.lower().endswith('.svg'):
            panels.append(a)
        else:
            out = a
    out, _ = annotate(plugin, model, out, panel=panels or None)
    print(dossier(plugin, model))
    print('picture:', out)
