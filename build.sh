#!/bin/sh
# Builds MKVToolNix-Mod: the libraries in libs/ (static, linked in), then
# MKVToolNix on them. Qt 6, Boost and the C / C++ runtime are the host's.
#
#   ./build.sh            debug build    -> dist/debug/   (bin/, share/)
#   ./build.sh release    release build  -> dist/release/
#   ./build.sh test       debug build of the unit tests, then runs them
#   ./build.sh deb        release build, then the packages mkvtoolnix and
#                         mkvtoolnix-gui -> dist/deb/
#   ./build.sh clean      removes build/ and dist/
#
# Everything else stays in build/<mode>/ (mk/main.mk says what is where);
# libs/ and the source tree are never written to. Running it again rebuilds
# only what changed. The GUI's online update check is always off.
set -e
cd "$(dirname "$0")"

mode=${1:-debug}
case $mode in
    debug|release|test|deb|clean) ;;
    *) echo "usage: $0 [debug|release|test|deb|clean]" >&2; exit 2 ;;
esac

if [ "$mode" = clean ]; then
    rm -rf build dist
    exit 0
fi

# what the host must have (Debian 13 package names)
missing=
need_tool() {   # need_tool <program> <package>
    command -v "$1" >/dev/null 2>&1 || missing="$missing $2"
}
need_file() {   # need_file <path pattern> <package>
    for f in $1; do [ -e "$f" ] && return 0; done
    missing="$missing $2"
}
need_tool cc gcc;            need_tool c++ g++;           need_tool make make
need_tool cmake cmake;       need_tool meson meson;       need_tool ninja ninja-build
need_tool pkg-config pkg-config;  need_tool rsync rsync;  need_tool git git
need_tool autoconf autoconf; need_tool automake automake; need_tool m4 m4
need_tool xsltproc xsltproc; need_tool gunzip gzip
need_tool qmake6 qt6-base-dev-tools;  need_tool readelf binutils
need_file /usr/share/xml/docbook/stylesheet/docbook-xsl docbook-xsl
need_file /usr/include/boost/version.hpp libboost-dev
need_file '/usr/lib/*/libboost_filesystem.so' libboost-filesystem-dev
pkg-config --exists Qt6Widgets 2>/dev/null || missing="$missing qt6-base-dev"
pkg-config --exists Qt6Multimedia 2>/dev/null || missing="$missing qt6-multimedia-dev"
pkg-config --exists Qt6Svg 2>/dev/null || missing="$missing qt6-svg-dev"
if [ "$mode" = deb ]; then
    need_tool dpkg-deb dpkg; need_tool dpkg-shlibdeps dpkg-dev
fi
if [ -n "$missing" ]; then
    echo "build.sh: missing on this system:$missing" >&2
    echo "          (Debian / Ubuntu: sudo apt install$missing)" >&2
    exit 1
fi

case $mode in
    debug|release) exec make -j"$(nproc)" -f mk/main.mk dist MODE=$mode ;;
    test)          exec make -j"$(nproc)" -f mk/main.mk test MODE=debug ;;
    deb)           exec make -j"$(nproc)" -f mk/main.mk deb MODE=release ;;
esac
