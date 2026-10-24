#!/bin/sh
# check-config.sh <build-config> <prefix>
#
# After configure: every library must come from our prefix. configure falls
# back to MKVToolNix's internal copies when it cannot find one, and those
# copies are gone (libs/ replaces them), so a fallback is refused here
# instead of failing later or linking something else.
set -e
cfg=$1 prefix=$2
fail=0

value() { sed -n "s/^$1 *= *//p" "$cfg" | head -1; }

want() {    # want KEY VALUE
    v=$(value "$1")
    if [ "$v" != "$2" ]; then
        echo "check-config: $1 = '$v' (wanted '$2')" >&2
        fail=1
    fi
}

ours() {    # ours KEY: the libraries it names are linked from our prefix
    v=$(value "$1")
    case "$v" in
        *"-L$prefix/lib"*) ;;
        # a plain -l (libogg, libvorbis: found without pkg-config): LDFLAGS
        # puts our prefix first; check-links.sh confirms it after the build
        *-L*|"") echo "check-config: $1 = '$v' (not from $prefix)" >&2; fail=1 ;;
    esac
}

want EBML_MATROSKA_INTERNAL no
want FMT_INTERNAL no
want PUGIXML_INTERNAL no
want NLOHMANN_JSON_INTERNAL no
want UTF8CPP_INTERNAL no
want GTEST_TYPE system
want USE_DVDREAD yes
want BUILD_GUI yes
for key in EBML_LIBS MATROSKA_LIBS PUGIXML_LIBS FLAC_LIBS OGG_LIBS VORBIS_LIBS ZLIB_LIBS CMARK_LIBS DVDREAD_LIBS GTEST_LIBS; do
    ours $key
done

if [ $fail != 0 ]; then
    echo "check-config: configure did not take every library from $prefix; see build/<mode>/configure.log" >&2
    exit 1
fi
echo "check-config: every library from $prefix"
