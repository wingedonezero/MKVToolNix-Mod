/*
   mkvextract-gui -- a small window for mkvextract

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   the names of the files written
*/

#pragma once

#include <QString>

#include "mkvextract-gui/identify.h"

namespace mtx::extract_gui {

// File name patterns (without the extension). Placeholders:
//   {FilenameNoExt} {Filename}
//   {TrackID} {TrackID:00} {TrackNumber} {TrackNumber:00} (also :0 / :000)
//   {TrackName} {Language} {LanguageIETF} {CodecID} {Codec} {Forced}
//   {Delay} {EffectiveDelay} {PixelWidth} {PixelHeight} {SamplingFrequency} {Channels}
//   {AttachmentID} {AttachmentFilename} {MimeType} {AttachmentFileSize}
// {Codec} is a short label (Mpeg2, Mpeg4, DD, DTS-HD MA, PGS, ...), {Forced}
// " (forced)" for a forced track, {Delay} the track's first timestamp and
// {EffectiveDelay} that minus the video's (ms).
struct Patterns {
  QString video      = QStringLiteral("{FilenameNoExt}_{TrackID:00}_[{Language}]_{Codec}");
  QString audio      = QStringLiteral("{FilenameNoExt}_{TrackID:00}_[{Language}]_{Codec}{Forced} DELAY {EffectiveDelay}ms");
  QString subtitles  = QStringLiteral("{FilenameNoExt}_{TrackID:00}_[{Language}]_{Codec}{Forced}");
  QString chapters   = QStringLiteral("{FilenameNoExt}_chapters");
  QString tags       = QStringLiteral("{FilenameNoExt}_tags");
  QString attachment = QStringLiteral("{AttachmentFilename}");

  void load();
  void save() const;
};

QString codecLabel(Track const &track);
QString extension(Track const &track);

// base names (no extension, no folder)
QString trackName(Patterns const &patterns, FileInfo const &info, Track const &track);
QString attachmentName(Patterns const &patterns, FileInfo const &info, Attachment const &attachment);
QString fileName(QString const &pattern, FileInfo const &info);

// folder + name + extension; with overwrite off, " (1)", " (2)", ... before
// the extension until the name is not taken (by a file or by taken)
QString outputPath(QString const &folder, QString const &baseName, QString const &extension, bool overwrite, QStringList &taken);

}
