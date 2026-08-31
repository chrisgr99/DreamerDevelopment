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

# WHICH PLATFORM, decided before the SDK is included. ARCH_MAC is set by the SDK's arch.mk,
# which is pulled in by plugin.mk at the bottom of this file — too late to choose sources
# with. The same test arch.mk makes is made here instead, honouring CROSS_COMPILE so that a
# Windows or Linux build cross-compiled on a Mac is still seen as Windows or Linux.
ifdef CROSS_COMPILE
	TARGET_MACHINE := $(CROSS_COMPILE)
else
	TARGET_MACHINE := $(shell $(CC) -dumpmachine)
endif

SOURCES += $(wildcard src/*.cpp)

# Objective-C++ ONLY ON MACOS. The SDK has a build rule for .mm on every platform, and a
# Windows or Linux compiler handed one either refuses it or wants an Objective-C runtime that
# is not installed. On those platforms src/pinch_other.cpp supplies the same three functions.
ifneq (,$(findstring -darwin,$(TARGET_MACHINE)))
	SOURCES += $(wildcard src/*.mm)
endif

# NO res DIRECTORY. Every panel and every face is drawn in code, so the plugin ships no
# artwork. Listing res here worked on this machine, where an empty res/ happened to exist, and
# failed on a clean checkout — which is what the VCV Library builds from.
DISTRIBUTABLES += $(wildcard LICENSE*)

include $(RACK_DIR)/plugin.mk

# The pinch monitor talks to NSEvent, so the framework must be linked explicitly. Without it
# the plugin builds cleanly and then fails to LOAD, because macOS uses a two-level namespace
# and the Objective-C symbols name no library. ARCH_MAC is defined by the include above.
ifdef ARCH_MAC
	LDFLAGS += -framework Cocoa
endif
