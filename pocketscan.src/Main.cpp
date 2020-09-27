
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#include <QApplication>
#include <QDate>
#include <QDebug>
#include <QDirIterator>
#include <QFileInfo>
#include <QMessageBox>

#include <MainWindow.h>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("AlexDemko");
    QCoreApplication::setOrganizationDomain("demko.ca");
    QCoreApplication::setApplicationName("PocketScan");

    QStringList args = QCoreApplication::arguments();

    MainWindow *window = new MainWindow;

    window->show();

    if (args.size() > 1) {
        QStringList sublist;

        for (int x = 1; x < args.size(); ++x) {
            if (QFileInfo(args[x]).isDir())
                expandImageDirectory(args[x], sublist);
            else
                sublist.append(args[x]);
        }

        if (!sublist.isEmpty()) {
            sublist.sort();

            QString ext = QFileInfo(sublist[0]).suffix().toLower();
            if (ext == "xml" || ext == "psbk") {
                if (window->project().loadXML(sublist[0]))
                    window->project().setFileName(sublist[0]);
            } else
                window->project().appendFiles(sublist);
        }

        window->project().notifyChange(0);
    }

    return app.exec();
}
