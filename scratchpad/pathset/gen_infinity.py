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
M={}

# ---------------- QuantumCompass ----------------
L=[
 "Sequencer and quantizer that builds a sequence from the notes switched on at the keyboard, then adds accidentals, octave shifts, ratchets and mutes to it",
 "Clicking adds that note to the pool the sequence draws from; holding it takes the note back out",
 "Switches the clock or position input between a clock, whose rising edges advance the sequence, and a voltage that picks the step directly",
 "Advances the sequence on each rising edge in clock mode; in sequence mode the voltage picks the step, 0V the first and 10V the last, wrapping outside that range",
 "Returns the sequence to its first step",
 "A rising edge returns the sequence to its first step",
 "Sets the chance that the sequence resets part way through; the display marks the first step in green and the steps that might reset it in red",
 "Sets which steps of the sequence have a chance of resetting it",
 "Sets how many steps the sequence plays, 1 to 64; changing it truncates the pattern rather than changing the notes in it",
 "Attenuates and inverts the steps CV before it is added to the step count",
 "Raises or lowers the step count through its attenuverter",
 "Sets how many steps of the sequence are silenced",
 "Attenuates and inverts the mutes CV before it is added to the mutes setting",
 "Raises or lowers the mutes setting through its attenuverter",
 "Sets how many steps play more than one note in a clock cycle; the extra notes are taken from the next three notes of the sequence",
 "Attenuates and inverts the ratchet CV before it is added to the ratchet setting",
 "Raises or lowers the ratchet setting through its attenuverter",
 "Allows a ratcheted step to play twice in one clock cycle",
 "Allows a ratcheted step to play three times in one clock cycle",
 "Allows a ratcheted step to play four times in one clock cycle",
 "Adds flattened or sharpened steps to the sequence, flats one way and sharps the other, and both once it is turned far enough",
 "Attenuates and inverts the accidentals CV before it is added to the accidentals setting",
 "Raises or lowers the accidentals setting through its attenuverter",
 "Adds steps shifted an octave up or an octave down, one direction each way and both once it is turned far enough",
 "Attenuates and inverts the octave shift CV before it is added to the octave shift setting",
 "Raises or lowers the octave shift setting through its attenuverter",
 "Adds randomness to the accidentals, octave shifts, ratchets and mutes, leaving the notes, the step count and the auto reset alone",
 "Attenuates and inverts the chaos CV before it is added to the chaos setting",
 "Raises or lowers the chaos setting through its attenuverter",
 "Carries a gate for the whole clock cycle of a step that is flattened",
 "Carries a gate for the whole clock cycle of a step that is sharpened",
 "Carries a gate for the whole clock cycle of a step shifted an octave down",
 "Carries a gate for the whole clock cycle of a step shifted an octave up",
 "Carries the pitch of the step being played",
 "Carries the gate of the step being played",
 "Menu — Compass Display Mode picks what the ring of lights shows, and it follows whichever of these knobs was turned last",
 "Menu — Set Scale switches on the notes of a common scale and gives a new random sequence each time it is used",
 "Menu — Shift Notes moves every note in the pool up or down; the keyboard wraps, so a C shifted down becomes a B an octave higher",
 "Menu — Accidental Mode sets what an accidental does: non diatonic moves a semitone, diatonic moves up one note of the scale, middle picks the pitch between two notes of the scale",
 "Note — Bypassing this module leaves every output at 0V",
]
M['QuantumCompass']={'lines':L,
 'param':tag([(rng(0,11),1),(12,2),(13,4),(16,6),(17,7),(14,8),(15,9),(22,11),(21,12),
              (29,14),(30,15),(18,17),(19,18),(20,19),(23,20),(24,21),(25,23),(26,24),(27,26),(28,27)]),
 'in':tag([(0,3),(1,5),(5,10),(7,13),(6,16),(2,22),(3,25),(4,28)]),
 'out':tag([(0,29),(1,30),(2,31),(3,32),(4,33),(5,34)]),
 'family':{'in':fam([(1,'trigger'),(rng(2,7),'cv')]),
           'out':fam([(0,'trigger'),(1,'trigger'),(2,'trigger'),(3,'trigger'),(4,'pitch'),(5,'trigger')])}}

# ---------------- WarpDrive ----------------
L=[
 "Two engines, each a voice of three oscillators called coils, with its own LFO, clock and envelope",
 "Sets the frequency of all three coils of that engine, coarsely",
 "Fine tunes the frequency of all three coils of that engine",
 "Sets the pitch of all three coils of that engine at a volt per octave",
 "Sets how strongly the high coil frequency modulates the main coil of that engine; none at centre, and the coil runs at twice the main coil's speed turned left, four times turned right",
 "Attenuates and inverts the high coil power CV before it is added to the high coil power setting",
 "Raises or lowers that engine's high coil power through its attenuverter",
 "Sets how strongly the low coil frequency modulates the main coil of that engine; none at centre, and the coil runs at half the main coil's speed turned left, a quarter turned right",
 "Attenuates and inverts the low coil power CV before it is added to the low coil power setting",
 "Raises or lowers that engine's low coil power through its attenuverter",
 "Sets how the high and low coil signals are combined before they modulate the main coil; at centre they are added, and the rest of the sweep gives six other combinations",
 "Attenuates and inverts the coil attachment CV before it is added to the coil attachment setting",
 "Raises or lowers that engine's coil attachment through its attenuverter",
 "Creates a new wave shape for the main coil of that engine",
 "A rising edge creates a new wave shape for the main coil of that engine",
 "Creates a new wave shape for the high coil of that engine",
 "Creates a new wave shape for the low coil of that engine",
 "When on, all three coils of that engine share one wave shape",
 "When on, the coils of the left and the right engine share one wave shape",
 "Audio from the main coil of that engine, before the envelope",
 "Audio from the high coil of that engine",
 "Audio from the low coil of that engine",
 "Sets how much modulation is added to that engine's high and low coil power: none at centre, from that engine's own LFO and envelope turned right, from the other engine's turned left",
 "Attenuates and inverts the alignment CV before it is added to the alignment setting",
 "Raises or lowers that engine's alignment through its attenuverter",
 "Sets where the alignment modulation comes from: the LFO alone turned fully left, the envelope alone turned fully right, and a blend between, which differs for the high and the low coil",
 "Attenuates and inverts the mixture CV before it is added to the mixture setting",
 "Raises or lowers that engine's mixture through its attenuverter",
 "Sets the rate of that engine's LFO, which also sets the rate of the clock derived from it",
 "Attenuates and inverts the LFO and clock rate CV before it is added to the rate setting",
 "Raises or lowers that engine's LFO and clock rate through its attenuverter",
 "Creates a new pattern for that engine's LFO and clock",
 "A rising edge creates a new pattern for that engine's LFO and clock",
 "Sets how the two clocks reach the envelopes: Duo clocks each envelope from its own engine, Sync clocks both from the left engine, and +Mod does that while the right LFO modulates the left LFO's rate",
 "Raw LFO of that engine",
 "Chaotic clock derived from that engine's LFO",
 "Triggers the envelope of that engine; it is normalled to the clock output of that engine",
 "Sets how fast that engine's envelope develops",
 "Attenuates and inverts the envelope rate CV before it is added to the envelope rate",
 "Raises or lowers that engine's envelope rate through its attenuverter",
 "Creates a new shape for that engine's envelope",
 "A rising edge creates a new shape for that engine's envelope",
 "When on, the right envelope takes the shape of the left envelope, each keeping its own rate and trigger",
 "Direct output of that engine's envelope generator",
 "Sets the shape of every wave on that side of the ship, the three coils, the LFO and the envelope alike",
 "Attenuates and inverts the gravitation CV before it is added to the gravitation setting",
 "Raises or lowers that engine's gravitation through its attenuverter",
 "Main coil of that engine through a VCA controlled by its envelope",
 "Note — Bypassing this module leaves every output at 0V",
]
M['WarpDrive']={'lines':L,
 'param':tag([([20,21],1),([22,23],2),([24,25],4),([26,27],5),([28,29],7),([30,31],8),
              ([32,33],10),([34,35],11),([12,13],13),([16,17],15),([18,19],16),([14,15],17),(11,18),
              ([40,41],22),([42,43],23),([36,37],25),([38,39],26),([44,45],28),([46,47],29),
              ([48,49],31),(50,33),([4,5],37),([6,7],38),([9,10],40),(8,42),
              ([0,1],44),([2,3],45)]),
 'in':tag([([14,15],3),([10,11],6),([12,13],9),([16,17],12),([6,7],14),([20,21],24),([18,19],27),
           ([22,23],30),([24,25],32),([8,9],36),([2,3],39),([4,5],41),([0,1],46)]),
 'out':tag([([4,5],19),([6,7],20),([8,9],21),([10,11],34),([12,13],35),([2,3],43),([0,1],47)]),
 'family':{'in':fam([([14,15],'pitch'),([10,11],'cv'),([12,13],'cv'),([16,17],'cv'),([6,7],'trigger'),
                     ([20,21],'cv'),([18,19],'cv'),([22,23],'cv'),([24,25],'trigger'),([8,9],'trigger'),
                     ([2,3],'cv'),([4,5],'trigger'),([0,1],'cv')]),
           'out':fam([([4,5],'audio'),([6,7],'audio'),([8,9],'audio'),([10,11],'cv'),([12,13],'trigger'),
                      ([2,3],'cv'),([0,1],'audio')])}}

# ---------------- Orchestrator ----------------
L=[
 "Modulation and gate source for a whole song: a head that sets the length of the song and moves eight arms, each arm turning that movement into modulation or into drum gates",
 "Sets how fast the arms move during the song, which sets how much the song changes; turning it moves the song's position for as long as it is turned",
 "Sets how long the whole module takes to repeat, and is scaled by the speed chosen on the menu",
 "Creates a new pattern for the movement of the arms; the pattern it leaves cannot be returned to",
 "A rising edge creates a new pattern for the movement of the arms",
 "Sets whether the song repeats or stops after one play through",
 "Starts or stops the song",
 "A rising edge starts or stops the song; in one-shot mode it can only start it",
 "A rising edge returns the song to its start, leaving it running or stopped as it was",
 "Carries 10V for one sample when the song reaches its end",
 "Creates a new pattern for that arm",
 "A rising edge creates a new pattern for that arm",
 "Switches that arm between modulation, where its outputs carry an oscillation, and drum, where they carry gate patterns",
 "Sets how long that arm's own pattern takes to repeat",
 "Sets the rate of that arm's pattern; turning it moves the song's position for as long as it is turned",
 "Sets the strength of that output: in modulation mode it attenuates the oscillation, and in drum mode it sets how many of the hits get through",
 "Sets the placement of that output: in modulation mode a DC offset on the modulation, and in drum mode the swing of the hits, with no swing at centre",
 "Nearest output of that arm: in modulation mode the arm's oscillation at quarter speed, in drum mode a gate pattern with the lowest threshold to turn on",
 "Middle output of that arm: in modulation mode the arm's oscillation at its own speed, in drum mode a gate pattern with its own sequence",
 "Furthest output of that arm: in modulation mode the arm's oscillation at four times speed, in drum mode a gate pattern with the highest threshold to turn on",
 "Menu — Speed multiplies the rate of everything in the module by ten or a hundred, which shortens the song to match",
 "Menu — Wave Arms sets the main pattern aside and waves each arm up and down out of phase, so the full range of the modulation can be heard; the complexity knob then sets how fast they wave",
 "Note — Bypassing this module leaves every output at 0V",
]
armA=[i*3 for i in range(8)]; armB=[1+i*3 for i in range(8)]; armC=[2+i*3 for i in range(8)]
M['Orchestrator']={'lines':L,
 'param':tag([(82,1),(81,2),(80,3),(83,5),(84,6),(rng(72,79),10),(rng(64,71),12),
              (rng(48,55),13),(rng(56,63),14),(rng(0,23),15),(rng(24,47),16)]),
 'in':tag([(8,4),(9,7),(10,8),(rng(0,7),11)]),
 'out':tag([(24,9),(armA,17),(armB,18),(armC,19)]),
 'family':{'in':fam([(8,'trigger'),(9,'trigger'),(10,'trigger'),(rng(0,7),'trigger')]),
           'out':fam([(24,'trigger')])}}

# ---------------- Shuttle ----------------
L=[
 "Additive oscillator summing a sine wave and its first fifteen harmonics, with the amplitudes generated at random or fed in on one polyphonic cable",
 "Sets the pitch of the output at a volt per octave",
 "Audio output of the summed sine waves",
 "Generates a new set of amplitudes for the sixteen sine waves",
 "A rising edge generates a new set of amplitudes for the sixteen sine waves",
 "Sixteen-channel polyphonic output carrying the amplitude of the fundamental and each of its fifteen harmonics",
 "Sixteen-channel polyphonic input that takes over the amplitudes of the fundamental and its harmonics; the location output keeps its own generated values while this is patched",
 "Note — Bypassing this module leaves every output at 0V",
]
M['Shuttle']={'lines':L,
 'param':tag([(0,3)]),
 'in':tag([(0,1),(1,4),(2,6)]),
 'out':tag([(0,2),(1,5)]),
 'family':{'in':fam([(0,'pitch'),(1,'trigger'),(2,'cv')]),
           'out':fam([(0,'audio'),(1,'cv')])}}

doc={'plugin':'PathSet-Infinity','source':'https://github.com/patheros/PathSetManuals','read':'2026-09-12','modules':M}
out=os.path.expanduser('~/ProgrammingProjects/DreamerDevelopment/research/help/PathSet-Infinity.json')
json.dump(doc,open(out,'w'),indent=1)
print('wrote',out)
