#!/bin/sh
# sync.sh <source> <copy> <list file>
#
# The copy of the MKVToolNix source that rake builds in: exactly the files git
# knows (tracked ones, and new ones not yet added that no ignore pattern
# matches), without our folders (libs/, mk/) and build.sh. The build's own
# files in the copy are left alone; a source file that is gone (deleted,
# renamed) is deleted from the copy too, found by the list of the last sync.
#
# Not .gitignore as a filter: MKVToolNix's patterns also match tracked files
# (`*.1` matches the folder rake.d/vendor/drake-0.9.2.0.3.1).
set -e
src=$1 copy=$2 list=$3
mkdir -p "$copy"
new=$list.new
cd "$src"
git ls-files -z --cached --others --exclude-standard -- . ':!libs' ':!mk' ':!build.sh' |
    xargs -0 sh -c 'for f; do [ -e "$f" ] || [ -L "$f" ] && printf "%s\0" "$f"; done' sh > "$new"
rsync -a --from0 --files-from="$new" "$src/" "$copy/"
if [ -f "$list" ]; then
    tr '\0' '\n' < "$list" | sort > "$list.old.txt"
    tr '\0' '\n' < "$new" | sort > "$list.new.txt"
    comm -23 "$list.old.txt" "$list.new.txt" | while IFS= read -r f; do
        rm -f "$copy/$f"
        echo "sync: removed $f (gone from the source)"
    done
    rm -f "$list.old.txt" "$list.new.txt"
fi
mv "$new" "$list"
