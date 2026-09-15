# How a help entry is written

Every entry is read aloud, one line at a time, by somebody who clicked that line. That is the whole of the design brief, and every rule below follows from it.

## Shape

An entry is a list of lines. The first line says what the module is. Every line after it is one port, one knob, one switch or one group of them, led by the name printed on the panel.

Menu options go last, on a line starting `Menu — `, and only where they matter to using the module.

**Name the item first, exactly as the menu prints it, then an em dash, then what it does.**

    Menu — Delay specification — chooses whether the Delay knob is read as a pitch, one cycle long, or as a time

The name comes first because that is what a person scans the menu for, and the em dash separates it from the description — both to the eye and to the voice, which reads an em dash as a pause. Half the menu lines written before this rule described an effect without ever naming the item, which leaves the reader hunting. Where an item has named choices, list them as they are printed, so they can be matched against what is on screen.

The names are literal strings in the maker's source and in the compiled binary — `createMenuItem("Digital Voltage Range", ...)` — so they can be checked rather than guessed.

**AND THE NAME IS QUOTED, SO THE VOICE RULES DO NOT APPLY TO IT.** Zilah's menu really does print *MSB waits for LSB*, which is personification we would never write; rewording it would falsify the one thing the line exists to get right. Print the name as the maker prints it. `validate_help.py` reads a menu line with that middle part taken out, so the style rules still judge everything we wrote ourselves and nothing we only copied.

Where a choice's own name is the problem — Grayscale's Startup state offers *Remember* — put the choices in that same exempt part, after a colon, rather than in the description:

    Menu — Startup state: Start, Stop, Remember — sets the playback state the module takes when a patch is loaded

**A COSMETIC MENU THAT EVERY MODULE IN A PLUGIN CARRIES GETS NO LINE.** Themes, skins, panel art, light and dark: these never answer the question somebody opens help with. Written per module they are pure repetition — dBiz would carry three identical theme lines on all 32 of its modules, 96 lines in which nothing is said, and they would be the ONLY thing under the menu heading for twenty of them. Leave them out.

Two exceptions. Where the theme IS the module — a blank panel whose whole function is to be looked at — it is the module's purpose and belongs. And where a plugin-wide menu item changes BEHAVIOUR rather than appearance, write it on every module that has it: Venom's *Lock all parameters* is exactly the thing that makes a module seem broken, because every knob refuses to move and the panel says nothing about why.

The absence of such a menu can still be worth a line where a reader would expect one — Bogaudio's ANALYZER-XL is the single module in that plugin without the panel submenu, and that is a fact about it.

**Where the menu is NOT the module's own, say so in the line.** These lines are gathered under a heading that reads "Right-click the panel for:", which is right for the large majority. Rack has four kinds of right-click menu — the module's, a knob's, a port's, and whatever a display carries — and a setting that lives on one of the others must say which, or the heading makes it confidently wrong.

One line per control, and **never a cap**. If you are running helper agents, do not give them a line limit: a run tonight set one at forty and it cost real content — 66 input jacks on one module ended up sharing a single catch-all line. There is no target length: a module with four jacks gets four lines and a module with fifteen controls gets fifteen. Length is not a cost here, because nobody reads the entry straight through — each line is clicked separately, and a line that covers two controls cannot be clicked for one of them. Group only where controls are genuinely identical, such as eight outputs that differ by number alone.

## A line is about the one control that was clicked

The reader clicked one jack, not the row it is in. So the line answers for that one: `The delayed signal at half the time set by TIME`, never `Three jacks carrying the delayed signal at a half, a third and a quarter`.

This is the most common way an entry goes wrong, because a manual describes a row in one sentence and copying its shape is the path of least resistance. Model277 has four output jacks — t, t/2, t/3 and t/4 — and an entry that gave t its own line and grouped the other three recited the group to somebody who had clicked one of them, and gave the wrong count as well.

So: no counting the controls, and no reciting a row. Where several controls really do the same thing, write the line as though about the one that was clicked and let the tags point all of them at it: `An attenuverter, -1 to +1, setting the level of the input on that row`, `One of the four inputs into the delay line`.

Counts are fine where they are a fact about the module rather than a list of what was clicked — "all four outputs carry the same shape" on a wave selector, "the sum, difference, maximum and minimum of its two inputs" on an arithmetic section.

## Every line stands alone

A line is heard on its own, with nothing before it and nothing after it. So:

- No word pointing at something the reader cannot see. Not "still", not "also", not "again", not "as VCO does", not "the same as above". If a module is a cut-down version of another, say what this one has, not what it lacks.
- No "up to the knob position" or any other sideways way of saying a thing. If the knob attenuates the CV, write that the knob attenuates the CV.
- Name the control outright and put its panel name first: `PHS — one per output, moving that output away from the fundamental`.

A line describing a group of identical controls — one per stage, one per band — is written as though about the one that was clicked: `PHS — where that wave sits in the mix`, not `PHS — four knobs, one per wave`.

## Never say where a control is

A line is reached by clicking the control it describes, so the reader is already pointing at it. "The large knob at the top", "the small knob under TIME", "the four columns down the right" — all of that is spent words, and on a spoken line the words are the whole cost.

Say what it does, not where it sits. `Sets the pitch, marked in volts; 0V gives C4 at 261.63Hz`, not `Large knob at the top, marked in volts — sets the pitch`.

The exception is a module's position in the rack, which is not about finding a control: an expander does say that it goes immediately to the right of the module it expands.

## Controls with no label, and labels used twice

Some controls have no name printed on them. Those lines simply start with what the control does, with no label and no location: `Sets the pitch, marked in volts`, `Modulates the width of the square wave`.

Some panels print the same word on two or three different things — GATE as a knob, an input and an output. Add the one word that separates them: `GATE knob`, `GATE input`, `GATE output`. The stand-alone rule wins over the bare-label rule, always: a line heard on its own must say which control it is about.

## A line for something that is not a control

A fact worth knowing that is not attached to any control goes on a line starting `Note — `.

**Write one wherever a module cannot be used without knowing something, and that is not a rare case.** This instruction used to read "use it rarely", which was wrong and cost the entries their most useful sentences. NYSTHI's Sussudio is a six-headed sample player that does nothing at all until you right-click a region and load a sample, and its entry never said so — because that fact belongs to no control, and the rule discouraged writing it. The reader who most needs help is the one looking at a module that appears to be broken.

So: **if a module does nothing until you do something, say what.** A file to load, a device to choose, an expander to place, a mode to leave. Then anything else that changes how the module is used and that clicking a control would never reveal — a gesture on a display, what an expander attaches to and on which side, a limit such as being mono only.

These lines are shown, with the `Note — ` stripped, when the module's title band is clicked. The prefix is an authoring mark that tells the generator the line belongs to the module rather than to a control; the reader never sees it.

## Never name the control either

A line is reached by clicking the control, and the note shows that control's name as its heading. So the line itself does not repeat it: write `Saturation on the saw wave`, not `SAT — saturation on the saw wave`.

The first line of an entry is the exception, because it is about the module rather than a control. So are lines starting `Menu — ` and `Note — `, where the prefix says where the thing lives rather than which control it is.

Where a label genuinely belongs inside a line — because the line mentions a different control, as in "V/OCT is added to it" — leave it. The rule is about the lead, not about the word.

## The panel is the authority on names

You still need the panel names, for two reasons: they tell you what the control is, and they are what a line refers to when it mentions a control other than the one being described.

Use the label actually printed on the panel, in the case it is printed in. The manual often calls a control something longer than the panel does; the person listening is looking at the panel.

Panel lettering is outlines, not text, so it cannot be read from the SVG. Render the panel and look at it:

    rsvg-convert -w 600 "<plugin res dir>/<Panel>.svg" -o /tmp/panel.png

`census/<Plugin>.json` in this directory has every model's parameter, input and output names as the maker configured them, which is usually enough to know what the controls are; the render settles what they are called.

**AND THE MODULE'S OWN ONE-LINE DESCRIPTION** is in the installed `plugin.json`, beside its name and tags. It is the text the module browser shows, and it is often the only statement of what a module is for: NYSTHI's Bitshifter describes itself there as "256 bits bitshifter with S&H and noise and inner LFO and VCO". Read it before writing a first line.

**THOSE NAMES ARE THE TOOLTIPS.** What Rack shows when you hover a jack is exactly this text, so where a maker has written a real sentence into a name — NYSTHI's Bitshifter has "Pulse in to switch between RND or VCO generators" — the module is documenting itself and the census already has it. Read the names before deciding a module is undocumented.

Some controls also carry a second line, in `paramDesc`/`inputDesc`/`outputDesc`. Only about 800 in the whole library do, and they are worth looking for because of what they usually say: normalling, and what a control does that its name does not convey. Venom, Stoermelder, Befaco, CountModula and Amalgamated Harmonics are the makers who wrote them.

**WHEN THE PANEL RENDERS BLANK, FIND A PICTURE OF THE RUNNING MODULE.** Some makers draw their controls at runtime rather than in the artwork — VCV's Chords, Reverb, Convolver, Compressor and Host, and most of JW-Modules — and rsvg-convert gives a blank rectangle for those.

The VCV library serves screenshots at **400 pixels wide, or 200**. Other widths 404; checked on 2026-09-11, when the 800 this spec used to name stopped working:

    curl -sL "https://library.vcvrack.com/screenshots/400/<plugin slug>/<model slug>.png" -o /tmp/panel.png

Verify you got an image and not an error page — `file /tmp/panel.png` — because a 404 saved to disk reads as a corrupt PNG rather than a failure.

Failing that, most makers keep screenshots in their own repository, usually under `doc/` or `images/`. **Those are often stale**: a JW-Modules run found four modules whose controls postdate the maker's own pictures. Where a repository image disagrees with the installed `plugin.json` and the census, the installed module wins.

**DRAW THE INDEX NUMBERS ONTO THE PANEL.** This is the single most useful thing you can do, and it turns "which jack is input 6" from inference into something you read off a picture. Inline the panel SVG and write each control's census index at its census position — parameters in one colour, inputs in another, outputs in a third — then look at the result.

`research/annotate.py` does this, and it is what made the NYSTHI port map possible; a run tonight wrote its own version at `scratchpad/sheet2.py`. Either is a few minutes of work that removes the whole class of invisible tagging error.

RENDER EVERY PANEL. It is not a check on the manual, it is a source in its own right: manuals go stale, and the panel is what the listener is sitting in front of. On Bogaudio — a careful maker with a good manual — the manual said ATACK and DECAY where the panel says ATTACK and RELEASE, and DECTECT where the panel says DETECT. Where they disagree the panel wins.

## Match the version that is installed

A maker's `main` branch documents the version they are working on, not the one in the rack. Check the installed version in the plugin's `plugin.json` and read the matching tag.

This is not pedantry. Fundamental's installed build is 2.6.4 while the website documents 2.6.5, which changed a filter's range; that was caught by checking. Venom used to be the other example here — installed 2.15.0 against a 2.16.2 `main` whose documentation had been restructured — and it has since been updated, so the installed build is now 2.16.2 itself. **Which is the point: a version written into this file is a fact about one afternoon.** Read the installed `plugin.json` rather than trusting any version named in the briefs, including this one. Cite the manual URL as `source`, and say in your report which version you actually read.

## Read the maker's source where there is any

Most makers publish their code, and it answers what the manual leaves out. Rack's own Core manual says nothing about the Audio module's level knob, nothing about its left-to-both normalling and nothing about the voltage thresholds in the CV-to-MIDI modules; all of that came from the source, and those are now the most useful lines in that entry.

Use it for what a manual cannot say: exact ranges and thresholds, what a control does at its extremes, what an undocumented button is for, what a port outputs when nothing is patched. Cite the manual as the `source` — the code is corroboration, not the record — and say in your report which facts came from it.

NYSTHI is the counter-example and the reason to check rather than assume: its repository holds a README, a screenshot and a changelog, and no code at all.

**AND WHERE THERE IS NO SOURCE, LOOK IN THE INSTALLED PLUGIN FOLDER.** NYSTHI ships a 204KB `CHANGELOG.md` beside its binary, and it is not a changelog in the ordinary sense — for many modules it is a control-by-control description written by the maker, including menu items and lamp colour legends. It describes **71 of the 149 installed NYSTHI modules**, and it is the only documentation those modules have anywhere:

    ~/Library/Application Support/Rack2/plugins-mac-arm64/<Plugin>/CHANGELOG.md

`strings` on the binary is the last resort and it works: it yielded the Programmer's menu wording verbatim, including one option the changelog omits. Check the plugin's own folder for any `.md`, `.txt` or `res/` documentation before concluding a maker has none.

## Voice

Plain, factual, second person where a person is addressed at all. No enthusiasm, no "simply" or "powerful". Concrete verbs. One idea per line.

**Nothing does what only a person does.** A control does not sit, live, listen, want, know, decide, watch or wait. This is not only a matter of taste: the personified verb is almost always standing in for the fact that has not been worked out. "PHS — where that wave sits in the mix" was hiding that PHS sets the phase of that wave, and somebody reading it learned nothing.

When a line resists, it is usually because the source did not say what the control does. Go back to the manual or the panel and find out, or leave the control out. Do not reach for a verb that sounds like an explanation.

Write in our own words. Do not copy the maker's sentences; read the manual, then say what it means.

## Units, because the lines are spoken

Write units exactly like this, whatever case the panel prints them in: `0-10V`, `3HP`, `-36dB`, `100Hz`, `1ms`, `2 seconds`. The plugin rewrites these for the synthesiser, and it matches that spelling. Panel casing governs the NAMES of controls, not the units.

## An expander names the module it expands

An expander's first line says which module it expands and that it sits immediately to the right of it. If one of its controls is completed by a knob on the base module, say so. That is not leaning on another entry — it is what the module actually is.

## Two rulings on families

**A time or rate input is `cv`, even when it doubles per volt.** This covers tempo and BPM jacks, and equally an envelope's rise and fall CVs that halve or double the stage per volt. Octave scaling on a time control is still a time control. A BPM jack scaled two to the power of volts is arithmetically the same as 1V/octave, but the colour is there to answer "what do I patch here", and nobody patches a keyboard into a tempo input. Reserve `pitch` for a port whose voltage sets an audible pitch or a filter frequency.

**Anything that tracks 1V per octave is `pitch`, filter cutoffs included.** The family answers "what do I patch here", and a cutoff that tracks 1V/octave takes the same cable a note pitch does. Reserve `cv` for inputs with no such scaling.

**A port whose meaning depends on what is plugged into the module gets no family at all.** Host's audio jacks carry whatever the hosted plugin sends; a mult carries whatever you give it. The rule Chris set stands: no colour says nobody knows, a wrong colour lies.

## When the manual does not say

Leave the control out rather than guess. An entry that covers eight of ten controls is useful; one that invents the other two is not.

## Tagging: the index is not the position

A tag says which line covers which jack or knob, by index. **Index order is not panel order, and assuming it is will reverse an entry without looking wrong.** NYSTHI's Model277 numbers its four output jacks from the bottom up: index 3 is the top jack, index 0 the bottom. An entry tagged from the numbering alone told somebody clicking the top jack about the bottom one.

`census/<Plugin>.json` carries `inputPos`, `outputPos` and `paramPos` beside the names — the centre of each control on the panel, as x and y, with y increasing downwards. Work out the order from those, then check it against the rendered panel. Never from the index.

**A tag points at a line that describes THAT control, or at nothing.** Where no line describes it, leave it untagged: the panel then says "nothing here describes this one yet", which is true and useful. Pointing at the nearest line instead is the same fault as a wrong colour — it reads perfectly and it lies. Bogaudio's NOISE had all five of its noise outputs pointing at a line about the polyphony menu, so clicking the blue noise jack answered a question nobody asked.

When you find a control with no line, the fix is usually to write the line, not to stretch a neighbouring one.

## A control that does nothing gets a line saying so

**An untagged control does not fall silent — it falls back to the maker's own tooltip**, shown marked as theirs. So leaving a dead control untagged hands the reader the one piece of text that has been proved false. LadyNina's small knob under Boost is never read by the module; untagged, clicking it answers "Drive CV".

So: a control that is on the panel, can be clicked, and has been PROVED to do nothing gets a line and a tag, saying that. `Nothing in the module reads this control; the boost CV is applied at a fixed 20dB per volt.` That is not a failure to describe it — on a panel with a dead knob it is the most useful line in the entry.

The distinction is proof, not suspicion. A control you could not work out still gets no line: silence beats invention. A control whose code you have read and which reads nothing is knowledge.

A control that can never be clicked — declared with no widget, or covered by another control at the same position — still gets nothing, because no click will ever reach it.

**How to tell: no position in the census means no line.** The positions come from the module widget's own `ParamWidget` and `PortWidget` children, and `Help.cpp` resolves a click by walking those same three lists. So a control missing from `paramPos`, `inputPos` or `outputPos` cannot be reached by this feature at all.

That holds even where a maker DOES draw the control. Several plugins put working controls inside one custom display widget — a step grid, a slider bank, a pattern strip — which is a single widget with no `ParamWidget` per control. A reader can turn those in the rack and can never click one for help. They get no line here; where such a control matters to using the module, a `Note — ` line is the place for it.

## Where we contradict the maker, the control's own line says so

The reader clicked that knob and is looking at that label. A line that quietly describes something other than what the panel says reads as our mistake. So the line describes what the code does and ends with a short clause naming the conflict: `Sets the decay of the snare body, despite the panel printing NOISE.`

One clause, not a paragraph — the full account belongs in the module's `notes`. And this is for a real conflict with something the reader can see or be shown: the panel lettering, the maker's tooltip, the published manual. Not for every place our wording differs from theirs.

Two habits that catch the rest: after tagging, read the tags back as sentences ("the top output is the full delay time — is it?"), and be suspicious whenever a tag map looks like a tidy run of 0, 1, 2, 3.

## Format

`research/help/<Plugin>.json`:

```json
{
  "plugin": "<plugin slug>",
  "source": "<the manual URL that was read>",
  "read": "<YYYY-MM-DD>",
  "modules": {
    "<model slug>": ["first line", "LABEL — what it does", "..."]
  }
}
```

Every model slug in the installed `plugin.json` must be present. Run `python3 research/validate_help.py` before finishing; it checks coverage and the rules above that can be checked mechanically.
