# MKVToolNix itself: a copy of the source in build/<mode>/mkvtoolnix/ (rake
# builds in place), configured on the libraries in build/<mode>/prefix/, built
# with rake, installed into build/<mode>/stage/ and copied to dist/<mode>/.

MTX       := $(BUILD)/mkvtoolnix
STAGE     := $(BUILD)/stage
TOOLS     := $(BUILD)/tools
MTX_TMP   := $(BUILD)/tmp

# The copy: the files git knows, without our folders (mk/sync.sh)
.PHONY: mtx-sync
mtx-sync:
	sh $(ROOT)/mk/sync.sh $(ROOT) $(MTX) $(BUILD)/mkvtoolnix-files

# pkg-config for configure: only our prefix (the host's .pc files are not
# seen, so no host copy of one of our libraries can be picked), and static (a
# static library needs the libraries it is built on: FLAC needs libogg, ...).
# Qt (qmake6) and Boost are not found through pkg-config.
PKG_CONFIG_DIRS := $(PREFIX)/lib/pkgconfig:$(PREFIX)/share/pkgconfig
$(call args_stamp,$(TOOLS)/pkg-config-args,$(PKG_CONFIG_DIRS) libdir --static)
$(TOOLS)/pkg-config: $(TOOLS)/pkg-config-args
	@mkdir -p $(dir $@)
	printf '#!/bin/sh\nPKG_CONFIG_LIBDIR=%s PKG_CONFIG_PATH= exec pkg-config --static "$$@"\n' '$(PKG_CONFIG_DIRS)' > $@
	chmod +x $@

# (no precompiled headers: rake's handed the unit tests one that does not
# match them, std::optional missing; every file reads its headers itself)
MTX_CONFIGURE := --prefix=/usr --disable-update-check --disable-precompiled-headers --enable-debug \
                 $(if $(filter release,$(MODE)),--enable-optimization,--disable-optimization)
MTX_ENV := TMPDIR=$(MTX_TMP) PKG_CONFIG=$(TOOLS)/pkg-config \
           CPPFLAGS='-I$(PREFIX)/include -I$(PREFIX)/include/utf8cpp' LDFLAGS='-L$(PREFIX)/lib'
$(call args_stamp,$(BUILD)/mkvtoolnix-args,$(MTX_CONFIGURE) $(MTX_ENV))

# configure: again when configure.ac, the ac/ checks, our arguments or a library
# change. Every library is required and seen only in our prefix, so configure
# stops with a message instead of building without one or with another copy.
$(MTX)/build-config: $(BUILD)/mkvtoolnix-args $(TOOLS)/pkg-config $(LIB_STAMPS) \
                     $(ROOT)/configure.ac $(wildcard $(ROOT)/ac/*.m4) $(ROOT)/build-config.in \
                     | mtx-sync
	@echo "== mkvtoolnix: configure"
	@mkdir -p $(MTX_TMP)
	cd $(MTX) && ./autogen.sh > $(BUILD)/autogen.log 2>&1
	cd $(MTX) && $(MTX_ENV) ./configure $(MTX_CONFIGURE) > $(BUILD)/configure.log
	touch $@

# rake decides itself what to rebuild
.PHONY: mtx-build
mtx-build: $(MTX)/build-config mtx-sync
	@echo "== mkvtoolnix: rake"
	cd $(MTX) && TMPDIR=$(MTX_TMP) ./drake -j$(JOBS) > $(BUILD)/rake.log 2>&1 || { tail -40 $(BUILD)/rake.log; exit 1; }

# the installed files (as the packages hold them), and dist/<mode>/ from them
.PHONY: mtx-stage
mtx-stage: mtx-build
	@echo "== mkvtoolnix: install into build/$(MODE)/stage"
	rm -rf $(STAGE)
	cd $(MTX) && TMPDIR=$(MTX_TMP) ./drake install DESTDIR=$(STAGE) > $(BUILD)/install.log 2>&1 || \
	    { tail -40 $(BUILD)/install.log; exit 1; }
	install -m 755 -t $(STAGE)/usr/bin $(addprefix $(MTX)/src/tools/,bluray_dump ebml_validator hevcc_dump xyzvc_dump)
	sh $(ROOT)/mk/check-links.sh $(STAGE)/usr/bin

dist: mtx-stage
	@echo "== dist/$(MODE)"
	@mkdir -p $(DIST)
	rsync -a --delete $(STAGE)/usr/ $(DIST)/

# ./build.sh test: MKVToolNix's unit tests (GoogleTest from libs/) and its
# source checks. Its product tests (tests/run.rb) need sample files that are
# not public and are not run.
test: dist
	@echo "== tests"
	cd $(MTX) && TMPDIR=$(MTX_TMP) ./drake -j$(JOBS) tests:run_unit
