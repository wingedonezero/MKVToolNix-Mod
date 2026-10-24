/*
   mkvextract-gui -- a small window for mkvextract

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   what to extract from a file, and running mkvextract for it
*/

#include <QFileInfo>
#include <QRegularExpression>

#include "mkvextract-gui/extractor.h"

using namespace Qt::StringLiterals;

namespace mtx::extract_gui {

Job
makeJob(FileInfo const &info,
        QString const &outputFolder,
        Selection const &selection,
        Options const &options,
        Patterns const &patterns) {
  Job job;
  QStringList taken, timestamps, tracks, attachments;

  job.file         = info.path;
  job.outputFolder = outputFolder;
  job.arguments    = QStringList{ u"--gui-mode"_s, info.path };

  for (auto idx : selection.tracks) {
    auto const &track = info.tracks[idx];
    auto base         = trackName(patterns, info, track);

    if (options.mode != ExtractMode::Tracks) {
      auto path   = outputPath(outputFolder, base + u"_timestamps"_s, u"txt"_s, options.overwrite, taken);
      timestamps << u"%1:%2"_s.arg(track.id).arg(path);
      job.outputs << path;
    }
    if (options.mode != ExtractMode::TimestampsOnly) {
      auto path = outputPath(outputFolder, base, extension(track), options.overwrite, taken);
      tracks << u"%1:%2"_s.arg(track.id).arg(path);
      job.outputs << path;
    }
  }

  for (auto idx : selection.attachments) {
    auto const &attachment = info.attachments[idx];
    QFileInfo name{attachmentName(patterns, info, attachment)};
    auto path              = outputPath(outputFolder, name.suffix().isEmpty() ? name.fileName() : name.completeBaseName(), name.suffix(), options.overwrite, taken);
    attachments << u"%1:%2"_s.arg(attachment.id).arg(path);
    job.outputs << path;
  }

  if (!timestamps.isEmpty())
    job.arguments << u"timestamps_v2"_s << timestamps;
  if (!tracks.isEmpty())
    job.arguments << u"tracks"_s << tracks;
  if (!attachments.isEmpty())
    job.arguments << u"attachments"_s << attachments;

  if (selection.chapters) {
    auto base = fileName(patterns.chapters, info);
    if (options.chapterFormat == ChapterFormat::CueSheet) {
      auto path = outputPath(outputFolder, base, u"cue"_s, options.overwrite, taken);
      job.arguments << u"cuesheet"_s << path;
      job.outputs << path;

    } else {
      auto simple = options.chapterFormat == ChapterFormat::Simple;
      auto path   = outputPath(outputFolder, base, simple ? u"txt"_s : u"xml"_s, options.overwrite, taken);
      job.arguments << u"chapters"_s;
      if (simple)
        job.arguments << u"--simple"_s;
      job.arguments << path;
      job.outputs << path;
    }
  }

  if (selection.tags) {
    auto path = outputPath(outputFolder, fileName(patterns.tags, info), u"xml"_s, options.overwrite, taken);
    job.arguments << u"tags"_s << path;
    job.outputs << path;
  }

  job.description = QObject::tr("%1: %2 file(s)").arg(QFileInfo{info.path}.fileName()).arg(job.outputs.size());

  return job;
}

Runner::Runner(QObject *parent)
  : QObject{parent}
{
  m_process.setProcessChannelMode(QProcess::MergedChannels);
  connect(&m_process, &QProcess::readyReadStandardOutput, this, &Runner::readOutput);
  connect(&m_process, &QProcess::finished,                this, &Runner::processFinished);
  connect(&m_process, &QProcess::errorOccurred,           this, [this](QProcess::ProcessError error) {
    if (error == QProcess::FailedToStart)
      processFinished(-1, QProcess::CrashExit);
  });
}

bool
Runner::running()
  const {
  return m_current >= 0;
}

void
Runner::run(QList<Job> &jobs) {
  if (running())
    return;

  m_jobs     = &jobs;
  m_abortAll = false;
  startNext();
}

void
Runner::startNext() {
  m_current = -1;
  if (!m_abortAll)
    for (int idx = 0, num = m_jobs->size(); idx < num; ++idx)
      if ((*m_jobs)[idx].status == Job::Status::Pending) {
        m_current = idx;
        break;
      }

  if (m_current < 0) {
    Q_EMIT finished();
    return;
  }

  auto &job  = (*m_jobs)[m_current];
  job.status = Job::Status::Running;
  m_pending.clear();

  Q_EMIT jobStarted(m_current);
  QStringList shown;
  for (auto const &argument : job.arguments)
    shown << (argument.contains(u' ') ? u"\"%1\""_s.arg(argument) : argument);
  Q_EMIT line(u"$ %1 %2"_s.arg(toolPath(u"mkvextract"_s), shown.join(u' ')));
  Q_EMIT progress(0);

  m_process.start(toolPath(u"mkvextract"_s), job.arguments);
}

void
Runner::readOutput() {
  static QRegularExpression const s_progress{uR"(^#GUI#progress\s+(\d+)%)"_s};

  m_pending += m_process.readAllStandardOutput();
  for (int end; (end = m_pending.indexOf('\n')) >= 0; ) {
    auto text = QString::fromUtf8(m_pending.left(end)).trimmed();
    m_pending.remove(0, end + 1);

    auto match = s_progress.match(text);
    if (match.hasMatch())
      Q_EMIT progress(match.captured(1).toInt());
    else if (!text.isEmpty())
      Q_EMIT line(text);
  }
}

void
Runner::processFinished(int exitCode,
                        QProcess::ExitStatus status) {
  if (m_current < 0)
    return;

  readOutput();
  if (!m_pending.isEmpty())
    Q_EMIT line(QString::fromUtf8(m_pending).trimmed());

  // mkvextract: 0 done, 1 done with warnings, 2 failed
  auto &job  = (*m_jobs)[m_current];
  job.status = job.status == Job::Status::Aborted                             ? Job::Status::Aborted
             : (status == QProcess::NormalExit) && (exitCode >= 0) && (exitCode <= 1) ? Job::Status::Done
             :                                                                           Job::Status::Failed;
  if (job.status == Job::Status::Failed)
    Q_EMIT line(tr("mkvextract failed (exit code %1)%2").arg(exitCode).arg(status == QProcess::NormalExit ? QString{} : u": "_s + m_process.errorString()));

  Q_EMIT jobFinished(m_current);
  startNext();
}

void
Runner::abort() {
  if (!running())
    return;

  (*m_jobs)[m_current].status = Job::Status::Aborted;
  Q_EMIT line(tr("Aborted."));
  m_process.kill();
}

void
Runner::abortAll() {
  m_abortAll = true;
  abort();
}

}
