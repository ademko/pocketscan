
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#include <MainWindow.h>

#include <QCoreApplication>
#include <QDirIterator>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QSettings>
#include <QUrl>

#include <AboutDialog.h>
#include <ImageAddButton.h>
#include <TabBar.h>
#include <TileView.h>

MainWindow *MainWindow::dm_instance;

void expandImageDirectory(const QString &dirname, QStringList &outlist) {
    QDirIterator ii(dirname);
    QString jpg("jpg"), png("png");

    while (ii.hasNext()) {
        QString next(ii.next());
        QString ext(QFileInfo(next).suffix().toLower());

        if (ext == jpg || ext == png)
            outlist.append(next);
    }
}

//
// MainWindow::RecentFiles
//

MainWindow::RecentFiles::RecentFiles(MainWindow *parent)
    : QMenu("Open &Recent"), dm_parent(parent) {
    load();
}

void MainWindow::RecentFiles::prependFile(const QString &fileName, int type) {
    // first, see if this already exists
    for (int x = 0; x < dm_actions.size(); ++x) {
        if (dm_actions[x]->property("recent.fileName").toString() == fileName) {
            // found it!
            // if this is the first entry, then we dont need to do anything at
            // all
            if (x == 0)
                return;
            else {
                removeAction(dm_actions[x]);
                // TODO should I delete it?
                dm_actions.erase(dm_actions.begin() + x);
            }
        }
    }
    QAction *action = new QAction(fileName, 0);

    action->setProperty("recent.fileName", fileName);
    action->setProperty("recent.type", type);

    connect(action, SIGNAL(triggered(bool)), dm_parent,
            SLOT(onRecentFileOpen()));

    if (dm_actions.empty()) {
        addAction(action);
        dm_actions.push_back(action);
    } else {
        insertAction(dm_actions[0], action);
        dm_actions.insert(dm_actions.begin(), action);
    }

    // finally, trim the last entries if there are too many actions
    while (dm_actions.size() > 10) {
        removeAction(*dm_actions.rbegin());
        // TODO should I delete it?
        dm_actions.pop_back();
    }

    save();
}

void MainWindow::RecentFiles::load(void) {
    QSettings settings;
    int count;

    count = settings.value("recentFiles.size", QVariant(0)).toInt();

    for (int x = 0; x < count; ++x) {
        QAction *action = new QAction(0);

        action->setProperty(
            "recent.fileName",
            settings.value("recentFiles.fileName." + QString::number(x)));
        action->setProperty("recent.type", settings.value("recentFiles.type." +
                                                          QString::number(x)));

        connect(action, SIGNAL(triggered(bool)), dm_parent,
                SLOT(onRecentFileOpen()));

        action->setText(action->property("recent.fileName").toString());

        addAction(action);
        dm_actions.push_back(action);
    }
}

void MainWindow::RecentFiles::save(void) {
    QSettings settings;

    settings.setValue("recentFiles.size", static_cast<int>(dm_actions.size()));

    for (int x = 0; x < dm_actions.size(); ++x) {
        settings.setValue("recentFiles.fileName." + QString::number(x),
                          dm_actions[x]->property("recent.fileName"));
        settings.setValue("recentFiles.type." + QString::number(x),
                          dm_actions[x]->property("recent.type"));
    }
}

//
//
// MainWindow
//
//

#ifndef NDEBUG
#define EDITION ULTIMATE_EDITION
#else
#define EDITION STANDARD_EDITION
#endif

MainWindow::MainWindow(void) : dm_steplist(EDITION), dm_recentfiles(this) {
    dm_instance = this;

    dm_edition = EDITION;

    setAcceptDrops(true);

    initGui();
    setWindowIcon(QIcon(":/logo.png"));

    updateTitle();
}

MainWindow::~MainWindow() {
    // we nuke this before the project
    delete dm_tiles;

    dm_instance = 0;
}

void MainWindow::onNew(void) {
    dm_project.clear();
    dm_project.notifyChange(0);

    updateTitle();
}

void MainWindow::onOpen(void) {
    QString fileName = QFileDialog::getOpenFileName(
        0, "Open Book", dm_project.fileName(),
        "PocketScan Books (*.psbk);;XML Files (*.xml);;All Files (*)");

    if (fileName.isEmpty())
        return;

    openFile(fileName);
}

void MainWindow::onRecentFileOpen(void) {
    QObject *orig = sender();

    assert(orig);

    QString fileName(orig->property("recent.fileName").toString());
    // int type(orig->property("recent.type").toInt());

    openFile(fileName);
}

void MainWindow::onSave(void) {
    if (dm_project.fileName().isEmpty()) {
        onSaveAs();
        return;
    }

    if (dm_project.saveXML(dm_project.fileName()))
        dm_recentfiles.prependFile(dm_project.fileName(), 0);
    else
        QMessageBox::critical(this, "Error Saving Book",
                              "There was an error when trying to save: " +
                                  dm_project.fileName());
}

void MainWindow::onSaveAs(void) {
    QString fileName = QFileDialog::getSaveFileName(
        this, "Save Book", dm_project.fileName(),
        "PocketScan Books (*.psbk);;All Files (*)");

    if (fileName.isEmpty())
        return;

    dm_project.setFileName(fileName);

    updateTitle();

    onSave();
}

void MainWindow::onExportImages(void) {}

void MainWindow::onPrintPreview(void) {
    QPrintPreviewDialog prev(&dm_printer, this);

    connect(&prev, SIGNAL(paintRequested(QPrinter *)), this,
            SLOT(onPrintPage(QPrinter *)));

    prev.exec();
}

void MainWindow::onPrint(void) {
    {
        QPrintDialog pdlg(&dm_printer, this);

        if (pdlg.exec() != QDialog::Accepted)
            return;
    }

    QProgressDialog progdlg("Printing Book", "Cancel", 0,
                            dm_project.files().size(), this);
    progdlg.setWindowModality(Qt::ApplicationModal);
    progdlg.setMinimumDuration(0);

    dm_project.exportToPrinter(&dm_printer);
}

void MainWindow::onPrintPDF(void) {
    QString fileName = QFileDialog::getSaveFileName(
        this, "Export to PDF File", dm_pdfoutfilename,
        "PDF Files (*.pdf);;All Files (*)");

    if (fileName.isEmpty())
        return;

    QProgressDialog progdlg("Exporting book to PDF", "Cancel", 0,
                            dm_project.files().size(), this);
    progdlg.setWindowModality(Qt::ApplicationModal);
    progdlg.setMinimumDuration(0);

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFileName(fileName);
    printer.setPageSize(QPrinter::Letter);

    dm_project.exportToPrinter(&printer, &progdlg);

    dm_pdfoutfilename = fileName;
}

void MainWindow::onPrintFiles(void) {
    // implement this oprint files function
    // maybe need to add a new file to Project
    // using some kind of file numbering thing
    QString fileName = QFileDialog::getSaveFileName(
        this, "Export to Image Files", dm_imageoutname,
        "JPEG Image Files (*.jpg *.jpeg);;PNG Image Files (*.png);;All Files "
        "(*)");

    if (fileName.isEmpty())
        return;

    // qDebug() << __FUNCTION__ << fileName;

    QProgressDialog progdlg("Exporting book to image files", "Cancel", 0,
                            dm_project.files().size(), this);
    progdlg.setWindowModality(Qt::ApplicationModal);
    progdlg.setMinimumDuration(0);

    dm_project.exportToFiles(fileName, &progdlg);

    dm_imageoutname = fileName;
}

void MainWindow::onShowAbout(void) {
    AboutDialog about(this, "PocketScan");

    about.addPixmap(QPixmap(":/logo.png"));
    about.addTitle("PocketScan", "", "2020", "Aleksander Demko");

    about.addContact("Aleksander Demko", "ademko@gmail.com",
                     "http://wexussoftware.com/pocketscan/");
    about.addVersion(
        "1.0.0 " +
        QString(dm_edition == ULTIMATE_EDITION ? "Ultimate" : "Standard")
#ifndef NDEBUG
    );
#else
            ,
        "", true, true, false);
#endif
    about.addDisclaimer();

    about.exec();
}

void MainWindow::onPrintPage(QPrinter *printer) {
    dm_project.exportToPrinter(printer);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasFormat("text/uri-list"))
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event) {
    QList<QUrl> urlList;
    QList<QUrl>::const_iterator ii;
    QString filename;
    QFileInfo info;
    QStringList files;

    if (event->mimeData()->hasUrls()) {
        urlList = event->mimeData()->urls();

        for (ii = urlList.begin(); ii != urlList.end(); ++ii) {
            filename = ii->toLocalFile();
            info = QFileInfo(filename);

            if (info.exists() && info.isFile())
                files.push_back(filename);
        }
        if (!files.isEmpty()) {
            files.sort();
            dm_project.appendFiles(files);
            dm_project.notifyChange(0);
        }
    }

    event->acceptProposedAction();
}

class MyFrame : public QFrame {
  public:
    MyFrame(void) {}

  private:
    virtual void paintEvent(QPaintEvent *event) {
        {
            QPainter dc(this);

            dc.save();
            dc.setBackground(Qt::white);
            dc.eraseRect(dc.window());
            dc.restore();
        }

        QFrame::paintEvent(event);
    }
};

void MainWindow::initGui(void) {
    // build menu
    QMenuBar *bar = new QMenuBar;
    QMenu *menu;

    menu = new QMenu("&Book");
    connect(menu->addAction("&New"), SIGNAL(triggered()), this, SLOT(onNew()));
    connect(menu->addAction("&Open..."), SIGNAL(triggered()), this,
            SLOT(onOpen()));
    menu->addMenu(&dm_recentfiles);
    connect(menu->addAction("&Save"), SIGNAL(triggered()), this,
            SLOT(onSave()));
    connect(menu->addAction("Save &As..."), SIGNAL(triggered()), this,
            SLOT(onSaveAs()));
    menu->addSeparator();
    // connect(menu->addAction("Export &image files..."), SIGNAL(triggered()),
    // this, SLOT(onExportImages()));
    connect(menu->addAction("Export to P&DF..."), SIGNAL(triggered()), this,
            SLOT(onPrintPDF()));
    connect(menu->addAction("Export to Image Files..."), SIGNAL(triggered()),
            this, SLOT(onPrintFiles()));
    menu->addSeparator();
    connect(menu->addAction("Page Pre&view..."), SIGNAL(triggered()), this,
            SLOT(onPrintPreview()));
    connect(menu->addAction("&Print..."), SIGNAL(triggered()), this,
            SLOT(onPrint()));
    menu->addSeparator();
    connect(menu->addAction("&About"), SIGNAL(triggered()), this,
            SLOT(onShowAbout()));
    menu->addSeparator();
    connect(menu->addAction("&Quit"), SIGNAL(triggered()),
            QCoreApplication::instance(), SLOT(quit()));
    bar->addMenu(menu);

    setMenuBar(bar);

    dm_tiles = new TileView(&dm_project);
    dm_scroller = new TileScroller(&dm_project, dm_tiles);
    // dm_addbut = new ImageAddButton(&dm_project);
    dm_tabbar = new TabBar(&dm_project);

    QFrame *mainwidget = new MyFrame;
    QVBoxLayout *mainlay = new QVBoxLayout;

    mainwidget->setFrameShape(QFrame::Panel);
    // mainwidget->setFrameShadow(QFrame::Raised);
    mainwidget->setLineWidth(4);
    mainwidget->setMidLineWidth(2);

    mainlay->addWidget(dm_tiles, 1);
    mainlay->addWidget(dm_scroller);
    mainwidget->setLayout(mainlay);

    /*QPalette pal = mainwidget->palette();
    pal.setColor(QPalette::Window, Qt::white);
    mainwidget->setPalette(pal);
    mainwidget->setAutoFillBackground(true);*/

    // build main widget
    QVBoxLayout *vlay = new QVBoxLayout;
    vlay->addWidget(dm_tabbar);
    vlay->addWidget(mainwidget, 1);

    QWidget *w = new QWidget;
    w->setLayout(vlay);
    setCentralWidget(w);
}

void MainWindow::openFile(const QString &fileName) {
    dm_project.clear();

    if (!dm_project.loadXML(fileName))
        QMessageBox::critical(this, "Error Opening Book",
                              "There was an error when trying to load: " +
                                  dm_project.fileName());
    else {
        dm_recentfiles.prependFile(fileName, 0);
        dm_project.setFileName(fileName);
    }

    dm_project.notifyChange(0);
    updateTitle();
}

void MainWindow::updateTitle(void) {
    if (dm_project.fileName().isEmpty())
        setWindowTitle(QCoreApplication::applicationName());
    else
        setWindowTitle(QFileInfo(dm_project.fileName()).fileName() + " - " +
                       QCoreApplication::applicationName());
}
