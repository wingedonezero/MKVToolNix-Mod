/*
   mkvextract-gui -- a small window for mkvextract

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   the main window
*/

#include <QCheckBox>
#include <QComboBox>
#include <QDirIterator>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QStatusBar>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <cmath>

#include "mkvextract-gui/main_window.h"

using namespace Qt::StringLiterals;

namespace mtx::extract_gui {

namespace {

auto const KindRole  = Qt::UserRole + 1;
auto const IndexRole = Qt::UserRole + 2;   // the track / attachment index in its file

QStringList const s_matroskaSuffixes{ u"mkv"_s, u"mka"_s, u"mks"_s, u"mk3d"_s, u"webm"_s };

QString
formatDuration(qint64 ns) {
  auto ms = ns / 1'000'000;
  return u"%1:%2:%3.%4"_s.arg(ms / 3'600'000).arg(ms / 60'000 % 60, 2, 10, QChar{u'0'}).arg(ms / 1000 % 60, 2, 10, QChar{u'0'}).arg(ms % 1000, 3, 10, QChar{u'0'});
}

}

std::optional<qint64>
effectiveDelay(FileInfo const &info,
               Track const &track) {
  auto video = std::find_if(info.tracks.begin(), info.tracks.end(), [](auto const &t) { return t.type == u"video"_s; });
  if (!track.minimumTimestamp)
    return {};
  if (video == info.tracks.end())
    return 0;                   // no video: nothing to be late against (as gMKVExtractGUI)
  if (!video->minimumTimestamp)
    return {};

  return std::llround(static_cast<double>(*track.minimumTimestamp - *video->minimumTimestamp) / 1'000'000);
}

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow{parent}
{
  setupUi();
  setAcceptDrops(true);
}

void
MainWindow::setupUi() {
  setWindowTitle(tr("mkvextract-gui"));
  resize(760, 640);

  auto central = new QWidget{this};
  auto layout  = new QVBoxLayout{central};

  // input files and what they hold
  auto inputBox    = new QGroupBox{tr("Input Files (you can drag and drop files or directories)"), central};
  auto inputLayout = new QVBoxLayout{inputBox};
  m_tree           = new QTreeWidget{inputBox};
  m_tree->setHeaderHidden(true);
  m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
  m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
  inputLayout->addWidget(m_tree);

  auto inputOptions = new QHBoxLayout;
  m_appendOnDrop    = new QCheckBox{tr("Append input on drag and drop"), inputBox};
  m_overwrite       = new QCheckBox{tr("Overwrite existing files"), inputBox};
  auto addButton    = new QPushButton{tr("Add..."), inputBox};
  auto selectButton = new QPushButton{tr("Select..."), inputBox};
  auto selectMenu   = new QMenu{selectButton};
  inputOptions->addWidget(m_appendOnDrop);
  inputOptions->addWidget(m_overwrite);
  inputOptions->addStretch();
  inputOptions->addWidget(addButton);
  inputOptions->addWidget(selectButton);
  inputLayout->addLayout(inputOptions);
  layout->addWidget(inputBox, 1);

  struct entry_t { QString label; ItemKind kind; QString type; };
  for (auto const &[label, kind, type] : { entry_t{ tr("All Tracks"),      ItemKind::Track,      QString{}     },
                                          entry_t{ tr("Video Tracks"),     ItemKind::Track,      u"video"_s     },
                                          entry_t{ tr("Audio Tracks"),     ItemKind::Track,      u"audio"_s     },
                                          entry_t{ tr("Subtitle Tracks"),  ItemKind::Track,      u"subtitles"_s },
                                          entry_t{ tr("Chapters"),         ItemKind::Chapters,   QString{}     },
                                          entry_t{ tr("Attachments"),      ItemKind::Attachment, QString{}     },
                                          entry_t{ tr("Tags"),             ItemKind::Tags,       QString{}     } }) {
    selectMenu->addAction(tr("Check %1").arg(label),   this, [this, kind, type]() { setChecked(kind, type, true); });
    selectMenu->addAction(tr("Uncheck %1").arg(label), this, [this, kind, type]() { setChecked(kind, type, false); });
  }
  selectButton->setMenu(selectMenu);

  // the selected file
  auto informationBox    = new QGroupBox{tr("Selected File Information"), central};
  auto informationLayout = new QVBoxLayout{informationBox};
  m_fileInformation      = new QPlainTextEdit{informationBox};
  m_fileInformation->setReadOnly(true);
  m_fileInformation->setMaximumHeight(80);
  informationLayout->addWidget(m_fileInformation);
  layout->addWidget(informationBox);

  auto outputBox    = new QGroupBox{tr("Output Directory for Selected File (you can drag and drop the directory)"), central};
  auto outputLayout = new QHBoxLayout{outputBox};
  m_outputFolder    = new QLineEdit{outputBox};
  m_useSource       = new QCheckBox{tr("Use Source"), outputBox};
  m_browse          = new QPushButton{tr("Browse..."), outputBox};
  m_useSource->setChecked(true);
  outputLayout->addWidget(m_outputFolder, 1);
  outputLayout->addWidget(m_useSource);
  outputLayout->addWidget(m_browse);
  layout->addWidget(outputBox);

  // what to do
  auto actionsBox    = new QGroupBox{tr("Actions"), central};
  auto actionsLayout = new QHBoxLayout{actionsBox};
  m_log              = new QPushButton{tr("Log..."), actionsBox};
  m_jobs             = new QPushButton{tr("Jobs..."), actionsBox};
  m_chapterFormat    = new QComboBox{actionsBox};
  m_extractMode      = new QComboBox{actionsBox};
  m_addJobs          = new QPushButton{tr("Add Jobs"), actionsBox};
  m_extract          = new QPushButton{tr("Extract"), actionsBox};
  m_chapterFormat->addItems({ tr("XML"), tr("Simple (OGM)"), tr("Cue sheet") });
  m_extractMode->addItems({ tr("Tracks"), tr("Tracks and timestamps"), tr("Timestamps only") });
  actionsLayout->addWidget(m_log);
  actionsLayout->addWidget(m_jobs);
  actionsLayout->addStretch();
  actionsLayout->addWidget(new QLabel{tr("Chapter"), actionsBox});
  actionsLayout->addWidget(m_chapterFormat);
  actionsLayout->addWidget(new QLabel{tr("Extract"), actionsBox});
  actionsLayout->addWidget(m_extractMode);
  actionsLayout->addWidget(m_addJobs);
  actionsLayout->addWidget(m_extract);
  layout->addWidget(actionsBox);

  setCentralWidget(central);

  // progress and the buttons that go with it
  m_progress      = new QProgressBar{this};
  m_totalProgress = new QProgressBar{this};
  m_options       = new QPushButton{tr("Options..."), this};
  m_abortAll      = new QPushButton{tr("Abort All"), this};
  m_abort         = new QPushButton{tr("Abort"), this};
  statusBar()->addWidget(m_progress, 1);
  statusBar()->addWidget(m_totalProgress, 1);
  statusBar()->addPermanentWidget(m_options);
  statusBar()->addPermanentWidget(m_abortAll);
  statusBar()->addPermanentWidget(m_abort);

  for (auto button : { m_log, m_jobs, m_addJobs, m_extract, m_options, m_abortAll, m_abort })
    button->setEnabled(false);

  connect(addButton,         &QPushButton::clicked,                  this, [this]() {
    auto paths = QFileDialog::getOpenFileNames(this, tr("Add input files"), {}, tr("Matroska files (*.mkv *.mka *.mks *.mk3d *.webm);;All files (*)"));
    addFiles(paths, true);
  });
  connect(m_tree,            &QTreeWidget::itemSelectionChanged,     this, &MainWindow::showSelectedFile);
  connect(m_tree,            &QTreeWidget::customContextMenuRequested, this, &MainWindow::showTreeContextMenu);
  connect(m_useSource,       &QCheckBox::toggled,                    this, &MainWindow::setOutputFolderOfSelectedFile);
  connect(m_outputFolder,    &QLineEdit::editingFinished,            this, &MainWindow::setOutputFolderOfSelectedFile);
  connect(m_browse,          &QPushButton::clicked,                  this, &MainWindow::browseOutputFolder);
}

void
MainWindow::addFiles(QStringList const &paths,
                     bool append) {
  if (!append) {
    m_tree->clear();
    m_files.clear();
  }

  // folders: the Matroska files in them and in their sub-folders
  QStringList files;
  for (auto const &path : paths) {
    if (!QFileInfo{path}.isDir()) {
      files << path;
      continue;
    }

    QStringList inFolder;
    QDirIterator it{path, QDir::Files, QDirIterator::Subdirectories};
    while (it.hasNext()) {
      auto file = it.next();
      if (s_matroskaSuffixes.contains(QFileInfo{file}.suffix().toLower()))
        inFolder << file;
    }
    inFolder.sort();
    files << inFolder;
  }

  QStringList errors;
  for (auto const &file : files) {
    auto known = std::find_if(m_files.begin(), m_files.end(), [&file](auto const &f) { return f.info.path == file; });
    if (known != m_files.end())
      continue;

    QString error;
    auto info = identify(file, error);
    if (!info) {
      errors << u"%1: %2"_s.arg(file, error);
      continue;
    }

    m_files << InputFile{ *info };
    addFile(file);
  }

  if (!errors.isEmpty())
    QMessageBox::warning(this, tr("Files left out"), errors.join(u'\n'));
}

void
MainWindow::addFile(QString const &path) {
  auto const index = m_files.size() - 1;
  auto const &file = m_files.last();
  auto fileItem    = new QTreeWidgetItem{m_tree, { QFileInfo{path}.fileName() }};
  fileItem->setData(0, KindRole,  static_cast<int>(ItemKind::File));
  fileItem->setData(0, IndexRole, index);
  fileItem->setToolTip(0, path);

  auto addChild = [fileItem](QString const &text, ItemKind kind, int childIndex) {
    auto item = new QTreeWidgetItem{fileItem, { text }};
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(0, Qt::Unchecked);
    item->setData(0, KindRole,  static_cast<int>(kind));
    item->setData(0, IndexRole, childIndex);
  };

  for (int idx = 0, num = file.info.tracks.size(); idx < num; ++idx)
    addChild(itemText(file, file.info.tracks[idx]), ItemKind::Track, idx);

  if (file.info.chapterEntries)
    addChild(tr("Chapters [%1 entries]").arg(file.info.chapterEntries), ItemKind::Chapters, 0);

  for (int idx = 0, num = file.info.attachments.size(); idx < num; ++idx) {
    auto const &attachment = file.info.attachments[idx];
    addChild(tr("Attachment %1 [%2][%3][%4 bytes]").arg(attachment.id).arg(attachment.fileName, attachment.mimeType).arg(attachment.size), ItemKind::Attachment, idx);
  }

  if (file.info.globalTagEntries || file.info.trackTagEntries)
    addChild(tr("Tags [%1 global, %2 track]").arg(file.info.globalTagEntries).arg(file.info.trackTagEntries), ItemKind::Tags, 0);

  fileItem->setExpanded(true);
}

QString
MainWindow::itemText(InputFile const &file,
                     Track const &track)
  const {
  auto text = tr("Track %1 [TID %2][%3][%4][%5]").arg(track.number).arg(track.id).arg(track.type, track.codecId, track.codec);
  if (!track.language.isEmpty())
    text += u"[%1]"_s.arg(track.language);
  if (!track.name.isEmpty())
    text += u"[%1]"_s.arg(track.name);
  if (track.forced)
    text += tr("[forced]");
  if (!track.pixelDimensions.isEmpty())
    text += u"[%1]"_s.arg(track.pixelDimensions);
  if (track.samplingFrequency)
    text += tr("[%1 Hz][%2 ch]").arg(track.samplingFrequency).arg(track.channels);
  if (track.type == u"audio"_s) {
    auto delay = effectiveDelay(file.info, track);
    text += delay ? tr("[delay %1 ms]").arg(*delay) : tr("[delay not known]");
  }

  return text;
}

int
MainWindow::selectedFileIndex()
  const {
  auto items = m_tree->selectedItems();
  if (items.isEmpty())
    return -1;

  auto item = items.first();
  while (item->parent())
    item = item->parent();

  return item->data(0, IndexRole).toInt();
}

void
MainWindow::showSelectedFile() {
  auto index = selectedFileIndex();
  auto valid = (index >= 0) && (index < m_files.size());

  for (QWidget *widget : std::initializer_list<QWidget *>{ m_outputFolder, m_useSource, m_browse })
    widget->setEnabled(valid);

  if (!valid) {
    m_fileInformation->clear();
    m_outputFolder->clear();
    return;
  }

  auto const &file = m_files[index];
  auto const &info = file.info;
  auto count       = [&info](QString const &type) {
    return std::count_if(info.tracks.begin(), info.tracks.end(), [&type](auto const &t) { return t.type == type; });
  };

  QStringList lines;
  lines << tr("File: %1").arg(info.path);
  lines << tr("Container: %1, duration %2%3").arg(info.containerType, formatDuration(info.duration), info.title.isEmpty() ? QString{} : tr(", title \"%1\"").arg(info.title));
  lines << tr("Tracks: %1 video, %2 audio, %3 subtitles; chapters: %4 entries; attachments: %5")
             .arg(count(u"video"_s)).arg(count(u"audio"_s)).arg(count(u"subtitles"_s)).arg(info.chapterEntries).arg(info.attachments.size());
  m_fileInformation->setPlainText(lines.join(u'\n'));

  QSignalBlocker blockUseSource{m_useSource};
  m_useSource->setChecked(file.useSourceFolder);
  m_outputFolder->setText(file.useSourceFolder ? QFileInfo{info.path}.absolutePath() : file.outputFolder);
  m_outputFolder->setReadOnly(file.useSourceFolder);
  m_browse->setEnabled(!file.useSourceFolder);
}

void
MainWindow::setOutputFolderOfSelectedFile() {
  auto index = selectedFileIndex();
  if ((index < 0) || (index >= m_files.size()))
    return;

  auto &file           = m_files[index];
  file.useSourceFolder = m_useSource->isChecked();
  if (!file.useSourceFolder && !m_outputFolder->isReadOnly())
    file.outputFolder = m_outputFolder->text();
  if (!file.useSourceFolder && file.outputFolder.isEmpty())
    file.outputFolder = QFileInfo{file.info.path}.absolutePath();

  showSelectedFile();
}

void
MainWindow::browseOutputFolder() {
  auto folder = QFileDialog::getExistingDirectory(this, tr("Output folder"), m_outputFolder->text());
  if (folder.isEmpty())
    return;

  m_outputFolder->setReadOnly(false);
  m_outputFolder->setText(folder);
  setOutputFolderOfSelectedFile();
}

void
MainWindow::removeSelectedFile() {
  auto index = selectedFileIndex();
  if ((index < 0) || (index >= m_files.size()))
    return;

  m_files.removeAt(index);
  delete m_tree->takeTopLevelItem(index);
  for (int idx = index, num = m_tree->topLevelItemCount(); idx < num; ++idx)
    m_tree->topLevelItem(idx)->setData(0, IndexRole, idx);
}

void
MainWindow::showTreeContextMenu(QPoint const &pos) {
  QMenu menu{this};
  auto index = selectedFileIndex();

  menu.addAction(tr("Remove Selected Input File"), this, &MainWindow::removeSelectedFile)->setEnabled(index >= 0);
  menu.addAction(tr("Remove All Input Files"),     this, [this]() { m_tree->clear(); m_files.clear(); });
  menu.addSeparator();
  menu.addAction(tr("Expand All"),   m_tree, &QTreeWidget::expandAll);
  menu.addAction(tr("Collapse All"), m_tree, &QTreeWidget::collapseAll);
  menu.exec(m_tree->viewport()->mapToGlobal(pos));
}

void
MainWindow::setChecked(ItemKind kind,
                       QString const &trackType,
                       bool checked) {
  for (int fileIdx = 0, numFiles = m_tree->topLevelItemCount(); fileIdx < numFiles; ++fileIdx) {
    auto fileItem    = m_tree->topLevelItem(fileIdx);
    auto const &file = m_files[fileItem->data(0, IndexRole).toInt()];

    for (int idx = 0, num = fileItem->childCount(); idx < num; ++idx) {
      auto item = fileItem->child(idx);
      if (static_cast<ItemKind>(item->data(0, KindRole).toInt()) != kind)
        continue;
      if ((kind == ItemKind::Track) && !trackType.isEmpty() && (file.info.tracks[item->data(0, IndexRole).toInt()].type != trackType))
        continue;
      item->setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
    }
  }
}

void
MainWindow::dragEnterEvent(QDragEnterEvent *event) {
  if (event->mimeData()->hasUrls())
    event->acceptProposedAction();
}

void
MainWindow::dropEvent(QDropEvent *event) {
  QStringList paths;
  for (auto const &url : event->mimeData()->urls())
    if (url.isLocalFile())
      paths << url.toLocalFile();

  // a folder dropped on the output folder sets it
  auto onOutput = m_outputFolder->rect().contains(m_outputFolder->mapFrom(this, event->position().toPoint()));
  if (onOutput && (paths.size() == 1) && QFileInfo{paths.first()}.isDir()) {
    m_useSource->setChecked(false);
    m_outputFolder->setReadOnly(false);
    m_outputFolder->setText(paths.first());
    setOutputFolderOfSelectedFile();

  } else
    addFiles(paths, m_appendOnDrop->isChecked());

  event->acceptProposedAction();
}

}
