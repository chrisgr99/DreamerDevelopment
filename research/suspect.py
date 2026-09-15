"""WHAT TO CHECK FIRST, WHEN CHECKING EVERYTHING IS TOO EXPENSIVE.

A second pass over the library caught real errors that the first pass could not: a menu declared
dead that was not, two port tags the wrong way round, claims carried into a brief as fact and
repeated. None of them would ever have been found by the agent that wrote them, because it had
already convinced itself. A fresh reader is the only instrument that finds those.

But a fresh reader over 4,000 modules is the whole job again. So this ranks the claims by how
likely they are to be wrong, and a verifier starts at the top. Every test below is a way a WRONG
entry looks different from a right one — none of them proves anything, and a hit is a place to
look rather than a fault.

    python3 research/suspect.py              # the ranked list
    python3 research/suspect.py Venom        # one plugin
    python3 research/suspect.py --limit 40   # how many of each kind to print
"""
import collections
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
HELP = os.path.join(HERE, 'help')
PLUGINS = os.path.expanduser('~/Library/Application Support/Rack2/plugins-mac-arm64')

# A `why` that names a mechanism is one somebody read. A `why` that names only a file is one
# somebody may have inferred from the shape of the code around it.
# Naming the API calls was tried and was hopeless: the first run flagged 1,652 citations that
# plainly named one, because the list did not know getNormalPolyVoltageSimd or isMonophonic. What
# separates a citation from a place is not a vocabulary — it is whether it says what the code
# DOES. So: a verb, or a call, or an address.
MECHANISM = re.compile(
    r'\b(reads?|loops?|clamps?|scales?|sums?|compares?|takes?|sets?|multipli|divid|adds?'
    r'|writes?|walks?|tests?|checks?|normal|maps?|rounds?|floors?|wraps?|counts?'
    # BOTH HALVES OF THE VERIFICATION PASS NAMED THE SAME LIMIT: whole plugins were flagged
    # while naming their mechanism precisely, because the list did not know the ordinary verbs
    # people reach for. Three quarters of 1,141 flags were false, and the verifiers' own
    # rewrites went on tripping it by accident of vocabulary. The real test is "the citation
    # names a place the mechanism is not", which is not the same as "the sentence lacks a verb".
    r'|uses?|passes|runs?|falls? back|assigns?|rescales?|treats?|returns?|indexes'
    r'|selects?|holds?|feeds?|drives?|latches|copies|averages?|inverts?|offsets?'
    r'|quantis|quantiz|truncat|subtracts?|rectif|attenuat|crossfade|accumulat)'
    r'|[A-Za-z_]\w*\(|\bis[A-Z]\w+|\bget[A-Z]\w+|\bset[A-Z]\w+|0x[0-9a-f]{4,}',
    re.I)
# Words in a port's own line that say what kind of signal it is, to be set against the fields.
# "clock" was in this list and should not have been: a clock RATE input is a continuous CV and
# the word appears in its line. Only the words that describe a signal which is a state.
SAYS_GATE = re.compile(r'rising edge|falling edge|\btrigger|\bgate\b|\bpulse|goes high', re.I)
# THE EDGE BELONGS TO A DIFFERENT JACK. "sampled at the instant the trigger arrives", "written
# into the step when a trigger reaches WRITE", "taken when the gate beside it is high" — this
# jack carries the value being sampled, and the gate word is describing its neighbour. Fifty-eight
# of one verifier's false alarms were this one shape, and all thirty-three ports of Venom's merge
# modules came in on it.
OTHERS_GATE = re.compile(
    r'\b(when|whenever|on each|at the (moment|instant)|sampled|captured|while)\b[^.]{0,40}'
    r'\b(trigger|gate|pulse|edge)\b'
    r'|\b(its|whose|the) (gate|trigger) (beside|it|is|goes|reaches)', re.I)
# "whatever level this reads" is how a logic buffer's line describes a HIGH or a LOW, and it
# tripped this rule twenty-two times in one plugin. A level that is set, scaled or attenuated is
# a value; a level that is merely read or copied may be a state.
#
# A VERIFIER READ ALL 461 OF THESE AND FOUND FOUR. Four hundred and fifty-seven false alarms is
# not a heuristic, it is a way of spending a day, so the noise it named is filtered below:
# `per volt` and `tempo` were dropped for the same reason `clock` was — they name a scaling or
# the knob beside the jack, and a scaling is as natural on a stepped control as on a continuous
# one. `level` now has to be the signal rather than the thing being chosen.
SAYS_LEVEL = re.compile(r'\bamount\b|\bdepth\b'
                        r'|\b(?<!whatever )level\b(?!\s+(this reads|read into|that|for that|is used))'
                        r'|scales the|scales it|attenuat|\bmix\b'
                        r'|covering|doubling|\bchance\b'
                        # A GATE WORD INSIDE A QUANTITY'S NAME. "pulse width" is a level and never
                        # an edge — sixty false alarms on its own. "Gate length", "gate spread",
                        # "gate threshold" are a time or a level *about* a gate. A low-pass gate is
                        # a VCA. And a count of pulses is a number.
                        r'|pulse[- ]width|low.?pass gate'
                        r'|gate (length|width|spread|hold|attack|release|threshold|time|level)'
                        r'|(number of|how many) pulses|pulses per', re.I)
# THE VALUE IS ALREADY SAID TO BE DISCRETE, so nothing is in doubt. Over two hundred of those
# false alarms were lines reading "one octave per volt, rounded to the nearest whole octave", or
# citations naming `snapEnabled` and `static_cast<int>` — the very evidence the test wants,
# sitting in the record it was about to flag.
DISCRETE = re.compile(r'\bround|\bfloor|truncat|quantis|quantiz|\bindex|nearest|whole\b'
                      r'|integer|\(int\)|\bsnap', re.I)


def tags():
    out = {}
    for name in sorted(os.listdir(PLUGINS)):
        path = os.path.join(PLUGINS, name, 'plugin.json')
        if not os.path.exists(path):
            continue
        with open(path) as f:
            doc = json.load(f)
        for m in doc.get('modules', []):
            out[(doc.get('slug'), m.get('slug'))] = [t.lower() for t in (m.get('tags') or [])]
    return out


def main():
    # A FLAG'S VALUE IS NOT A PLUGIN NAME. Taking every non-flag word as the filter turned
    # `--limit 4` into "only plugins starting with 4", which matched nothing and reported a clean
    # library — the most dangerous possible failure for a tool whose whole job is finding faults.
    argv = sys.argv[1:]
    limit = 25
    only = None
    skip = False
    for i, a in enumerate(argv):
        if skip:
            skip = False
            continue
        if a == '--limit':
            limit = int(argv[i + 1])
            skip = True
        elif not a.startswith('--'):
            only = a

    tagged = tags()
    # HOW MUCH THIS MAKER'S SILENCE IS WORTH: the fraction of their own modules they tag
    # Polyphonic. Computed from the installed manifests, which is where the tags live.
    counts = collections.Counter()
    polys = collections.Counter()
    for (plug, _model), t in tagged.items():
        counts[plug] += 1
        if 'polyphonic' in t or 'poly' in t:
            polys[plug] += 1
    tagrate = {p: polys[p] / counts[p] for p in counts if counts[p]}
    found = collections.defaultdict(list)

    for name in sorted(os.listdir(HELP)):
        if not name.endswith('.json'):
            continue
        # NOT EVERY JSON FILE HERE IS A PLUGIN — UNINSTALLED.json holds entries for models their
        # plugin no longer registers, and nothing in it can be shown, so nothing in it can be wrong.
        if name == 'UNINSTALLED.json':
            continue
        with open(os.path.join(HELP, name)) as f:
            doc = json.load(f)
        plugin = doc['plugin']
        if only and not plugin.lower().startswith(only.lower()):
            continue
        if doc.get('partial'):
            continue
        for model, entry in doc.get('modules', {}).items():
            if not isinstance(entry, dict):
                continue
            where = '%s/%s' % (plugin, model)
            lines = entry.get('lines') or []
            props = (entry.get('props') or {}).get('in') or {}
            tagmap = entry.get('in') or {}
            fam = (entry.get('family') or {}).get('in') or {}
            notes = entry.get('notes') or ''

            # THE MAKER SAYS ONE THING AND WE SAY THE OTHER. Either can be wrong — Bogaudio tags
            # UNISON polyphonic and every input reads channel one — but the pair is worth a look.
            t = tagged.get((plugin, model))
            if t is not None and props:
                anypoly = any(p.get('poly') for p in props.values())
                settled = any('poly' in p for p in props.values())
                if settled and ('polyphonic' in t or 'poly' in t) and not anypoly:
                    found['the maker calls it polyphonic; every input we settled is mono'].append(where)
                if settled and anypoly and 'polyphonic' not in t and 'poly' not in t:
                    # A MAKER WHO HAS NEVER USED THE TAG IS NOT DISAGREEING WITH US. A verifier
                    # settled all 342 of these and corrected nothing: every one was a stale tag.
                    # But 288 of them sat in plugins where the tag carries no information at all —
                    # 4ms tags none of its fifty modules and half of them derive from a polyphonic
                    # base class. Only a maker who tags most of their modules and skips this one is
                    # making a claim worth checking, so the rate is shown and the silent makers
                    # are dropped.
                    rate = tagrate.get(plugin, 0.0)
                    if rate > 0.0:
                        found['we found polyphonic inputs; the maker does not tag it'].append(
                            '%s (this maker tags %.0f%% of their modules)' % (where, rate * 100))

            for port, one in props.items():
                at = '%s in%s' % (where, port)
                why = one.get('why', '')
                # A CITATION THAT NAMES NO MECHANISM. The rule is that nothing may be written
                # that cannot be pointed at; a `why` with no verb in it points at a file.
                if why and not MECHANISM.search(why):
                    found['the why names no mechanism, only a place'].append('%s — %s' % (at, why[:70]))
                # THE LINE AND THE FIELDS DISAGREE about what kind of signal this is.
                li = tagmap.get(port)
                if li is not None and 0 <= int(li) < len(lines):
                    text = lines[int(li)]
                    if one.get('step') == 'continuous' and SAYS_GATE.search(text) \
                            and not SAYS_LEVEL.search(text) \
                            and not OTHERS_GATE.search(text):
                        found['called continuous, but its line describes a gate'].append(
                            '%s — %s' % (at, text[:70]))
                    # A TRIGGER PORT'S STEP FIELD IS NEVER IN DOUBT, whatever words its line
                    # happens to carry — eighty of these were Schmitt-triggered gates whose lines
                    # mention a level, an amount or a chance. The family is read a few lines down
                    # already; consulting it here costs one condition.
                    # And a value read only on a clock edge is NOT stepped: that is a
                    # sample-and-hold in time, not a discrete value, and it is what produced the
                    # one real error in this category that nobody would have guessed.
                    if one.get('step') == 'stepped' and SAYS_LEVEL.search(text) \
                            and not SAYS_GATE.search(text) \
                            and fam.get(port) != 'trigger' \
                            and not DISCRETE.search(text) and not DISCRETE.search(why):
                        found['called stepped, but its line describes a level'].append(
                            '%s — %s' % (at, text[:70]))
                # A FAMILY AND A FIELD THAT CANNOT BOTH BE RIGHT.
                if fam.get(port) == 'trigger' and one.get('step') == 'continuous':
                    found['coloured as a trigger, recorded as continuous'].append(at)
                if fam.get(port) == 'audio' and one.get('step') == 'stepped':
                    found['coloured as audio, recorded as stepped'].append(at)

            # A MODULE WITH NO NOTE AT ALL. Most modules need something before they do anything,
            # and an entry with many controls and nothing to say about starting them is the shape
            # of a prerequisite that was missed.
            has_note = any(str(l).startswith('Note —') for l in lines)
            if not has_note and not notes and len(tagmap) >= 6:
                found['many jacks and nothing said about what it needs first'].append(where)

    order = sorted(found.items(), key=lambda kv: -len(kv[1]))
    total = sum(len(v) for v in found.values())
    print('%d things worth a second look\n' % total)
    for kind, items in order:
        print('== %s (%d)' % (kind, len(items)))
        for x in items[:limit]:
            print('   ' + x)
        if len(items) > limit:
            print('   ... and %d more' % (len(items) - limit))
        print()


if __name__ == '__main__':
    main()
