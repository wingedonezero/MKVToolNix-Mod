# ./build.sh deb: the packages mkvtoolnix and mkvtoolnix-gui from the release
# build, into dist/deb/ (mk/deb.sh says how they are made).
#
# Version <epoch>:<upstream version>-<our revision>: the epoch keeps apt from
# replacing them with upstream's packages (which have none); raise
# DEB_REVISION for a new build of the same upstream version.

DEB_EPOCH    := 1
DEB_REVISION := 1mod1
MTX_VERSION  := $(shell sed -n '/^AC_INIT/{s/[^,]*,\[//;s/\].*//;p;}' $(ROOT)/configure.ac)
DEB_VERSION  := $(DEB_EPOCH):$(MTX_VERSION)-$(DEB_REVISION)
DEB_MAINT    := $(shell git -C $(ROOT) config user.name) <$(shell git -C $(ROOT) config user.email)>

deb: mtx-stage
ifneq ($(MODE),release)
	$(error the packages are made from the release build: make -f mk/main.mk deb MODE=release)
endif
	@echo "== dist/deb ($(DEB_VERSION))"
	sh $(ROOT)/mk/deb.sh $(STAGE) $(ROOT)/dist/deb '$(DEB_VERSION)' '$(DEB_MAINT)' $(BUILD)/deb
