import json, os
def rng(a,b): return list(range(a,b+1))
def tag(pairs):
    d={}
    for idxs,line in pairs:
        for i in (idxs if isinstance(idxs,(list,range)) else [idxs]): d[str(i)]=line
    return d
def fam(pairs):
    d={}
    for idxs,f in pairs:
        for i in (idxs if isinstance(idxs,(list,range)) else [idxs]): d[str(i)]=f
    return d

COMMON=[
 "Advances the sequence one step on each rising edge",                                   # clock
 "Returns the sequence to its first step and returns every setting to the state last chosen by hand",  # reset
 "Carries the gate of the note being played",                                            # gate
 "Carries the pitch of the note being played, and holds its last value while a note is muted",  # cv
 "Carries a gate on the first step after the sequence reaches its end",                  # eoc
 "Carries a gate while the sequence is ratcheting",                                      # ratchet gate
 "Carries a gate while a note is muted",                                                 # mute gate
 "Clicking steps that note through the chosen scale; right-clicking opens a menu for setting any note, randomizing, and changing the scale or root",  # notes
]
MENUS=[
 "Menu — Set Scale picks the scale the notes come from and randomizes every note on the module",
 "Menu — Root Note changes the root and shifts every entered note by the interval between the old root and the new one",
 "Menu — Range sets how many octaves the notes span, centred on the fifth above the root",
 "Menu — Ratchet Speed sets how fast a ratchet plays: whole notes match the clock, half notes twice per clock, quarter notes four times",
 "Menu — Randomize Notes randomizes every note within the chosen scale, or shifts some of the notes an octave up or down",
 "Menu — Active Cursor Color picks the colour of the ring that marks the note being played",
 "Menu — Add Expander creates the play head expander, which adds three more playheads, or the randomizer expander, which adds buttons and triggers that randomize notes; each goes to the right",
]
BYPASS="Note — Bypassing this module connects the clock input to the gate output"

M={}

# ---------------- Grid ----------------
L=["Sequencer that draws its sequence from a grid of nine notes, with knobs biasing which part of the grid the playhead favours"]+COMMON+[
 "Biases the sequence towards the top or the bottom row of the grid",
 "Attenuates and inverts the vertical bias CV before it is added to the vertical bias setting",
 "Raises or lowers the vertical bias through its attenuverter",
 "Biases the sequence towards the left or the right column of the grid",
 "Attenuates and inverts the horizontal bias CV before it is added to the horizontal bias setting",
 "Raises or lowers the horizontal bias through its attenuverter",
 "Sets how many steps play before the sequence repeats; turned fully up the sequence never repeats",
 "Attenuates and inverts the sequence length CV before it is added to the sequence length",
 "Raises or lowers the sequence length through its attenuverter",
 "Sets how often a step plays several notes in quick succession; how many are played comes from the ratchet speed on the menu",
 "Attenuates and inverts the ratchet CV before it is added to the ratchet setting",
 "Raises or lowers the ratchet setting through its attenuverter",
 "Sets how often a step is silenced; while a step is silent the gate output stays low and the CV output holds its previous value",
 "Attenuates and inverts the mute CV before it is added to the mute setting",
 "Raises or lowers the mute setting through its attenuverter",
 "Offsets where the sequence starts, which picks a different sequence; the offset takes effect the next time the sequence restarts",
 "Attenuates and inverts the sequence start CV before it is added to the start offset",
 "Raises or lowers the start offset through its attenuverter",
 "Sets how many phrases play before the run of phrases repeats",
 "Attenuates and inverts the phrase count CV before it is added to the phrase count",
 "Raises or lowers the phrase count through its attenuverter",
 "Sets how many of the phrases repeat the same sequence and how many are variants of it",
 "Attenuates and inverts the phrase consistency CV before it is added to the consistency setting",
 "Raises or lowers the phrase consistency through its attenuverter",
 "Changes which sequences the variant phrases use",
 "Attenuates and inverts the phrase step CV before it is added to the phrase step setting",
 "Raises or lowers the phrase step through its attenuverter",
]+MENUS+[BYPASS]
b=9  # first index after module line + COMMON
M['Grid']={'lines':L,
 'param':tag([(rng(18,26),8),(0,b+0),(1,b+1),(4,b+3),(5,b+4),(2,b+6),(3,b+7),(6,b+9),(7,b+10),
              (8,b+12),(9,b+13),(10,b+15),(11,b+16),(12,b+18),(13,b+19),(16,b+21),(17,b+22),(14,b+24),(15,b+25)]),
 'in':tag([(0,1),(1,2),(2,b+2),(3,b+5),(4,b+8),(5,b+11),(6,b+14),(7,b+17),(8,b+20),(10,b+23),(9,b+26)]),
 'out':tag([(0,3),(1,4)]),
 'family':{'in':fam([(0,'trigger'),(1,'trigger'),(rng(2,10),'cv')]),
           'out':fam([(0,'trigger'),(1,'pitch')])}}

# ---------------- Bridge ----------------
L=["Sequencer of four rows of notes, with bridges between the rows that skip, mute, ratchet, borrow, swap or jump between them"]+COMMON+[
 "Clicking steps that bridge through its effects: none, skip, mute, ratchet, borrow, swap and jump; right-clicking sets the effect directly",
 "Sets the length of that row; above 8 the row ping-pongs, repeating the note at each end",
 "Attenuates and inverts that row's length CV before it is added to the row length",
 "Raises or lowers that row's length through its attenuverter",
 "Sets the chance that a bridge in that row applies its effect to the notes in the two rows it joins",
 "Attenuates and inverts that odds CV before it is added to the odds setting",
 "Raises or lowers those odds through its attenuverter",
 "Sets which row the sequence starts on, and offsets every playhead when the play head expander is attached",
 "Attenuates and inverts the start row CV before it is added to the start row",
 "Raises or lowers the start row through its attenuverter",
]+MENUS+[
 "Menu — Bridges initializes every bridge to no effect, or randomizes all of them",
 BYPASS]
b=9
M['Bridge']={'lines':L,
 'param':tag([(rng(14,45),8),(rng(46,69),b+0),(rng(0,3),b+1),(rng(4,7),b+2),(rng(8,10),b+4),(rng(11,13),b+5),(70,b+7),(71,b+8)]),
 'in':tag([(7,1),(8,2),(rng(0,3),b+3),(rng(4,6),b+6),(9,b+9)]),
 'out':tag([(0,3),(1,4),(2,5),(3,6),(4,7)]),
 'family':{'in':fam([(7,'trigger'),(8,'trigger'),(rng(0,6),'cv'),(9,'cv')]),
           'out':fam([(0,'trigger'),(1,'pitch'),(2,'trigger'),(3,'trigger'),(4,'trigger')])}}

# ---------------- Ring ----------------
L=["Sequencer of four concentric rings of notes read as four eight-step lines, with jump points between the lines and rotations that shift the notes"]+COMMON+[
 "Sets the length of that line; above 8 the line ping-pongs, repeating the note at each end",
 "Attenuates and inverts that line's length CV before it is added to the line's length",
 "Raises or lowers that line's length through its attenuverter",
 "Sets how often a note on that line is silenced; while a note is silent the gate output stays low and the CV output holds its previous value",
 "Attenuates and inverts that line's mute CV before it is added to the mute setting",
 "Raises or lowers that line's mute setting through its attenuverter",
 "Sets how often a note on that line plays several times in quick succession; how many are played comes from the ratchet speed on the menu",
 "Attenuates and inverts that line's ratchet CV before it is added to the ratchet setting",
 "Raises or lowers that line's ratchet setting through its attenuverter",
 "Sets how far the playhead moves at that jump point once a line has been played through; at 1.3 it jumps two steps three times in ten and one step the rest of the time",
 "Attenuates and inverts that jump CV before it is added to the jump setting",
 "Raises or lowers that jump setting through its attenuverter",
 "Rotates the notes of that ring: at 1 or -1 the ring turns one step each time the playhead finishes a line, positive turning it clockwise and negative counterclockwise",
 "Attenuates and inverts that ring's cycle CV before it is added to the cycle setting",
 "Raises or lowers that ring's cycle setting through its attenuverter",
 "A rising edge turns that ring one step to the left, whatever its cycle setting is",
 "A rising edge turns that ring one step to the right, whatever its cycle setting is",
 "Carries a gate whenever that ring turns, in either direction and from either its cycle setting or its triggers",
]+MENUS+[
 "Note — A jump point fires only when the playhead is travelling in its direction, so a jump setting of 0 can leave the playhead on one line",
 BYPASS]
b=9
M['Ring']={'lines':L,
 'param':tag([(rng(40,71),8),(rng(8,11),b+0),(rng(12,15),b+1),(rng(16,19),b+3),(rng(20,23),b+4),
              (rng(24,27),b+6),(rng(28,31),b+7),(rng(32,39),b+9),(rng(72,79),b+10),
              (rng(0,3),b+12),(rng(4,7),b+13)]),
 'in':tag([(24,1),(25,2),(rng(12,15),b+2),(rng(16,19),b+5),(rng(20,23),b+8),(rng(26,33),b+11),
           (rng(0,3),b+14),(rng(4,7),b+15),(rng(8,11),b+16)]),
 'out':tag([(4,3),(5,4),(6,5),(7,6),(8,7),(rng(0,3),b+17)]),
 'family':{'in':fam([(24,'trigger'),(25,'trigger'),(rng(0,3),'cv'),(rng(4,11),'trigger'),(rng(12,23),'cv'),(rng(26,33),'cv')]),
           'out':fam([(4,'trigger'),(5,'pitch'),(6,'trigger'),(7,'trigger'),(8,'trigger'),(rng(0,3),'trigger')])}}

# ---------------- Crossing ----------------
L=["Sequencer over a field of notes with an intersection between every four of them, each intersection steering, muting or ratcheting the playhead"]+COMMON+[
 "Clicking steps that intersection through its effects: none, mute, ratchet, horizontal, vertical, cross up, cross down and roundabout; right-clicking sets the effect directly",
 "Sets the chance that a ratchet intersection makes the playhead play the next few notes quickly",
 "Attenuates and inverts the ratchet CV before it is added to the ratchet setting",
 "Raises or lowers the ratchet setting through its attenuverter",
 "Sets the chance that a mute intersection silences the next note; while a note is silent the gate output stays low and the CV output holds its previous value",
 "Attenuates and inverts the mute CV before it is added to the mute setting",
 "Raises or lowers the mute setting through its attenuverter",
 "Sets how likely the playhead is to stay in a roundabout rather than leave it",
 "Attenuates and inverts the roundabout CV before it is added to the roundabout setting",
 "Raises or lowers the roundabout setting through its attenuverter",
 "Sets the chance that the playhead does the opposite of the route an intersection sets for it",
 "Attenuates and inverts the opposite chance CV before it is added to that setting",
 "Raises or lowers the opposite chance through its attenuverter",
]+MENUS+[
 "Menu — Crossings initializes every intersection to no effect, or randomizes all of them",
 BYPASS]
b=9
M['Crossing']={'lines':L,
 'param':tag([(rng(8,43),8),(rng(44,68),b+0),(0,b+1),(1,b+2),(2,b+4),(3,b+5),(4,b+7),(5,b+8),(6,b+10),(7,b+11)]),
 'in':tag([(0,1),(1,2),(2,b+3),(3,b+6),(4,b+9),(5,b+12)]),
 'out':tag([(0,3),(1,4),(2,5),(3,6),(4,7)]),
 'family':{'in':fam([(0,'trigger'),(1,'trigger'),(rng(2,5),'cv')]),
           'out':fam([(0,'trigger'),(1,'pitch'),(2,'trigger'),(3,'trigger'),(4,'trigger')])}}

# ---------------- 4Expander ----------------
L=["Play head expander for any Rainbow sequencer, placed immediately to the right of it, running three more playheads over the same notes",
 "Advances that playhead on each rising edge; with nothing patched here the sequencer's own clock advances it",
 "Returns that playhead to its first step",
 "Carries the pitch of the note that playhead is on",
 "Carries the gate of the note that playhead is on",
 "Carries a gate while the note that playhead is on is muted",
 "Carries a gate while that playhead is ratcheting",
 "Carries a gate on the first step after that playhead reaches the end of its sequence",
 "Note — The first of the four playheads is the sequencer's own, so its ports here repeat what the sequencer carries",
]
M['4Expander']={'lines':L,
 'in':tag([(rng(0,3),1),(rng(4,7),2)]),
 'out':tag([(rng(0,3),3),(rng(4,7),4),(rng(8,11),5),(rng(12,15),6),(rng(16,19),7)]),
 'family':{'in':fam([(rng(0,7),'trigger')]),
           'out':fam([(rng(0,3),'pitch'),(rng(4,19),'trigger')])}}

# ---------------- RndExpander ----------------
L=["Randomizer expander for any Rainbow sequencer, placed immediately to the right of it, with four buttons and triggers that randomize notes on it",
 "Randomizes the notes chosen for that button; holding it for a second enters configuration mode, where clicking a note on the sequencer adds it to or removes it from that button's set",
 "A rising edge randomizes the notes chosen for that button",
]
M['RndExpander']={'lines':L,
 'param':tag([(rng(0,3),1)]),
 'in':tag([(rng(0,3),2)]),
 'family':{'in':fam([(rng(0,3),'trigger')])}}

doc={'plugin':'PathSet-Rainbows','source':'https://github.com/patheros/PathSetManuals','read':'2026-09-12','modules':M}
out=os.path.expanduser('~/ProgrammingProjects/DreamerDevelopment/research/help/PathSet-Rainbows.json')
json.dump(doc,open(out,'w'),indent=1)
print('wrote',out)
