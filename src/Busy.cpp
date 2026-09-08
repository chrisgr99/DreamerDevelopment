#include "Busy.hpp"

#include <atomic>

/** ALIGNED TO ITS OWN CACHE LINE, so that the one thing the audio thread reads every sample is
not sharing a line with anything the user interface thread writes. Two variables in one line are
one variable as far as the hardware is concerned, and a write to either sends the line back and
forth between the cores. */
alignas(64) static std::atomic<int> gBusy{0};


void busyAdd(int delta) {
	gBusy.fetch_add(delta, std::memory_order_release);
}

bool busyAny() {
	return gBusy.load(std::memory_order_acquire) > 0;
}
