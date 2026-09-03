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

# OpenGL has to be named on Windows. The pinch overlay photographs the frame with
# glReadPixels; macOS and Linux resolve that at load time, and MinGW does not — it linked
# everything else and stopped at that one symbol.
ifdef ARCH_WIN
	LDFLAGS += -lopengl32
endif

# DEVELOPMENT INSTALL — see the same target in the MPX plugin for the whole story. Briefly:
# `make install` copies a package that Rack unpacks on startup, and one copied while Rack is
# running can be cleared at shutdown without ever being unpacked, so the next start silently
# loads the old build. And the copy has to UNLINK first: writing over an existing dylib leaves
# macOS holding a code signature against pages that have changed, and the kernel then kills Rack
# on load rather than reporting anything.
RACK_USER_DIR ?= $(HOME)/Library/Application Support/Rack2
PLUGIN_DIR = $(RACK_USER_DIR)/plugins-mac-arm64/$(SLUG)

dev: $(TARGET)
	@rm -f "$(RACK_USER_DIR)/plugins-mac-arm64/"$(SLUG)-*.vcvplugin
	@codesign --force --sign - $(TARGET) 2>/dev/null || true
	@mkdir -p "$(PLUGIN_DIR)"
	@rm -f "$(PLUGIN_DIR)/plugin.dylib"
	@cp $(TARGET) "$(PLUGIN_DIR)/plugin.dylib"
	@cp plugin.json "$(PLUGIN_DIR)/"
	@cp LICENSE "$(PLUGIN_DIR)/" 2>/dev/null || true
	@xattr -c "$(PLUGIN_DIR)/plugin.dylib" 2>/dev/null || true
	@codesign -v "$(PLUGIN_DIR)/plugin.dylib" && echo "signature valid"
# EVERY SYMBOL OF OURS HAS TO BE DEFINED, and the compiler will not say so. A plugin is loaded
# into Rack's flat namespace, so a function that is declared and called but never defined links
# without a word and fails at dlopen — the whole plugin does not appear, both modules vanish
# from the browser, and the only trace is a line in Rack's log. It cost a session's work being
# invisible once. This is a second's work at every build.
	@if nm -u "$(PLUGIN_DIR)/plugin.dylib" | c++filt \
		| grep -E "palette|drui|cableFocus|clip[A-Z]|scope[A-Z]|monitor[A-Z]|injector"; then \
		echo "^^ UNDEFINED SYMBOLS OF OURS — the plugin will not load"; \
		exit 1; \
	fi
	@echo "installed to $(PLUGIN_DIR)"

.PHONY: dev
