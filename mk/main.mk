# The build, run by build.sh (make -f mk/main.mk <target> MODE=<mode>).
#
#   build/<mode>/prefix/      the libraries from libs/, static, installed
#   build/<mode>/libs/<name>/ each library's own build folder
#   build/<mode>/mkvtoolnix/  a copy of the MKVToolNix source, built there
#                             (rake builds in place; the source tree is
#                             never written to)
#   build/<mode>/stage/       `rake install` of that build (prefix /usr)
#   dist/<mode>/              the programs and their data: bin/, share/
#   dist/deb/                 the packages (./build.sh deb)
#
# Running it again rebuilds only what changed.

MODE ?= debug
ifeq ($(filter debug release,$(MODE)),)
$(error MODE must be debug or release)
endif

ROOT   := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
LIBS   := $(ROOT)/libs
BUILD  := $(ROOT)/build/$(MODE)
PREFIX := $(BUILD)/prefix
DIST   := $(ROOT)/dist/$(MODE)
JOBS   ?= $(shell nproc)

# a file holding a rule's arguments, rewritten only when they change, so a
# changed argument rebuilds what depends on it
str_eq = $(and $(findstring x$(1),x$(2)),$(findstring x$(2),x$(1)))
define args_stamp
$(shell mkdir -p $(dir $(1)))$(if $(call str_eq,$(file <$(1)),$(2)),,$(file >$(1),$(2)))
endef

# every file of a source tree (the prerequisites of a sub-build)
src_files = $(shell find $(1) -type f ! -path '*/.git/*')

.PHONY: all dist libs test deb
all: dist

include $(ROOT)/mk/libs.mk
include $(ROOT)/mk/mkvtoolnix.mk
include $(ROOT)/mk/deb.mk
include $(ROOT)/mk/mtx.mk
