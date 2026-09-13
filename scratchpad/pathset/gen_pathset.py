import json, os

def rng(a,b): return list(range(a,b+1))
def tag(pairs):
    d={}
    for idxs,line in pairs:
        for i in (idxs if isinstance(idxs,(list,range)) else [idxs]):
            d[str(i)]=line
    return d
def fam(pairs):
    d={}
    for idxs,f in pairs:
        for i in (idxs if isinstance(idxs,(list,range)) else [idxs]):
            d[str(i)]=f
    return d

M={}

# ---------------- ShiftyMod ----------------
lines=[
 "Shift register for gates with a controllable delay on each of its seven rows, turning one clock or trigger into seven varied gate patterns",
 "Spreads the rows in time: at 0 all seven rows hit on the same beat, at 1 each row hits one beat later than the row above it, and at 2 two beats later",
 "Sets the chance that a row keeps its current delay value rather than rolling a new one, 0 to 100%; at 100% the pattern of delays stops changing",
 "Sets how many clock beats pass between hits entering the shift register, 0 to 15, and how often the chance of holding a delay value is rolled",
 "Sets the speed of the internal clock, 10 to 5000 beats per minute; the internal clock runs only while nothing is patched to Clock",
 "Advances the shift register one step on each rising edge, reading high above 2V and low below 0.1V; an internal clock runs when nothing is patched here",
 "A rising edge puts a hit into the shift register; while this is patched the clock divider no longer makes hits of its own",
 "Sets that row's delay before the ramp is added, 0-10V spanning 16 beats and then scaled by that row's knob; unpatched, the row uses an internal noise value",
 "Scales that row's delay voltage, from none of it to its full 16 beats, and scales the internal noise value when nothing is patched to that row's delay input",
 "Adds up to three extra hits to that row's gate, at timings that change as the knob turns and that shift again with the delay value for the row",
 "Suppresses hits on that row, sweeping through 693 patterns of 24 beats, from every hit passing to no hit passing",
 "Carries 10V while the clock is high and that row has a hit that its mute pattern let through, and 0V otherwise",
 "Menu — Add Expander (right 8HP) creates the Shifty expander beside this module, adding a CV input and an attenuverter for most of these knobs",
 "Note — Bypassing this module routes the Clock input to all seven gate outputs",
]
M['ShiftyMod']={'lines':lines,
 'param':tag([(0,1),(1,2),(2,3),(24,4),(rng(3,9),8),(rng(10,16),9),(rng(17,23),10)]),
 'in':tag([(0,5),(1,6),(rng(2,8),7)]),
 'out':tag([(rng(0,6),11)]),
 'family':{'in':fam([(0,'trigger'),(1,'trigger'),(rng(2,8),'cv')]),
           'out':fam([(rng(0,6),'trigger')])}}

# ---------------- ShiftyExpander ----------------
lines=[
 "Expander for Shifty, sitting immediately to the right of it, with a CV input and an attenuverter for the clock, ramp, sample and hold, echo and mute settings",
 "Attenuates and inverts the clock rate CV; at full, 10V adds 5000 beats per minute to Shifty's clock rate",
 "Attenuates and inverts the clock divider CV; at full, 10V adds 16 beats to Shifty's clock divider, rounded down to a whole number of beats",
 "Attenuates and inverts the ramp CV; at full, 10V adds two beats per step to Shifty's ramp",
 "Attenuates and inverts the sample and hold CV; at full, 10V adds 100% to Shifty's chance of holding a delay value",
 "Attenuates and inverts the CV for that row's echo setting on Shifty; at full attenuverter, 2.5V covers the whole range of the echo knob",
 "Attenuates and inverts the CV for that row's mute setting on Shifty; at full attenuverter, 2.5V covers the whole range of the mute knob",
 "Raises or lowers the rate of Shifty's internal clock, through its attenuverter",
 "Raises or lowers Shifty's clock divider, through its attenuverter",
 "Raises or lowers Shifty's ramp, through its attenuverter",
 "Raises or lowers Shifty's chance of holding a delay value, through its attenuverter",
 "Raises or lowers the echo setting for that row on Shifty, through its attenuverter",
 "Raises or lowers the mute setting for that row on Shifty, through its attenuverter",
 "Note — These controls reach Shifty only when this module is placed immediately to the right of it",
]
M['ShiftyExpander']={'lines':lines,
 'param':tag([(0,1),(1,2),(2,3),(3,4),(rng(4,10),5),(rng(11,17),6)]),
 'in':tag([(0,7),(1,8),(2,9),(3,10),(rng(4,10),11),(rng(11,17),12)]),
 'family':{'in':fam([(rng(0,17),'cv')])}}

# ---------------- IceTray ----------------
lines=[
 "Speed shifter and six tape delays, called ice cubes, each keeping its last recording until it is recorded over",
 "Sets the numerator of the record speed, a whole number from 1 to 11; the numerator divided by the denominator is the speed the input is recorded at",
 "Attenuates and inverts the numerator CV before it is added to the numerator setting",
 "Sets the denominator of the record speed, a whole number from 1 to 11; the numerator divided by the denominator is the speed the input is recorded at",
 "Attenuates and inverts the voltage at the numerator CV input before it is added to the denominator setting, rather than the voltage at the denominator CV input",
 "Sets the chance that a cube keeps its current solidity, 0 to 100%; at 100% solidity changes only when a cube is clicked",
 "Attenuates and inverts the frozen track CV before it is added to the frozen percentage",
 "Sets how many seconds are recorded into one cube before recording moves to another, 0.1 to 10 seconds; it is ignored while the record clock is patched",
 "Sets whether playback restarts at the beginning of each cube, or carries on from the position at which the previous cube ended",
 "Sets how many times one cube is played before another is chosen, 1 to 10; a voltage at the repeat count input multiplies that number",
 "Sets the order the cubes play in: turned left they are skipped through in increasingly large random steps, turned right the order is shuffled but repeats the same way",
 "Sets how much of the output is fed back into the recording cube, 0 to 100%; the fed-back signal is sped up and pitch shifted by the speed settings",
 "Attenuates and inverts the feedback CV before it is added to the feedback percentage",
 "Clicking changes that cube's solidity: bright blue records and plays, dark blue only plays, and black is skipped over with its last recording kept",
 "Left channel of the audio recorded into the cubes",
 "Right channel of the audio recorded into the cubes",
 "A trigger moves recording to another cube; while this is patched a cube records for up to 10 seconds rather than the time set by the record length knob",
 "A trigger ends the cube being played early and moves playback to another cube",
 "Raises or lowers the record speed numerator through its scalar; the denominator's scalar reads this same voltage",
 "Nothing in the module reads this input; the denominator's scalar is applied to the voltage at the numerator CV input instead",
 "Multiplies the repeat count set by the knob, using the size of the voltage and ignoring its sign",
 "Scales the pattern setting, 10V giving the knob its full effect and 0V giving none of it, with negative voltage reversing which end of the range is used",
 "Raises or lowers the frozen percentage through its scalar",
 "Raises or lowers the feedback percentage through its scalar",
 "Left channel of the cubes being played back",
 "Right channel of the cubes being played back",
 "Menu — Clear Cubes erases the recording in all six cubes; recordings are otherwise saved with the patch",
 "Menu — Pitch Correction holds the recorded pitch as the speed changes; turning it off saves processor time and lets pitch follow speed",
 "Note — Bypassing this module passes the left and right inputs straight to the outputs",
]
M['IceTray']={'lines':lines,
 'param':tag([(0,1),(1,2),(2,3),(3,4),(4,5),(5,6),(6,7),(7,8),(8,9),(9,10),(10,11),(11,12),(rng(12,17),13)]),
 'in':tag([(0,14),(9,15),(1,16),(2,17),(3,18),(4,19),(5,20),(6,21),(7,22),(8,23)]),
 'out':tag([(0,24),(1,25)]),
 'family':{'in':fam([(0,'audio'),(9,'audio'),(1,'trigger'),(2,'trigger'),(3,'cv'),(5,'cv'),(6,'cv'),(7,'cv'),(8,'cv')]),
           'out':fam([(0,'audio'),(1,'audio')])}}

# ---------------- AstroVibe ----------------
lines=[
 "Three orbiters, each a stereo oscillator or LFO whose waveform comes from a randomly generated planetary signature",
 "Generates a new planetary signature for that orbiter, changing its waveform and, in the notes waveform, its sequence of between 2 and 22 notes",
 "Sets the frequency of that orbiter: audible pitch in the audible mode, and rate in the LFO mode, where it runs a thousand times slower",
 "Rotates that orbiter's wave field, changing its timbre and its balance between left and right; the effect is stronger in the tones waveform",
 "Switches that orbiter between the atomic engine, which is purer and quieter, and the black hole engine, which is buzzier and louder",
 "Switches that orbiter between notes, a melodic sequence stepped by the clock, and tones, which sounds continuously and can be crunchier",
 "Switches that orbiter between audible, where its output is added to the master outputs, and LFO, where it runs a thousand times slower",
 "Sets the level of that orbiter's outputs, and inverts them when turned left of centre",
 "A rising edge generates a new planetary signature for that orbiter",
 "Steps that orbiter to the next note of its sequence in the notes waveform, and does nothing in the tones waveform; unpatched, it takes the clock of the orbiter before it",
 "Multiplies that orbiter's frequency by two to the power of the voltage; unpatched, it takes the voltage given to the orbiter before it",
 "Adds to that orbiter's rotation, 10V covering the whole range of the rotation knob; unpatched, it takes the left output of the previous orbiter when that one is an LFO",
 "Rotates that orbiter's wave field further, 5V turning it one full revolution; unpatched, it takes the right output of the previous orbiter when that one is an LFO",
 "Above 5V the engine switch for that orbiter is inverted, below 0V the switch acts as set, and between the two the engine flips back and forth in proportion to the voltage",
 "Above 5V the waveform switch for that orbiter is inverted, below 0V the switch acts as set, and between the two the waveform flips back and forth in proportion to the voltage",
 "Sums the left channel of every orbiter set to audible",
 "Sums the right channel of every orbiter set to audible",
 "Left channel of that orbiter alone, at its gain setting, in either the audible or the LFO mode",
 "Right channel of that orbiter alone, at its gain setting, in either the audible or the LFO mode",
 "Menu — Internal Routing off stops the clock, pitch, rotation and spin from being passed between orbiters, leaving each orbiter to read its own inputs",
 "Note — Bypassing this module leaves every output at 0V",
]
M['AstroVibe']={'lines':lines,
 'param':tag([(rng(0,2),1),(rng(3,5),2),(rng(6,8),3),(rng(9,11),4),(rng(12,14),5),(rng(15,17),6),(rng(18,20),7)]),
 'in':tag([(rng(0,2),8),(rng(3,5),9),(rng(6,8),10),(rng(9,11),11),(rng(12,14),12),(rng(15,17),13),(rng(18,20),14)]),
 'out':tag([(0,15),(1,16),(rng(2,4),17),(rng(5,7),18)]),
 'family':{'in':fam([(rng(0,2),'trigger'),(rng(3,5),'trigger'),(rng(6,8),'pitch'),(rng(9,11),'cv'),(rng(12,14),'cv'),(rng(15,20),'cv')]),
           'out':fam([(rng(0,7),'audio')])}}

# ---------------- GlassPane / PlusPane shared node lines ----------------
node_lines=[
 "Triggers that node; patch it from the output of another node or from an external gate",
 "A second trigger input into that node, doing what its X input does",
 "Sets that node's mode: blue steps through its outputs in turn, orange picks one of them at random, and purple plays its connected nodes in quick succession",
 "A rising edge moves that node on to its next mode",
 "Sets the voltage that node sends to the CV output while it is the active step; the knobs span -1V to 1V until the range is changed on the menu",
 "First output of that node, reached before its other outputs in cycle mode and the likeliest of the three when the odds are weighted",
 "Second output of that node, reached after output A in cycle mode and less likely than A when the odds are weighted",
 "Third output of that node, reached after output B in cycle mode and the least likely of the three when the odds are weighted",
]
menu_lines=[
 "Menu — Range sets the voltage the CV knobs span; changing it moves every node's output value at once without moving the knobs",
 "Menu — Cycle sets how cycle mode works: evenly visits each output the same number of times; weighted returns to the CV value between outputs, so A comes round more often than B and B more often than C",
 "Menu — Odds sets the chances in random mode: evenly gives the CV value and the three outputs equal chances; weighted makes the CV value six times as likely as C, A three times and B twice",
 "Menu — Ratchet Speed sets how fast ratchet mode plays: whole notes match the clock, half notes twice per clock, triplets three times, quarter notes four times, dynamic from the ports patched",
]

lines=[
 "Network sequencer of sixteen nodes, where the patch cables between nodes set which step follows which, allowing branches, loops and ratchets",
 "Advances the sequencer one step on each rising edge",
 "Returns the sequencer to the first node and returns every node to the mode last chosen by hand",
 "Carries the gate of the step being played",
 "Carries the CV value of the node being played",
]+node_lines+menu_lines+[
 "Menu — Low Performance Mode makes each node look for a trigger only while the clock is high",
 "Menu — +Pane Expander adds an eight-node expander on either side, and several can be chained",
 "Note — A node with none of its outputs patched to another node sends the sequence back to the first node on its next trigger",
 "Note — Bypassing this module leaves every output at 0V",
]
gp_x=[2+i*2 for i in range(16)]; gp_y=[3+i*2 for i in range(16)]
gp_modetrig=rng(34,49)
gp_A=[2+i*3 for i in range(16)]; gp_B=[3+i*3 for i in range(16)]; gp_C=[4+i*3 for i in range(16)]
M['GlassPane']={'lines':lines,
 'param':tag([(rng(0,15),7),(rng(16,31),9)]),
 'in':tag([(0,1),(1,2),(gp_x,5),(gp_y,6),(gp_modetrig,8)]),
 'out':tag([(0,3),(1,4),(gp_A,10),(gp_B,11),(gp_C,12)]),
 'family':{'in':fam([(0,'trigger'),(1,'trigger'),(gp_x,'trigger'),(gp_y,'trigger'),(gp_modetrig,'trigger')]),
           'out':fam([(0,'trigger'),(1,'cv'),(gp_A,'trigger'),(gp_B,'trigger'),(gp_C,'trigger')])}}

lines=[
 "Eight-node expander for Glass Pane, placed immediately to the left or the right of it, adding its nodes to the same network; several can be chained",
]+node_lines+menu_lines+[
 "Note — The clock, reset, gate output and CV output of Glass Pane drive these nodes too",
 "Note — Each expander keeps its own range, cycle, odds and ratchet speed settings",
]
pp_x=[i*2 for i in range(8)]; pp_y=[1+i*2 for i in range(8)]
pp_modetrig=rng(16,23)
pp_A=[i*3 for i in range(8)]; pp_B=[1+i*3 for i in range(8)]; pp_C=[2+i*3 for i in range(8)]
M['PlusPane']={'lines':lines,
 'param':tag([(rng(0,7),3),(rng(8,15),5)]),
 'in':tag([(pp_x,1),(pp_y,2),(pp_modetrig,4)]),
 'out':tag([(pp_A,6),(pp_B,7),(pp_C,8)]),
 'family':{'in':fam([(pp_x,'trigger'),(pp_y,'trigger'),(pp_modetrig,'trigger')]),
           'out':fam([(pp_A,'trigger'),(pp_B,'trigger'),(pp_C,'trigger')])}}

# ---------------- Nudge ----------------
lines=[
 "Modulation source giving five CV values that hold steady until a nudge moves them to new ones",
 "Sets how long each nudge takes to reach its new value, 0 to 10 seconds",
 "Attenuates and inverts the slew CV before it is added to the slew time",
 "Sets how far a value can move in one nudge, 0.01V to 5V",
 "Attenuates and inverts the step CV before it is added to the step size",
 "Sets the largest voltage the outputs reach, 0.01V to 10V",
 "Attenuates and inverts the range CV before it is added to the range",
 "Sets how likely a value is to move in the same direction as its last nudge; turned left of centre it favours the opposite direction",
 "Attenuates and inverts the velocity CV before it is added to the velocity setting",
 "Sets the span of the outputs: negative voltages only, bipolar, or positive voltages only",
 "Moves all five values to new ones, each taking the slew time to arrive",
 "Sets what the five CV inputs do: add to the outputs, or set each output's chance of moving when nudged",
 "Added to that output in offset mode; in chance mode it sets how likely that output is to move when nudged, 0V for never and 10V for always",
 "Raises or lowers the slew time through its attenuverter",
 "Raises or lowers the step size through its attenuverter",
 "Raises or lowers the range through its attenuverter",
 "Raises or lowers the velocity setting through its attenuverter, 5V covering its whole range at full attenuverter",
 "A rising edge moves all five values to new ones",
 "Carries that value, sliding to each new one over the slew time",
 "Note — In chance mode an unpatched CV input leaves its output with a fixed chance of moving: 10% for the first, then 30%, 50%, 70% and 90%",
 "Note — Bypassing this module routes each CV input to the CV output of the same number",
]
M['Nudge']={'lines':lines,
 'param':tag([(0,1),(1,2),(2,3),(3,4),(4,5),(5,6),(6,7),(7,8),(8,9),(9,10),(10,11)]),
 'in':tag([(rng(0,4),12),(5,13),(6,14),(7,15),(8,16),(9,17)]),
 'out':tag([(rng(0,4),18)]),
 'family':{'in':fam([(rng(0,4),'cv'),(rng(5,8),'cv'),(9,'trigger')]),
           'out':fam([(rng(0,4),'cv')])}}

# ---------------- OneShot ----------------
lines=[
 "One-shot sequencer of four values that interrupts the sequencer patched through it and plays a short sequence of its own",
 "Sets one of the four values this sequencer plays; the knobs span -1V to 1V until the range is changed on the menu",
 "Sets how many steps the sequence plays, 1 to 16; the first value always plays first and the fourth last, so short lengths skip values and long lengths repeat them",
 "Sets whether the sequence starts from the same position each time it plays, or carries on from where it left off so that each play differs from the last",
 "Sets how often a value is swapped for another at random, 0 to 100%, coming out differently each time the sequence plays",
 "Sets how varied the order of the values is, 0 to 100%, in a pattern that repeats the same way each time the sequence cycles",
 "Starts the sequence on the next clock; either the clock input or the gate input has to be running for it to play",
 "A rising edge starts the sequence on the next clock",
 "Steps this sequence; with nothing patched here the gate input steps it instead",
 "Gate from the sequencer up the chain, passed to the gate output whenever this sequence is not playing",
 "CV from the sequencer up the chain, passed to the CV output whenever this sequence is not playing",
 "Carries 10V for the step after this sequence finishes",
 "Carries the clock while this sequence is playing, and 0V at every other time",
 "Carries the clock while this sequence is playing, and the gate input at every other time",
 "Carries the value being played while this sequence is playing, and the CV input at every other time",
 "Menu — Range sets the voltage the four knobs span; changing it moves the output values at once without moving the knobs",
 "Note — Bypassing this module connects the gate input to the gate output and the CV input to the CV output",
]
M['OneShot']={'lines':lines,
 'param':tag([(rng(0,3),1),(4,2),(5,3),(6,4),(7,5),(8,6)]),
 'in':tag([(0,7),(1,8),(2,9),(3,10)]),
 'out':tag([(0,11),(1,12),(2,13),(3,14)]),
 'family':{'in':fam([(0,'trigger'),(1,'trigger'),(2,'trigger'),(3,'cv')]),
           'out':fam([(0,'trigger'),(1,'trigger'),(2,'trigger'),(3,'cv')])}}

doc={'plugin':'PathSet',
 'source':'https://github.com/patheros/PathSetManuals',
 'read':'2026-09-12',
 'modules':M}
out=os.path.expanduser('~/ProgrammingProjects/DreamerDevelopment/research/help/PathSet.json')
json.dump(doc,open(out,'w'),indent=1)
print('wrote',out)
