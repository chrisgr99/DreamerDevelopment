"""THE RANGES ALREADY WRITTEN DOWN, PULLED OUT OF THE SENTENCES THEY ARE HIDING IN.

Every line in the help was written by somebody reading a manual, and about a tenth of the input
lines state a voltage range while saying something else: "a gate of 5V or more", "0-10V, or -5V to
+5V when the menu is set away from unipolar". Those are scanned facts. They are simply in prose,
where nothing can sort them or compare them between modules, which is exactly what the Rack forum
keeps asking for.

This reads them out. It is extraction, not research: every figure it reports is already in a line
that somebody checked, so nothing here can invent a number the way a scan can.

WHAT IT WILL NOT DO IS GUESS WHOSE RANGE IT IS. A line describing a jack often mentions a knob, an
output, or another port in the same breath, and the range may belong to any of them. So a find is
only offered when the sentence has one range in it and nothing else it could belong to; everything
else is reported as ambiguous and left for a person or an agent. The counts at the end say how
big each pile is, which is the number that decides whether the next stage is worth running.

    python3 research/harvest_ranges.py            # what it found, and how sure
    python3 research/harvest_ranges.py --show 20  # with examples of each kind
"""
import collections
import json
import os
import re
import sys

HELP = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'help')

# HOW THE ENTRIES ACTUALLY WRITE A RANGE, read off the corpus rather than imagined. The project's
# own units rule means these are spelled consistently: no space before V, an en dash or the word
# "to" between the ends, ± for a symmetric pair.
NUM = r'[-+−]?\d+(?:\.\d+)?'
RANGE = re.compile(
    r'(?:±\s?%(n)s\s?V'                      # ±5V
    r'|%(n)s\s?V?\s*(?:to|-|–|—)\s*%(n)s\s?V'   # 0 to 10V, -5V to +5V, 0-10V
    r')' % {'n': NUM})
# A single figure with a unit is a range only when the line says it is a limit or a level.
SINGLE = re.compile(r'(?:above|below|over|under|at least|from)\s+(%s)\s?V' % NUM, re.I)

# WHAT ELSE THE SENTENCE COULD BE TALKING ABOUT. A line mentioning any of these may be giving that
# thing's range rather than this port's, and there is no way to tell from the text alone.
OTHER = re.compile(r'\bknob|\bslider|\bswitch|\boutput|\bbutton|\battenuverter|\bmenu\b', re.I)

# BUT ONLY WHEN THEY ARE IN THE SAME BREATH. "Scales that channel's level, 0V to 10V; with nothing
# patched the slider alone sets it" gives the PORT's range and then says what happens without a
# cable — two clauses, and the slider is not a candidate for the figure at all. The lines are
# written to be heard, so their clauses are marked plainly, and a semicolon or a "; with" is a
# real boundary rather than a guess about grammar.
BREAK = re.compile(r';|—| – ')


def owns_it(line, found):
    """Whether the range in this line can only be the port's own."""
    first = BREAK.split(line)[0]
    # The figure has to be in the opening clause, and nothing else it could belong to with it.
    return bool(re.search(re.escape(found), first)) and not OTHER.search(first)


def main():
    show = 0
    if '--show' in sys.argv:
        show = int(sys.argv[sys.argv.index('--show') + 1])
    counts = collections.Counter()
    examples = collections.defaultdict(list)
    for name in sorted(os.listdir(HELP)):
        if not name.endswith('.json'):
            continue
        with open(os.path.join(HELP, name)) as f:
            doc = json.load(f)
        for model, entry in doc['modules'].items():
            if not isinstance(entry, dict):
                continue
            lines = entry.get('lines') or []
            tagged = entry.get('in') or {}
            families = (entry.get('family') or {}).get('in') or {}
            for port in sorted(tagged, key=int):
                at = int(tagged[port])
                if not (0 <= at < len(lines)):
                    continue
                line = lines[at]
                found = RANGE.findall(line) or SINGLE.findall(line)
                where = '%s/%s in%s' % (doc['plugin'], model, port)
                if not found:
                    counts['no range in the line'] += 1
                    continue
                # More than one figure, or something else in the sentence that could own it.
                if len(found) > 1:
                    kind = 'ambiguous: more than one range in the line'
                elif not owns_it(line, found[0]):
                    kind = 'ambiguous: a control or an output shares the clause'
                elif families.get(port) == 'trigger':
                    # A gate height is exactly what the forum wants and is rarely shared with
                    # anything else in the sentence.
                    kind = 'CLEAR: a gate or trigger level'
                else:
                    kind = 'CLEAR: one range, nothing else it could belong to'
                counts[kind] += 1
                if len(examples[kind]) < show:
                    examples[kind].append('%-34s %s' % (where, line[:140]))

    width = max(len(k) for k in counts)
    for key in sorted(counts):
        print('%-*s %6d' % (width, key, counts[key]))
    for key in sorted(examples):
        print('\n== %s' % key)
        for line in examples[key]:
            print('   ' + line)


if __name__ == '__main__':
    main()
