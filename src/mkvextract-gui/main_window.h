/*
   mkvextract-gui -- a small window for mkvextract

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   the main window
*/

#pragma once

#include <QMainWindow>

#include "mkvextract-gui/extractor.h"

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

namespace mtx::extract_gui {

// One input file: what it holds and where its parts go.
struct InputFile {
  FileInfo info;
  bool useSourceFolder{true};
  QString outputFolder;
};

class MainWindow: public QMainWindow {
  Q_OBJECT

public:
  // what a tree item stands for (Qt::UserRole + 1)
  enum class ItemKind { File, Track, Chapters, Attachment, Tags };

protected:
  QList<InputFile> m_files;
  Patterns m_patterns;
  QList<Job> m_running;         // the jobs Extract started
  Runner *m_runner{};
  QStringList m_logLines;

  QTreeWidget *m_tree{};
  QCheckBox *m_appendOnDrop{}, *m_overwrite{}, *m_useSource{};
  QPlainTextEdit *m_fileInformation{};
  QLineEdit *m_outputFolder{};
  QPushButton *m_browse{}, *m_log{}, *m_jobs{}, *m_addJobs{}, *m_extract{}, *m_options{}, *m_abortAll{}, *m_abort{};
  QComboBox *m_chapterFormat{}, *m_extractMode{};
  QProgressBar *m_progress{}, *m_totalProgress{};

public:
  explicit MainWindow(QWidget *parent = nullptr);

  void addFiles(QStringList const &paths, bool append);

protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dropEvent(QDropEvent *event) override;

  void setupUi();
  void addFile(QString const &path);
  void showSelectedFile();
  void setOutputFolderOfSelectedFile();
  void browseOutputFolder();
  void removeSelectedFile();
  void showTreeContextMenu(QPoint const &pos);
  void setChecked(ItemKind kind, QString const &trackType, bool checked);

  void extract();
  void jobStarted(int index);
  void jobFinished(int index);
  void allFinished();
  void setRunning(bool running);
  QList<Job> jobsForCheckedItems();
  Selection selection(int fileIndex) const;
  Options options() const;
  QString outputFolder(InputFile const &file) const;

  int selectedFileIndex() const;
  QString itemText(InputFile const &file, Track const &track) const;
};

}
