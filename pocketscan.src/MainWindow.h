
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#ifndef __INCLUDED_POCKETSCAN_MAINWINDOW_H__
#define __INCLUDED_POCKETSCAN_MAINWINDOW_H__

#include <QHBoxLayout>
#include <QMainWindow>
#include <QMenu>
#include <QPrinter>

#include <Project.h>

class TileView;
class TileScroller;
class ImageAddButton;
class TabBar;

// not recursive... should this be?
// will not clear outlist, only append to it
void expandImageDirectory(const QString &dirname, QStringList &outlist);

class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    enum {
        DEMO_EDITION = 0,
        STANDARD_EDITION = 1,
        PRO_EDITION = 2,
        ULTIMATE_EDITION = 3,
    };

  public:
    MainWindow(void);

    virtual ~MainWindow();

    static MainWindow *instance(void) { return dm_instance; }

    int edition(void) const { return dm_edition; }

    const StepList &stepList(void) const { return dm_steplist; }

    Project &project(void) { return dm_project; }

  public slots:
    void onNew(void);
    void onOpen(void);
    void onRecentFileOpen(void);
    void onSave(void);
    void onSaveAs(void);

    void onExportImages(void);

    void onPrintPreview(void);
    void onPrint(void);
    void onPrintPDF(void);
    void onPrintFiles(void);

    void onShowAbout(void);

    void onPrintPage(QPrinter *printer);

  protected:
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent *event);

  private:
    void initGui(void);
    void openFile(const QString &fileName);
    void updateTitle(void);

  private:
    class RecentFiles : public QMenu {
      public:
        RecentFiles(MainWindow *parent);
        void prependFile(const QString &fileName, int type);

      private:
        void load(void);
        void save(void);

      private:
        MainWindow *dm_parent;

        typedef std::vector<QAction *> actions_t;

        actions_t dm_actions;
    };

  private:
    static MainWindow *dm_instance;

    int dm_edition;
    StepList dm_steplist;

    RecentFiles dm_recentfiles;

    TileView *dm_tiles;
    TileScroller *dm_scroller;
    // ImageAddButton *dm_addbut;
    TabBar *dm_tabbar;

    Project dm_project;

    QPrinter dm_printer;
    QString dm_pdfoutfilename, dm_imageoutname;
};

#endif
