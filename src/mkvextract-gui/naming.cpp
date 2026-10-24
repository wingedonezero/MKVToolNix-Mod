/*
   mkvextract-gui -- a small window for mkvextract

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   the names of the files written
*/

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSettings>

#include <cmath>

#include "mkvextract-gui/naming.h"

using namespace Qt::StringLiterals;

namespace mtx::extract_gui {

namespace {

// {Name} or {Name:00}: the value, numbers padded to the zeros' count
QString
expand(QString pattern,
       QHash<QString, QString> const &values,
       QHash<QString, qint64> const &numbers) {
  static QRegularExpression const s_placeholder{uR"(\{([A-Za-z]+)(?::(0+))?\})"_s};

  QString result;
  qsizetype last = 0;
  for (auto it = s_placeholder.globalMatch(pattern); it.hasNext(); ) {
    auto match = it.next();
    auto name  = match.captured(1);
    QString value;

    if (numbers.contains(name))
      value = u"%1"_s.arg(numbers[name], match.captured(2).size(), 10, QChar{u'0'});
    else if (values.contains(name))
      value = values[name];
    else
      value = match.captured(0);            // not a placeholder: kept as it is

    result += pattern.mid(last, match.capturedStart() - last) + value;
    last    = match.capturedEnd();
  }

  return result + pattern.mid(last);
}

// no folder separators, no characters other systems refuse (as Disc-Remuxer: DTS:X -> DTS-X)
QString
safe(QString name) {
  static QRegularExpression const s_unsafe{uR"([/\\:*?"<>|\x00-\x1f])"_s};
  return name.replace(s_unsafe, u"-"_s).trimmed();
}

QHash<QString, QString>
fileValues(FileInfo const &info) {
  QFileInfo file{info.path};
  return { { u"FilenameNoExt"_s, file.completeBaseName() }, { u"Filename"_s, file.fileName() } };
}

}

void
Patterns::load() {
  QSettings settings;
  settings.beginGroup(u"patterns"_s);
  video      = settings.value(u"video"_s,      video).toString();
  audio      = settings.value(u"audio"_s,      audio).toString();
  subtitles  = settings.value(u"subtitles"_s,  subtitles).toString();
  chapters   = settings.value(u"chapters"_s,   chapters).toString();
  tags       = settings.value(u"tags"_s,       tags).toString();
  attachment = settings.value(u"attachment"_s, attachment).toString();
}

void
Patterns::save()
  const {
  QSettings settings;
  settings.beginGroup(u"patterns"_s);
  settings.setValue(u"video"_s,      video);
  settings.setValue(u"audio"_s,      audio);
  settings.setValue(u"subtitles"_s,  subtitles);
  settings.setValue(u"chapters"_s,   chapters);
  settings.setValue(u"tags"_s,       tags);
  settings.setValue(u"attachment"_s, attachment);
}

QString
codecLabel(Track const &track) {
  auto const &id = track.codecId;

  if (id == u"V_MPEG1"_s)                                   return u"Mpeg1"_s;
  if (id == u"V_MPEG2"_s)                                   return u"Mpeg2"_s;
  if (id == u"V_MPEG4/ISO/AVC"_s)                           return u"Mpeg4"_s;
  if (id == u"V_MPEGH/ISO/HEVC"_s)                          return u"MpegH"_s;
  if ((id == u"V_MS/VFW/FOURCC"_s) && (track.codec == u"VC-1"_s)) return u"VC-1"_s;
  if (id == u"A_AC3"_s)                                     return u"DD"_s;
  if (id == u"A_EAC3"_s)                                    return u"DDplus"_s;
  if (id == u"A_TRUEHD"_s)                                  return track.codec.contains(u"Atmos"_s) ? u"TrueHD Atmos"_s : u"TrueHD"_s;
  if (id == u"A_MLP"_s)                                     return u"MLP"_s;
  if (id == u"A_DTS"_s)                                     return track.codec.contains(u"Master Audio"_s)    ? u"DTS-HD MA"_s
                                                                 : track.codec.contains(u"High Resolution"_s) ? u"DTS-HD HR"_s
                                                                 : track.codec.contains(u"Express"_s)         ? u"DTS Express"_s
                                                                 :                                             u"DTS"_s;
  if (id.startsWith(u"A_MPEG/L"_s))                         return u"MPEG audio"_s;
  if (id.startsWith(u"A_AAC"_s))                            return u"AAC"_s;
  if (id.startsWith(u"A_PCM"_s))                            return u"LPCM"_s;
  if (id == u"S_HDMV/PGS"_s)                                return u"PGS"_s;
  if (id == u"S_HDMV/TEXTST"_s)                             return u"TextST"_s;
  if (id == u"S_VOBSUB"_s)                                  return u"VobSub"_s;
  if (id.startsWith(u"S_TEXT/UTF8"_s) || (id == u"S_TEXT/ASCII"_s)) return u"SRT"_s;

  return safe(track.codec);
}

// what mkvextract writes for the codec (xtr_base_c::create_extractor)
QString
extension(Track const &track) {
  static QList<std::pair<QString, QString>> const s_byCodecId{
    { u"V_MPEG4/ISO/AVC"_s,  u"h264"_s   }, { u"V_MPEGH/ISO/HEVC"_s, u"hevc"_s }, { u"V_MPEG1"_s,       u"m1v"_s  },
    { u"V_MPEG2"_s,          u"m2v"_s    }, { u"V_AV1"_s,            u"ivf"_s  }, { u"V_VP8"_s,         u"ivf"_s  },
    { u"V_VP9"_s,            u"ivf"_s    }, { u"V_THEORA"_s,         u"ogv"_s  }, { u"V_REAL/"_s,       u"rm"_s   },
    { u"V_UNCOMPRESSED"_s,   u"raw"_s    }, { u"A_AC3"_s,            u"ac3"_s  }, { u"A_EAC3"_s,        u"eac3"_s },
    { u"A_DTS"_s,            u"dts"_s    }, { u"A_TRUEHD"_s,         u"thd"_s  }, { u"A_MLP"_s,         u"mlp"_s  },
    { u"A_MPEG/L3"_s,        u"mp3"_s    }, { u"A_MPEG/L2"_s,        u"mp2"_s  }, { u"A_MPEG/L1"_s,     u"mp1"_s  },
    { u"A_PCM"_s,            u"wav"_s    }, { u"A_FLAC"_s,           u"flac"_s }, { u"A_ALAC"_s,        u"caf"_s  },
    { u"A_VORBIS"_s,         u"ogg"_s    }, { u"A_OPUS"_s,           u"opus"_s }, { u"A_AAC"_s,         u"aac"_s  },
    { u"A_REAL/"_s,          u"ra"_s     }, { u"A_TTA1"_s,           u"tta"_s  }, { u"A_WAVPACK4"_s,    u"wv"_s   },
    { u"S_TEXT/UTF8"_s,      u"srt"_s    }, { u"S_TEXT/ASCII"_s,     u"srt"_s  }, { u"S_TEXT/SSA"_s,    u"ssa"_s  },
    { u"S_TEXT/ASS"_s,       u"ass"_s    }, { u"S_TEXT/USF"_s,       u"usf"_s  }, { u"S_TEXT/WEBVTT"_s, u"vtt"_s  },
    { u"S_VOBSUB"_s,         u"idx"_s    }, { u"S_HDMV/PGS"_s,       u"sup"_s  }, { u"S_HDMV/TEXTST"_s, u"textst"_s },
    { u"S_KATE"_s,           u"ogg"_s    },
  };

  if (track.codecId == u"V_MS/VFW/FOURCC"_s)
    return track.codec == u"VC-1"_s ? u"vc1"_s : u"avi"_s;

  for (auto const &[prefix, ext] : s_byCodecId)
    if (track.codecId.startsWith(prefix))
      return ext;

  return u"bin"_s;
}

QString
trackName(Patterns const &patterns,
          FileInfo const &info,
          Track const &track) {
  auto values = fileValues(info);
  values.insert({ { u"TrackName"_s,    track.name },     { u"Language"_s,     track.language }, { u"LanguageIETF"_s, track.languageIetf },
                  { u"CodecID"_s,      track.codecId },  { u"Codec"_s,        codecLabel(track) },
                  { u"Forced"_s,       track.forced ? u" (forced)"_s : QString{} } });

  auto dimensions = track.pixelDimensions.split(u'x');
  QHash<QString, qint64> numbers{ { u"TrackID"_s, track.id }, { u"TrackNumber"_s, track.number },
                                  { u"SamplingFrequency"_s, track.samplingFrequency }, { u"Channels"_s, track.channels } };
  if (dimensions.size() == 2) {
    numbers[u"PixelWidth"_s]  = dimensions[0].toLongLong();
    numbers[u"PixelHeight"_s] = dimensions[1].toLongLong();
  }
  numbers[u"Delay"_s]          = track.minimumTimestamp ? std::llround(static_cast<double>(*track.minimumTimestamp) / 1'000'000) : 0;
  numbers[u"EffectiveDelay"_s] = effectiveDelay(info, track).value_or(0);

  auto const &pattern = track.type == u"video"_s ? patterns.video : track.type == u"audio"_s ? patterns.audio : patterns.subtitles;
  return safe(expand(pattern, values, numbers));
}

QString
attachmentName(Patterns const &patterns,
               FileInfo const &info,
               Attachment const &attachment) {
  auto values = fileValues(info);
  values.insert({ { u"AttachmentFilename"_s, attachment.fileName }, { u"MimeType"_s, attachment.mimeType } });

  return safe(expand(patterns.attachment, values, { { u"AttachmentID"_s, attachment.id }, { u"AttachmentFileSize"_s, attachment.size } }));
}

QString
fileName(QString const &pattern,
         FileInfo const &info) {
  return safe(expand(pattern, fileValues(info), {}));
}

QString
outputPath(QString const &folder,
           QString const &baseName,
           QString const &extension,
           bool overwrite,
           QStringList &taken) {
  auto name = [&](int n) {
    auto withNumber = n ? u"%1 (%2)"_s.arg(baseName).arg(n) : baseName;
    return QDir{folder}.filePath(extension.isEmpty() ? withNumber : u"%1.%2"_s.arg(withNumber, extension));
  };

  auto path = name(0);
  for (int n = 1; taken.contains(path) || (!overwrite && QFileInfo::exists(path)); ++n)
    path = name(n);

  taken << path;
  return path;
}

}
