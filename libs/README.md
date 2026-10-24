# libs — our libraries

These are **our copies** of the libraries MKVToolNix-Mod is built on, each
at its current stable release. They replace MKVToolNix's git submodules
(`lib/fmt`, `lib/libebml`, `lib/libmatroska`) and its older bundled copies
(`lib/pugixml`, `lib/nlohmann-json`, `lib/utf8-cpp`); MKVToolNix's own pieces
in `lib/` (avilib, librmff) stay where they are.

Each library was added in a commit of its own holding the upstream release
unmodified, so `git log -- libs/<name>` / `git diff <that commit> --
libs/<name>` show every change we make. Where a library needs different
behaviour it is edited directly; nothing is kept as a patch series.

From the host instead (not in here): Qt 6 (base, Multimedia, SVG), Boost
(headers + filesystem), the C / C++ runtime, and the build tools.

`libs/.gitignore` (ours) re-includes every file, so that MKVToolNix's own
ignore patterns (`*.1`, `*.o`, `*.exe`, ...) cannot leave a library file out.

## What is changed today

Nothing: every folder is its upstream release byte for byte (git cannot
store empty folders: zlib's two are not kept).

## The releases

| Folder | Upstream release | Source | Check |
|---|---|---|---|
