
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#include <TabBar.h>

#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>

#include <MainWindow.h>

TabBar::TabBar(Project *p) : dm_project(p) {
    dm_project->addListener(this);

    initGui();
}

TabBar::~TabBar() { dm_project->removeListener(this); }

void TabBar::handleProjectChanged(Listener *source) {
    const StepList &slist = MainWindow::instance()->stepList();
    if (slist.steps()[dm_tabber->currentIndex()] == dm_project->step())
        return;

    updateLabels();

    int index = MainWindow::instance()->stepList().indexOf(dm_project->step());
    if (index >= 0)
        dm_tabber->setCurrentIndex(index);
}

void TabBar::onBut1(void) {
    if (dm_project->step() == StepList::ROTATE_STEP) {
        QFileDialog dlg;

        // qDebug() << QImageReader::supportedImageFormats();
        QStringList filters;
        filters << "Images files (*.jpg *.jpeg *.png *.bmp *.tif *.tiff *.gif)"
                << "JPEG Image files (*.jpg *.jpeg)"
                << "Any files (*)";

        dlg.setWindowTitle("Add Images");
        dlg.setFileMode(QFileDialog::ExistingFiles);
        dlg.setAcceptMode(QFileDialog::AcceptOpen);
        dlg.setNameFilters(filters);

        if (dlg.exec() != QDialog::Accepted)
            return;

        dm_project->appendFiles(dlg.selectedFiles());
        dm_project->notifyChange(0);
        return;
    }
    if (dm_project->step() == StepList::EXPORT_STEP) {
        MainWindow::instance()->onPrintPDF();
        return;
    }
}

void TabBar::onBut2(void) {
    if (dm_project->step() == StepList::ROTATE_STEP) {
        QFileDialog dlg;

        // QStringList filters;
        // filters << "JPEG Image files (*.jpg *.jpeg)" << "Any files (*)";

        dlg.setWindowTitle("Add Image Folder");
        dlg.setFileMode(QFileDialog::Directory);
        dlg.setAcceptMode(QFileDialog::AcceptOpen);
        // dlg.setNameFilters(filters);

        if (dlg.exec() != QDialog::Accepted)
            return;

        QStringList imglist;

        for (int i = 0; i < dlg.selectedFiles().size(); ++i)
            expandImageDirectory(dlg.selectedFiles()[i], imglist);

        if (imglist.size() <= 0)
            return;
        if (imglist.size() >= 100) {
            if (QMessageBox::question(this, "Add Image Folder",
                                      "Are you sure you want to add " +
                                          QString::number(imglist.size()) +
                                          " images to your book?",
                                      QMessageBox::Yes | QMessageBox::Cancel) !=
                QMessageBox::Yes)
                return;
        }

        imglist.sort();

        dm_project->appendFiles(imglist);
        dm_project->notifyChange(0);
        return;
    }
    if (dm_project->step() == StepList::EXPORT_STEP) {
        MainWindow::instance()->onPrint();
        return;
    }
}

void TabBar::onTabChange(int index) {
    const StepList &slist = MainWindow::instance()->stepList();
    if (slist.steps()[index] == dm_project->step())
        return;

    dm_project->setStep(slist.steps()[index]);
    dm_project->notifyChange(0);

    updateLabels();
}

void TabBar::initGui(void) {
    QHBoxLayout *hlay = new QHBoxLayout;
    QVBoxLayout *v, *bigbox;

    dm_tabber = new QTabBar;
    dm_but1 = new QPushButton("");
    dm_but2 = new QPushButton("");
    dm_title = new QLabel;
    dm_desc = new QLabel;

    const StepList &slist = MainWindow::instance()->stepList();
    for (int s = 0; s < slist.steps().size(); ++s) {
        switch (slist.steps()[s]) {
        case StepList::ROTATE_STEP:
            dm_tabber->addTab("Add, Sort and Rotate");
            break;
        case StepList::CROP_STEP:
            dm_tabber->addTab("Image Clipping");
            break;
        case StepList::LEVEL_STEP:
            dm_tabber->addTab("Adjust Levels");
            break;
        case StepList::CONTRAST_STEP:
            dm_tabber->addTab("Brightness and Contrast");
            break;
        case StepList::HISTO_STEP:
            dm_tabber->addTab("Histogram");
            break;
        case StepList::EXPORT_STEP:
            dm_tabber->addTab("Save Your Book");
            break;
        default:
            dm_tabber->addTab("Unkown");
            break;
        }
    }

    QFont f = dm_title->font();
    f.setPointSize(f.pointSize() * 2);
    dm_title->setFont(f);

    // connect(dm_prevbut, SIGNAL(clicked(bool)), this, SLOT(onPrevBut()));
    // connect(dm_nextbut, SIGNAL(clicked(bool)), this, SLOT(onNextBut()));
    connect(dm_but1, SIGNAL(clicked(bool)), this, SLOT(onBut1()));
    connect(dm_but2, SIGNAL(clicked(bool)), this, SLOT(onBut2()));

    connect(dm_tabber, SIGNAL(currentChanged(int)), this,
            SLOT(onTabChange(int)));

    updateLabels();

    // hlay->addStretch(1);
    // hlay->addWidget(dm_prevbut);

    v = new QVBoxLayout;
    v->addWidget(dm_but1);
    v->addWidget(dm_but2);
    hlay->addLayout(v);

    v = new QVBoxLayout;
    v->addWidget(dm_title);
    v->addWidget(dm_desc);
    hlay->addLayout(v, 1);

    // hlay->addWidget(dm_nextbut);
    // hlay->addStretch(1);

    bigbox = new QVBoxLayout;

    bigbox->addWidget(dm_tabber);
    bigbox->addLayout(hlay);

    // layout thing (in the future, make this smarter/deduced from runtime info
    dm_desc->setMinimumSize(QSize(0, 40));

    setLayout(bigbox);

    // qDebug() << minimumSize() << size();
    // setMinimumSize(QSize(0, dm_title->minimumSize().height() +
    // dm_desc->minimumSize().height()*2)); qDebug() << minimumSize() << size();
}

void TabBar::updateLabels(void) {
    // dm_prevbut->setEnabled(dm_project->step() > 0);
    // dm_nextbut->setEnabled(dm_project->step() < StepList::LAST_STEP);

    switch (dm_project->step()) {
    case StepList::ROTATE_STEP:
        dm_title->setText(stepString() + "Add, Sort and Rotate Images");
        dm_desc->setText("Add images to your book. Rotate them as needed. You "
                         "may also reorder\n"
                         "the images, aswell as remove any unwanted images.");
        dm_but1->setIcon(QIcon(":/img_add.png"));
        dm_but1->setText("Add Images...");
        dm_but1->setVisible(true);
        dm_but2->setIcon(QIcon(":/file_add.png"));
        dm_but2->setText("Add Folder...");
        dm_but2->setVisible(true);
        break;
    case StepList::CROP_STEP:
        dm_title->setText(stepString() + "Clip Images");
        dm_desc->setText("Use the box tool to clip (cut out) the actual "
                         "pages from the images.");
        dm_but1->setVisible(false);
        dm_but2->setVisible(false);
        break;
    case StepList::LEVEL_STEP:
        dm_title->setText(stepString() + "Adjust Levels");
        dm_desc->setText(
            "For pages that have mostly text, you can use the level tool\n"
            "to filter the page, making them more suitable for printing.");
        dm_but1->setVisible(false);
        dm_but2->setVisible(false);
        break;
    case StepList::CONTRAST_STEP:
        dm_title->setText(stepString() + "Contrast and Brightness");
        dm_desc->setText(
            "You can adjust the contrast and brightness of the page\n"
            "page, making them more suitable for printing.");
        dm_but1->setVisible(false);
        dm_but2->setVisible(false);
        break;
    case StepList::HISTO_STEP:
        dm_title->setText(stepString() + "Histogram Levels");
        dm_desc->setText(
            "Use the histogram to fine tune the level adjustments.");
        dm_but1->setVisible(false);
        dm_but2->setVisible(false);
        break;
    case StepList::EXPORT_STEP:
        dm_title->setText(stepString() + "Done - Save Your Book");
        dm_desc->setText(
            "You can now print your book, make a PDF or save your book.\n"
            "The Book menu has additional options, like exporting to image "
            "files.");
        dm_but1->setIcon(QIcon());
        dm_but1->setText("Export to PDF...");
        dm_but1->setVisible(true);
        dm_but2->setIcon(QIcon());
        dm_but2->setText("Print Book...");
        dm_but2->setVisible(true);
        break;
    }
}

QString TabBar::stepString(void) {
    return "Step " +
           QString::number(
               MainWindow::instance()->stepList().indexOf(dm_project->step()) +
               1) +
           ": ";
}
