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

Every file of a release is added with `git add -f`, so neither MKVToolNix's
ignore patterns (`*.1`, `*.o`, `*.exe`, ...) nor a library's own
`.gitignore` (libmatroska's ignores its `Makefile`s) can leave a file out;
`libs/.gitignore` (ours) re-includes everything against the top-level
patterns for later edits.

## What is changed today

Nothing: every folder is its upstream release byte for byte (git cannot
store empty folders: zlib's two are not kept).

## The releases

| Folder | Upstream release | Source | Check |
|---|---|---|---|
| `libebml/` | libebml 1.4.7 | https://dl.matroska.org/downloads/libebml/libebml-1.4.7.tar.xz | tarball sha256 `5b08214f929ee54c6187c370f84235fc9d0f2a2258c4d320d68eae6e2bdfd3f7` (sha512 = published `.sha512.txt`); signature good (Moritz Bunkus, D919 9745 B054 5F2E 8197 062B 0F92 290A 445B 9007, signing subkey 3301 A29D 88D0 1A0C F999 954F 74AF 00AD F2E3 2C85) |
| `libmatroska/` | libmatroska 1.7.2 | https://dl.matroska.org/downloads/libmatroska/libmatroska-1.7.2.tar.xz | tarball sha256 `f3e4d406daf7f1399962c43940e4a87de089474d5bcfbf2e6ca516e98bb87cfc` (sha512 = published `.sha512.txt`); signature as libebml |
