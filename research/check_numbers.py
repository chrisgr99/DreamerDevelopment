#!/usr/bin/env python3
"""Checks that the numbers in a help entry are actually in the plugin.

WHY THIS EXISTS. An agent writing an entry for a plugin with no published source has to read the
compiled binary, and one of them did not: it produced a confident list of DSP constants — a
19490Hz clock, a 352.5Hz corner, a 0.2675Hz floor — none of which appears anywhere in the file.
The agent that took the work over found that by checking every value against the binary, threw the
file away and rewrote it from ground it could stand behind.

Nothing else catches this. The style validator checks how a line reads and where its tag points,
and a fabricated number reads better than an honest one. So this is the other half: for every
number a line quotes, is that number in the binary at all?

WHAT A MISS DOES AND DOES NOT MEAN. A number can be absent for good reasons — computed at run
time (a range divided by ten, a percentage, a count of jacks), stored as an integer, derived from
the sample rate, or written in the source as an expression rather than a literal. So a miss is a
QUESTION, not a verdict. What matters is the rate: an entry whose distinctive numbers are nearly
all present was read off the binary, and one where they are nearly all missing was not.

    python3 research/check_numbers.py                 # every plugin with no published source
    python3 research/check_numbers.py Grayscale       # one of them
"""
import array
import bisect
import glob
import json
import os
import random
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
HELP = os.path.join(HERE, 'help')
PLUGINS = os.path.expanduser(
    '~/Library/Application Support/Rack2/plugins-mac-arm64')

# THE PLUGINS THIS IS FOR. Where a maker publishes source, a quoted range was read from it and a
# wrong number is a misreading. These are the ones whose numbers could only have come from the
# compiled file, which is exactly where invention is possible and undetectable.
NO_SOURCE = [
    'Autodafe-DrumKit', 'MM_Tools', 'StellareModular', 'Grayscale',
    'Hora-treasureFree', 'Hora-Mixers', 'AlrightDevices', 'EricaCopies',
    'Blamsoft-XFXReverb', 'Blamsoft-XFXWave', 'Blamsoft-XFXF35',
    'Instruo', 'DanTModules', 'FLAG-Free',
    'VultModulesFree', 'VultModules', 'VultCompacts',
]

# SMALL WHOLE NUMBERS SAY NOTHING. A count of jacks, a two-position switch, a five-volt gate:
# every binary contains them, so asking whether they are present answers nothing. Everything
# larger is kept — including the round ones like 100 and 1000, because throwing those away threw
# away most of the real specifications too and left several entries with nothing to check.
SMALL = 12

# The number, and whatever unit is glued to it. A line says 1.5kHz where the binary holds 1500,
# and 44.1kHz where it holds 44100, so the unit has to be read or every such figure looks absent.
NUMBER = re.compile(r'(?<![A-Za-z0-9_.])(\d+(?:\.\d+)?)\s*(kHz|KHz|KHZ|kV|ms|k\b|K\b)?')

# What a suffix means the stored constant probably is. A value is looked for as written AND as
# scaled, because a maker may store either — milliseconds as 0.005 or as 5.
SCALES = {'kHz': 1000.0, 'KHz': 1000.0, 'KHZ': 1000.0, 'kV': 1000.0,
          'k': 1000.0, 'K': 1000.0, 'ms': 0.001}


def constants(path):
    """Every plausible float in a binary, sorted. Read at each byte alignment, because a
    constant is not promised to sit on a four-byte boundary."""
    data = open(path, 'rb').read()
    out = set()
    for code, size in (('f', 4), ('d', 8)):
        for off in range(size):
            end = off + ((len(data) - off) // size) * size
            if end <= off:
                continue
            a = array.array(code)
            a.frombytes(data[off:end])
            for v in a:
                # Anything outside this is noise: denormals, exponents from random bytes, and
                # values no panel would ever quote.
                if 1e-4 < v < 1e7:
                    out.add(round(v, 6))
    # Integers are often stored as integers, so read those too.
    for code, size in (('i', 4), ('h', 2)):
        for off in range(size):
            end = off + ((len(data) - off) // size) * size
            if end <= off:
                continue
            a = array.array(code)
            a.frombytes(data[off:end])
            for v in a:
                if 0 < v < 10000000:
                    out.add(float(v))
    return sorted(out)


def near(sorted_values, want, rel=1e-6):
    """Is `want` in there, as a literal rather than by coincidence?

    THE TOLERANCE IS THE WHOLE TEST, and a loose one proves nothing. A 800KB binary holds about
    75,000 distinct plausible constants, so a window of half a per cent catches 98.6% of RANDOM
    numbers — which is how the first version of this cleared the very fabrications it was written
    to find. Measured against 4,000 random plausible values on one binary:

        half a per cent   98.6% false hits      useless
        a tenth           63.9%                 useless
        1e-4              20.1%                 weak
        1e-5               2.9%                 usable
        1e-6               0.3%                 what this uses
        exact              0.0%                 too strict for a float32 round trip

    1e-6 is loose enough that a value stored as float32 still matches after the round trip, and
    tight enough that a number nobody wrote is very unlikely to be there.
    """
    lo = want * (1 - rel) - 1e-9
    hi = want * (1 + rel) + 1e-9
    i = bisect.bisect_left(sorted_values, lo)
    return i < len(sorted_values) and sorted_values[i] <= hi


def numbers_in(entry):
    """Every distinctive number quoted in a module's lines, with the line it came from."""
    lines = entry['lines'] if isinstance(entry, dict) else entry
    out = []
    for line in lines:
        for m in NUMBER.finditer(line):
            try:
                v = float(m.group(1))
            except ValueError:
                continue
            if v <= SMALL and v == int(v):
                continue
            want = [v]
            scale = SCALES.get((m.group(2) or '').strip())
            if scale:
                want.append(v * scale)
            out.append((want, line))
    return out


def baseline(values, seed=1, probes=3000):
    """How often a number nobody wrote would be found here anyway.

    THE RATE MEANS NOTHING ON ITS OWN. A big binary holds more constants and so clears more
    numbers by luck, and the entries differ in how many figures they quote. So every binary is
    asked the same question about random plausible values first, and the entry's rate is read
    against that. A rate at the baseline is no evidence of anything; a rate far above it is."""
    rnd = random.Random(seed)
    hits = 0
    for _ in range(probes):
        w = 10 ** rnd.uniform(-2, 4) * rnd.choice([1, 1, 1, 2.5, 3.3])
        if near(values, w):
            hits += 1
    return 100.0 * hits / probes


def main():
    wanted = sys.argv[1:] or NO_SOURCE
    print('%-22s %6s %6s %6s %6s   %s'
          % ('plugin', 'nums', 'found', 'rate', 'chance', 'verdict'))
    print('-' * 86)
    misses = {}
    for plug in wanted:
        path = os.path.join(HELP, plug + '.json')
        binary = os.path.join(PLUGINS, plug, 'plugin.dylib')
        if not os.path.exists(path):
            print('%-22s   no help file' % plug)
            continue
        if not os.path.exists(binary):
            print('%-22s   no binary at %s' % (plug, binary))
            continue
        values = constants(binary)
        total = found = 0
        gone = []
        for slug, entry in json.load(open(path))['modules'].items():
            for want, line in numbers_in(entry):
                total += 1
                if any(near(values, w) for w in want):
                    found += 1
                else:
                    gone.append((slug, want[0], line))
        rate = 100.0 * found / total if total else 0.0
        base = baseline(values)
        if total < 5:
            verdict = 'too few numbers to judge'
        elif rate >= base + 40:
            verdict = 'read from the binary'
        elif rate >= base + 15:
            verdict = 'mostly real, some misses'
        else:
            verdict = 'NO BETTER THAN CHANCE — check by hand'
        print('%-22s %6d %6d %5.0f%% %6.0f%%   %s'
              % (plug, total, found, rate, base, verdict))
        if gone:
            misses[plug] = gone
    print()
    for plug, gone in misses.items():
        print('=== %s: %d number(s) not in the binary ===' % (plug, len(gone)))
        for slug, v, line in gone[:12]:
            print('  %-24s %-12g %s' % (slug, v, line[:90]))
        if len(gone) > 12:
            print('  ... and %d more' % (len(gone) - 12))
        print()
    return 0


if __name__ == '__main__':
    sys.exit(main())
