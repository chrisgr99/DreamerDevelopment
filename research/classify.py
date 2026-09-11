"""Classify VCV's own ports from the census, and say which ones cannot be decided by name.

THREE TIERS, and they are tried in this order.

  1. The port's own name, by word. These are the rules worth having, because they carry to the
     rest of the library: "gate", "trigger", "clock" mean the same thing in everybody's plugin.
  2. The module, where a name is generic. "Cell 3" is a control voltage on one module and a gate
     on the next, and only the module can say which.
  3. Nothing — which is the honest answer for a jack that takes whatever you patch into it. A
     polyphonic merge, a mult, an attenuverter and a scope are all of them, and no rule should
     pretend otherwise.
"""
import json, re, collections, os

CENSUS = os.path.expanduser('~/Library/Application Support/Rack2/DreamerDevelopment/census.json')

# ---- tier one: the word rules ----------------------------------------------------------------
# (pattern, family). Order matters: the first match wins, so the specific come first.
WORDS = [
    (r'1 ?v/?oct|v/?oct|volt per octave|\bpitch\b|\bnote\b|\broot\b', 'PITCH'),
    (r'\btrigger\b|\btrig\b|\bgate\b|\bclock\b|\breset\b|\bsync\b|\bstrobe\b|\brun\b|\bstop\b'
     r'|\bstart\b|\bcontinue\b|\beoc\b|\beof\b|\bmute\b|\bflip\b|\bflop\b|\bhold\b|\bpush\b'
     r'|\bnot [ab]\b|\bx?nor\b|\bx?or\b|\bn?and\b|a[<>]b|retrigger|\bstep \d', 'GATE'),
    (r'\baudio\b|\bleft\b|\bright\b|\bmono\b|\bmix\b|\bsend\b|\breturn\b|\bsignal\b|\bwet\b'
     r'|\bcarrier\b|\bmodulator\b|\bsum\b|\bsine\b|\bsaw(tooth)?\b|\bsquare\b|\btriangle\b'
     r'|\bnoise\b|\bdevice (in|out)put\b|\bfilter\b|\bmid\b|\bside\b|wavetable', 'AUDIO'),
    (r'\bcv\b|modulation|\bmod\b|\bamount\b|\bdepth\b|\blevel\b|\bvelocity\b|\bpressure\b'
     r'|\baftertouch\b|\bvolume\b|\bpan\b|\bfrequency\b|\bfreq\b|\bcutoff\b|\bresonance\b'
     r'|\bshape\b|\bwidth\b|\battack\b|\bdecay\b|\bsustain\b|\brelease\b|\btune\b|\bfm\b|\bam\b'
     r'|\bfold\b|\bdrive\b|\bfeedback\b|\brate\b|\bspeed\b|\bcolou?r\b|\btone\b|\benvelope\b'
     r'|\bslew\b|\bglide\b|\bsweep\b|\bsnap\b|\bspace\b|\bmetal\b|\baccent\b|\bposition\b'
     r'|\bdiffusion\b|\breflectivity\b|\bmorph\b|\bthreshold\b|\bgain\b|\btempo\b|\bsteps\b'
     r'|\bdelay\b|\bhigh-?pass\b|\blow-?pass\b|\btime\b|\bcrossfade\b|\bspread\b|\bstepped\b'
     r'|\blinear\b|\bsmooth\b|\bexponential\b|\bexternal\b|\bvoltage\b|\bsample ?& ?hold\b'
     r'|\btrack ?& ?hold\b|\bhold ?& ?track\b|\baddress\b|\bmaximum\b|\bminimum\b|\bclip\b'
     r'|\blimit\b', 'CV'),
]

# ---- tier two: the module rules ---------------------------------------------------------------
# plugin/model -> (family for inputs, family for outputs). None means "leave it to tier three".
MODULES = {
    'Core/CV-CC':                 ('CV', 'CV'),
    'Core/MIDICCToCVInterface':   ('CV', 'CV'),
    'VCV-Host/Host-CC':           ('CV', 'CV'),
    'Core/CV-Gate':               ('GATE', 'GATE'),
    'VCV-Host/Host-Gate':         ('GATE', 'GATE'),
    'Core/CV-MIDI':               ('CV', None),
    'Fundamental/Mixer':          ('AUDIO', 'AUDIO'),
    'Fundamental/VCMixer':        ('AUDIO', 'AUDIO'),
    'Fundamental/VCA':            ('AUDIO', 'AUDIO'),
    'Fundamental/Unity':          ('AUDIO', 'AUDIO'),
    'Fundamental/VCA-1':          ('AUDIO', 'AUDIO'),
    'Fundamental/MidSide':        ('AUDIO', 'AUDIO'),
    'Fundamental/Logic':          ('GATE', 'GATE'),
    'Fundamental/Gates':          ('GATE', 'GATE'),
    'Fundamental/Push':           ('GATE', 'GATE'),
    'Fundamental/RandomValues':   (None, 'CV'),
    'Fundamental/Random':         ('CV', 'CV'),
    'Fundamental/SHASR':          ('CV', 'CV'),
    'Fundamental/8vert':          ('CV', 'CV'),
    'Fundamental/Process':        ('CV', 'CV'),
    'VCV-Chords/Chords':          (None, 'PITCH'),
    'VCV-SoundStage/SoundStage':  ('AUDIO', 'AUDIO'),
    'VCV-Drums/Kick':             ('CV', 'AUDIO'),
    'VCV-Drums/Snare':            ('CV', 'AUDIO'),
    'VCV-Drums/Tom':              ('CV', 'AUDIO'),
    'VCV-Drums/Rim':              ('CV', 'AUDIO'),
    'VCV-Drums/Clap':             ('CV', 'AUDIO'),
    'VCV-Drums/ClosedHat':        ('CV', 'AUDIO'),
    'VCV-Drums/OpenHat':          ('CV', 'AUDIO'),
    'VCV-Drums/Crash':            ('CV', 'AUDIO'),
    'VCV-Drums/Ride':             ('CV', 'AUDIO'),
    'VCV-Drums/DrumMachine':      ('CV', 'AUDIO'),
    'Core/AudioInterface':        ('AUDIO', 'AUDIO'),
    'Core/AudioInterface2':       ('AUDIO', 'AUDIO'),
    'Core/AudioInterface16':      ('AUDIO', 'AUDIO'),
    'VCV-Host/Host':              ('AUDIO', 'AUDIO'),
    'VCV-Host/Host-FX':           ('AUDIO', 'AUDIO'),
    'Fundamental/VCF':            (None, 'AUDIO'),
    'Fundamental/Delay':          (None, 'AUDIO'),
    'VCV-Pro/Compressor':         (None, 'CV'),
    'VCV-Pro/Convolver':          ('CV', None),
    'VCV-Pro/Reverb':             ('CV', None),
    'VCV-Pro/Chorus':             ('CV', None),
    'VCV-Pro/Flanger':            ('CV', None),
}

# ---- tier three: the jacks that genuinely have no type ----------------------------------------
# Utilities that pass whatever they are given. Naming them here is how we say "unknown on
# purpose" rather than "not thought about".
ANY = {
    'Fundamental/Merge', 'Fundamental/Split', 'Fundamental/Sum', 'Fundamental/Viz',
    'Fundamental/Mult', 'Fundamental/Mutes', 'Fundamental/Scope', 'Fundamental/Fade',
    'Fundamental/SequentialSwitch1', 'Fundamental/SequentialSwitch2', 'Fundamental/Compare',
    'VCV-Host/Host-XL', 'VCV-Host/Host-CV',
}


def classify(plugin, model, kind, name, description):
    text = (name + ' ' + description).lower()
    for pattern, family in WORDS:
        if re.search(pattern, text):
            return family, 'name'
    key = plugin + '/' + model
    if key in MODULES:
        family = MODULES[key][0 if kind == 'input' else 1]
        if family:
            return family, 'module'
    if key in ANY:
        return None, 'takes anything'
    return None, 'unknown'


def main():
    census = json.load(open(CENSUS))
    rows = []
    for m in census['modules']:
        for kind, key in (('input', 'inputs'), ('output', 'outputs')):
            for p in m[key]:
                family, how = classify(m['plugin'], m['model'], kind, p['name'],
                                       p['description'])
                rows.append({'plugin': m['plugin'], 'model': m['model'], 'name': m['name'],
                             'kind': kind, 'index': p['index'], 'port': p['name'],
                             'description': p['description'], 'family': family, 'how': how})

    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'vcv-ports.md')
    out = '/Users/chrisgr/ProgrammingProjects/DreamerDevelopment/research/vcv-ports.md'
    by_module = collections.OrderedDict()
    for r in rows:
        by_module.setdefault((r['plugin'], r['model'], r['name']), []).append(r)

    counts = collections.Counter((r['family'] or r['how']) for r in rows)
    hows = collections.Counter(r['how'] for r in rows)

    with open(out, 'w') as f:
        f.write('# VCV\'s own ports, classified\n\n')
        f.write('%d ports across %d modules, from the census taken in Rack. '
                'Every port here is named by its maker: 839 of 840 have a name, which is why '
                'this family is worth doing first.\n\n' % (len(rows), len(by_module)))
        f.write('| decided by | ports |\n| --- | --- |\n')
        for how, n in hows.most_common():
            f.write('| %s | %d |\n' % (how, n))
        f.write('\n| family | ports |\n| --- | --- |\n')
        for fam, n in counts.most_common():
            f.write('| %s | %d |\n' % (fam, n))

        f.write('\n## Ports nothing decides\n\n')
        f.write('These are the ones to settle by testing. A jack marked "takes anything" is not '
                'a gap: a merge, a mult, an attenuverter and a scope carry whatever you patch '
                'into them, and white is the right colour for those. The ones marked unknown '
                'are the real list.\n\n')
        unknown = [r for r in rows if r['how'] == 'unknown']
        if unknown:
            f.write('| module | dir | # | port | description |\n| --- | --- | --- | --- | --- |\n')
            for r in unknown:
                f.write('| %s/%s | %s | %d | %s | %s |\n' % (r['plugin'], r['model'], r['kind'],
                        r['index'], r['port'] or '(no name)', r['description']))
        else:
            f.write('None.\n')

        f.write('\n## Every module\n')
        for (plugin, model, name), ports in by_module.items():
            f.write('\n### %s — %s\n\n' % (name, plugin + '/' + model))
            f.write('| dir | # | port | family | decided by |\n| --- | --- | --- | --- | --- |\n')
            for r in ports:
                f.write('| %s | %d | %s | %s | %s |\n' % (r['kind'], r['index'],
                        r['port'] or '(no name)', r['family'] or '—', r['how']))

    print('ports: %d across %d modules' % (len(rows), len(by_module)))
    print('decided by:', dict(hows))
    print('families:', dict(counts))
    print('\nunknown:')
    for r in rows:
        if r['how'] == 'unknown':
            print('  %s/%s %s #%d %r' % (r['plugin'], r['model'], r['kind'], r['index'],
                                         r['port']))


main()
