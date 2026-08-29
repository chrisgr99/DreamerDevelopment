#pragma once
/** Monitors: clip one onto a terminal and hear it through the Test Gear module's MONITOR jack.
See Monitor.cpp for why they sum rather than switch, and why nothing is inserted into the
signal being listened to. */
#include "plugin.hpp"

/** Monitors that can exist at once. Generous, since each costs one tap with no history. */
static const int MONITOR_MAX = 16;

/** AUDIO THREAD. The summed, DC-blocked monitor bus for this sample. Called from
TestGear::process, and costs one atomic load when no monitor exists. */
float monitorMix(float sampleTime);

/** Clips a monitor onto a port. UI thread. Either an input or an output. */
void monitorCreate(app::PortWidget* port, bool place = true);

/** Every monitor's attachment and settings, for saving with the patch. */
json_t* monitorToJson();
void monitorFromJson(json_t* arrayJ);
void monitorRestoreStep();
/** Shows or hides every monitor, without removing any. */
void monitorSetVisible(bool visible);
