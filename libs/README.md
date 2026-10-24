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

Every folder is its upstream release byte for byte (git cannot store empty
folders: zlib's two are not kept), except:

| Library | Change | Why |
|---|---|---|
| `libebml/` | `EBMLConfig.cmake.in`: `find_dependency(utf8cpp)` before its targets are loaded (+5 lines) | built static against an installed utf8-cpp, its `EBML::ebml` target names `utf8cpp::utf8cpp`, which its package file never loaded, so libmatroska's `find_package(EBML)` failed. (Without an installed utf8-cpp, libebml downloads utf8-cpp 3.2.5 from GitHub at build time; the build now builds ours first and switches downloads off.) |

## The releases

| Folder | Upstream release | Source | Check |
|---|---|---|---|
| `libebml/` | libebml 1.4.7 | https://dl.matroska.org/downloads/libebml/libebml-1.4.7.tar.xz | tarball sha256 `5b08214f929ee54c6187c370f84235fc9d0f2a2258c4d320d68eae6e2bdfd3f7` (sha512 = published `.sha512.txt`); signature good (Moritz Bunkus, D919 9745 B054 5F2E 8197 062B 0F92 290A 445B 9007, signing subkey 3301 A29D 88D0 1A0C F999 954F 74AF 00AD F2E3 2C85) |
| `libmatroska/` | libmatroska 1.7.2 | https://dl.matroska.org/downloads/libmatroska/libmatroska-1.7.2.tar.xz | tarball sha256 `f3e4d406daf7f1399962c43940e4a87de089474d5bcfbf2e6ca516e98bb87cfc` (sha512 = published `.sha512.txt`); signature as libebml |
| `fmt/` | fmt 12.2.0 | https://github.com/fmtlib/fmt/releases/tag/12.2.0 (`fmt-12.2.0.zip`) | zip sha256 `a2f4a8d51178f954e4c339007f77edd76ba0cb2e36f87a48e5a5403d9be5878f` (= the digest GitHub shows for the release asset); no signature published |
| `pugixml/` | pugixml 1.16 | https://github.com/zeux/pugixml/releases/tag/v1.16 (`pugixml-1.16.tar.gz`) | tarball sha256 `4cee1ca4aad395170f4c7a07824f3bdd41f28316c6e1e1090a1425b278ec0b4b` (= the digest GitHub shows for the release asset); no signature published |
| `nlohmann-json/` | nlohmann JSON 3.12.0 | https://github.com/nlohmann/json/releases/tag/v3.12.0 (`json.tar.xz`: the sources without tests and docs) | tarball sha256 `42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa`; signature good (Niels Lohmann, 7971 67AE 41C0 A6D9 232E 4845 7F3C EA63 AE25 1B69, the fingerprint in its README) |
| `utf8-cpp/` | utfcpp 4.2.1 | https://github.com/nemtrif/utfcpp/releases/tag/v4.2.1 (`archive/refs/tags/v4.2.1.tar.gz`) | tarball sha256 `6d6a5493a111884cc085ee31babfe6d9960c8fb08fc80a64852eaeea8323dbc1` (GitHub tag archive; no asset, digest or signature published) |
| `libogg/` | libogg 1.3.6 | https://downloads.xiph.org/releases/ogg/libogg-1.3.6.tar.xz | tarball sha256 `5c8253428e181840cd20d41f3ca16557a9cc04bad4a3d04cce84808677fa1061` (= published `SHA256SUMS`); no signature published |
| `libvorbis/` | libvorbis 1.3.7 | https://downloads.xiph.org/releases/vorbis/libvorbis-1.3.7.tar.xz | tarball sha256 `b33cc4934322bcbf6efcbacf49e3ca01aadbea4114ec9589d1b1e9d20f72954b` (= published `SHA256SUMS`); signature good (Ralph Giles, B7B0 0AEE 1F96 0EEA 0FED 66FB 9259 A8F2 D2D4 4C84; made 2020-07-04, the key has expired since; key from keyserver.ubuntu.com by the signature's fingerprint, not cross-checked on the project's site) |
| `flac/` | FLAC 1.5.0 | https://downloads.xiph.org/releases/flac/flac-1.5.0.tar.xz | tarball sha256 `f2c1c76592a82ffff8413ba3c4a1299b6c7ab06c734dee03fd88630485c2b920` (= published `SHA256SUMS.txt`); no signature published |
| `zlib/` | zlib 1.3.2 | https://zlib.net/zlib-1.3.2.tar.xz | tarball sha256 `d7a0654783a4da529d1bb793b7ad9c3318020af77667bcae35f95d0e42a792f3`; signature good (Mark Adler, 5ED4 6A67 21D3 6558 7791 E2AA 783F CD8E 58BC AFBA). Its two empty folders are not kept (git cannot store them) |
| `cmark/` | cmark 0.31.2 | https://github.com/commonmark/cmark/releases/tag/0.31.2 (`archive/refs/tags/0.31.2.tar.gz`) | tarball sha256 `f9bc5ca38bcb0b727f0056100fac4d743e768872e3bacec7746de28f5700d697` (GitHub tag archive; no asset, digest or signature published) |
| `gmp/` | GMP 6.3.0 | https://gmplib.org/download/gmp/gmp-6.3.0.tar.xz | tarball sha256 `a3c2b80201b89e68616f4ad30bc66aee4927c3ce50e33929ca819d5c43538898`; signature good (Niels Möller, 343C 2FF0 FBEE 5EC2 EDBE F399 F359 9FF8 28C6 7298; made 2023-07-30, the key has expired since; key from keyserver.ubuntu.com by the signature's fingerprint, not cross-checked on the project's site) |
| `libdvdread/` | libdvdread 7.1.1 | https://download.videolan.org/pub/videolan/libdvdread/7.1.1/ | tarball sha256 `a0d47876548bec806774bbf8dbf20bb19ba139464383156b32eb8e59915b90a9` (= published `.sha256`); signature good (VideoLAN Release Signing Key, 65F7 C6B4 206B D057 A7EB 7378 7180 713B E58D 1ADC; made 2026-07-20, the key has expired since; key from keyserver.ubuntu.com by the signature's fingerprint, not cross-checked on the project's site) |
| `googletest/` | GoogleTest 1.18.0 | https://github.com/google/googletest/releases/tag/v1.18.0 (`googletest-1.18.0.tar.gz`) | tarball sha256 `6e3191c1455468b3fc35a417fb565c1c5071aee1b7e7f85e30cf48a98d37d8b5` (= the digest GitHub shows for the release asset); no signature published |
