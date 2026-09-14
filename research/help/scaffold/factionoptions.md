# factionoptions — what the census already says

Installed version 2.0.17. Source: NOT PUBLISHED — the binary and the maker's manuals are what there is

Across the plugin: 0 unnamed inputs, 0 inputs with no panel jack.

8 modules. For each: how many controls, then anything the census flags.

## ResonanceHub — Resonance Hub

Maker's own summary: Connect your patch to other VCV Rack users over the internet for real-time collaboration. Handles authentication, room joining, and peer-to-peer WebRTC connections. Pair with CV Peer expanders to send and receive CV/MIDI with collaborators anywhere in the world. When a remote peer joins, the Hub adds a CV Peer expander to the right automatically, so you don't have to add one per collaborator by hand. On first use, your OS may prompt to allow Rack through the firewall.

Tags: External

0 in, 0 out, 0 controls

## CVPeer — CV Peer

Maker's own summary: Bidirectional CV link to one remote collaborator over the internet. Place to the right of a Resonance Hub - or let the Hub add one automatically each time a peer connects. 4 inputs send your CV outward, 4 outputs receive theirs, and the gate output goes high while the peer is connected.

Tags: External, Expander, Quad

4 in, 5 out, 2 controls

## MIDIIO — MIDI I/O

Maker's own summary: Bridge a remote collaborator to local MIDI hardware. Attaches to a CV Peer and routes 4 incoming and 4 outgoing MIDI ports to physical devices on your machine (keyboards, drum machines, external synths). Lets a partner play your hardware over the network. The Hub can be set to add a MIDI I/O alongside each auto-added CV Peer.

Tags: External, Expander, MIDI, Quad

0 in, 0 out, 1 controls

## ResonanceMonitor — Resonance Monitor

Maker's own summary: Diagnostic display for a Resonance Hub. Place to the left of a Hub to see live connection state, connected peers, latency, signaling status, and CV/MIDI data flow rates. Useful for troubleshooting collaboration sessions.

Tags: External, Expander, Visual

0 in, 0 out, 0 controls

## NetCV — Net CV

Maker's own summary: Turn live network conditions into modulation sources. Outputs CV scaled to round-trip latency, packet loss, jitter, and connected peer count - patch into mixers or effects to react when the link gets unstable. Place next to a Resonance Hub.

Tags: External, Expander

0 in, 7 out, 0 controls

## CVMix — CV Mix

Maker's own summary: Mix bus that sums and averages incoming CV from every connected remote peer into 4 unified channels. Useful for group jams where multiple collaborators contribute to the same patch without needing one CV Peer per source. Place anywhere to the right of a Resonance Hub.

Tags: External, Expander, Mixer

0 in, 4 out, 0 controls

## DmxHub — DMX Hub

Maker's own summary: Drive stage lighting from VCV Rack over Ethernet using sACN (E1.31) or Art-Net. Transmits one DMX universe (512 channels) to lighting consoles, LED fixtures, moving heads, or pixel controllers on your local network. Pairs well with Open Lighting Architecture (OLA) for bridging to USB DMX interfaces. Add DMX Fixture expanders to the right to patch CV inputs onto specific channels. On first use, your OS may prompt to allow Rack to find devices on your local network.

Tags: External

0 in, 0 out, 0 controls

## DmxFixture — DMX Fixture

Maker's own summary: 4-channel CV-to-DMX patch bay for a DMX Hub - typically used for one RGBA or RGBW lighting fixture like a PAR can, LED panel, or moving head. CV inputs drive consecutive DMX channels with per-channel level and mute. Chain multiple fixtures to the right of the Hub to address larger lighting rigs.

Tags: External, Expander, Quad

4 in, 0 out, 11 controls

