/*
   mkvextract-gui -- a small window for mkvextract

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   what a Matroska file holds, as mkvmerge's JSON identification says it
*/

#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

#include "mkvextract-gui/identify.h"

using namespace Qt::StringLiterals;

namespace mtx::extract_gui {

QString
toolPath(QString const &name) {
  auto next = QCoreApplication::applicationDirPath() + u'/' + name;
  return QFileInfo{next}.isExecutable() ? next : name;
}

std::optional<FileInfo>
identify(QString const &path,
         QString &error) {
  QProcess process;
  process.start(toolPath(u"mkvmerge"_s), { u"-J"_s, path });
  if (!process.waitForFinished(-1) || (process.exitStatus() != QProcess::NormalExit)) {
    error = QObject::tr("mkvmerge could not be run: %1").arg(process.errorString());
    return {};
  }

  auto json = QJsonDocument::fromJson(process.readAllStandardOutput()).object();
  auto container = json.value(u"container"_s).toObject();

  if (!container.value(u"recognized"_s).toBool() || !container.value(u"supported"_s).toBool()) {
    auto errors = json.value(u"errors"_s).toArray();
    error = errors.isEmpty() ? QObject::tr("not a file mkvmerge can read") : errors.first().toString();
    return {};
  }

  FileInfo info;
  auto containerProperties = container.value(u"properties"_s).toObject();
  info.path          = path;
  info.containerType = container.value(u"type"_s).toString();
  info.title         = containerProperties.value(u"title"_s).toString();
  info.duration      = containerProperties.value(u"duration"_s).toInteger();

  for (auto const &value : json.value(u"tracks"_s).toArray()) {
    auto object     = value.toObject();
    auto properties = object.value(u"properties"_s).toObject();
    Track track;

    track.id                = object.value(u"id"_s).toInt();
    track.type              = object.value(u"type"_s).toString();
    track.codec             = object.value(u"codec"_s).toString();
    track.number            = properties.value(u"number"_s).toInt();
    track.codecId           = properties.value(u"codec_id"_s).toString();
    track.language          = properties.value(u"language"_s).toString();
    track.name              = properties.value(u"track_name"_s).toString();
    track.forced            = properties.value(u"forced_track"_s).toBool();
    track.isDefault         = properties.value(u"default_track"_s).toBool();
    track.pixelDimensions   = properties.value(u"pixel_dimensions"_s).toString();
    track.samplingFrequency = properties.value(u"audio_sampling_frequency"_s).toInt();
    track.channels          = properties.value(u"audio_channels"_s).toInt();
    if (properties.contains(u"minimum_timestamp"_s))
      track.minimumTimestamp = properties.value(u"minimum_timestamp"_s).toInteger();

    info.tracks << track;
  }

  for (auto const &value : json.value(u"attachments"_s).toArray()) {
    auto object = value.toObject();
    info.attachments << Attachment{ object.value(u"id"_s).toInt(),        object.value(u"file_name"_s).toString(),
                                    object.value(u"content_type"_s).toString(), object.value(u"description"_s).toString(),
                                    object.value(u"size"_s).toInteger() };
  }

  for (auto const &value : json.value(u"chapters"_s).toArray())
    info.chapterEntries += value.toObject().value(u"num_entries"_s).toInt();
  for (auto const &value : json.value(u"global_tags"_s).toArray())
    info.globalTagEntries += value.toObject().value(u"num_entries"_s).toInt();
  for (auto const &value : json.value(u"track_tags"_s).toArray())
    info.trackTagEntries += value.toObject().value(u"num_entries"_s).toInt();

  return info;
}

}
