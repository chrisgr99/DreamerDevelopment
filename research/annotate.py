"""Draw each port's index onto the panel it belongs to.

THE WHOLE DIFFICULTY OF READING A PANEL is knowing which hole is port 3, and that is the one
thing a picture does not say. The scan knows where every jack sits; the panel says what each one
is for. Putting the first on top of the second turns a guessing exercise into a reading one.

Positions come from census-positions.json and are in the module's own pixels. A panel SVG is
drawn in user units, which the viewBox maps to those pixels — so the annotation is wrapped in
the inverse of that mapping and lands exactly on the jack.

  python3 annotate.py <model slug> [out.png]
"""
import json, os, re, subprocess, sys

ROOT = os.path.expanduser('~/Library/Application Support/Rack2/plugins-mac-arm64/NYSTHI')
HERE = os.path.dirname(os.path.abspath(__file__))
POS = os.path.expanduser('~/Library/Application Support/Rack2/DreamerDevelopment/census-positions.json')
NAMES = os.path.expanduser('~/Library/Application Support/Rack2/DreamerDevelopment/census.json')


def panel_for(model):
    """The panel file this model draws, from the offline scan's own matching."""
    scan = json.load(open(os.path.join(HERE, 'nysthi-ports.json'), encoding='utf-8'))
    for m in scan['modules']:
        if m['slug'] == model and m.get('panel'):
            return m['panel']
    return None


def annotate(model, out=None, scale=2.0):
    pos = json.load(open(POS, encoding='utf-8'))
    entry = next((m for m in pos['modules'] if m['model'] == model), None)
    if not entry:
        raise SystemExit('no positions for ' + model)
    panel = panel_for(model)
    if not panel:
        raise SystemExit('no panel found for ' + model)

    svg = open(os.path.join(ROOT, 'res', panel), encoding='utf-8', errors='ignore').read()
    head = re.search(r'<svg\b[^>]*>', svg, re.S).group(0)
    # SOME PANELS PUT width AFTER viewBox, or in millimetres, or not on the first line — the
    # match has to be anywhere in the tag and tolerant of a unit suffix.
    wm = re.search(r'\bwidth\s*=\s*"([\d.]+)', head)
    w = float(wm.group(1)) if wm else 0.0
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
    marks.append('</g>')

    stamped = svg.replace('</svg>', '\n'.join(marks) + '\n</svg>')
    tmp = '/tmp/annotate-%s.svg' % model
    open(tmp, 'w', encoding='utf-8').write(stamped)
    out = out or os.path.join('/tmp', 'panel-%s.png' % model)
    subprocess.run(['rsvg-convert', '-w', str(int(w * scale)), tmp, '-o', out], check=True)
    return out, entry


def dossier(model):
    """Everything known about this module in one block, to read beside the picture."""
    pos = json.load(open(POS, encoding='utf-8'))
    nam = json.load(open(NAMES, encoding='utf-8'))
    p = next((m for m in pos['modules'] if m['model'] == model), None)
    n = next((m for m in nam['modules'] if m['model'] == model and m['plugin'] == 'NYSTHI'), None)
    lines = ['== %s  (%s)  %.0f x %.0f px' % (model, p['name'], p['width'], p['height'])]
    ins = sorted([q for q in p['ports'] if q['kind'] == 'input'], key=lambda q: (q['cy'], q['cx']))
    outs = sorted([q for q in p['ports'] if q['kind'] == 'output'], key=lambda q: (q['cy'], q['cx']))
    for kind, group in (('in', ins), ('out', outs)):
        for q in group:
            name = ''
            if n:
                src = n['inputs'] if kind == 'in' else n['outputs']
                hit = next((x for x in src if x['index'] == q['index']), None)
                name = (hit['name'] if hit else '') or ''
            lines.append('  %-3s %2d at %5.0f,%5.0f  %s' % (kind, q['index'], q['cx'], q['cy'], name))
    if n and n['params']:
        lines.append('  params:')
        for q in sorted(p['params'], key=lambda z: (z['cy'], z['cx'])):
            hit = next((x for x in n['params'] if x['index'] == q['index']), None)
            lines.append('    %2d at %5.0f,%5.0f  %s' % (q['index'], q['cx'], q['cy'],
                                                          (hit['name'] if hit else '') or ''))
    return '\n'.join(lines)


if __name__ == '__main__':
    model = sys.argv[1]
    out, _ = annotate(model, sys.argv[2] if len(sys.argv) > 2 else None)
    print(dossier(model))
    print('picture:', out)
