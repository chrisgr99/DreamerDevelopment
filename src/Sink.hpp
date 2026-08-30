#pragma once
/** Hidden cables that make an unconnected output actually produce a signal — see Sink.cpp. */
#include "plugin.hpp"

/** Outputs that can be woken at once, and therefore hidden inputs on the Test Gear module. */
static const int SINK_MAX = 12;

/** UI THREAD. Lays and removes the sink cables to match what the viewers are watching. Called
once a frame from the overlay's housekeeping; cheap when there is nothing to do. */
void sinkStep();
