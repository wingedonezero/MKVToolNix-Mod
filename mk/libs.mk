# The libraries in libs/: each with its own build system, out of tree, static
# only, installed into build/<mode>/prefix/ (include/, lib/, lib/pkgconfig/).
# libs/ itself is never written to. A library is rebuilt when one of its files,
# its arguments or a library it is built on changes.

LIB_TYPE := $(if $(filter release,$(MODE)),Release,Debug)
STAMPS   := $(BUILD)/libs/stamps

CMAKE_COMMON := -G Ninja -DCMAKE_BUILD_TYPE=$(LIB_TYPE) -DBUILD_SHARED_LIBS=OFF \
                -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
                -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_PREFIX_PATH=$(PREFIX) \
                -DCMAKE_FIND_USE_PACKAGE_REGISTRY=OFF -DBUILD_TESTING=OFF

# $(call cmake_lib,name,extra arguments,libraries it is built on)
define cmake_lib
$(1)_ARGS := $(CMAKE_COMMON) $(2)
$$(call args_stamp,$(BUILD)/libs/$(1)/args,$$($(1)_ARGS))
$(STAMPS)/$(1): $(BUILD)/libs/$(1)/args $$(call src_files,$(LIBS)/$(1)) $(foreach d,$(3),$(STAMPS)/$(d))
	@echo "== $(1)"
	@mkdir -p $(BUILD)/libs/$(1)/build $(STAMPS)
	cmake -S $(LIBS)/$(1) -B $(BUILD)/libs/$(1)/build $$($(1)_ARGS) > $(BUILD)/libs/$(1)/cmake.log
	cmake --build $(BUILD)/libs/$(1)/build -j $(JOBS) > $(BUILD)/libs/$(1)/build.log
	cmake --install $(BUILD)/libs/$(1)/build > $(BUILD)/libs/$(1)/install.log
	touch $$@
endef

$(eval $(call cmake_lib,libebml,,))
$(eval $(call cmake_lib,libmatroska,,libebml))
# (no "d" on the debug library's name: MKVToolNix's configure links -lfmt)
$(eval $(call cmake_lib,fmt,-DFMT_TEST=OFF -DFMT_DOC=OFF -DFMT_INSTALL=ON -DFMT_DEBUG_POSTFIX=,))
$(eval $(call cmake_lib,pugixml,,))
$(eval $(call cmake_lib,nlohmann-json,-DJSON_BuildTests=OFF -DJSON_Install=ON,))
$(eval $(call cmake_lib,utf8-cpp,,))
$(eval $(call cmake_lib,libogg,-DINSTALL_DOCS=OFF,))
$(eval $(call cmake_lib,libvorbis,,libogg))
$(eval $(call cmake_lib,flac,-DBUILD_CXXLIBS=OFF -DBUILD_PROGRAMS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_DOCS=OFF \
                             -DINSTALL_MANPAGES=OFF -DWITH_OGG=ON,libogg))
$(eval $(call cmake_lib,zlib,-DZLIB_BUILD_SHARED=OFF -DZLIB_BUILD_STATIC=ON -DZLIB_BUILD_TESTING=OFF,))
$(eval $(call cmake_lib,cmark,,))
$(eval $(call cmake_lib,googletest,-DINSTALL_GTEST=ON,))

# GMP: its own configure + make. Its generated files (configure, Makefile.in,
# ...) must not look older than their inputs, or make would try to rerun the
# autotools, so it is built from a copy whose files all carry one time.
GMP_ARGS := --prefix=$(PREFIX) --libdir=$(PREFIX)/lib --disable-shared --enable-static --with-pic
$(call args_stamp,$(BUILD)/libs/gmp/args,$(GMP_ARGS))
$(STAMPS)/gmp: $(BUILD)/libs/gmp/args $(call src_files,$(LIBS)/gmp)
	@echo "== gmp"
	@mkdir -p $(BUILD)/libs/gmp/build $(STAMPS)
	rsync -a --delete $(LIBS)/gmp/ $(BUILD)/libs/gmp/src/
	find $(BUILD)/libs/gmp/src -exec touch -h -d '2000-01-01 00:00:00' {} +
	cd $(BUILD)/libs/gmp/build && $(BUILD)/libs/gmp/src/configure $(GMP_ARGS) > ../configure.log
	$(MAKE) -C $(BUILD)/libs/gmp/build -j$(JOBS) > $(BUILD)/libs/gmp/build.log
	$(MAKE) -C $(BUILD)/libs/gmp/build install > $(BUILD)/libs/gmp/install.log
	touch $@

# libdvdread: meson; libdvdcss is left out (only the IFO tables are read, for
# DVD chapters).
DVDREAD_ARGS := --prefix=$(PREFIX) --libdir=lib --default-library=static --buildtype=$(MODE) \
                -Db_staticpic=true -Dlibdvdcss=disabled -Denable_docs=false
$(call args_stamp,$(BUILD)/libs/libdvdread/args,$(DVDREAD_ARGS))
$(STAMPS)/libdvdread: $(BUILD)/libs/libdvdread/args $(call src_files,$(LIBS)/libdvdread)
	@echo "== libdvdread"
	@mkdir -p $(BUILD)/libs/libdvdread $(STAMPS)
	rm -rf $(BUILD)/libs/libdvdread/build
	meson setup $(BUILD)/libs/libdvdread/build $(LIBS)/libdvdread $(DVDREAD_ARGS) > $(BUILD)/libs/libdvdread/meson.log
	ninja -C $(BUILD)/libs/libdvdread/build -j $(JOBS) install > $(BUILD)/libs/libdvdread/build.log
	touch $@

LIB_NAMES := libebml libmatroska fmt pugixml nlohmann-json utf8-cpp libogg libvorbis flac zlib cmark googletest \
             gmp libdvdread
LIB_STAMPS := $(foreach n,$(LIB_NAMES),$(STAMPS)/$(n))

libs: $(LIB_STAMPS)
