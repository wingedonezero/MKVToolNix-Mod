/*
   mkvextract-gui -- a small window for mkvextract

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   the log, jobs and options windows
*/

#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "mkvextract-gui/dialogs.h"

using namespace Qt::StringLiterals;

namespace mtx::extract_gui {

LogDialog::LogDialog(QWidget *parent)
  : QDialog{parent}
{
  setWindowTitle(tr("Log"));
  resize(800, 400);

  auto layout = new QVBoxLayout{this};
  m_text      = new QPlainTextEdit{this};
  m_text->setReadOnly(true);
  m_text->setLineWrapMode(QPlainTextEdit::NoWrap);
  layout->addWidget(m_text);

  auto buttons = new QHBoxLayout;
  auto clear   = new QPushButton{tr("Clear"), this};
  auto save    = new QPushButton{tr("Save..."), this};
  auto close   = new QPushButton{tr("Close"), this};
  buttons->addWidget(clear);
  buttons->addWidget(save);
  buttons->addStretch();
  buttons->addWidget(close);
  layout->addLayout(buttons);

  connect(clear, &QPushButton::clicked, m_text, &QPlainTextEdit::clear);
  connect(close, &QPushButton::clicked, this,   &QDialog::hide);
  connect(save,  &QPushButton::clicked, this,   [this]() {
    auto path = QFileDialog::getSaveFileName(this, tr("Save the log"), u"mkvextract-gui.log"_s);
    QFile file{path};
    if (!path.isEmpty() && file.open(QIODevice::WriteOnly | QIODevice::Text))
      file.write(m_text->toPlainText().toUtf8());
  });
}

void
LogDialog::append(QString const &line) {
  m_text->appendPlainText(line);
}

JobsDialog::JobsDialog(QWidget *parent,
                       QList<Job> &jobs)
  : QDialog{parent}
  , m_jobs{jobs}
{
  setWindowTitle(tr("Jobs"));
  resize(700, 320);

  auto layout = new QVBoxLayout{this};
  m_list      = new QTreeWidget{this};
  m_list->setHeaderLabels({ tr("Status"), tr("File"), tr("Output folder") });
  m_list->setRootIsDecorated(false);
  m_list->setSelectionMode(QAbstractItemView::ExtendedSelection);
  layout->addWidget(m_list);

  auto buttons    = new QHBoxLayout;
  m_run           = new QPushButton{tr("Run Jobs"), this};
  m_remove        = new QPushButton{tr("Remove Selected"), this};
  m_clearFinished = new QPushButton{tr("Clear Finished"), this};
  auto close      = new QPushButton{tr("Close"), this};
  buttons->addWidget(m_run);
  buttons->addWidget(m_remove);
  buttons->addWidget(m_clearFinished);
  buttons->addStretch();
  buttons->addWidget(close);
  layout->addLayout(buttons);

  connect(close,           &QPushButton::clicked, this, &QDialog::hide);
  connect(m_remove,        &QPushButton::clicked, this, [this]() {
    QList<int> rows;
    for (auto item : m_list->selectedItems())
      rows << m_list->indexOfTopLevelItem(item);
    std::sort(rows.begin(), rows.end(), std::greater<int>{});
    for (auto row : rows)
      if (m_jobs[row].status != Job::Status::Running)
        m_jobs.removeAt(row);
    refresh();
  });
  connect(m_clearFinished, &QPushButton::clicked, this, [this]() {
    m_jobs.removeIf([](auto const &job) { return (job.status != Job::Status::Pending) && (job.status != Job::Status::Running); });
    refresh();
  });

  refresh();
}

void
JobsDialog::refresh() {
  auto statusText = [](Job::Status status) {
    switch (status) {
      case Job::Status::Pending: return tr("pending");
      case Job::Status::Running: return tr("running");
      case Job::Status::Done:    return tr("done");
      case Job::Status::Failed:  return tr("failed");
      default:                   return tr("aborted");
    }
  };

  m_list->clear();
  for (auto const &job : m_jobs)
    new QTreeWidgetItem{m_list, { statusText(job.status), job.description, job.outputFolder }};
  m_list->resizeColumnToContents(0);
  m_list->resizeColumnToContents(1);
}

void
JobsDialog::setRunning(bool running) {
  m_run->setEnabled(!running);
  m_clearFinished->setEnabled(!running);
}

OptionsDialog::OptionsDialog(QWidget *parent,
                             Patterns const &patterns)
  : QDialog{parent}
{
  setWindowTitle(tr("Options"));
  resize(720, 0);

  auto layout = new QVBoxLayout{this};
  auto form   = new QFormLayout;
  m_video      = new QLineEdit{this};
  m_audio      = new QLineEdit{this};
  m_subtitles  = new QLineEdit{this};
  m_chapters   = new QLineEdit{this};
  m_tags       = new QLineEdit{this};
  m_attachment = new QLineEdit{this};
  form->addRow(tr("Video tracks:"),     m_video);
  form->addRow(tr("Audio tracks:"),     m_audio);
  form->addRow(tr("Subtitle tracks:"),  m_subtitles);
  form->addRow(tr("Chapters:"),         m_chapters);
  form->addRow(tr("Tags:"),             m_tags);
  form->addRow(tr("Attachments:"),      m_attachment);
  layout->addWidget(new QLabel{tr("File name patterns (the extension is added):"), this});
  layout->addLayout(form);

  auto help = new QLabel{tr("Placeholders: {FilenameNoExt} {Filename} {TrackID} {TrackID:00} {TrackNumber} {TrackNumber:00} "
                            "{TrackName} {Language} {LanguageIETF} {CodecID} {Codec} {Forced} {Delay} {EffectiveDelay} "
                            "{PixelWidth} {PixelHeight} {SamplingFrequency} {Channels} {AttachmentID} {AttachmentFilename} "
                            "{MimeType} {AttachmentFileSize}. {Codec}: a short label (Mpeg4, DD, DTS-HD MA, PGS, ...); "
                            "{Forced}: \" (forced)\" for a forced track; {EffectiveDelay}: an audio track's delay against "
                            "the video in ms."), this};
  help->setWordWrap(true);
  layout->addWidget(help);

  auto buttons = new QDialogButtonBox{QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults, this};
  layout->addWidget(buttons);

  connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  connect(buttons->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, [this]() { setPatterns(Patterns{}); });

  setPatterns(patterns);
}

void
OptionsDialog::setPatterns(Patterns const &patterns) {
  m_video->setText(patterns.video);
  m_audio->setText(patterns.audio);
  m_subtitles->setText(patterns.subtitles);
  m_chapters->setText(patterns.chapters);
  m_tags->setText(patterns.tags);
  m_attachment->setText(patterns.attachment);
}

Patterns
OptionsDialog::patterns()
  const {
  Patterns result;
  result.video      = m_video->text();
  result.audio      = m_audio->text();
  result.subtitles  = m_subtitles->text();
  result.chapters   = m_chapters->text();
  result.tags       = m_tags->text();
  result.attachment = m_attachment->text();
  return result;
}

}
