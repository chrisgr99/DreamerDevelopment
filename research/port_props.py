"""HOW MUCH OF "WHAT DOES THIS JACK EXPECT" WE CAN ANSWER, AND HOW MUCH IS LEFT.

The Rack forum keeps asking the same three things about an input port — a sensible voltage range,
whether the signal is continuous or stepped, whether it takes polyphony — and answering them with
a scope and a test rig. Some of that falls out of what the help already records, so make_help.py
works it out at build time and the JSON stores only what somebody established by hand.

THE RULES LIVE IN make_help.py, AND THIS ASKS IT. Counting them here with a second copy of the
rules is how the two drift apart and how a report comes to describe something nobody ships; so
this calls the very function the generator calls, and counts what comes back.

It writes nothing. It says how many ports still have no answer, field by field, which is the only
honest way to decide whether a hand pass is worth starting."""
import collections
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from make_help import props_of  # noqa: E402  the one definition of the rules

HELP = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'help')

# What each phrase is an answer TO. The generator joins the facts it has with a middle dot, in a
# fixed order, so the parts can be told apart again by what they look like.
FIELDS = (
    ('range', lambda part: part[0].isdigit() or part.startswith(('-', '±'))),
    ('shape', lambda part: part in ('continuous', 'stepped')),
    ('poly',  lambda part: part in ('polyphonic', 'one channel only')),
)


def count(path):
    with open(path) as f:
        doc = json.load(f)
    counts = collections.Counter()
    for entry in doc.get('modules', {}).values():
        if not isinstance(entry, dict):
            continue
        families = (entry.get('family') or {}).get('in') or {}
        if not families:
            counts['ports with no family'] += len(entry.get('in') or {})
            continue
        phrases = props_of(entry)
        for port, family in families.items():
            counts['ports typed'] += 1
            counts['  by family: ' + family] += 1
            at = int(port)
            parts = phrases[at].split(' · ') if at < len(phrases) and phrases[at] else []
            for field, looks_like in FIELDS:
                got = any(looks_like(p) for p in parts)
                counts['%s %s' % (field, 'known' if got else 'BLANK')] += 1
    return counts


def main():
    total = collections.Counter()
    for name in sorted(os.listdir(HELP)):
        if name.endswith('.json'):
            total += count(os.path.join(HELP, name))
    width = max(len(k) for k in total)
    for key in sorted(total):
        print('%-*s %6d' % (width, key, total[key]))


if __name__ == '__main__':
    main()
