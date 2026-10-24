# MKVToolNix-Mod: changes from MKVToolNix 103.0

MKVToolNix-Mod is MKVToolNix 103.0 changed so that streams from video discs
(DVD, HD DVD, Blu-ray, UHD) go into Matroska and come back out exactly. This
file lists every change: what it does, why, and what it means for a program
that runs mkvmerge / mkvextract (section 1 first, for that).

Each change is one commit (`git log release-103.0..`); the commit messages
have the full details and checks.

## 1. For programs using MKVToolNix-Mod

What a program that called MKVToolNix 103 has to know:

| What | Before (103.0) | Now | What to do |
|---|---|---|---|
| Command line, options files (`@file.json`), `mkvmerge -J` | | unchanged | nothing |
| Video times from elementary streams (MPEG-1/2, H.264, HEVC, VC-1) | each frame length cut to whole ns and added up: about 1 frame in 20 stored 1 ms early | counted exactly from the stream's own clock, rounded to the ms once | drop workarounds for 103's early frames (e.g. moving subtitles to match them); `--engage cut_frame_durations` gives 103's times again |
| H.264 soft pulldown (pic_struct) from elementary streams | every frame two fields (a 3:2 stream ran 20 % fast) | the fields each frame says | nothing; the header stays 29.97 as before |
| VC-1 skipped pictures with a repeated field | the repeat ignored (HD DVD films up to ~1 s early) | counted | nothing |
| Timestamp files (`--timestamps`) | | unchanged | nothing |
| HD DVD sub-pictures (VobSub .idx/.sub) | cut to ~2 KB, unreadable | kept whole | nothing |
| H.264 access unit delimiters | dropped | kept, first in their frame | nothing; extracted .h264 starts with the AUD |
| HEVC parameter sets | VPS/SPS/PPS put in front of every key frame | left as in the stream (added only where an appended encode needs them) | nothing; `--engage normalize_parameter_sets` = 103's way |
| mkvextract, VC-1 (`V_MS/VFW/FOURCC`, WVC1) | an AVI file | a `.vc1` elementary stream | expect `.vc1`, not `.avi` |
| mkvextract, H.264 / HEVC start | SPS/PPS from the header put in front even when the first frame has them | only the ones the first frame lacks, behind its AUD | nothing |
| Splitting (`--split`) | each part began with a fraction of a ms of the previous chapter | the part's own chapter at 0 | nothing (`%c` names are right now) |
| Interface languages | ~30 listed, none installed | English only | do not pass `--ui-language` other than `en_US` |
| mkvextract-gui | | a new small program | optional |
| Install | | debs `mkvtoolnix` + `mkvtoolnix-gui` `1:103.0-1mod1` | see section 5 |

Not changed (known, left as upstream has them):

- A split or remux of an MKV (MKV in, MKV out) takes the stored ms times:
  TrueHD frames (0.833 ms) that share a ms are put 1 ms later (about 1 in 6);
  remuxing from the extracted elementary stream gives the exact times back.
- mkvextract of a split part of a title made of several encodes (a "play all"
  of separately encoded episodes): the header's PPS of the first encode is
  written before the part's first frame (decoding is unaffected).
- `--fix-bitstream-timing-information` writes 60 instead of 59.94 into the
  SPS for 60000/1001p (upstream issue #6317).
- H.264 end of sequence / end of stream NAL units are dropped (no effect on
  decoding).
- Frame times are stored in milliseconds (TimestampScale 1,000,000).

## 2. Timing

### Exact frame clock (ea68e974b, 175c542d9, 9660b776c, 92b9d6c46)

**What.** Frames without a timestamp of their own (every frame of an
elementary stream file; the frames between PES times in a program stream) are
counted in whole units (fields, or frames for HEVC) from the last known
timestamp, and each time is worked out exactly as start + count x unit
(`mtx::frame_clock_c`, exact fractions) and rounded to the nearest ns once.
MPEG-1/2, H.264, HEVC and VC-1 use it. A changed frame rate starts a new
count at the current time.

**Why.** 103.0 added up frame lengths cut to whole nanoseconds (33,366,666
instead of 33,366,666.67 at 29.97; 41,708,333 at 23.976), so the times fell
further behind the stream's clock with every frame; stored in ms, more and
more frames landed 1 ms early (on a 23.976 Blu-ray episode 1,540 of 34,095;
on a DVD 15,371 of 260,830). Any truncation per frame is wrong; counting does
not drift.

**Result.** Every picture's stored time = the disc's own time rounded to the
ms (checked on DVD, Blu-ray MPEG-2 / H.264, UHD HEVC, HD DVD VC-1). The
header (DefaultDuration) is unchanged.

**Switch.** `--engage cut_frame_durations` = 103.0's times (the MKV is then
byte-identical to 103.0's on MPEG-2 and VC-1, and on H.264 / HEVC apart from
the access unit delimiters and parameter sets below).

### H.264 soft pulldown (a02ec319d)

**What.** The picture timing SEI's `pic_struct` is read (with the frame's SPS),
and a frame lasts the fields H.264 Table D-1 gives (2, 1, 1, 2, 2, 3, 3, 4, 6).
A frame missing its picture timing message (when the SPS says there should be
one) lasts two fields, with a warning, as FFmpeg does.

**Why.** 103.0 never read pic_struct: a 3:2 soft-pulldown stream (frames
shown for three fields) was timed 20 % too fast (4.004 s came out as 3.203 s).

**Result.** Times as FFmpeg's decoder gives them; the header stays the
stream's own two-field frame (29.97), as 103.0 and the MPEG-2 path give it.
HEVC pic_struct is not read (UHD is progressive).

### VC-1 skipped pictures (568fc6d9f)

**What.** A skipped picture's TFF / RFF (or RPTFRM) is read like any other
picture's (SMPTE 421M).

**Why.** 103.0 stopped reading at "skipped", so a skipped picture that repeats
a field counted as two fields: HD DVD films with pulldown ran up to about
1 s early by the end.

## 3. Streams kept as they are

### HD DVD sub-pictures (145373ec4)

**What.** The .idx/.sub reader and the sub-picture code read HD DVD units
(32-bit sizes and offsets, their extra commands), so each unit goes into the
MKV and back out whole; the duration comes from the unit's own stop command.

**Why.** 103.0 read them as DVD units (16-bit): every HD DVD unit was cut to
about 2 KB and no decoder could show it. DVD units are unchanged (MKVs
byte-identical to 103.0's).

### H.264 access unit delimiters (95e343b14)

**What.** Access unit delimiters are kept, as the first NAL unit of their
frame (H.264 and HEVC). mkvextract writes the header's SPS/PPS with the first
frame, behind its AUD, and only those it does not carry itself.

**Why.** 103.0 dropped H.264 AUDs (once dropped they cannot be restored;
MPEG-2 TS requires them, ISO/IEC 14496-15 allows them), and mkvextract put a
duplicate SPS/PPS in front of the first frame. Now an H.264 stream extracts
back unit for unit.

### HEVC parameter sets (88df37e0c)

**What.** VPS/SPS/PPS are left as the stream has them. They are put in front
of a key frame only while the active ones differ from the header's (an
appended encode with other settings) and the frame does not carry them.
`hvcC` is written with array_completeness 0, as FFmpeg writes it.

**Why.** Since 2021 upstream put all parameter sets in front of every key
frame, which changed every stream that carries them once. The appended-encode
case it was made for (#3034) still decodes.

**Switches.** `--engage normalize_parameter_sets` = 103.0's way;
`--engage dont_normalize_parameter_sets` = never add any.

### mkvextract: VC-1 as .vc1 (5e7ce2589)

**What.** A VC-1 track (`V_MS/VFW/FOURCC`, FourCC WVC1) is written as a VC-1
elementary stream (SMPTE 421M Annex E): sequence header and entry point from
the header unless the first frame has them, then the frames, each with its
start code. Other VfW codecs still go to AVI.

**Why.** 103.0 wrote every VfW track as AVI, which disc tools do not take for
VC-1. VC-1 stays stored as `V_MS/VFW/FOURCC` (players do not read `V_VC1`).

## 4. Splitting

### No leftover of the previous chapter (62e538ee0)

**What.** When the chapters for a split part are picked, a chapter starting
less than 1 ms after the part's first frame starts the part (at 0), and one
reaching less than 1 ms into the part is left out of it.

**Why.** A part starts with a key frame stored up to 1 ms before its split
point (frame times are whole ms; chapters are exact): the chapter before
seemed to reach a fraction of a millisecond into the part and was kept at 0
in front of the part's own chapter, and `%c` named the file after it
(upstream issue #3781 is the audio-only form). Splits in the middle of a
chapter keep that chapter as before. Only the chapter lists change.

## 5. Programs and build

### mkvextract-gui (12e95b28d, a2ca2f02a, 3829b328d, 88669e2c6, 92b0815d7)

A small separate program, laid out like gMKVExtractGUI, to extract from
Matroska files with mkvextract: files and folders, each file's tracks /
chapters / attachments / tags to check, one mkvextract pass per file, jobs,
a log, and file names in Disc-Remuxer's style by default
(`<file>_<ID>_[<language>]_<codec>`, audio with ` DELAY <n>ms`, forced
tracks with ` (forced)`), extensions as mkvextract writes them. In the
`mkvtoolnix-gui` package, with a desktop entry and a man page.

### GUI icons built in, dist/ (d80f0c1d5)

The GUI's icons are compiled into `mkvtoolnix-gui` (103.0 read them from
`/usr/share/mkvtoolnix/qt_resources.rcc`, so a GUI started from the build
folder took the installed MKVToolNix's file). `dist/<mode>/` holds only the
six programs; release builds have no debug information and are stripped
(800 MB to 40 MB).

### Interface languages: English only (d921b36f4)

The translations are not part of MKVToolNix-Mod; the language list now holds
only English (it still named about 30 languages that could not be loaded).

### Build: make, libraries in libs/, Linux only (dd5eae893 ... 9d1644f06, a5e79acd7)

- `./build.sh [debug|release|test|deb|clean]` with make (`mk/`) replaces the
  rake build (see the README).
- The libraries (libebml, libmatroska, fmt, pugixml, nlohmann JSON,
  utf8-cpp, libogg, libvorbis, FLAC, zlib, cmark, GMP, libdvdread,
  GoogleTest) are our copies of their current releases in `libs/`, built and
  linked in statically (`libs/README.md`: versions, checksums, the one
  change).
- Linux only: the Windows / macOS packaging, the AppImage script, the
  translations and upstream's Ruby product tests are removed.
- The packages `mkvtoolnix` and `mkvtoolnix-gui` (`./build.sh deb`) have
  version `1:103.0-1mod1` and replace the installed packages of the same
  name (`sudo apt install ./dist/deb/*.deb`). The epoch 1 makes them newer
  than every version without one (upstream's repository at
  mkvtoolnix.download, e.g. `103.0-0~debian13bunkus01`, and later releases
  there), so apt does not replace them by itself; to go back, install a
  specific upstream version (`apt install mkvtoolnix=<version>
  mkvtoolnix-gui=<version>`).
