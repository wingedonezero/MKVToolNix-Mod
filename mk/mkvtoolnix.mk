# MKVToolNix, built with make out of the source tree (the source is never
# written to), on the libraries in build/<mode>/prefix/:
#
#   build/<mode>/conf/   configure (autoconf), run on a copy of configure.ac
#                        and ac/: config.h, build-config (-> config.mk)
#   build/<mode>/gen/    generated sources: moc (.moc), uic (forms .h), the
#                        version header, the GUI's resource file, man pages
#   build/<mode>/obj/    objects and the internal libraries (.a)
#   build/<mode>/bin/    the programs and the unit test programs
#   build/<mode>/stage/  the install (prefix /usr); dist/<mode>/ is copied from it
#
# The same files, flags, libraries and link order as upstream's Rakefile had
# (checked against it: the same compiles, installed files and muxed bytes).

CONF     := $(BUILD)/conf
GEN      := $(BUILD)/gen
OBJ      := $(BUILD)/obj
BIN      := $(BUILD)/bin
STAGE    := $(BUILD)/stage
TOOLS    := $(BUILD)/tools
MTX_TMP  := $(BUILD)/tmp
SRC      := $(ROOT)

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

# Every library is required by configure and seen only in our prefix, so it
# stops with a message instead of building without one or with another copy.
# debug: debug information (-g3), no optimization; release: optimized, no
# debug information (the option only adds -g3 -DDEBUG, and no code tests DEBUG)
MTX_CONFIGURE := --prefix=/usr --disable-update-check \
                 $(if $(filter release,$(MODE)),--enable-optimization --disable-debug,--disable-optimization --enable-debug)
MTX_ENV := TMPDIR=$(MTX_TMP) PKG_CONFIG=$(TOOLS)/pkg-config \
           CPPFLAGS='-I$(PREFIX)/include -I$(PREFIX)/include/utf8cpp' LDFLAGS='-L$(PREFIX)/lib'
$(call args_stamp,$(BUILD)/configure-args,$(MTX_CONFIGURE) $(MTX_ENV))

# --- configure ---------------------------------------------------------------
# configure's own files are copied into conf/ and generated there (autoheader,
# autoconf; automake only adds config.guess / config.sub / install-sh).
CONF_FILES := configure.ac build-config.in install-sh $(patsubst $(SRC)/%,%,$(wildcard $(SRC)/ac/*.m4))

$(CONF)/build-config: $(addprefix $(SRC)/,$(CONF_FILES)) $(BUILD)/configure-args $(TOOLS)/pkg-config $(LIB_STAMPS)
	@echo "== mkvtoolnix: configure"
	rm -rf $(CONF) && mkdir -p $(CONF)/ac $(MTX_TMP)
	cd $(SRC) && cp -p $(CONF_FILES) --parents $(CONF)/
	cd $(CONF) && autoheader && autoconf && { automake --add-missing --copy > /dev/null 2>&1 || true; }
	cd $(CONF) && $(MTX_ENV) ./configure $(MTX_CONFIGURE) > $(BUILD)/configure.log

# build-config as make variables, each with an MTX_ in front ($(x) / ${x}
# references too), so none collides with ours
$(CONF)/config.mk: $(CONF)/build-config
	sed -e 's/\$$[({]\([A-Za-z0-9_]*\)[)}]/$$(MTX_\1)/g' -e 's/^\([A-Za-z0-9_][A-Za-z0-9_]*\) *= */MTX_\1 = /' $< > $@

ifeq ($(filter clean libs,$(MAKECMDGOALS)),)
include $(CONF)/config.mk
endif

# --- flags (the Rakefile's, for Linux and gcc) -------------------------------
yes = $(filter yes,$(1))

MTX_CFLAGS_COMMON := -Wall -Wno-comment -Wfatal-errors -D_FILE_OFFSET_BITS=64 -DQT_NO_KEYWORDS \
    -DMTX_LOCALE_DIR=\"$(MTX_localedir)\" -DMTX_PKG_DATA_DIR=\"$(MTX_pkgdatadir)\" -DMTX_DOC_DIR=\"$(MTX_docdir)\" \
    -fstack-protector-strong $(if $(call yes,$(MTX_USE_OPTIMIZATION)),-O3) $(if $(call yes,$(MTX_USE_DEBUG)),-g3 -DDEBUG) \
    $(MTX_MATROSKA_CFLAGS) $(MTX_EBML_CFLAGS) $(MTX_PUGIXML_CFLAGS) $(MTX_CMARK_CFLAGS) $(MTX_DVDREAD_CFLAGS) $(MTX_FLAC_CFLAGS) \
    $(MTX_EXTRA_CFLAGS) $(MTX_USER_CPPFLAGS) -fPIC
MTX_CXXFLAGS := $(MTX_CFLAGS_COMMON) $(MTX_STD_CXX) -Wnon-virtual-dtor -Wextra -Wno-missing-field-initializers -Wunused \
    -Wpedantic -Wno-maybe-uninitialized -Wlogical-op -Wno-extra-semi -Wmisleading-indentation -Wduplicated-cond \
    -Wshadow-compatible-local -Wduplicated-branches -Wno-deprecated-copy -Wno-stringop-overflow \
    $(MTX_QT_CFLAGS) $(MTX_BOOST_CPPFLAGS) $(MTX_USER_CXXFLAGS) $(MTX_GTEST_CFLAGS)
MTX_CFLAGS   := $(MTX_CFLAGS_COMMON) $(MTX_USER_CFLAGS)
# rake's "-I. -Ilib -Ilib/avilib-0.6.10 -Isrc", out of tree: config.h and the
# generated headers first
MTX_INCLUDES := -I$(CONF) -I$(GEN)/src -I$(SRC) -I$(SRC)/lib -I$(SRC)/lib/avilib-0.6.10 -I$(SRC)/src
MTX_LDFLAGS  := -fstack-protector-strong $(MTX_EXTRA_LDFLAGS) $(MTX_USER_LDFLAGS) $(MTX_LDFLAGS_RPATHS) $(MTX_BOOST_LDFLAGS)

# --- the sources ---------------------------------------------------------------
# rake leaves out files for other systems: a path part starting "macos." /
# "macos_" / "windows." / "windows_"
for_linux    = $(shell printf '%s\n' $(1) | grep -Ev '(^|/)(macos|windows)[_.]')
files_in     = $(sort $(wildcard $(addprefix $(SRC)/$(1)/,*.c *.cpp *.cc)))
rel          = $(patsubst $(SRC)/%,%,$(1))
obj_of       = $(addprefix $(OBJ)/,$(addsuffix .o,$(basename $(1))))
qobject_h    = $(call rel,$(shell grep -lw Q_OBJECT $$(find $(SRC)/src/$(1) -name '*.h' ! -path '*/forms/*') 2>/dev/null))
moc_obj_of   = $(addprefix $(OBJ)/,$(addsuffix .moco,$(basename $(1))))

# a library: every source in its folders (minus the exceptions)
# $(call mtx_lib,name,folders,exceptions)
define mtx_lib
$(1)_SRC := $$(call for_linux,$$(filter-out $(3),$$(call rel,$$(foreach d,$(2),$$(call files_in,$$(d))))))
$(1)_LIB := $(OBJ)/lib$(1).a
endef
SRC_COMMON_DIRS := src/common $(call rel,$(sort $(shell find $(SRC)/src/common -mindepth 1 -maxdepth 1 -type d)))
$(eval $(call mtx_lib,avi,lib/avilib-0.6.10,))
$(eval $(call mtx_lib,rmff,lib/librmff,))
$(eval $(call mtx_lib,mpegparser,src/mpegparser,))
$(eval $(call mtx_lib,mtxcommon,$(SRC_COMMON_DIRS),))
$(eval $(call mtx_lib,mtxinput,src/input,))
$(eval $(call mtx_lib,mtxoutput,src/output,))
$(eval $(call mtx_lib,mtxmerge,src/merge,src/merge/mkvmerge.cpp))
$(eval $(call mtx_lib,mtxextract,src/extract,src/extract/mkvextract.cpp))
$(eval $(call mtx_lib,mtxpropedit,src/propedit,src/propedit/propedit.cpp))
$(eval $(call mtx_lib,mtxunittest,tests/unit,))
# the common library's Qt classes (moc), as rake's qt_dependencies_and_sources
mtxcommon_MOCH := $(call for_linux,$(call qobject_h,common))

# the GUI: its forms (uic), its Qt classes (moc), every .cpp
GUI_UI   := $(call rel,$(sort $(shell find $(SRC)/src/mkvtoolnix-gui/forms -name '*.ui')))
GUI_UIH  := $(addprefix $(GEN)/,$(GUI_UI:.ui=.h))
GUI_MOCH := $(call for_linux,$(call qobject_h,mkvtoolnix-gui))
GUI_SRC  := $(call for_linux,$(call rel,$(sort $(shell find $(SRC)/src/mkvtoolnix-gui -name '*.cpp'))))

# mkvextract-gui: a small window for mkvextract (Qt only)
XGUI_MOCH := $(call for_linux,$(call qobject_h,mkvextract-gui))
XGUI_SRC  := $(call for_linux,$(call rel,$(sort $(shell find $(SRC)/src/mkvextract-gui -name '*.cpp'))))

TOOLS_ALL := ac3parser base64tool bluray_dump checksum diracparser dovic_dump dts_dump ebml_validator hevcc_dump \
             pgs_dump vc1parser xyzvc_dump
TEST_APPS := common gui merge propedit

# --- generated files ----------------------------------------------------------
# the version: upstream's rule (the release, ".N" for N commits after its tag)
MTX_DESCRIBE := $(shell git -C $(SRC) describe HEAD 2>/dev/null)
MTX_VERSION_STR := $(MTX_PACKAGE_VERSION)$(shell printf '%s' '$(MTX_DESCRIBE)' | \
    sed -n 's/^release-$(subst .,\.,$(MTX_PACKAGE_VERSION))-\([1-9][0-9]*\)-.*/.\1/p')
$(call args_stamp,$(GEN)/version-args,$(MTX_VERSION_STR))
$(GEN)/src/common/mkvtoolnix_version.h: $(GEN)/version-args
	@mkdir -p $(dir $@)
	printf '#define MKVTOOLNIX_VERSION "%s"\n' '$(MTX_VERSION_STR)' > $@

$(GEN)/%.h: $(SRC)/%.ui
	@mkdir -p $(dir $@)
	$(MTX_UIC) --translate QTR $< > $@

# moc gets the include paths and the Qt defines, as rake gives them
MTX_MOCFLAGS := $(MTX_INCLUDES) $(filter -D%,$(MTX_QT_CFLAGS))
$(GEN)/%.moc: $(SRC)/%.h
	@mkdir -p $(dir $@)
	$(MTX_MOC) $(MTX_MOCFLAGS) -nw $< > $@

# The GUI's resource file. Its .qrc names files as ../../share/...; most are
# icons kept compressed (.svgz) and unpacked for it (rake unpacks them in the
# source tree), so the .qrc and its files are put together in gen/res/.
QRC     := src/mkvtoolnix-gui/qt_resources.qrc
RES     := $(GEN)/res
RES_REL := $(shell sed -n "s/.*<file[^>]*>\.\.\/\.\.\/\(.*\)<\/file>.*/\1/p" $(SRC)/$(QRC))
$(RES)/%.svg: $(SRC)/%.svgz
	@mkdir -p $(dir $@)
	gunzip -c $< > $@
$(RES)/%: $(SRC)/%
	@mkdir -p $(dir $@)
	cp $< $@
# compiled into the GUI (upstream installs it as qt_resources.rcc and reads it
# from /usr/share/mkvtoolnix; built in, a GUI anywhere has its own icons)
$(GEN)/qt_resources.cpp: $(RES)/$(QRC) $(addprefix $(RES)/,$(RES_REL))
	$(MTX_RCC) -name qt_resources -o $@ $<
$(OBJ)/gen/qt_resources.o: $(GEN)/qt_resources.cpp
	@mkdir -p $(dir $@)
	$(MTX_CXX) $(MTX_CXXFLAGS) $(MTX_INCLUDES) -c -o $@ -x c++ $<

MANPAGES := mkvmerge mkvinfo mkvextract mkvpropedit mkvtoolnix-gui mkvextract-gui
$(GEN)/man/%.1: $(SRC)/doc/man/%.xml
	@mkdir -p $(dir $@)
	$(MTX_XSLTPROC) $(MTX_XSLTPROC_FLAGS) -o $@ $(MTX_DOCBOOK_ROOT)/manpages/docbook.xsl $<

# --- compiling ----------------------------------------------------------------
# every object waits for the generated headers (-MMD tracks the rest)
GEN_HEADERS := $(GEN)/src/common/mkvtoolnix_version.h $(GUI_UIH)

$(OBJ)/%.o: $(SRC)/%.cpp | $(GEN_HEADERS)
	@mkdir -p $(dir $@)
	$(MTX_CXX) $(MTX_CXXFLAGS) $(MTX_INCLUDES) -c -MMD -MP -o $@ -x c++ $<
$(OBJ)/%.o: $(SRC)/%.c | $(GEN_HEADERS)
	@mkdir -p $(dir $@)
	$(MTX_CC) $(MTX_CFLAGS) $(MTX_INCLUDES) -c -MMD -MP -o $@ $<
$(OBJ)/%.moco: $(GEN)/%.moc | $(GEN_HEADERS)
	@mkdir -p $(dir $@)
	$(MTX_CXX) $(MTX_CXXFLAGS) $(MTX_INCLUDES) -c -MMD -MP -o $@ -x c++ $<

ALL_LIBS := avi rmff mpegparser mtxcommon mtxinput mtxoutput mtxmerge mtxextract mtxpropedit mtxunittest
define mtx_lib_rule
$$($(1)_LIB): $$(call obj_of,$$($(1)_SRC)) $$(call moc_obj_of,$$($(1)_MOCH))
	@mkdir -p $$(dir $$@)
	rm -f $$@ && ar crS $$@ $$^ && ranlib $$@
endef
$(foreach l,$(ALL_LIBS),$(eval $(call mtx_lib_rule,$(l))))

# --- linking (the Rakefile's library order) -----------------------------------
LIB = $($(1)_LIB)
COMMON_LIBS := $(call LIB,mtxcommon) $(MTX_MATROSKA_LIBS) $(MTX_EBML_LIBS) $(MTX_BOOST_FILESYSTEM_LIB) $(MTX_FLAC_LIBS) \
    -lz $(MTX_PUGIXML_LIBS) $(MTX_LIBINTL_LIBS) $(MTX_ICONV_LIBS) -lfmt $(MTX_STDCPPFS_LIBS)
COMMON_TAIL := -lgmp -lstdc++ $(MTX_CMARK_LIBS) $(MTX_DVDREAD_LIBS)
COMMON_CLI  := $(COMMON_LIBS) $(MTX_QT_LIBS_NON_GUI) $(COMMON_TAIL)
link = $(MTX_CXX) $(MTX_LDFLAGS) -o $@ $(1)

$(BIN)/mkvmerge: $(OBJ)/src/merge/mkvmerge.o $(foreach l,mtxmerge mtxinput mtxoutput mtxcommon avi rmff mpegparser,$(call LIB,$(l)))
	@mkdir -p $(dir $@)
	$(call link,$< $(call LIB,mtxmerge) $(call LIB,mtxinput) $(call LIB,mtxoutput) $(call LIB,mtxmerge) $(COMMON_CLI) \
	  $(call LIB,avi) $(call LIB,rmff) $(call LIB,mpegparser) $(call LIB,mtxcommon) -lgmp -lvorbis -logg $(MTX_LINK_STATICALLY))
$(BIN)/mkvinfo: $(call obj_of,$(call for_linux,$(call rel,$(call files_in,src/info)))) $(call LIB,mtxcommon)
	@mkdir -p $(dir $@)
	$(call link,$(filter %.o,$^) $(COMMON_CLI) $(MTX_LINK_STATICALLY))
$(BIN)/mkvextract: $(OBJ)/src/extract/mkvextract.o $(foreach l,mtxextract mtxcommon avi rmff,$(call LIB,$(l)))
	@mkdir -p $(dir $@)
	$(call link,$< $(call LIB,mtxextract) $(COMMON_CLI) $(call LIB,avi) $(call LIB,rmff) -lvorbis -logg $(MTX_LINK_STATICALLY))
$(BIN)/mkvpropedit: $(OBJ)/src/propedit/propedit.o $(foreach l,mtxpropedit mtxcommon,$(call LIB,$(l)))
	@mkdir -p $(dir $@)
	$(call link,$< $(call LIB,mtxpropedit) $(COMMON_CLI) $(MTX_LINK_STATICALLY))
$(BIN)/mkvtoolnix-gui: $(call moc_obj_of,$(GUI_MOCH)) $(call obj_of,$(GUI_SRC)) $(OBJ)/gen/qt_resources.o $(call LIB,mtxcommon)
	@mkdir -p $(dir $@)
	$(call link,$(filter %.o %.moco,$^) $(COMMON_LIBS) $(COMMON_TAIL) $(MTX_QT_LIBS) -lvorbis -logg $(MTX_LINK_STATICALLY))

$(BIN)/mkvextract-gui: $(call moc_obj_of,$(XGUI_MOCH)) $(call obj_of,$(XGUI_SRC))
	@mkdir -p $(dir $@)
	$(call link,$(filter %.o %.moco,$^) $(MTX_QT_LIBS))

tool_src = src/tools/$(1).cpp $(if $(filter ebml_validator,$(1)),src/tools/element_info.cpp)
define mtx_tool
$(BIN)/$(1): $$(call obj_of,$$(call tool_src,$(1))) $$(call LIB,mtxcommon)
	@mkdir -p $$(dir $$@)
	$$(call link,$$(filter %.o,$$^) $$(COMMON_CLI))
endef
$(foreach t,$(TOOLS_ALL),$(eval $(call mtx_tool,$(t))))

# the unit tests: tests/unit/<app>/*.cpp on libmtxunittest + GoogleTest
test_libs_merge    := mtxmerge
test_libs_propedit := mtxpropedit
test_srcs_gui      := src/mkvtoolnix-gui/util/json.cpp src/mkvtoolnix-gui/util/string.cpp
define mtx_test
$(BIN)/tests/$(1): $$(call obj_of,$$(call rel,$$(call files_in,tests/unit/$(1))) $$(test_srcs_$(1))) \
                   $$(foreach l,$$(test_libs_$(1)) mtxunittest mtxcommon,$$(call LIB,$$(l)))
	@mkdir -p $$(dir $$@)
	$$(call link,$$(filter %.o,$$^) $$(foreach l,$$(test_libs_$(1)),$$(call LIB,$$(l))) $$(call LIB,mtxunittest) \
	  $$(COMMON_CLI) $$(MTX_GTEST_LIBS) -lpthread)
endef
$(foreach t,$(TEST_APPS),$(eval $(call mtx_test,$(t))))

PROGRAMS := mkvmerge mkvinfo mkvextract mkvpropedit mkvtoolnix-gui mkvextract-gui
.PHONY: mtx-programs mtx-tests mtx-man
mtx-programs: $(addprefix $(BIN)/,$(PROGRAMS) $(TOOLS_ALL))
mtx-tests: $(addprefix $(BIN)/tests/,$(TEST_APPS))
mtx-man: $(addprefix $(GEN)/man/,$(addsuffix .1,$(MANPAGES)))

# --- install (as upstream's install task; English only) -----------------------
INSTALL_DATA := install -m 644
D = $(STAGE)
.PHONY: mtx-install
mtx-install: mtx-programs mtx-man
	@echo "== mkvtoolnix: install into build/$(MODE)/stage"
	rm -rf $(D)
	install -d $(D)$(MTX_bindir) $(D)$(MTX_man1dir) $(D)$(MTX_desktopdir) $(D)$(MTX_mimepackagesdir) \
	    $(D)$(MTX_metainfodir) $(D)$(MTX_icondir)/scalable/apps $(D)$(MTX_pkgdatadir)/sounds
	install -t $(D)$(MTX_bindir) $(addprefix $(BIN)/,$(PROGRAMS) bluray_dump ebml_validator hevcc_dump xyzvc_dump)
	$(INSTALL_DATA) -t $(D)$(MTX_man1dir) $(addprefix $(GEN)/man/,$(addsuffix .1,$(MANPAGES)))
	$(INSTALL_DATA) -t $(D)$(MTX_mimepackagesdir) $(wildcard $(SRC)/share/mime/*.xml)
	$(INSTALL_DATA) -t $(D)$(MTX_desktopdir) $(SRC)/share/desktop/org.bunkus.mkvtoolnix-gui.desktop $(SRC)/share/desktop/mkvextract-gui.desktop
	$(INSTALL_DATA) -t $(D)$(MTX_metainfodir) $(SRC)/share/metainfo/org.bunkus.mkvtoolnix-gui.metainfo.xml
	for p in mkvextract:mkvextract mkvinfo:mkvinfo mkvmerge:mkvtoolnix mkvpropedit:mkvpropedit mkvtoolnix-gui:mkvtoolnix; do \
	    $(INSTALL_DATA) $(RES)/share/icons/mkvtoolnix/$${p#*:}.svg $(D)$(MTX_icondir)/scalable/apps/$${p%%:*}.svg; done
	$(INSTALL_DATA) -t $(D)$(MTX_pkgdatadir)/sounds $(wildcard $(SRC)/share/sounds/*.webm)
	sh $(ROOT)/mk/check-links.sh $(D)$(MTX_bindir)

# dist/<mode>/: only the six programs, side by side (the debs carry the rest);
# release ones stripped (build/release/ keeps them as linked)
dist: mtx-install
	@echo "== dist/$(MODE)"
	rm -rf $(DIST)
	@mkdir -p $(DIST)
	install -t $(DIST) $(addprefix $(STAGE)$(MTX_bindir)/,$(PROGRAMS))
	$(if $(filter release,$(MODE)),strip --strip-unneeded $(addprefix $(DIST)/,$(PROGRAMS)))

# ./build.sh test: the unit tests, run as upstream's tests:run_unit ran them
# (GoogleTest from libs/); its product tests needed sample files that are not
# public and are not part of MKVToolNix-Mod
test: mtx-tests
	@echo "== tests"
	for t in $(TEST_APPS); do LC_ALL=C $(BIN)/tests/$$t || exit 1; done

-include $(shell find $(OBJ) -name '*.d' 2>/dev/null)

# generated files (moc, uic, the unpacked icons) are kept, not removed as
# intermediate files after use
.SECONDARY:
