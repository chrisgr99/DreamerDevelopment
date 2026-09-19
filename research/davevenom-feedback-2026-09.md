# DaveVenom feedback — Clarity, Test Gear, Help, Dark

Collected 2026-09-19 from the VCV community forum. Thirteen posts by DaveVenom across three threads.

Threads:

- Clarity: a module to enhance Rack UX — https://community.vcvrack.com/t/26103
- Clarity — ports and cables coloured by signal type — https://community.vcvrack.com/t/26128
- Quick reference for input port properties (Idea) — https://community.vcvrack.com/t/26068

His notes on process: all testing was on Windows; he asks whether the Clarity and Test Gear notes belong in a separate thread, and whether you would prefer them as GitHub Issues.

## Part 1 — Action list

### From post 32, 2026-09-17 (the long list)

https://community.vcvrack.com/t/26068/32

#### General

- Remove the screws. They currently overlap faceplate graphics; reformatting the faceplates is the alternative, removing them is simpler.

#### Test Gear

- Switch and Attenuverter should work on outputs, not only inputs.
- Consider renaming Switch to Mute.
- Note is redundant with VCO — remove Note.
- VCO is not voltage controlled; rename to Oscillator / Osc.
- DC Level and Volt/Oct do the same thing at different scales. Combine into one tool with a scale option, Volts or Pitch (assuming 0V = C4, v/oct). Suggested name: Constant Voltage.
- Voltmeter Peak should show both min and max, or let the user select current, min or max.
- Scope: could not get the speed slow enough to be useful with LFO signals, suspects other issues he could not identify. Resizing is temperamental — hard to grab the edge.
- Bug: after attaching a test gear widget to a port, click-and-drag on that port no longer creates a cable. Once he clicks and drags away from the port, subsequent drags work again.

#### Help

- Scroll sometimes appears when needed and sometimes does not; no pattern identified.
- Help does not recognise controls or ports that are not direct children of the faceplate. Venom Envelope Factory hides unused stages by putting each stage's controls and ports on a child faceplate; those are the ones missed.
- Multiple audio readings can be started and overlap. Starting a new one should terminate any prior reading.
- Clicking a line to speak it is counter-productive: a user who can see well enough to pick the line probably does not need speech, and a user who cannot identify the line cannot use it.
- If speak is enabled, option-clicking a module, control or port should speak the entire help automatically, with navigation to move forward and back through lines and to stop.
- Help popup and speak should be independently enabled, so option-click can speak, open the popup, or both.
- Let the user configure which key and mouse click combination activates help.
- Help should not be compiled into the plugin — maintenance is untenable at the pace new modules appear. Store each plugin's help file somewhere in the VCV installation and load on demand, so users can add and correct help without recompiling.
- He dislikes the current YAML format: attributes for one control or port are spread through the file. Wants a hierarchical arrangement — all module-level attributes in one place, all attributes for each control or port collected together.
- Given a hierarchical structure he would prefer JSON: editors do bracket matching, and it suits on-demand loading.

#### Dark

- VCV modules that already have light and dark themes still get some graphical components modified by Dark. Example: VCV Audio.
- Some faceplate components lose contrast after Dark is applied. Example: VCV Host, whose horizontal linework is lost.

He says he would definitely use Dark if these are resolved.

### From the earlier posts, 2026-09-01 to 2026-09-08

Some of these were addressed in 2.0.2 and 2.0.3 — check before acting.

#### Bugs reported 2026-09-01 (post 7)

- Cable trace does not work if the cable trace handle overlaps a port.
- Cable trace handle does not appear on some cables; correlated with certain modules, Venom AD/ASR consistently.
- The hidden state of widgets is not honoured when drawing consistent knobs and colour-coded jacks. Example: Venom Envelope Factory.

#### Draw pointer behaviour (post 7)

- Right-click on a parameter widget (Windows) shows the parameter value graphic, competing with the native VCV popup menu. It should not appear in that case.
- Clicking a parameter widget activates the mouse movement trace, requiring an extra click away from the parameter to disable it.

#### Feature requests (post 7)

- Terminology is inconsistent — Clarity says "Jack", Test Gear says "Terminal". He prefers "Port", then "Jack".
- Put the two Clarity context menu options on the faceplate as parameters.
- Separate mouse movement trace from the draw pointer option.
- Allow the user to override a port's category.
- Allow users to define their own colour and port categorisation schemes via a JSON file.

#### Follow-ups 2026-09-03 (posts 10, 19, 25)

- GitHub releases page showed 2.0.1; 2.0.2 was only reachable through the Actions tab, as a zip rather than a full release.
- Cable trace should switch off automatically on the first normal left click after activation, rather than requiring the handle to be found again.
- Scrolling with the mouse wheel pops up the Clarity parameter window as parameters pass under the cursor. Should not happen.
- No idea how to format the "rules" entry. Write the default rules into colours.json so the file is self-documenting, and add a context menu option to restore defaults.
- "Show popup on adjust" now looks redundant against the VCV View option "Show tooltips". He prefers the VCV tooltips because they also cover ports and lights and show widget descriptions — Venom uses the parameter description to show whether a parameter is locked, and port descriptions to show normalled connections. Suggestion: replace it with "Show custom popups" that activates on hover over any widget, carries the extra port and light information including descriptions, and lets the user set size and possibly colour.

#### Test Gear and Clarity interaction 2026-09-07 to 09-08 (posts 3, 9, 11)

- If Test Gear is removed while no Clarity is present, the widgets stay on screen but are dysfunctional. They should be removed. With a Clarity present they are removed correctly.
- With more widgets available, the instruction text now interferes with the "Monitor out" label.
- If a Clarity has never been placed, Test Gear implements all the Clarity default GUI changes. Once a Clarity has been placed and then removed, Test Gear no longer implements the Clarity features.

#### On the help project generally, 2026-09-15 (post 21)

Not defects — his framing of where this should live.

- Submit a formal feature request to VCV; he thinks it belongs in the core platform. His hunch is Andrew would not put it in Rack 2, but Rack 3 is being worked on.
- Storage: a JSON file with a well documented structure, ideally shipped with each plugin, but also providable by third parties. Open question — if a developer ships a bare-bones file and someone else prepares a fuller one, should the user be able to choose which to display?
- Complex modules will have long descriptions: the popup should scroll, perhaps resize, perhaps open in a separate persistent window. Hierarchical popups for very long text, with a link in the main popup opening a child modal above it.
- Some controls and ports change function depending on other options. The description could list all possibilities, but better if the module could report which description is current, with an indication that the function is changeable.

## Part 2 — Source posts, verbatim

### Clarity: a module to enhance Rack UX — post 7, 2026-09-01

https://community.vcvrack.com/t/26103/7

Really impressive work! My favorite feature is the Clarity "Draw pointer (for screen recording)" option.

I have some Clarity bugs and feature requests for you. I haven't spent much time with the Test Gear.

Bugs

- Cable trace does not work if the cable trace handle overlaps a port
- Cable trace handle is not appearing on some cables. Problem seems to be correlated with certain modules. Venom AD/ASR consistently has problems
- You are not honoring the hidden state of widgets when drawing your consistent knobs and color coded jacks. Take a look at Venom Envelope Factory for example.

Awkward "Draw pointer" behavior that is close to being a bug

- Right click (windows) on parameter widget shows your parameter value graphic, which competes with native VCV popup menu. I think your parameter display should not appear in this case.
- Clicking on parameter widget activates the mouse movement trace, which requires an extra mouse click away from the parameter to disable. I don't think mouse movement trace should appear when clicking on parameter widget.

Feature requests

- Be consistent with terminology between Clarity and Test Gear. Clarity uses "Jack", Test Gear uses "Terminal". I prefer "Jack". I feel that "Port" is another option - that is the term I prefer. But "Jack" is good.
- You only have two Clarity context menu options. Why not make them all parameters on the faceplate?
- I think the mouse movement trace should be its own option, and not bundled with the draw pointer option.
- Perhaps allow user to override port (jack) category
- Perhaps allow users to define their own color and port categorization schemes via a json file.

### post 10, 2026-09-03

https://community.vcvrack.com/t/26103/10

Wow - it is really rounding into shape!

(quoting 2.0.2 release note) The link points to the main page where the 2.0.1 release is listed. I had to go to the Actions tab to get the 2.0.2 version, and that is the zip file, not the full release. It is weird. The Action details show it as the 2.0.2 version release as being successful, but I can't find it.

(quoting the menu options question) Yes, and YES! I have dabbled in tutorial videos, and I wished for this exact functionality. I am sure I am not the only one that would like this. It could be good for performance live broadcasts as well.

(quoting the JSON rules note) Cool. I looked for the entries, but failed to find them. Then again, I am operating on only a couple hours sleep, so maybe that is on me.

(quoting the per-model override note) I like it!

I have a couple more observations:

- I wonder if the cable trace should be automatically turned off on the first normal (left on Windows) mouse click after it is activated. I don't see any advantage to having to hunt for the cable handle to turn it off.
- When scrolling with the mouse wheel the Clarity parameter window is popping up as parameters pass under the cursor. That should not happen.

### post 19, 2026-09-03

https://community.vcvrack.com/t/26103/19

No, I managed to install 2.0.2 and find the Signal family menu item. But I never saw the DreamerDevelopment folder with the colours.json file. I found it now with 2.0.3.

I have no idea how to format the "rules" entry.

It would be nice if the default rules were entered into the colours.json file. Then it would be sort of self documenting. You could have a context menu option to restore default rules.

### post 25, 2026-09-03

https://community.vcvrack.com/t/26103/25

Great!

I was surprised to see you eliminated the mouse movement trace. But I would never use it, so I won't miss it.

The "Show popup on adjust" option seems redundant with the VCV View "Show tooltips" option now that you adjusted the size to match the size of the VCV tooltips. I like the VCV tooltips better in that they work for ports and lights as well, and also show the widget description when populated. I use the parameter description in my Venom plugin to show whether a parameter is Locked, and for ports it displays any normalled connections.

Maybe replace "Show popup on adjust" with "Show custom popups", activate when hovering over a widget, add the extra info for ports and lights, including the descriptions, and let the user set the size and maybe the color? If the default VCV popup size works for you, then maybe you don't need your own version.

### post 27, 2026-09-04

https://community.vcvrack.com/t/26103/27

(on the tooltip contrast explanation) So not even the dark high contrast UI setting helps? Interesting. OK, I see why you have your custom popups now.

(on the different-questions distinction) The port tooltip also shows current voltage value when patched, as well as current sources or destinations, depending on type. So they are not strictly static. I should think it could be useful to replicate all those tooltips with your custom design to make them more legible for you (and others)

Even if the tooltip info is static, it can still be very useful. Faceplate labels may often be cryptic due to lack of space. The tooltips are an opportunity to provide more detailed information about the parameter or port or light. With Venom modules I try to be concise but fully descriptive in my names that show in the tooltips.

I have to confess that I have not been good about catering to the color blind population. The small button values on my Venom modules are color coded, without any other distinguishing visual characteristic. I hope the tooltips showing the value in words is adequate for those that cannot distinguish the colors.

### Clarity announcement thread — posts 3, 9, 11, 2026-09-07 to 09-08

https://community.vcvrack.com/t/26128/3

Your report was not ignored. @chrisgr99 thought it had been resolved.

I saw a dramatic reduction in TestGear CPU usage in recent builds on Windows. Did you not see an improvement on Linux? Or have you not installed the latest version?

https://community.vcvrack.com/t/26128/9

Ah, yes.

If Test Gear is removed and there is not a Clarity present, then the widget(s) remain on the screen, though they are dysfunctional. I agree they should be removed.

If Clarity is present when Test Gear is removed, then the widgets are removed properly.

One other issue: Now that the number of available widgets has increased, the instruction text interferes with the "Monitor out" label.

https://community.vcvrack.com/t/26128/11

I saw that behavior a while ago, but hadn't been able to reproduce it until recently.

I think if you never place a Clarity, then Test Gear will implement all the Clarity default GUI changes.

But once Clarity has been placed and then removed, then Test Gear no longer implements the Clarity features.

### Quick reference for input port properties — post 21, 2026-09-15

https://community.vcvrack.com/t/26068/21

This is a great idea (set of ideas) ! Definitely there should be a formal feature request to VCV - it really belongs as part of the core VCV platform. It would be great to have extensive pop-up help for each module / port / control / light. Getting people to read manuals is like pulling teeth. But having built in help is the lowest barrier possible for self education.

My hunch is Andrew would not put this in VCV 2. But maybe it can be incorporated into VCV 3, which is actively being worked on. In the mean time your plugin can be invaluable until then (maybe for ever if Andrew declines to implement).

Some things to think about:

- Where is the info stored? Perhaps a JSON file with well documented structure. Ideally it should be part of each plugin distribution, something each developer can provide. But also something that can be provided by third parties in cases where the developer fails to do so. Then there is the question of what do do if the developer provides a bare bones file that is missing important information, and someone prepares a better, more complete version. Should there be a way to select which version to display?
- Some modules are exceptionally complex, and descriptions could become quite long. The pop up should be scrollable, perhaps resizable. Perhaps also an option to open in a separate window that persists. For really long descriptions hierarchical pop-ups could be used. The main pop up could have a link that when selected opens a child modal popup above.
- Some controls / ports change their function depending on how other options are configured. The description could list all the possibilities. But it would be cool if the module could inform which description is relevant at the time so the correct info would be displayed, along with some indication that the functionality is changeable.

### post 32, 2026-09-17 — the long list

https://community.vcvrack.com/t/26068/32

Kudos, high praise, much respect for all the work you have been doing with Clarity, Test Gear, Help. I am going to list things I don't like or think should be improved, but don't take offense. Calling out all the things I like and am impressed with would be exhaustive to write, and to read!

I will put notes for Clarity, Test Gear, and Help all in this post. Perhaps Clarity and Test Gear notes belong in a separate thread. Let me know if you prefer this info as GitHub Issues.

All my testing was on a Windows machine.

General

I think your modules would look better without the screws. As it is the screws currently overlap elements of your faceplate graphics. You could reformat your faceplates a bit to make the screws look better, but it seems simpler to just remove the screws.

Test Gear

- I think Switch and Attenuverter would work well on outputs, not just inputs
- Perhaps Switch should be renamed Mute?
- Note is redundant with VCO. I think Note should be removed.
- VCO is not really voltage controlled, so should it simply be called Oscillator, abbreviated Osc?
- DC Level and Volt/Oct both do the same thing, but just use different scales. Perhaps combine them into one with an option to choose scale of Volts or Pitch (assuming 0V = C4, v/oct). Tool could be called Constant Voltage
- It would be nice if Voltmeter Peak showed both min and max. Or allow user to select current, min, or max.
- I struggled to configure Scope to work effectively with LFO signals. I couldn't get the speed slow enough to be useful, but I think there are other issues. I'm just not sure what they are. Also resizing was very temperamental. I struggled to grab on to the edge to resize.
- After attaching a test gear to a port, click and drag on the port no longer creates a cable. Once I click and drag away from the port, then subsequent click and drag operations successfully create a cable.

Help

- Scroll sometimes appears when it is needed, sometimes doesn't. I haven't figured out a pattern yet.
- Help does not recognize controls or ports on my Venom Envelope Factory that are not direct children of the faceplate. Envelope Factory hides stages that are not needed by placing all the controls and ports for a stage on a child faceplate that can be hidden or shown. These are the controls that are not being recognized by help.
- It is possible to instantiate multiple audio readings that overlap. Ideally only one should be allowed at one time - starting a new one should terminate any prior reading.
- Clicking on a line to speak seems counter-productive
  - If a user can see well enough to determine which line to click, then speak probably isn't needed
  - If user can't identify the line(s) to click, then it is kind of useless
- I think if speak is enabled, then simply option clicking the module/control/port should automatically speak the entire help.
  - navigation options could be added to scroll forward, backward through individual lines, and/or stop the speaking
- Help popup and speak could be totally separate options that can be enabled/disabled independently. Option clicking an item could then speak, or open the popup, or both.
- Maybe allow the user to configure what key and mouse click combination is used to activate the help.
- I am surprised the help must be compiled into the plugin. Maintenance seems untenable with the pace that new modules are being created. Wouldn't it be better if each plugin help file was stored somewhere in the VCV installation and loaded on demand? Then users can add, correct help without recompilation
- I really dislike the YMAL format you currently have. It is hard to correlate all the various attributes that are spread throughout the file to their corresponding control or port. It would be much easier to read and maintain if the file were arranged hierarchically. It would be much easier if all the module level attributes are collected in one place. Likewise, all the attributes for each control or port should be collected together.
- Assuming you adopt a hierarchical structure, then I think JSON would be easier to maintain directly. There are plenty of text editors that can do bracket matching to assist. JSON would also be more conducive to on demand loading.

Dark

I like the concept, but I think there are issues with the execution. If these issues get resolved, then I definitely would use it!

- I don't understand why VCV modules that have light and dark themes still have some of the graphical components modified by Dark. For example, the VCV Audio module.
- Some faceplate components don't have enough contrast after Dark is applied. For example, the VCV Host has horizontal linework that gets lost after Dark is applied.
