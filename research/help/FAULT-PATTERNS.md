# What keeps turning out to be wrong

Every fault below was found by an agent writing this library, in a real shipped plugin, on one day. None of them is exotic and none was found by accident: once you know a shape, you see it. Read this list before you start a plugin and check for each one as you read the source. A hit is worth a clause in the affected control's own line, and the full account in `notes`.

## Controls that do nothing

- **A jack configured, drawn on the panel, and read nowhere in `process`.** Say so on its line. Untagged it would fall back to the maker's tooltip, which is a lie the help would be repeating.
- **A jack configured with no PortWidget at all.** No cable can reach it. It gets no tag and no line — but the *other* jacks may be renumbered around it, so read the widget constructor, not the enum.
- **A knob whose effect saturates well before the end of its travel**, leaving a provable dead zone. State what the control does; leave the derived figure for `notes`.
- **A control read only in one mode**, inert in the others. Say which.
- **A parameter that exists only on a menu slider or inside a custom display**, with no `ParamWidget`. It can never be clicked for help, so it needs a `Note —` or a `Menu —` line, never a tag.

## Controls wired to the wrong thing

- **A port read through the wrong enum constant** — a ParamId used to index `inputs`, say. One jack is read in place of another and a second is ignored entirely. Nothing on the panel shows this; only the code does.
- **Two controls wearing each other's tooltips**, usually a left/right or A/B pair where `configParam` was copied and one name not changed.
- **`configParam` called twice for one control and never for its sibling.** The sibling then has no ParamQuantity and **cannot be clicked at all**.
- **`configOutput` called for the first jack of a row and never for the rest**, leaving every other tooltip empty.
- **Outputs wired across** — the jack printed X carrying y, and so on.
- **A `configSwitch` whose choice names are in the reverse order of its values**, so the tooltip reads "Off" in the position that turns the thing on.

## Signals that are not what the panel implies

- **A gate or trigger input tested against a threshold no ordinary signal reaches.** One module tests for exactly 10V, so a 5V gate does nothing at all.
- **A trigger read as a level rather than an edge**, so a held gate pins it, or a slow ramp retriggers it every sample.
- **A CV jack that replaces, multiplies, or `setValue`s its knob rather than adding to it.** A patched cable sitting at 0V then silences the control, and the knob is dead while the cable is in. This is the single most common surprise in the whole library.
- **An output with a constant offset baked in**, so it does not swing where the panel says.
- **A CV whose full sweep takes a different voltage than the maker documents** — 1V covering a whole knob is common where the code forgot to divide.

## Things that are silent or inert on arrival

The brief calls this the most useful thing in the dataset, and it keeps being true. Look for it every time:

- a control whose **default value makes the module silent** — a level at zero, a gate switch off, a knob whose centre is a dead zone;
- a **default outside the control's own configured range**;
- **`process` returning early** until some jack is patched, so the module does nothing until a particular cable is in — sometimes an *output* cable;
- an **expander inert** until its parent sits on the correct side;
- a **file, device, or tuning that must be loaded** before anything happens.

## Lights and displays

- **A light configured and set every sample but never given a `LightWidget`** — it can be neither seen nor clicked.
- **A lamp fed an already-clamped value**, so it never reaches full brightness.
- **Lamps that brighten the wrong way**, e.g. an over-voltage warning that lights for negative voltages only.

## Placement

- **Index order is very often not panel order** — sometimes reversed, sometimes scattered. Tag from the census positions and the rendered panel, never from the enum.
- **Two parameters sometimes share one panel position**, one hidden at a time by a menu choice. `Help.cpp` skips hidden widgets, so both are reachable and **both** should be tagged.
- **A bank of controls inside one custom widget** shares one position or has none. Those need `Note —` lines.

## When the panel, the tooltip and the manual disagree

The lines follow **the panel**, because that is what the person is looking at, and they name the disagreement. Where the code disagrees with all of them, the code wins and the conflict goes in `notes`.

## Settling which commit shipped

Makers routinely leave the version string alone for dozens or hundreds of commits. Ways that have worked:

- `nm` for a symbol that exists on only one side of a change;
- `strings` for a menu name, a resource path or an error message;
- hashing the installed `res/` art against each commit's blob;
- comparing the installed `plugin.json` byte for byte;
- disassembling the one behavioural difference that remains — a write-back that is present in one commit and absent in the other.

Say in your report which commit you used and how you settled it.

## A caution about `check_numbers.py`

A low rate is not by itself evidence against a manual. Small constants — a 0.1V trigger floor, a 2V threshold — are materialised by the compiler inside instructions as a `mov`/`movk` pair or an `fmov` immediate, and never appear as stored literals. Say what the rate was and which misses are of that kind.
