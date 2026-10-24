MKVToolNix-Mod (MKVToolNix 103.0)
==================================

# Table of contents

1. [Introduction](#1-introduction)
2. [Building MKVToolNix-Mod](#2-building-mkvtoolnix-mod)
    1. [What the host needs](#21-what-the-host-needs)
    2. [Building](#22-building)
    3. [Unit tests](#23-unit-tests)
3. [Reporting bugs & security issues & getting support](#3-reporting-bugs-security-issues-getting-support)
    1. [Reporting bugs](#31-reporting-bugs)
    2. [Reporting seucrity issues)(#32-reporting-security-issues)
    3. [Getting support](#33-getting-support)
4. [Tests](#4-tests)
5. [Code of Conduct](#5-code-of-conduct)
6. [Included third-party components and their licenses](#6-included-third-party-components-and-their-licenses)
    1. [avilib](#61-avilib)
    2. [libEBML](#63-libebml)
    3. [libMatroska](#64-libmatroska)
    4. [librmff](#65-librmff)
    5. [nlohmann's JSON](#66-nlohmanns-json)
    6. [pugixml](#67-pugixml)
    7. [utf8-cpp](#68-utf8-cpp)
    8. [Oxygen icons and sound files](#69-oxygen-icons-and-sound-files)
    9. [MKVToolNix icons](#610-mkvtoolnix-icons)
    10. [QtWaitingSpinner](#611-qtwaitingspinner)
    11. [Fancy tab widget](#612-fancy-tab-widget)
    12. [fmt](#613-fmt)
    13. [QgsCollapsibleGroupBox](#613-qgscollapsiblegroupbox)

-----------------

# 1. Introduction

With these tools one can get information about (via mkvinfo) Matroska
files, extract tracks/data from (via mkvextract) Matroska files and create
(via mkvmerge) Matroska files from other media files. Matroska is a new
multimedia file format aiming to become THE new container format for
the future. You can find more information about it and its underlying
technology, the Extensible Binary Meta Language (EBML), at

http://www.matroska.org/

The full documentation for each command is now maintained in its
man page only. Type `mkvmerge -h` to get you started.

This code comes under the GPL v2 (see www.gnu.org or the file COPYING).
Modify as needed.

The icons are based on the work of Alexandr Grigorcea and modified by
Eduard Geier. They're licensed under the terms of the
[Creative Commons Attribution 3.0 Unported license](http://creativecommons.org/licenses/by/3.0/).

The newest version can always be found at
https://mkvtoolnix.download/

Moritz Bunkus <mo@bunkus.online>


# 2. Building MKVToolNix-Mod

MKVToolNix-Mod is MKVToolNix 103.0 with the changes listed in
[MOD_CHANGES.md](MOD_CHANGES.md). It builds on Linux, with make, from
one script; the old rake build and the other platforms are not part of
it.

## 2.1. What the host needs

The libraries MKVToolNix is built on are in `libs/` (our copies of
their releases, see [libs/README.md](libs/README.md)) and are built
and linked in statically. From the host come Qt 6 (base, Multimedia,
SVG), Boost (headers and `filesystem`), the C / C++ runtime and the
build tools. `build.sh` checks for them and names the missing Debian /
Ubuntu packages:

    gcc g++ make cmake meson ninja-build pkg-config rsync git autoconf
    automake m4 xsltproc gzip binutils docbook-xsl libboost-dev
    libboost-filesystem-dev qt6-base-dev qt6-base-dev-tools
    qt6-multimedia-dev qt6-svg-dev

and for the packages also `dpkg-dev`.

## 2.2. Building

    ./build.sh            # debug build   -> dist/debug/
    ./build.sh release    # release build -> dist/release/
    ./build.sh test       # debug build of the unit tests, then runs them
    ./build.sh deb        # release build, then the packages -> dist/deb/
    ./build.sh clean      # removes build/ and dist/

`dist/<mode>/` holds the six programs, flat: `mkvmerge`, `mkvinfo`,
`mkvextract`, `mkvpropedit`, `mkvtoolnix-gui` and `mkvextract-gui`. A
release build has no debug information and is stripped; the GUI's
icons are built into it.

`dist/deb/` holds the packages `mkvtoolnix` and `mkvtoolnix-gui`
(version `1:103.0-1mod1`), which install like the distribution's own
packages (`sudo apt install ./dist/deb/*.deb`). The GUI's sounds and
the man pages come with them.

Everything else stays in `build/<mode>/`: the libraries (`prefix/`),
MKVToolNix's configure output, generated sources and objects, the
programs with the helper tools and the unit tests (`bin/`), and the
install tree the packages are made from (`stage/`). The source tree
and `libs/` are never written to. Running `build.sh` again rebuilds
only what changed. The GUI's online update check is always off.

## 2.3. Unit tests

The unit tests are in `tests/unit/` (Google Test, from `libs/`), one
program per part: `common`, `merge`, `propedit` and `gui`.
`./build.sh test` builds and runs them all. To build them without
running them, and to run a part of them:

    make -f mk/main.mk MODE=debug mtx-tests
    build/debug/bin/tests/common --gtest_filter='FrameClock*'

# 3. Reporting bugs & security issues & getting support

# 3.1. Reporting bugs

If you're sure you've found a bug — e.g. if one of my programs crashes
with an obscur error message, or if the resulting file is missing part
of the original data, then by all means submit a bug report.

I use [Codeberg's issue system](https://codeberg.org/mbunkus/mkvtoolnix/issues)
as my bug database. You can submit your bug reports there. Please be as
verbose as possible — e.g. include the command line, if you use Windows
or Linux etc.pp.

If at all possible, please include sample files as well so that I can
reproduce the issue. If they are larger than 1 MB, please upload
them somewhere and post a link in the issue. You can also upload them
to my FTP server. Details on how to connect can be found in the
[MKVToolNix FAQ](https://codeberg.org/mbunkus/mkvtoolnix/wiki/FTP-server).

# 3.2. Reporting security issues

If you think you've found a security issue, please reach out via email
to the author, Moritz Bunkus <mo@bunkus.online>. Please don't file an
issue on Codeberg for it as security issues are usually sensitive &
Codeberg doesn't support private/restricted access to issues. I will
prioritize reported security issues over regular bug reports. Thank
you.

# 3.3. Getting support

The issue tracker above is not meant for general support which you can
find in the following places:

* The [MKVToolNix help forum](https://help.mkvtoolnix.download/) is
  suitable for all kinds of questions.
* The MKVToolNix thread on [Doom9's
  forum](http://forum.doom9.org/showthread.php?t=155732) is more
  suited for in-depth technical questions.
* There's also the IRC channel `#matroska` on the [Libera IRC
  network](https://libera.chat/) where we hang out. The main
  MKVToolNix author Moritz Bunkus is known as "mosu" there.


# 4. Tests

The unit tests in `tests/unit/` (section 2.3) are MKVToolNix-Mod's test
suite. Upstream's second suite, which checks the programs' output on
sample files, needs files that are not public and is not part of
MKVToolNix-Mod, nor is upstream's continuous integration.

# 5. Code of Conduct

Please note that this project is released with a
[Code of Conduct](CODE_OF_CONDUCT.md). By participating in this project
you agree to abide by its terms.

# 6. Included third-party components and their licenses

MKVToolNix includes and uses the following libraries & artwork:

## 6.1. avilib

Reading and writing AVI files. Originally part of the `transcode`
package.

  * Copyright: 1999 Rainer Johanni <Rainer@Johanni.de>
  * License: GNU General Public License v2 or later
  * URL: the `transcode` project doesn't seem to have a home page anymore
  * Corresponding files: `lib/avilib-0.6.10/*`

## 6.2. libEBML

A C++ library to parse EBML files

  * Copyright: 2002-2021 Steve Lhomme et. al.
  * License: GNU Lesser General Public License v2.1 or later (see `doc/licenses/LGPL-2.1.txt`)
  * URL: http://www.matroska.org/
  * Corresponding files: `lib/libebml/*`

## 6.3. libMatroska

A C++ library to parse Matroska files

  * Copyright: 2002-2020 Steve Lhomme et. al.
  * License: GNU Lesser General Public License v2.1 or later (see `doc/licenses/LGPL-2.1.txt`)
  * URL: http://www.matroska.org/
  * Corresponding files: `lib/libmatroska/*`

## 6.4. librmff

librmff is short for 'RealMedia file format access library'. It aims
at providing the programmer an easy way to read and write RealMedia
files.

  * Copyright: Moritz Bunkus
  * License: GNU Lesser General Public License v2.1 or later (see `doc/licenses/LGPL-2.1.txt`)
  * URL: https://www.bunkus.org/videotools/librmff/index.html
  * Corresponding files: `lib/librmff/*`

## 6.5. nlohmann's JSON

JSON for Modern C++

  * Copyright: 2013-2021 Niels Lohmann
  * License: MIT (see `doc/licenses/nlohmann-json-MIT.txt`)
  * URL: https://github.com/nlohmann/json
  * Corresponding files: `lib/nlohmann-json/*`

## 6.6. pugixml

An XML processing library

  * Copyright: 2006–2020 by Arseny Kapoulkine <arseny.kapoulkine@gmail.com>
  * License: MIT (see `doc/licenses/pugixml-MIT.txt`)
  * URL: https://pugixml.org/
  * Corresponding files: `lib/pugixml/*`

## 6.7. utf8-cpp

UTF-8 with C++ in a Portable Way

  * Copyright: 2006-2021 Nemanja Trifunovic
  * License: Boost Software License 1.0 (see `doc/licenses/Boost-1.0.txt`)
  * URL: https://github.com/nemtrif/utfcpp/
  * Corresponding files: `lib/utf8-cpp/*`

## 6.8. Oxygen icons and sound files

A lot of the icons included in this package originate from the Oxygen
Project. These include all files in the `share/icons/oxygen`
sub-directory.

All of the sound files in the `share/sounds` sub-directory originate
from the Oxygen project.

  * License: GNU Lesser General Public License v3 (see `doc/licenses/LGPL-3.0.txt`)
  * URL: https://techbase.kde.org/Projects/Oxygen
  * Corresponding files:
    * `share/icons/oxygen/*`
    * `share/sounds/*`

## 6.9. MKVToolNix icons

  * Copyright:
    * 2011 Alexandr Grigorcea <cahr.gr@gmail.com>
    * 2012 Eduard Geier <edu.g@online.de>
    * 2012 Ben Humpert <ben@an3k.de>
  * License: Creative Commons Attribution 3.0 Unported (CC BY 3.0) (see `doc/licenses/CC-BY-3.0.txt`)
  * Corresponding files: `share/icons/*/mkv*`

## 6.10. QtWaitingSpinner

A highly configurable, custom Qt widget for showing "waiting" or
"loading" spinner icons in Qt applications

  * Copyright:
    * 2012–2014 by Alexander Turkin
    * 2014 by William Hallatt
    * 2015 by Jacob Dawid
  * License: MIT (see `doc/licenses/QtWaitingSpinner-MIT.txt`)
  * URL: https://github.com/snowwlex/QtWaitingSpinner
  * Corresponding files: `src/mkvtoolnix-gui/util/waiting_spinning_widget.{h,cpp}`

## 6.11. Fancy tab widget

A beefed-up tab widget class for Qt extracted from the Qt Creator project

  * Copyright: 2011 Nokia Corporation and/or its subsidiary(-ies).
  * License: GNU General Public License v2 (see `COPYING`)
  * Corresponding files: `src/mkvtoolnix-gui/util/fancy_tab_widget.{h,cpp}`

## 6.12. fmt

Small, safe and fast formatting library

  * Copyright: 2012–present by Victor Zverovich
  * License: BSD (see `doc/licenses/fmt-BSD.txt`)
  * URL: https://fmt.dev/latest/
  * Corresponding files: `lib/fmt/*`

## 6.13. QgsCollapsibleGroupBox

A groupbox that collapses/expands when toggled. Extracted from the QGIS project.

  * Copyright: 2012–present by Etienne Tourigny
  * License: GNU General Public License v2 or later (see `COPYING`)
  * URL: https://github.com/qgis/QGIS
  * Corresponding files: `src/mkvtoolnix-gui/util/qgs_collapsible_group_box.{h,cpp}`

## 6.14. Windows theme

The theme (palette) used on Windows

  * Copyright: 2016 The Qt Company Ltd.
  * License: GNU General Public License v2 (see `COPYING`)
  * Corresponding files: `src/mkvtoolnix-gui/app/windows.cpp`

