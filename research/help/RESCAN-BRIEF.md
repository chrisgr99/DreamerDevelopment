Amend the existing VCV Rack help entries for **<Plugin>** — do not rewrite the control lines.

Read `~/ProgrammingProjects/DreamerDevelopment/research/help/STYLE.md` first, especially the two
sections that changed: "A line for something that is not a control" and the menu rules under
"Shape". Also read `research/help/AGENT-BRIEF.md` for the sources and how to verify. Work from
`~/ProgrammingProjects/DreamerDevelopment`, on `research/help/<Plugin>.json` only.

WHY THIS PASS EXISTS. A module's entry answers "what does this control do" well and answers
"how do I use this thing" badly, because the brief never asked. NYSTHI's Sussudio is a sample
player that does NOTHING until you right-click a region and load a sample, and its entry never
said so. That fact belongs to no control, so nothing collected it — and the old rule told agents
to write such lines "rarely". That instruction is now reversed.

FOUR THINGS TO ADD OR FIX, per module:

1. PREREQUISITES. If the module does nothing until the user does something, write a `Note — `
   line saying what. A file or sample to load and how to load it, a device to choose, an
   expander to place, a mode to come out of, a button to arm. This is the most valuable thing
   in this pass; look for it on every module and be certain before you say a module needs
   nothing.

2. MENU ITEM NAMES. Every `Menu — ` line must begin with the item's name exactly as the menu
   prints it, then an em dash, then what it does:
       Menu — Delay specification — chooses whether the Delay knob is read as a pitch, one cycle
       long, or as a time
   Roughly half the existing menu lines describe an effect and never name the item. The names
   are literal strings in the source (`createMenuItem("...")`) or in the binary, so check them
   rather than guessing. Where an item offers named choices, list them as printed.

2b. NO LINE FOR A COSMETIC PLUGIN-WIDE MENU — themes, skins, panel art, light and dark. See
   STYLE.md. Exceptions: a blank panel whose whole purpose is how it looks, and any plugin-wide
   item that changes BEHAVIOUR rather than appearance, which goes on every module that has it.

3. WHERE THE MENU IS. These lines are shown under a heading reading "Right-click the panel for:".
   That is right for the module's own menu. Rack also has a knob's menu, a port's menu, and
   menus that a display carries — if a setting lives on one of those, the line must say which,
   or the heading is confidently wrong.

4. OTHER MODULE-LEVEL FACTS worth a `Note — ` line: a gesture on a display or a panel that no
   control reveals (double-click, drag, a key held while dragging), what an expander attaches to
   and on WHICH side, and a limit such as being mono only or needing a particular sample rate.

DO THE WORK YOURSELF. Do not spawn sub-agents to split the plugin up. One agent on this pass did,
reported that it was waiting for them, and ended its turn with the file byte-identical — their
work lived in their contexts and reached nothing. Edit the JSON as you go, in batches you choose,
so progress is always on disk rather than in a context that can vanish, and validate after each
batch so a mistake is caught near where it was made. If a plugin is too large for one turn, do as
much as you can and say exactly which module slugs you finished.

WHAT NOT TO DO. Do not reword control lines that are fine. Do not add facts you cannot attribute
to the code, the panel, the maker's own text or a shipped preset or patch — a usage claim is as
fabricable as a number, and one entry in this project had to be thrown away and rewritten for
exactly that. Where you cannot establish something, leave it out and say so in `notes`.

TWO SOURCES THIS PASS SHOULD USE that the first one did not: the plugin's shipped presets
(`*.vcvm` under the installed plugin folder — 1,983 of them across 25 plugins) and any demo
patches (`*.vcv`). A preset is the author naming the settings they thought worth having; a patch
is the author wiring the module up. Both are evidence of intended use and neither can be
invented.

Validate with `python3 research/validate_help.py <Plugin>` until clean, then report: how many
prerequisite notes you added, how many menu lines you renamed, how many menus turned out not to
be the module's own, and anything a person should check.

## Never run `git checkout`, `git restore` or `git stash`

The whole `research/help/` tree is uncommitted work in progress. An agent on an earlier wave ran `git checkout` on one file to undo a formatting change of its own and destroyed a previous pass over that plugin. If you make a mess of a file, fix it forwards — re-read it, correct it, validate it. Never reach for git to undo anything.
