# DreamRack UI — see README.md
#
# Build against the official Rack SDK, NOT the RackFork tree. The whole point of this plugin
# is that it runs on stock VCV Rack, and building against the fork's headers would let a
# dependency on something only the fork has slip in unnoticed.
#
#   make                 build the plugin
#   make install         build and copy into Rack's user plugins folder
#   make dist            build a .vcvplugin package for distribution

RACK_DIR ?= ../Rack-SDK

SOURCES += $(wildcard src/*.cpp)
SOURCES += $(wildcard src/*.mm)

DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)

include $(RACK_DIR)/plugin.mk

# The pinch monitor talks to NSEvent, so the framework must be linked explicitly. Without it
# the plugin builds cleanly and then fails to LOAD, because macOS uses a two-level namespace
# and the Objective-C symbols name no library. ARCH_MAC is defined by the include above.
ifdef ARCH_MAC
	LDFLAGS += -framework Cocoa
endif
