#!/bin/sh
# check-links.sh <bin folder>
#
# After the build: no program may load a shared copy of a library we link in
# from libs/ (that would be the host's, not ours). Only the host's Qt, Boost
# and the C / C++ runtime are loaded at run time.
set -e
fail=0
for f in "$1"/*; do
    [ -f "$f" ] || continue
    needed=$(readelf -d "$f" 2>/dev/null | sed -n 's/.*(NEEDED).*\[\(.*\)\]/\1/p')
    for lib in $needed; do
        case "$lib" in
            libebml.so*|libmatroska.so*|libfmt.so*|libpugixml.so*|libogg.so*|libvorbis*.so*|libFLAC.so*|libz.so*| \
            libcmark.so*|libgmp.so*|libdvdread.so*|libgtest*.so*)
                echo "check-links: $(basename "$f") loads $lib (should be linked in from libs/)" >&2
                fail=1 ;;
        esac
    done
done
[ $fail = 0 ] || exit 1
echo "check-links: no program loads a host copy of our libraries"
