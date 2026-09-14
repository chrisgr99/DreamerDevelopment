#!/usr/bin/env python3
"""Checks the help entries against the rules in research/help/STYLE.md.

What can be checked mechanically is checked here, so that what has to be read by a person is only
the wording. Run it before generating the table:

    python3 research/validate_help.py            # every maker
    python3 research/validate_help.py Venom      # one of them
"""
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
HELP = os.path.join(HERE, 'help')
PLUGINS = os.path.expanduser(
    '~/Library/Application Support/Rack2/plugins-mac-arm64')

# A word pointing at something the reader cannot see. The whole entry is heard one line at a time.
LEANING = re.compile(
    r'\b(still|as (on|it does|above|with)|identical to|a subset of|same as|as the \w+ does)\b',
    re.I)
# The sideways way of saying a limit. "up to ten seconds" is a range and is fine; "up to the knob"
# is a relationship dressed as one.
SIDEWAYS = re.compile(r'up to (the|its|whatever)\b', re.I)
SELLING = re.compile(r'\b(simply|just|easy|easily|powerful|versatile|perfect(?! balance))\b',
                     re.I)
# A thing doing what only a person does. It reads as writing rather than as fact, and it is
# usually covering for not having said what the control does — "where the wave sits in the mix"
# was standing in for "the phase of that wave".
PERSON = re.compile(
    r'\b(sits?|lives?|listens?|wants?|knows?|decides?|remembers?|thinks?|sees?|watches?|waits?'
    r'|likes|prefers|happily|tells you|asks)\b', re.I)
# Words that point instead of saying. If a line needs one, it has not been written yet.
VAGUE = re.compile(r'\b(somehow|sort of|kind of|handles|deals with|takes care of|affects how)\b',
                   re.I)
# THE ONLY SPELLINGS A RANGE MAY TAKE, so the column can be compared down its length rather than
# read line by line. Matches what make_help.py already produces from the lines it harvests.
RANGE_OK = re.compile(
    r'^(?:\u00b1\d+(?:\.\d+)?V'
    r'|-?\d+(?:\.\d+)? to [-+]?\d+(?:\.\d+)?V'
    r'|1V per octave'
    r'|high above \d+(?:\.\d+)?V)$')

# A line reached by clicking the control does not need to name it. The heading on the note
# already does, and the reader is pointing at the thing.
LABELLED = re.compile(r'^[A-Z0-9][^a-z]{0,20} — ')
# A line that recites a row to somebody who clicked one thing in it.
GROUPED = re.compile(
    r'^(the )?(two|three|four|five|six|seven|eight|ten|twelve|sixteen)'
    r' (jacks|knobs|inputs|outputs|buttons|ports|rows|columns|sliders)\b', re.I)


# Rack's own Core is not a plugin folder: it is built into the application, and its manifest
# lives inside the app bundle.
CORE = ('/Applications/VCV Rack 2 Free.app/Contents/Resources/Core.json')


def models_of(slug):
    path = os.path.join(PLUGINS, slug, 'plugin.json')
    if slug == 'Core' and os.path.exists(CORE):
        path = CORE
    if not os.path.exists(path):
        return None
    with open(path) as f:
        return [m['slug'] for m in json.load(f).get('modules', [])]


def check(path):
    name = os.path.basename(path)
    with open(path) as f:
        doc = json.load(f)
    problems = []
    # NOT EVERY JSON FILE HERE IS AN ENTRY. A worklist was put in this directory and the validator
    # died on it — `doc.get` on a list. A checking tool that crashes on an unexpected file checks
    # nothing at all until somebody notices, so a stranger is reported and stepped over.
    if not isinstance(doc, dict):
        return ['%s: not a help entry (a %s at the top level); it does not belong here'
                % (name, type(doc).__name__)]
    plugin = doc.get('plugin')
    if not plugin:
        return ['%s: no plugin slug' % name]
    if not doc.get('source'):
        problems.append('%s: no source URL' % name)

    entries = doc.get('modules', {})
    installed = models_of(plugin)
    # A file still being written says so, and is not nagged about the models it has not reached.
    partial = bool(doc.get('partial'))
    if partial:
        pass
    elif installed is None:
        problems.append('%s: %s is not installed, so coverage cannot be checked' % (name, plugin))
    elif not partial:
        missing = [m for m in installed if m not in entries]
        extra = [m for m in entries if m not in installed]
        if missing:
            problems.append('%s: %d models with no entry: %s'
                            % (name, len(missing), ', '.join(missing[:8])))
        if extra:
            problems.append('%s: %d entries for models that are not installed: %s'
                            % (name, len(extra), ', '.join(extra[:8])))

    # A tag pointing at a line about the context menu is a tag pointing at the nearest line
    # rather than the right one — see STYLE.md.
    for model, entry in sorted(entries.items()):
        if isinstance(entry, dict):
            lines_ = entry.get('lines', [])
            for kind in ('param', 'in', 'out'):
                for idx, li in (entry.get(kind) or {}).items():
                    if not isinstance(li, int) or li >= len(lines_):
                        continue
                    text = lines_[li]
                    # ONLY a line that is wholly about the menu. A line that describes the
                    # control and mentions a menu option in passing — "the output soft-clips at
                    # 12V; hard clipping or none on the menu" — is a good line for that output.
                    if text.startswith(('Menu — ', 'Note — ')):
                        problems.append(
                            '%s/%s: %s %s is tagged to a line about the menu: "%s"'
                            % (name, model, kind, idx, text[:60]))

            # WHAT A JACK EXPECTS, WHICH IS A FIELD AND NOT A SENTENCE.
            #
            # Forty agents writing free text into the same slot is forty schemas, and the
            # generator can only read one. So the shape is policed here: which keys exist, what
            # kind of value each takes, and — for the range — one spelling of one fact, because
            # a column somebody is scanning for a match is worthless if 0-10V and "0 to 10 volts"
            # are both in it. Anything an agent cannot put in these terms belongs in `notes` or
            # in the port's own line, where prose is what is wanted.
            props = (entry.get('props') or {}) if isinstance(entry, dict) else {}
            for kind, holder in props.items():
                if kind not in ('in', 'out'):
                    problems.append('%s/%s: props has "%s", which is not in or out'
                                    % (name, model, kind))
                    continue
                for idx, one in (holder or {}).items():
                    at = '%s/%s props %s %s' % (name, model, kind, idx)
                    if not isinstance(one, dict):
                        problems.append('%s: not an object' % at)
                        continue
                    for key, value in one.items():
                        if key == 'poly':
                            if not isinstance(value, bool):
                                problems.append('%s: poly is %r, not true or false' % (at, value))
                        elif key == 'step':
                            if value not in ('continuous', 'stepped'):
                                problems.append('%s: step is %r, not continuous or stepped'
                                                % (at, value))
                        elif key == 'range':
                            if not isinstance(value, str) or not RANGE_OK.match(value):
                                problems.append(
                                    '%s: range is %r; write it as "0 to 10V", "±5V", '
                                    '"1V per octave" or "high above 1V"' % (at, value))
                        elif key == 'negative':
                            # WHAT A NEGATIVE VOLTAGE DOES, which is three different behaviours
                            # that all look alike from outside. Clamped away before use; subtracted
                            # from a knob until the total hits its floor; or carried through.
                            if value not in ('ignored', 'subtracts', 'swings'):
                                problems.append(
                                    '%s: negative is %r, not ignored, subtracts or swings' % (at, value))
                        elif key == 'normal':
                            # WHAT AN UNPATCHED JACK READS. Usually a voltage, sometimes another
                            # jack, so this is the one field that cannot take a closed vocabulary —
                            # it is held short instead, and to a phrase rather than a sentence.
                            if not isinstance(value, str) or not value or len(value) > 48:
                                problems.append('%s: normal is %r; a short phrase, 48 characters '
                                                'at most' % (at, value))
                            elif value.endswith('.'):
                                problems.append('%s: normal is a phrase, not a sentence: %r'
                                                % (at, value))
                        elif key == 'sumRange':
                            if not isinstance(value, str) or not RANGE_OK.match(value):
                                problems.append(
                                    '%s: sumRange is %r; the same spellings as range' % (at, value))
                        elif key == 'polarity':
                            if value not in ('unipolar', 'bipolar'):
                                problems.append('%s: polarity is %r, not unipolar or bipolar'
                                                % (at, value))
                        elif key != 'why':
                            problems.append('%s: "%s" is not a field here' % (at, key))
                    if 'why' not in one:
                        # WHERE IT CAME FROM, OR IT DID NOT HAPPEN. This project has already
                        # shipped fabricated constants once; a figure with no source beside it is
                        # exactly what that looked like.
                        problems.append('%s: no "why" saying where this was established' % at)

    for model, entry in sorted(entries.items()):
        where = '%s/%s' % (name, model)
        # An entry is a list of lines, or an object carrying those lines plus the tags saying
        # which line covers which jack. Only the lines are checked here.
        lines = entry.get('lines', []) if isinstance(entry, dict) else entry
        if isinstance(lines, str):
            problems.append('%s: prose, not lines' % where)
            continue
        if not lines:
            problems.append('%s: empty' % where)
            continue
        # A GENEROUS CEILING, NOT A TARGET. Probably Note MathNerd has 66 input jacks and
        # Manic Compression MB has 59 attenuverters; a cap near the size of a large module makes
        # writers merge unlike controls onto one line, which is the fault this whole file exists
        # to prevent. This is here only to catch a runaway generator.
        if len(lines) > 200:
            problems.append('%s: %d lines, which is more than any module has controls'
                            % (where, len(lines)))
        for i, line in enumerate(lines):
            # The first line is about the module, not a control, and may lead with its name.
            if i > 0 and not line.startswith(('Menu — ', 'Note — ')) and LABELLED.match(line):
                problems.append('%s line %d names the control it describes: "%s"'
                                % (where, i, line.split(' — ')[0]))
            # ONLY WHEN THE LINE COUNTS THE VERY THING IT IS ATTACHED TO. "The four outputs",
            # tagged to an output, recites the row. "The two inputs added together", tagged to an
            # output, describes that output in terms of what feeds it, which is correct and the
            # natural way to say it.
            hit = GROUPED.match(line)
            if i > 0 and hit and isinstance(entry, dict):
                noun = hit.group(3).lower()
                kinds = {'inputs': 'in', 'ports': 'in', 'outputs': 'out',
                         'knobs': 'param', 'buttons': 'param', 'sliders': 'param'}
                kind = kinds.get(noun)
                if kind and i in ((entry.get(kind) or {}).values()):
                    problems.append(
                        '%s line %d recites a row to somebody who clicked one of it: "%s"'
                        % (where, i, line[:60]))
            if len(line) > 200:
                problems.append('%s line %d: %d characters, too long to hear in one piece'
                                % (where, i, len(line)))
            # A MENU ITEM'S NAME IS THE MAKER'S WORDS, NOT OURS. A menu line reads
            # "Menu — <the item as the panel prints it> — what it does", and the middle part is
            # quoted: Zilah really does print "MSB waits for LSB". Rewording it would falsify the
            # one thing the line exists to get right, so the style rules read the line with that
            # name taken out. Everything we wrote ourselves is still checked.
            ours = line
            if line.startswith('Menu — '):
                parts = line.split(' — ')
                if len(parts) > 2:
                    ours = ' — '.join([parts[0]] + parts[2:])
            for rule, why in ((LEANING, 'leans on something the reader cannot see'),
                              (SIDEWAYS, 'says a relationship sideways'),
                              (SELLING, 'sells rather than says'),
                              (PERSON, 'gives a thing a person\'s verb'),
                              (VAGUE, 'points instead of saying')):
                hit = rule.search(ours)
                if hit:
                    problems.append('%s line %d %s: "%s"' % (where, i, why, hit.group(0)))
    return problems


def main():
    only = sys.argv[1] if len(sys.argv) > 1 else None
    problems = []
    files = 0
    for name in sorted(os.listdir(HELP)):
        if not name.endswith('.json'):
            continue
        # NOT EVERY JSON FILE HERE IS A PLUGIN. UNINSTALLED.json holds entries written for models
        # their plugin no longer registers — kept so the work survives a maker's removal, but
        # loaded by nothing and shipped nowhere, so there is nothing here to validate.
        if name == 'UNINSTALLED.json':
            continue
        if only and not name.lower().startswith(only.lower()):
            continue
        files += 1
        problems += check(os.path.join(HELP, name))
    for p in problems:
        print(p)
    print('%d file(s), %d problem(s)' % (files, len(problems)))
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
