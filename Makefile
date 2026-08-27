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
