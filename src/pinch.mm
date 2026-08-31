/** Trackpad pinch, macOS — see pinch.hpp */
#include "pinch.hpp"

// ARCH_MAC comes from here, NOT from the compiler: it is a make variable and is never passed
// as -DARCH_MAC. A file that tests it without including this header silently compiles its
// non-Mac branch, which is exactly how this shipped as a do-nothing stub once before.
#include <arch.hpp>

#if defined ARCH_MAC

#import <Cocoa/Cocoa.h>
#include <atomic>
#include <chrono>

namespace drui {

static std::atomic<double> pending{0.0};
static std::atomic<double> lastEventTime{0.0};
static id monitor = nil;

static double nowSeconds() {
	using namespace std::chrono;
	return duration<double>(steady_clock::now().time_since_epoch()).count();
}

void pinchInit() {
	if (monitor)
		return;
	// LOCAL, not global: a global monitor would require the user to grant input-monitoring
	// permission, which this feature has no business asking for.
	monitor = [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskMagnify
		handler:^NSEvent* (NSEvent* event) {
			pending.store(pending.load(std::memory_order_relaxed) + [event magnification],
				std::memory_order_relaxed);
			lastEventTime.store(nowSeconds(), std::memory_order_relaxed);
			return event;
		}];
}

float pinchTake() {
	// exchange, not load-then-clear: pinch events arrive in a fast stream and one landing
	// between the two operations would be silently dropped.
	return (float) pending.exchange(0.0, std::memory_order_relaxed);
}

double pinchIdleTime() {
	const double last = lastEventTime.load(std::memory_order_relaxed);
	if (last <= 0.0)
		return 1e9;
	return nowSeconds() - last;
}

}

#endif
