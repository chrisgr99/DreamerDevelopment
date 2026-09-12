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
SELLING = re.compile(r'\b(simply|just|easy|easily|powerful|versatile|perfect)\b', re.I)
# A thing doing what only a person does. It reads as writing rather than as fact, and it is
# usually covering for not having said what the control does — "where the wave sits in the mix"
# was standing in for "the phase of that wave".
PERSON = re.compile(
    r'\b(sits?|lives?|listens?|wants?|knows?|decides?|remembers?|thinks?|sees?|watches?|waits?'
    r'|likes|prefers|happily|tells you|asks)\b', re.I)
# Words that point instead of saying. If a line needs one, it has not been written yet.
VAGUE = re.compile(r'\b(somehow|sort of|kind of|handles|deals with|takes care of|affects how)\b',
                   re.I)
# A line reached by clicking the control does not need to name it. The heading on the note
# already does, and the reader is pointing at the thing.
LABELLED = re.compile(r'^[A-Z0-9][^a-z]{0,20} — ')
# A line that recites a row to somebody who clicked one thing in it.
GROUPED = re.compile(
    r'^(the )?(two|three|four|five|six|seven|eight|ten|twelve|sixteen)'
    r' (jacks|knobs|inputs|outputs|buttons|ports|rows|columns|sliders)\b', re.I)


def models_of(slug):
    path = os.path.join(PLUGINS, slug, 'plugin.json')
    if not os.path.exists(path):
        return None
    with open(path) as f:
        return [m['slug'] for m in json.load(f).get('modules', [])]


def check(path):
    name = os.path.basename(path)
    with open(path) as f:
        doc = json.load(f)
    problems = []
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
        if len(lines) > 40:
            problems.append('%s: %d lines, which is more controls than a module has'
                            % (where, len(lines)))
        for i, line in enumerate(lines):
            # The first line is about the module, not a control, and may lead with its name.
            if i > 0 and not line.startswith(('Menu — ', 'Note — ')) and LABELLED.match(line):
                problems.append('%s line %d names the control it describes: "%s"'
                                % (where, i, line.split(' — ')[0]))
            if i > 0 and GROUPED.match(line):
                problems.append('%s line %d recites a row to somebody who clicked one of it: "%s"'
                                % (where, i, line[:60]))
            if len(line) > 200:
                problems.append('%s line %d: %d characters, too long to hear in one piece'
                                % (where, i, len(line)))
            for rule, why in ((LEANING, 'leans on something the reader cannot see'),
                              (SIDEWAYS, 'says a relationship sideways'),
                              (SELLING, 'sells rather than says'),
                              (PERSON, 'gives a thing a person\'s verb'),
                              (VAGUE, 'points instead of saying')):
                hit = rule.search(line)
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
