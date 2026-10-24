/*
   mkvextract-gui -- a small window for mkvextract

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   what to extract from a file, and running mkvextract for it
*/

#pragma once

#include <QObject>
#include <QProcess>

#include "mkvextract-gui/naming.h"

namespace mtx::extract_gui {

enum class ExtractMode   { Tracks, TracksAndTimestamps, TimestampsOnly };
enum class ChapterFormat { Xml, Simple, CueSheet };

// what is checked in one file
struct Selection {
  QList<int> tracks, attachments;  // indices into the file's lists
  bool chapters{}, tags{};
  bool empty() const { return tracks.isEmpty() && attachments.isEmpty() && !chapters && !tags; }
};

struct Options {
  ExtractMode mode{ExtractMode::Tracks};
  ChapterFormat chapterFormat{ChapterFormat::Xml};
  bool overwrite{};
};

// One mkvextract run: everything chosen from one file, in one pass. The
// parts in gMKVExtractGUI's order: timestamps, tracks, attachments,
// chapters, tags.
struct Job {
  QString file, outputFolder, description;
  QStringList arguments, outputs;
  enum class Status { Pending, Running, Done, Failed, Aborted } status{Status::Pending};
};

Job makeJob(FileInfo const &info, QString const &outputFolder, Selection const &selection, Options const &options, Patterns const &patterns);

// Runs jobs one after the other.
class Runner: public QObject {
  Q_OBJECT

protected:
  QList<Job> *m_jobs{};
  QProcess m_process;
  int m_current{-1};
  bool m_abortAll{};
  QByteArray m_pending;         // output not ending in a line break yet

public:
  explicit Runner(QObject *parent = nullptr);

  void run(QList<Job> &jobs);  // the pending ones
  bool running() const;
  void abort();                 // the current job
  void abortAll();              // and the ones after it

Q_SIGNALS:
  void jobStarted(int index);
  void jobFinished(int index);
  void progress(int percent);
  void line(QString const &text);
  void finished();

protected:
  void startNext();
  void readOutput();
  void processFinished(int exitCode, QProcess::ExitStatus status);
};

}
