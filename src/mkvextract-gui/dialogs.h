/*
   mkvextract-gui -- a small window for mkvextract

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   the log, jobs and options windows
*/

#pragma once

#include <QDialog>

#include "mkvextract-gui/extractor.h"

class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTreeWidget;

namespace mtx::extract_gui {

// mkvextract's output, as it comes
class LogDialog: public QDialog {
protected:
  QPlainTextEdit *m_text{};

public:
  explicit LogDialog(QWidget *parent);
  void append(QString const &line);
};

// the queued jobs and their state
class JobsDialog: public QDialog {
protected:
  QList<Job> &m_jobs;
  QTreeWidget *m_list{};

public:
  QPushButton *m_run{}, *m_remove{}, *m_clearFinished{};

  JobsDialog(QWidget *parent, QList<Job> &jobs);
  void refresh();
  void setRunning(bool running);
};

// the file name patterns
class OptionsDialog: public QDialog {
protected:
  QLineEdit *m_video{}, *m_audio{}, *m_subtitles{}, *m_chapters{}, *m_tags{}, *m_attachment{};

public:
  OptionsDialog(QWidget *parent, Patterns const &patterns);
  Patterns patterns() const;
  void setPatterns(Patterns const &patterns);
};

}
