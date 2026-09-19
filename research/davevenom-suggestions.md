# Suggestions for answering DaveVenom

Written 2026-09-18 against the code as it stands: Clarity and Test Gear at 2.0.9 plus the unreleased knob arcs, Help at 2.0.1. The feedback itself is in davevenom-feedback-2026-09.md beside this file.

## How to answer him

Thank him, and say yes to GitHub Issues: a list this long is easier to track one item at a time, and he can see when each one closes. I would open the issues ourselves from this list, so he does not have to type it again, and link them in one forum reply. Keep Clarity and Test Gear in the Clarity thread, and Help in the Help thread, as he suggests.

Tell him which earlier reports are already fixed, below, so he is not re-testing them.

## Already fixed, to tell him

- Test Gear widgets left on screen after Test Gear is removed with no Clarity present: fixed in 2.0.6.
- The instruction text running into the Monitor out label: fixed in 2.0.6.
- Test Gear applying Clarity's changes when no Clarity had ever been placed, and not applying them after one was removed: fixed in 2.0.7. Test Gear no longer applies any of Clarity's changes on its own.
- The releases page showing an old version: every version since 2.0.7 is a full release, and 2.0.9 is the latest.
- The mouse movement trace is gone, which settles his two points about it.

## Clarity: done, so not to be raised again

Checked against the code and changelog on 2026-09-19. He has seen these already.

- Drawing over hidden widgets (2.0.2, his report).
- The trace handle refusing a press next to a port (2.0.2).
- The value readout drawing over Rack's right-click menu (2.0.2, his report).
- The popup while the wheel scrolls past controls: it now shows only when the scroll changed a value.
- Clarity's options as buttons on the panel.
- Overriding a port's type from its right-click menu; custom colours and rule sets.
- The mouse movement trace, removed.
- "Terminal" survives only in code comments.

## Clarity: decided and built (2026-09-19, not yet released)

1. **Bypass turns Clarity off**, from another source: a bypassed Clarity does nothing, as though removed, and keeps every button's setting. Another Clarity in the rack goes on working.
2. **No screws** on Clarity, Test Gear or Dark.
3. **Pop-up text size** in the right-click menu for the knob readout, which stays above the control. And a separate new button, **Tooltip readability**: Rack's own tooltips drawn larger, in high contrast, directly below the control; text size and white-on-black or classic light yellow in the menu. This answers his "Show custom popups" without changing Clarity's pop-up.
4. **The trace** already ended on a click on bare panel; a click on empty rack now ends it too. A click on a control or port keeps it lit.
5. **The trace handle missing on some cables** was already fixed in 2.0.2: the handle's hit area was far smaller than the drawn handle, and which cables it failed on depended on the angle they left their port, so it looked like certain modules.
6. **Default rules** were already written into colours.json; the file now opens with an explanation of every rule field, and **Restore default rules** puts the rules back without changing the colours.

## General

- **Screws: removed** (2026-09-19).

## Test Gear

- **Switch renamed Mute: agree.** It is what the thing does.
- **Mute and Attenuverter on outputs: agree for Mute, and possible for Attenuverter.** A mute on an output takes out every cable leaving it, which is easy. An attenuverter on an output has no one signal to scale, because each cable leaving it is read by a different module. It could scale the signal at every destination together, which gives him what he wants.
- **VCO renamed Oscillator, and Note folded into it: agree.** One oscillator, with its frequency shown in hertz or as a note.
- **DC Level and Volt/Oct combined as Constant Voltage: agree,** with the readout switchable between volts and a note name, nought volts being C4.
- **Voltmeter peak showing minimum and maximum: agree.** Show both; it costs one more number.
- **Scope too fast for an LFO: confirmed, and it is a real limit, not a setting.** The scope reads 4,096 samples, about 85 milliseconds at 48 kilohertz, so one cycle of a 1 hertz LFO can never fit. It needs a longer history for slow signals: keep one sample in every so many, so the window can reach several seconds, with a rolling display at the slow end as a hardware scope does. He also finds the resize edge hard to grab; widen the area that catches the mouse.
- **A port with a Test Gear widget on it will not start a cable on the first drag: a bug.** The widget is taking the first drag. It needs reproducing; the fix is to let a drag that starts on the port itself go to Rack.

## Help

- **Controls on a hidden-or-shown inner panel are not found: confirmed.** Help asks the module for its controls and jacks, which gives only the ones attached directly to its panel. It also measures their positions relative to whatever they are attached to. Envelope Factory attaches each stage's controls to its own inner panel. The fix is to search the whole widget tree, convert positions to the module's panel, and skip anything inside a hidden panel. Clarity's knob and jack drawing has the same fault, which is his earlier report about hidden widgets, so one fix serves both.
- **Readings overlapping: confirmed, on Windows only.** On the Mac and Linux a new reading stops the old one; on Windows the old one is left to finish and the new one queues behind it. Keep the Windows speech process and stop it before starting another.
- **Option-click speaking the whole entry: not adopted.** Option-click stays as it is, and clicking one line at a time to hear it stays the way to listen. Tell him it is a deliberate choice, for the same reason as the next item: Chris reads what he can and clicks a line to hear it when he needs to.
- **The popup and speech switched on and off separately: not adopted.** They stay together. Explain in the reply that this is what works best for Chris's eyesight: he can partly read the text in the popup, and sometimes wants a line spoken as well, especially a long one. So the popup is always there to read, and speech is there on a click when it helps.
- **A choice of key and click for help: adopted.** Option-click stays the default; the Help module's right-click menu lets the user choose another modifier and mouse button combination. A choice that clashes with Rack or another plugin is allowed — the user decides whether the clash matters to them.
- **The scroll bar appearing sometimes and not others: needs a module where it happens.** Ask him for one.
- **Help loaded from files, in JSON, one block per control: adopted as he proposed.** The design is DreamerHelp/design/help-database.md. A maker can ship a JSON help file inside their own plugin, one per module, and Help uses it: the maker's text wins wherever it exists, and DreamerHelp's own database fills in whatever the maker's file leaves out. The database is generated by analysis, so every module that can be analysed has help from the start, and makers' files are never merged into it. That answers his question about a bare file against a fuller one: the maker's entries win, and the database covers the rest. The format puts each control's text beside it, as he asked. Files are read when a module's help is first opened, not at startup, and a reload menu item lets a maker see edits at once. The reply can thank him for the idea and invite him to be the first maker to ship a file, for Venom.
- **Controls whose meaning changes with other settings:** Help could show the module's own tooltip description beside our text. The module writes that description at runtime, so it is current when the text is not.
- **A feature request to VCV:** worth doing once the file format has settled, pointing at the plugin as a working example. It is not urgent.

## Dark

- **VCV's own modules still altered: intended, but he is right.** Dark tones down the white patch behind VCV's output jacks, even on panels that have their own dark artwork. A maker who has drawn a dark theme has decided how it should look. I would leave any module with its own dark theme completely alone, and offer the output-patch toning as a menu option, off by default.
- **Lines lost on VCV Host: needs looking at.** Thin rules on the light panel are being darkened with the panel instead of being kept as lines. The rule that keeps small dark shapes as light lettering should apply to thin lines as well.

## Order I would take them in

1. The two confirmed bugs that affect his own modules: nested controls in Help and Clarity, and the Windows overlapping speech. He is a maker with a large plugin, and those are the ones he will test first.
2. The Test Gear port-drag bug and the scope's slow-signal range.
3. The Test Gear renames and merges: Mute, Oscillator, Constant Voltage, minimum and maximum. They are small and can go out together in one release.
4. Help from files, as already designed in help-database.md. This is the largest, and it changes how all the help is maintained, so it is worth doing before more help is written in the old format.
5. Dark's treatment of themed makers, then the contrast rule.
6. The Clarity items still to check, as each is confirmed.
