#!/usr/bin/env python3
"""Turns research/help/*.json into src/HelpText.cpp.

One JSON file per maker, written by hand from that maker's own manual; this flattens the lot
into a table the plugin can carry. Never edit HelpText.cpp — edit the JSON and run this.

    python3 research/make_help.py
"""
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
HELP = os.path.join(HERE, 'help')
OUT = os.path.join(os.path.dirname(HERE), 'src', 'HelpText.cpp')


def cstr(s):
    out = []
    for ch in s:
        if ch == '"':
            out.append('\\"')
        elif ch == '\\':
            out.append('\\\\')
        elif ch == '\n':
            out.append('\\n')
        else:
            out.append(ch)
    return '"' + ''.join(out) + '"'


def tags_of(entry):
    """A module's entry is a list of lines, and optionally a "tags" object beside it saying which
    line covers which jack or knob. Accepts either:

        "Model": ["line", "line"]
        "Model": {"lines": [...], "in": {"0": 2, "1": 3}, "out": {...}, "param": {...}}

    A tag map is port index -> line index. Anything not mentioned is -1, meaning nothing here
    describes that control."""
    if isinstance(entry, dict):
        lines = entry.get('lines', [])
        maps = (entry.get('in', {}), entry.get('out', {}), entry.get('param', {}))
    else:
        lines = entry
        maps = ({}, {}, {})
    if isinstance(lines, str):
        lines = [lines]
    out = []
    for m in maps:
        if not m:
            out.append([])
            continue
        highest = max(int(k) for k in m)
        table = [-1] * (highest + 1)
        for k, v in m.items():
            if not (0 <= int(v) < len(lines)):
                raise ValueError('tag points at line %s, which does not exist' % v)
            table[int(k)] = int(v)
        out.append(table)
    return lines, out


# What a family word means to Clarity's palette. Kept in step with FAM_* in src/Palette.hpp.
FAMILIES = {'audio': 0, 'cv': 1, 'trigger': 2, 'pitch': 3}

# WHAT A JACK EXPECTS, AS FACTS RATHER THAN A SENTENCE.
#
# The Rack forum has been asking for a year how to know what an input port wants — a sensible
# voltage range, whether the signal is continuous or stepped, whether it takes polyphony — and
# resorting to scopes and test rigs for the answer. Two of those three fall straight out of the
# family we already recorded for 13,711 input ports, so they are derived here rather than stored:
# the JSON keeps only what a person established by hand, and changing a rule below does not mean
# rewriting a hundred files.
#
# THE LINE THIS WILL NOT CROSS. A volt-per-octave port IS one volt per octave and IS continuous;
# that is what the standard means. A trigger or gate port is stepped because the signal is a state
# rather than a value — but its HEIGHT is not decided by the family, since 5V and 10V gates are
# both common, so no range is claimed. Audio and CV get continuity and nothing else: both have a
# conventional range, and "conventionally" would be doing all the work in that sentence. A port's
# real range is a fact about the module, and this project has already been bitten once by numbers
# that were plausible rather than checked (see research/help/NUMBER-CHECK.md).
#
# AND THE FAMILY IS NOT ENOUGH ON ITS OWN FOR THE RANGE. The families were decided for COLOUR,
# and colour tolerates a port that is pitch-ish: Bidoo's pErCO takes an attenuated frequency
# modulation whose effect depends on where a knob sits, and Impromptu's Chord-Key reads 2V as the
# twenty-fifth chord. Both are fairly drawn as pitch. Neither is one volt per octave, and saying
# so would be the same failure as a plausible constant. So the range is claimed only where the
# port's OWN LINE says per-octave — which is a fact somebody read off a manual, not an inference
# from a colour. See PER_OCTAVE below.
DERIVED = {
    'pitch':   {'step': 'continuous'},
    # A TRIGGER PORT IS STEPPED BY DEFINITION — but only if it really is one. Of 4,204
    # trigger-tagged inputs, 3,705 lines talk about gates, edges, pulses or clocks and 499 do not;
    # NYSTHI's LOGAN20 has one described as "one of the twenty voltages written into each row of
    # the file", which is a continuous CV wearing a trigger colour because the colour was chosen
    # for a jack's company rather than its contents. So this family, like CV, waits for the line
    # to agree — see STEPPED below.
    'trigger': {},
    'audio':   {'step': 'continuous'},
    # CV GETS NOTHING, and this is the same trap the range fell into. A family was chosen to
    # COLOUR a jack, and "control voltage" covers both a filter cutoff and a selector that reads
    # one chord type per volt — AaronStatic's ChordCV is exactly that. Continuous is true of most
    # of them and false of enough to matter, and there is no way to tell which from the colour. So
    # the largest family in the library says nothing about its shape until somebody establishes
    # it per port.
    'cv':      {},
    # MPX carries a note stream and not a voltage at all, so all three are the wrong question.
    # Listed to say so deliberately rather than by omission.
    'mpx':     {},
}


# THE WAYS THE ENTRIES ACTUALLY SAY IT, counted rather than imagined: "volt per octave", "a volt
# to the octave", "an octave per volt", "one octave per volt", "V/OCT". A port whose own line says
# one of these has had somebody read a manual and write it down, which is the standard the rest of
# this project holds to.
PER_OCTAVE = re.compile(
    r'volt[- ]per[- ]octave|V/OCT|(?:a|one|1) ?V(?:olt)? (?:to|per) the octave'
    r'|(?:an?|one|1) octave per volt|1V per octave', re.I)

# AND THE RANGES ALREADY WRITTEN DOWN, IN THE SENTENCES THEY ARE HIDING IN.
#
# About a twelfth of the input lines state a range while saying something else — "Scales that
# channel's level, 0V to 10V", "A rising edge above 1V restarts every track". Somebody read those
# off a manual, so they are as good as anything a fresh scan would produce, and better than a
# convention. They are simply in prose, where nothing can sort them or compare them between
# modules, which is the whole of what the forum is asking for.
#
# WHOSE RANGE IT IS, THOUGH, IS NOT GUESSED. A line about a jack often mentions a knob or an
# output in the same breath and the figure may belong to either. So a figure counts only when it
# is in the line's opening clause with nothing else that could own it — the lines are written to
# be heard, so their clauses are marked plainly and a semicolon is a real boundary. Everything
# else is left alone and reported by research/harvest_ranges.py for somebody to settle.
# THE LANGUAGE OF A SIGNAL THAT IS A STATE RATHER THAN A VALUE. A line that talks about edges,
# gates, pulses, clocks or a threshold is describing something stepped, whoever wrote it.
STEPPED = re.compile(
    r'rising edge|falling edge|\bedge\b|\btrigger|\bgate\b|\bgates\b|\bpulse|\bclock'
    r'|counted as (?:true|1)|counts as true|above \d+(?:\.\d+)?\s?V|\bhigh\b|\btoggl|\blatch', re.I)

NUM = r'[-+−]?\d+(?:\.\d+)?'
SYMMETRIC = re.compile(r'±\s?(%s)\s?V' % NUM)
SPAN = re.compile(r'(%(n)s)\s?V?\s*(?:to|-|–)\s*(%(n)s)\s?V' % {'n': NUM})
THRESHOLD = re.compile(r'(?:above|at least|over)\s+(%s)\s?V' % NUM, re.I)
OTHER = re.compile(r'\bknob|\bslider|\bswitch|\boutput|\bbutton|\battenuverter|\bmenu\b', re.I)
BREAK = re.compile(r';|—| – ')


def range_in_line(line, family):
    """The voltage range this line states for its own port, or None.

    Conservative by design: one figure, in the opening clause, with nothing else in that clause it
    could belong to. A line with two ranges in it is somebody's careful sentence about two modes,
    and picking one of them would be inventing a fact."""
    first = BREAK.split(line)[0]
    if OTHER.search(first):
        return None
    hits = SYMMETRIC.findall(first)
    if len(hits) == 1 and not SPAN.search(first):
        return '±%sV' % hits[0]
    spans = SPAN.findall(first)
    if len(spans) == 1:
        lo, hi = spans[0]
        # ONE SPELLING FOR ONE FACT. The entries write a symmetric pair four different ways —
        # "-5 to 5V", "-5V to +5V", "-10 to +10V", "±5V" — and a column somebody is scanning for
        # a match wants them to look the same. Collapse them all to the ± form.
        try:
            if float(lo.replace('−', '-')) == -float(hi.replace('+', '')):
                return '±%sV' % hi.lstrip('+')
        except ValueError:
            pass
        return '%s to %sV' % (lo.rstrip('V'), hi)
    if spans:
        return None
    # A gate's height is what the forum most wants, and a threshold is how the lines state it.
    if family == 'trigger':
        hits = THRESHOLD.findall(first)
        if len(hits) == 1:
            return 'high above %sV' % hits[0]
    return None


def polarity_of(rng):
    """Unipolar or bipolar, worked out from a range that is already established.

    A range says this outright — 0 to 10V is unipolar, ±5V is bipolar — so nobody should have to
    write both down, and a port with a range is not asked for its polarity. The field exists for
    the other case, which is commoner than it sounds: code that centres a signal, or clamps
    asymmetrically, often shows the polarity plainly while pinning no range at all.

    A volt-per-octave port is bipolar. Nothing in the standard says so, but C4 is 0V and every
    note below it is negative, which is the whole point of the convention."""
    if not rng:
        return None
    if rng.startswith('±') or rng == '1V per octave':
        return 'bipolar'
    if rng.startswith('high above '):
        return 'unipolar'
    hit = SPAN.search(rng)
    if hit:
        try:
            return 'bipolar' if float(hit.group(1).replace('−', '-')) < 0 else 'unipolar'
        except ValueError:
            return None
    return None


def props_of(entry, kind='in'):
    """One short phrase per port, or an empty list where there is nothing to say.

    Assembled here rather than in the plugin so that the rules live in one place and the cost is
    paid once, at build time. Order is fixed — range, then shape, then polyphony — so that the
    line can be read at a glance and heard in one piece, and so that a missing fact reads as a
    gap rather than as a different fact."""
    if not isinstance(entry, dict):
        return []
    families = (entry.get('family') or {}).get(kind) or {}
    # What somebody established by hand, which always wins over the derivation.
    known = ((entry.get('props') or {}).get(kind)) or {}
    # EVERY PORT EITHER SIDE KNOWS ABOUT. A family is a colouring decision and plenty of ports
    # never got one — 953 across the library, 141 of them in Bogaudio alone — but a port somebody
    # established a fact about has that fact whether or not anybody chose it a colour. Walking the
    # families alone dropped those on the floor.
    ports = set(families) | set(known)
    if not ports:
        return []
    lines = entry.get('lines') or []
    tagged = entry.get(kind) or {}
    highest = max(int(k) for k in ports)
    out = [''] * (highest + 1)
    for port in ports:
        family = families.get(port)
        got = dict(DERIVED.get(family) or {})
        # The port's own line is the evidence for a range, not its colour.
        at = tagged.get(port)
        if at is not None and 0 <= int(at) < len(lines):
            line = lines[int(at)]
            if family == 'trigger' and STEPPED.search(line):
                got['step'] = 'stepped'
            if family == 'pitch' and PER_OCTAVE.search(line):
                got['range'] = '1V per octave'
            else:
                found = range_in_line(line, family)
                if found:
                    got['range'] = found
        got.update(known.get(port) or {})
        poly = got.get('poly')
        # POLARITY ONLY WHERE THE RANGE DOES NOT ALREADY SAY IT. "0 to 10V \u00b7 unipolar" says
        # one thing twice and spends a third of the line doing it; the word earns its place
        # exactly when no range could be pinned down.
        rng = got.get('range')
        # A NEGATIVE VOLTAGE SWINGING THROUGH IS WHAT BIPOLAR MEANS, so these two fields are one
        # fact asked twice, and `negative` is the better half: it tells a jack that ignores a
        # negative voltage apart from one that subtracts it from a knob, where "unipolar" would
        # cover both.
        neg = got.get('negative')
        polar = got.get('polarity') or (
            'bipolar' if neg == 'swings' else 'unipolar' if neg == 'ignored' else None
        ) or polarity_of(rng)
        parts = [rng, None if rng else polar, got.get('step'),
                 None if poly is None else ('polyphonic' if poly else 'one channel only')]
        parts = [p for p in parts if p]
        if not parts:
            continue
        # THE QUICK LINE STAYS QUICK. Four facts in a fixed order is what makes a column of these
        # scannable, so anything more goes on a second line underneath, in plain words — its own
        # paragraph, so it can be clicked and heard on its own and skipped by somebody who only
        # wanted the first.
        more = []
        if got.get('normal'):
            more.append('unpatched it reads %s' % got['normal'])
        if neg == 'ignored':
            more.append('a negative voltage does nothing here')
        elif neg == 'subtracts':
            more.append('a negative voltage subtracts from the knob')
        if got.get('sumRange'):
            more.append('the knob and this input are summed and the total held to %s'
                        % got['sumRange'])
        text = ' · '.join(parts)
        if more:
            text += '\n\n' + '; '.join(more) + '.'
        out[int(port)] = text
    return out if any(out) else []


def families_of(entry):
    """The family per input and per output, as palette numbers, -1 where we did not say.

    These are read off the panels while the help was written, and they are what Clarity colours a
    jack from — so they are emitted into the same table rather than living only in the JSON."""
    if not isinstance(entry, dict):
        return [], []
    fam = entry.get('family') or {}
    out = []
    for kind in ('in', 'out'):
        m = fam.get(kind) or {}
        if not m:
            out.append([])
            continue
        highest = max(int(k) for k in m)
        table = [-1] * (highest + 1)
        for k, v in m.items():
            if v in FAMILIES:
                table[int(k)] = FAMILIES[v]
        out.append(table)
    return out


def main():
    entries = []          # (plugin, model, [lines], [in, out, param], [infam, outfam], [props])
    sources = []          # (plugin, url, count)
    for name in sorted(os.listdir(HELP)):
        if not name.endswith('.json'):
            continue
        with open(os.path.join(HELP, name)) as f:
            doc = json.load(f)
        plugin = doc['plugin']
        mods = doc['modules']
        for model, entry in sorted(mods.items()):
            lines, tables = tags_of(entry)
            entries.append((plugin, model, lines, tables, families_of(entry),
                            props_of(entry, 'in'), props_of(entry, 'out')))
        sources.append((plugin, doc.get('source', ''), len(mods)))

    entries.sort()

    with open(OUT, 'w') as f:
        f.write('/** GENERATED by research/make_help.py — do not edit.\n\n')
        f.write('What each module is, in point form, written from its maker\'s own manual.\n')
        f.write('See Help.hpp for what this is for and where the wording rules come from.\n\n')
        f.write('Sources:\n')
        for plugin, url, count in sources:
            f.write('  %-24s %4d modules  %s\n' % (plugin, count, url))
        f.write('*/\n#include "Help.hpp"\n\n#include <cstddef>\n\n')

        # ONE POOL OF PHRASES, AND A BYTE PER PORT. There are 13,711 typed input ports and only a
        # handful of distinct things to say about them, so the phrases are pooled and each port
        # holds an index into the pool. A string per port would have cost a third of a megabyte to
        # say the same few sentences over and over.
        pool = []
        seen = {}
        for e in entries:
            for phrase in e[5] + e[6]:
                if phrase and phrase not in seen:
                    seen[phrase] = len(pool)
                    pool.append(phrase)
        f.write('/** What a jack expects, as short phrases shared by every port that wants one. */\n')
        f.write('const char* const HELP_PROP_TEXT[] = {\n')
        for phrase in pool:
            f.write('\t%s,\n' % cstr(phrase))
        f.write('};\nconst int HELP_PROP_TEXT_COUNT = %d;\n\n' % len(pool))

        for i, (plugin, model, lines, tables, fams, props, oprops) in enumerate(entries):
            f.write('static const char* const L%d[] = {\n' % i)
            for line in lines:
                f.write('\t%s,\n' % cstr(line))
            f.write('};\n')
            for kind, table in zip(('I', 'O', 'P'), tables):
                if table:
                    f.write('static const short %s%d[] = {%s};\n'
                            % (kind, i, ','.join(str(x) for x in table)))
            for kind, table in zip(('FI', 'FO'), fams):
                if table:
                    f.write('static const signed char %s%d[] = {%s};\n'
                            % (kind, i, ','.join(str(x) for x in table)))
            for mark, table in (('PR', props), ('PO', oprops)):
                if table:
                    f.write('static const short %s%d[] = {%s};\n'
                            % (mark, i, ','.join(str(seen[p]) if p else '-1' for p in table)))
        f.write('\n/** Sorted by plugin then model, so it can be searched rather than walked. */\n')
        f.write('const HelpEntry HELP[] = {\n')
        for i, (plugin, model, lines, tables, fams, props, oprops) in enumerate(entries):
            cells = []
            for kind, table in zip(('I', 'O', 'P'), tables):
                cells.append('%s%d, %d' % (kind, i, len(table)) if table else 'NULL, 0')
            for kind, table in zip(('FI', 'FO'), fams):
                cells.append('%s%d, %d' % (kind, i, len(table)) if table else 'NULL, 0')
            cells.append('PR%d, %d' % (i, len(props)) if props else 'NULL, 0')
            cells.append('PO%d, %d' % (i, len(oprops)) if oprops else 'NULL, 0')
            f.write('\t{%s, %s, L%d, %d, %s},\n'
                    % (cstr(plugin), cstr(model), i, len(lines), ', '.join(cells)))
        f.write('};\n')
        f.write('const int HELP_COUNT = %d;\n' % len(entries))

    tagged = sum(1 for e in entries if any(e[3]))
    typed = sum(sum(1 for x in t if x >= 0) for e in entries for t in e[4])
    print('%d modules across %d makers, %d with controls tagged, %d ports typed -> %s'
          % (len(entries), len(sources), tagged, typed, OUT))
    return 0


if __name__ == '__main__':
    sys.exit(main())
