#!/bin/sh
# deb.sh <stage> <out folder> <version> <maintainer> <work folder>
#
# The packages mkvtoolnix and mkvtoolnix-gui from a release build's staged
# install (rake install DESTDIR=<stage>, prefix /usr), the way upstream's
# packaging/debian makes them: the same files per package (*.install), the
# same descriptions, preinst scripts, copyright and changelog; programs
# stripped. Our own version (with an epoch, so apt does not replace the
# packages with upstream's), maintainer and dependencies (dpkg-shlibdeps:
# only the host's Qt, Boost and C / C++ runtime, our libraries are linked
# in). No -dbg package.
set -e
stage=$1 out=$2 version=$3 maintainer=$4 work=$5
root=$(cd "$(dirname "$0")/.." && pwd)
deb=$root/packaging/debian
arch=$(dpkg --print-architecture)
exclude='org.bunkus.mkvtoolnix-gui.xml'      # as upstream's dh_install -X

rm -rf "$work"; mkdir -p "$work" "$out"
cd "$stage"
find . -type f -o -type l | sed 's#^\./##' | grep -v "$exclude" | sort > "$work/staged"
: > "$work/packaged"

# the stanza of one package in upstream's control template, with our values
stanza() {
    awk -v p="Package: $1" '$0 == p { on = 1 } on && /^$/ { exit } on' "$deb/control.erb" |
    sed -e "s/\${binary:Version}/$version/" -e 's/, *\${misc:Depends}//' -e '/^Package:/d' -e '/^Architecture:/d' |
    # a relation field continued on the next line goes on one line
    awk '/^ / && f ~ /^(Depends|Suggests|Replaces):/ { sub(/^ +/, " "); line = line $0; next }
         { if (line != "") print line; line = $0; f = $0 }
         END { if (line != "") print line }'
}

for pkg in mkvtoolnix mkvtoolnix-gui; do
    p=$work/$pkg
    mkdir -p "$p/DEBIAN" "$p/usr/share/doc/$pkg"

    # the files: each line of <pkg>.install is a path or pattern under the stage
    while read -r pattern; do
        [ -n "$pattern" ] || continue
        found=0
        for f in $pattern; do
            [ -e "$f" ] || continue
            found=1
            cp -a --parents "$f" "$p/"
        done
        [ $found = 1 ] || { echo "deb.sh: $pkg: nothing staged for '$pattern'" >&2; exit 1; }
    done < "$deb/$pkg.install"
    find "$p" -path "*$exclude" -delete
    (cd "$p" && find usr -type f -o -type l) | sort >> "$work/packaged"

    # programs stripped, man pages and the changelog compressed (Debian policy)
    for f in "$p"/usr/bin/*; do
        [ -f "$f" ] && strip --strip-unneeded --remove-section=.comment --remove-section=.note "$f"
    done
    find "$p/usr/share/man" -name '*.1' -exec gzip -9n {} + 2>/dev/null || true
    install -m 644 "$deb/copyright" "$p/usr/share/doc/$pkg/copyright"
    gzip -9nc "$root/NEWS.md" > "$p/usr/share/doc/$pkg/changelog.gz"
    printf '%s (%s) unstable; urgency=low\n\n  * MKVToolNix %s, built by MKVToolNix-Mod (./build.sh deb).\n\n -- %s  %s\n' \
           $pkg "$version" "${version#*:}" "$maintainer" "$(date -R -r "$stage")" |
        gzip -9n > "$p/usr/share/doc/$pkg/changelog.Debian.gz"
    if [ $pkg = mkvtoolnix ]; then
        install -m 644 -t "$p/usr/share/doc/$pkg" "$root/AUTHORS" "$root/README.md"
        mkdir -p "$p/usr/share/doc/$pkg/examples" && cp -a "$root"/examples/* "$p/usr/share/doc/$pkg/examples/"
    fi
    # docs over 4 kB compressed, as dh_compress does (copyright never)
    find "$p/usr/share/doc" -type f -size +4k ! -name '*.gz' ! -name copyright -exec gzip -9n {} +
    [ -f "$deb/$pkg.menu" ] && install -D -m 644 "$deb/$pkg.menu" "$p/usr/share/menu/$pkg"
    [ -f "$deb/$pkg.preinst" ] && install -m 755 "$deb/$pkg.preinst" "$p/DEBIAN/preinst"

    # dependencies of the programs (dpkg-shlibdeps wants a debian/control)
    mkdir -p "$work/shlibs-$pkg/debian"
    printf 'Source: mkvtoolnix\n\nPackage: %s\nArchitecture: any\n' $pkg > "$work/shlibs-$pkg/debian/control"
    shlibs=$(cd "$work/shlibs-$pkg" && dpkg-shlibdeps -O "$p"/usr/bin/* 2>/dev/null | sed -n 's/^shlibs:Depends=//p')
    [ -n "$shlibs" ] || { echo "deb.sh: $pkg: dpkg-shlibdeps gave nothing" >&2; exit 1; }

    {
        echo "Package: $pkg"
        echo "Version: $version"
        echo "Architecture: $arch"
        echo "Maintainer: $maintainer"
        echo "Installed-Size: $(du -sk --exclude=DEBIAN "$p" | cut -f1)"
        echo "Section: video"
        echo "Priority: optional"
        echo "Homepage: https://mkvtoolnix.download/"
        stanza $pkg | sed "s/\${shlibs:Depends}/$shlibs/"
    } > "$p/DEBIAN/control"
    # permissions as Debian's dh_fixperms sets them (not the build's umask)
    find "$p" -type d -exec chmod 755 {} +
    find "$p/usr" -type f -exec chmod 644 {} +
    chmod 755 "$p"/usr/bin/*
    (cd "$p" && find usr -type f -exec md5sum {} + | sort -k2) > "$p/DEBIAN/md5sums"
    chmod 644 "$p/DEBIAN/control" "$p/DEBIAN/md5sums"

    dpkg-deb --root-owner-group -Zxz -z9 --build "$p" "$out/${pkg}_${version#*:}_$arch.deb" >/dev/null
    echo "deb.sh: $out/${pkg}_${version#*:}_$arch.deb"
done

# every staged file is in a package (as upstream's dh_missing --fail-missing)
sort -o "$work/packaged" "$work/packaged"
missing=$(comm -23 "$work/staged" "$work/packaged")
if [ -n "$missing" ]; then
    echo "deb.sh: staged but in no package:" >&2
    echo "$missing" | sed 's/^/  /' >&2
    exit 1
fi
