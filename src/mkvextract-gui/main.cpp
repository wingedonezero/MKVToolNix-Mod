/*
   mkvextract-gui -- a small window for mkvextract

   Distributed under the GPL v2
   see the file COPYING for details
   or visit https://www.gnu.org/licenses/old-licenses/gpl-2.0.html

   the program: files given on the command line are loaded
*/

#include <QApplication>

#include "mkvextract-gui/main_window.h"

using namespace Qt::StringLiterals;

int
main(int argc,
     char **argv) {
  QApplication app{argc, argv};
  QApplication::setApplicationName(u"mkvextract-gui"_s);
  QApplication::setOrganizationName(u"mkvextract-gui"_s);

  mtx::extract_gui::MainWindow window;
  window.show();

  auto files = QApplication::arguments().mid(1);
  if (!files.isEmpty())
    window.addFiles(files, true);

  return app.exec();
}
