/*
   mkvextract-gui -- a small window for mkvextract

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   what a Matroska file holds, as mkvmerge's JSON identification says it
*/

#pragma once

#include <QList>
#include <QString>

#include <optional>

namespace mtx::extract_gui {

struct Track {
  int id{}, number{};
  QString type;                 // video, audio, subtitles, buttons
  QString codec, codecId, language, name;
  bool forced{}, isDefault{};
  std::optional<qint64> minimumTimestamp; // ns: the track's first timestamp
  QString pixelDimensions;
  int samplingFrequency{}, channels{};
};

struct Attachment {
  int id{};
  QString fileName, mimeType, description;
  qint64 size{};
};

struct FileInfo {
  QString path, title, containerType;
  qint64 duration{};            // ns; 0: not known
  QList<Track> tracks;
  QList<Attachment> attachments;
  int chapterEntries{}, globalTagEntries{}, trackTagEntries{};
  bool hasCueSheet() const { return chapterEntries > 0; }
};

// the mkvmerge (or mkvextract) next to this program, else the one in PATH
QString toolPath(QString const &name);

// Runs mkvmerge -J on path; on failure error says why.
std::optional<FileInfo> identify(QString const &path, QString &error);

}
