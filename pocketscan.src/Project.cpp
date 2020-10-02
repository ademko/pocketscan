
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#include <Project.h>

#include <assert.h>
#include <math.h>

#include <algorithm>

#include <QColor>
#include <QDebug>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QPainter>
#include <QPrinter>
#include <QStringList>

#include <hydra/Exif.h>

#include <AutoClip.h>
#include <FileNameSeries.h>
#include <ImageAlg.h>
#include <ImageFileCache.h> // for calcAspect
#include <MainWindow.h>
#include <MathUtil.h>

using namespace hydra;

#define IGNORE_NODEPATH_EXCEPTIONS(x)                                          \
    try {                                                                      \
        x;                                                                     \
    } catch (const NodePath::error &) {                                        \
    }

static void cap(int &i) {
    if (i < 0)
        i = 0;
    else if (i > LevelAlg::WHITE)
        i = LevelAlg::WHITE;
}

static void cap(QPointF &p) {
    if (p.x() < 0)
        p.rx() = 0;
    else if (p.x() > 1.0)
        p.rx() = 1.0;
    if (p.y() < 0)
        p.ry() = 0;
    else if (p.y() > 1.0)
        p.ry() = 1.0;
}

static void saveXML(QPointF pt, NodePath p) {
    p.setPropVal("x", pt.x());
    p.setPropVal("y", pt.y());
}

static void loadXML(QPointF &pt, hydra::NodePath p) {
    pt = QPointF(p.getPropAsDouble("x"), p.getPropAsDouble("y"));
}

//
//
// TransformOp
//
//

TransformOp::TransformOp(void) { reset(); }

void TransformOp::reset(void) { dm_rotatecode = 0; }

QImage TransformOp::apply(QImage img) {
    if (dm_rotatecode == 0)
        return img;

    QTransform x;
    x.rotate(dm_rotatecode * 90);
    return img.transformed(x);
}

QSize TransformOp::applySize(QSize s) {
    if (dm_rotatecode % 2 == 1)
        return QSize(s.height(), s.width());
    else
        return s;
}

void TransformOp::rotateLeft(void) {
    dm_rotatecode--;
    if (dm_rotatecode < 0)
        dm_rotatecode = 3;
}

void TransformOp::rotateRight(void) {
    dm_rotatecode++;
    if (dm_rotatecode > 3)
        dm_rotatecode = 0;
}

void TransformOp::saveXML(hydra::NodePath p) {
    p.setPropVal("rotateCode", dm_rotatecode);
}

void TransformOp::loadXML(hydra::NodePath p) {
    dm_rotatecode = p.getPropAsLong("rotateCode");
}

//
//
// ClipOp
//
//

ClipOp::ClipOp(void) {
    reset();

    dm_size = 0;
}

bool ClipOp::isReset(void) const {
    return dm_corners[0] == QPointF(0, 0) && dm_corners[1] == QPointF(1, 0) &&
           dm_corners[2] == QPointF(1, 1) && dm_corners[3] == QPointF(0, 1);
}

void ClipOp::cap(void) {
    ::cap(dm_corners[0]);
    ::cap(dm_corners[1]);
    ::cap(dm_corners[2]);
    ::cap(dm_corners[3]);
}

static inline bool XlessThan(const QPointF &left, const QPointF &right) {
    return left.x() < right.x();
}

void ClipOp::rearrange(void) {
    // first, sort the corners by X value
    qSort(dm_corners.begin(), dm_corners.end(), XlessThan);

    // the 2nd point (in X-order) is actually the last point in the corners
    std::swap(dm_corners[1], dm_corners[3]);

    // now sort the two pairs by Y value
    if (dm_corners[3].y() < dm_corners[0].y())
        std::swap(dm_corners[0], dm_corners[3]);
    if (dm_corners[2].y() < dm_corners[1].y())
        std::swap(dm_corners[1], dm_corners[2]);

    /*qDebug()
      << dm_corners[0]
      << dm_corners[1]
      << dm_corners[2]
      << dm_corners[3];*/
}

void ClipOp::reset(void) {
    dm_corners[0] = QPointF(0, 0);
    dm_corners[1] = QPointF(1, 0);
    dm_corners[2] = QPointF(1, 1);
    dm_corners[3] = QPointF(0, 1);
}

QImage ClipOp::apply(QImage img, QSize maxsize) {
    // check for fast case
    if (isReset() || dm_size != MAX_SIZE)
        return img;

    InterClipAlg alg(img, dm_corners);

    if (maxsize.isValid())
        alg.resizeOutputByMax(maxsize);

    alg.run();

    return alg.output();
}

void ClipOp::saveXML(hydra::NodePath p) {
    ::saveXML(dm_corners[0], p["topLeft"]);
    ::saveXML(dm_corners[1], p["topRight"]);
    ::saveXML(dm_corners[2], p["bottomRight"]);
    ::saveXML(dm_corners[3], p["bottomLeft"]);

    p.setPropVal("size", dm_size);
}

void ClipOp::loadXML(hydra::NodePath p) {
    ::loadXML(dm_corners[0], p("topLeft"));
    ::loadXML(dm_corners[1], p("topRight"));
    ::loadXML(dm_corners[2], p("bottomRight"));
    ::loadXML(dm_corners[3], p("bottomLeft"));

    dm_size = MAX_SIZE;
    IGNORE_NODEPATH_EXCEPTIONS(dm_size = p.getPropAsLong("size"));
}

//
//
// Histogram
//
//

Histogram::Histogram(void) { dm_countmax = -1; }

Histogram::Histogram(const QImage &img) { computeHistogram(img); }

void Histogram::computeHistogram(const QImage &img) {
    HistoAlg alg(img);

    alg.run();

    dm_count = alg.countArray();
    dm_countmax = alg.countMax();
}

void Histogram::meanAndStdDev(double &mean, double &stddev) const {
    double sum = 0;
    int count = 0;

    for (int bin = 0; bin < HistoAlg::SIZE; ++bin) {
        sum += bin * dm_count[bin];
        count += dm_count[bin];
    }

    if (count == 0) {
        mean = 0;
        stddev = 0;
        return;
    }
    mean = sum / count;

    sum = 0;
    for (int bin = 0; bin < HistoAlg::SIZE; ++bin) {
        double diff = (bin - mean);
        sum += diff * diff * dm_count[bin];
    }

    stddev = sqrt(sum / count);
}

//
//
// LevelOp
//
//

LevelOp::LevelOp(void) { setMagicValue(LevelAlg::WHITE / 2); }

void LevelOp::reset(void) {
    dm_marks[0] = 0;
    dm_marks[1] = LevelAlg::WHITE / 2;
    dm_marks[2] = LevelAlg::WHITE;
    dm_range[0] = 0;
    dm_range[1] = LevelAlg::WHITE;
}

bool LevelOp::isReset(void) const {
    return dm_marks[0] == 0 && dm_marks[1] == LevelAlg::WHITE / 2 &&
           dm_marks[2] == LevelAlg::WHITE && dm_range[0] == 0 &&
           dm_range[1] == LevelAlg::WHITE;
}

QImage LevelOp::apply(QImage img) {
    if (isReset())
        return img;

    LevelAlg alg(img, dm_marks, dm_range);

    alg.run();

    return alg.output();
}

void LevelOp::setMagicValue(int mid) {
    dm_marks[0] = mid - 20;
    dm_marks[1] = mid;
    dm_marks[2] = mid + 10;

    cap(dm_marks[0]);
    cap(dm_marks[1]);
    cap(dm_marks[2]);

    dm_range[0] = 0;
    dm_range[1] = LevelAlg::WHITE;
}

void LevelOp::setBCValue(int b, int c) {
    reset();

    if (b < 0)
        dm_range[1] += b;
    if (b > 0)
        dm_range[0] += b;

    if (c < 0) {
        dm_range[0] -= c; // - a - is a plus
        dm_range[1] += c;
    }
    if (c > 0) {
        dm_marks[0] += c;
        dm_marks[2] -= c;
    }

    // sanity checks
    if (dm_marks[0] >= dm_marks[1])
        dm_marks[0] = dm_marks[1] - 1;
    if (dm_marks[2] <= dm_marks[1])
        dm_marks[2] = dm_marks[1] + 1;
    if (dm_range[0] >= dm_range[1])
        std::swap(dm_range[0], dm_range[1]);
}

int LevelOp::brightnessValue(void) const {
    int left = dm_range[0];
    int right = LevelAlg::WHITE - dm_range[1];
    int common = std::min(left, right);

    if (left > common)
        return left - common;
    if (right > common)
        return -(right - common); // darkness
    return 0;
}

int LevelOp::contrastValue(void) const {
    if (dm_marks[0] > 0 && dm_marks[0] == LevelAlg::WHITE - dm_marks[2])
        return dm_marks[0];

    int left = dm_range[0];
    int right = LevelAlg::WHITE - dm_range[1];
    int common = std::min(left, right);

    assert(common >= 0);
    return -common;
}

void LevelOp::saveXML(hydra::NodePath p) {
    p.setPropVal("lo", dm_marks[0]);
    p.setPropVal("mid", dm_marks[1]);
    p.setPropVal("hi", dm_marks[2]);
    p.setPropVal("range_lo", dm_range[0]);
    p.setPropVal("range_hi", dm_range[1]);
}

void LevelOp::loadXML(hydra::NodePath p) {
    dm_marks[0] = p.getPropAsLong("lo");
    dm_marks[1] = p.getPropAsLong("mid");
    dm_marks[2] = p.getPropAsLong("hi");

    dm_range[0] = 0;
    dm_range[1] = LevelAlg::WHITE;

    IGNORE_NODEPATH_EXCEPTIONS(dm_range[0] = p.getPropAsLong("range_lo");)
    IGNORE_NODEPATH_EXCEPTIONS(dm_range[1] = p.getPropAsLong("range_hi");)
}

//
// StepList
//

/*StepList::StepList(void)
{
  init(MainWindow::instance()->edition());
}*/

StepList::StepList(int edition) { init(edition); }

int StepList::indexOf(int steptype) const {
    for (int index = 0; index < dm_steps.size(); ++index)
        if (dm_steps[index] == steptype)
            return index;
    return -1;
}

void StepList::init(int edition) {
    dm_steps.push_back(StepList::ROTATE_STEP);
    dm_steps.push_back(StepList::CROP_STEP);
    dm_steps.push_back(StepList::LEVEL_STEP);
    if (edition >= MainWindow::ULTIMATE_EDITION)
        dm_steps.push_back(StepList::CONTRAST_STEP);
    if (edition >= MainWindow::ULTIMATE_EDITION)
        dm_steps.push_back(StepList::HISTO_STEP);
    dm_steps.push_back(StepList::EXPORT_STEP);
}

//
//
// Project::FileEntry
//
//

Project::FileEntry::FileEntry(void) {
    didExifCheck = false;
    didClipCheck = false;
    didlevelCheck = false;
    usingLevel = false;
    usingClip = false;
}

Project::FileEntry::FileEntry(const QString &_fileName) : fileName(_fileName) {
    didExifCheck = false;
    didClipCheck = false;
    didlevelCheck = false;
    usingLevel = false;
    usingClip = false;
}

TransformOp Project::FileEntry::computeAutoTransformOp(void) {
    short exifrot = hydra::detectExifRotate(fileName);

    TransformOp ret;

    if (exifrot == 1)
        ret.rotateLeft();
    else if (exifrot == 2)
        ret.rotateRight();

    return ret;
}

bool Project::FileEntry::computeAutoClipOp(const QImage &shrunkimage,
                                           ClipOp &outputop, QImage *outimg) {
    int count = AutoClip()(shrunkimage, outputop.corners(), outimg);

    outputop.size() = 4;

    return count > 0;
}

bool Project::FileEntry::computeAutoLevelOp(const Histogram &his,
                                            LevelOp &outputop) {
    double mean, stddev;

    his.meanAndStdDev(mean, stddev);

    // old version
    /*int marks[LevelOp::SIZE];
    double cen;

    cen = mean - 1.5*stddev;
    marks[1] = cen;
    marks[0] = cen - 2*stddev;
    marks[2] = cen + stddev/2;

    cap(marks[0]);
    cap(marks[1]);
    cap(marks[2]);

    autoLevelOp[0] = marks[0];
    autoLevelOp[1] = marks[1];
    autoLevelOp[2] = marks[2];*/

    // new version that uses the magic thing
    outputop.setMagicValue(mean - 1.5 * stddev);

    return stddev <= 30;
}

void Project::FileEntry::saveXML(hydra::NodePath p, const QString &projectdir) {
    QString relname(
        QDir(projectdir)
            .relativeFilePath(QFileInfo(fileName).absoluteFilePath()));
    p.setPropVal("fileName", relname);
    // qDebug() << projectdir << fileName << relname;

    p.setPropVal("didExifCheck", didExifCheck ? 1 : 0);
    p.setPropVal("usingLevel", usingLevel ? 1 : 0);
    p.setPropVal("didLevelCheck", didlevelCheck ? 1 : 0);
    p.setPropVal("usingClip", usingClip ? 1 : 0);
    p.setPropVal("didClipCheck", didClipCheck ? 1 : 0);

    transformOp.saveXML(p["transformOp"]);
    clipOp.saveXML(p["clipOp"]);
    levelOp.saveXML(p["levelOp"]);
    // autoLevelOp.saveXML(p["autoLevelOp"]);
}

bool Project::FileEntry::loadXML(hydra::NodePath p, const QString &projectdir) {
    QString relname = p.getPropAsString("fileName");
    fileName = QDir(projectdir).filePath(relname);
    // qDebug() << projectdir << fileName << relname;

    if (!QFileInfo(fileName).exists())
        return false;

    didExifCheck = p.getPropAsLong("didExifCheck") != 0;
    usingLevel = p.getPropAsLong("usingLevel") != 0;
    try {
        // the old tag
        didlevelCheck = p.getPropAsLong("didAutoCheck") != 0;
    } catch (const NodePath::error &) {
        didlevelCheck = p.getPropAsLong("didLevelCheck") != 0;
    }

    transformOp.loadXML(p("transformOp"));
    clipOp.loadXML(p("clipOp"));
    levelOp.loadXML(p("levelOp"));
    // autoLevelOp.loadXML(p("autoLevelOp"));

    usingClip = true;
    IGNORE_NODEPATH_EXCEPTIONS(usingClip = p.getPropAsLong("usingClip") != 0);
    didClipCheck = false;
    IGNORE_NODEPATH_EXCEPTIONS(didClipCheck =
                                   p.getPropAsLong("didClipCheck") != 0);

    return true;
}

//
//
// Project
//
//

Project::Project(void) { clear(); }

void Project::setFileName(const QString &fileName) { dm_filename = fileName; }

int Project::isDuplicate(int index) {
    assert(index >= 0);
    assert(index < dm_files.size());

    int start = index;

    // find the start of this duplicate chain
    while (start - 1 >= 0 &&
           dm_files[start - 1].fileName == dm_files[index].fileName)
        start--;

    // we found the dup set leader, so lets count where i am from there
    if (start < index)
        return index - start;

    // start == index, so either im the leader or im not a duplicate

    if (index + 1 < dm_files.size() &&
        dm_files[index].fileName == dm_files[index + 1].fileName)
        return 0;

    return -1;
}

void Project::clear(void) {
    dm_filename.clear();
    dm_files.clear();
    dm_step = 0;
}

void Project::appendFiles(const QStringList &_filenames) {
    for (QStringList::const_iterator ii = _filenames.begin();
         ii != _filenames.end(); ++ii)
        dm_files.push_back(FileEntry(*ii));
}

void Project::addListener(Listener *l) { dm_listeners.push_back(l); }

void Project::removeListener(Listener *l) {
    ListenerList::iterator ii =
        std::find(dm_listeners.begin(), dm_listeners.end(), l);

    if (ii == dm_listeners.end())
        return;

    dm_listeners.erase(ii);
}

void Project::notifyChange(Listener *source) {
    ListenerList::iterator ii = dm_listeners.begin();

    for (; ii != dm_listeners.end(); ++ii)
        (*ii)->handleProjectChanged(source);
}

// im unsure if this helps
#define YIELD                                                                  \
    if (progdlg) {                                                             \
        progdlg->setValue(pageno);                                             \
    }

bool Project::exportToPrinter(QPrinter *printer, QProgressDialog *progdlg) {
    // parallel processing?
    /*QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFileName(filename);
    printer.setPageSize(QPrinter::Letter);*/

    QPainter dc(printer);

    if (progdlg)
        progdlg->setRange(0, dm_files.size());

    for (int pageno = 0; pageno < dm_files.size(); ++pageno) {
        FileEntry &entry = dm_files[pageno];

        // dc.drawText(100, 100, entry.fileName);

        YIELD;
        QImage img = *fileCache().getImage(entry.fileName).get();
        YIELD;
        img = entry.transformOp.apply(img);
        YIELD;
        img = entry.clipOp.apply(img);
        YIELD;
        if (entry.usingLevel)
            img = entry.levelOp.apply(img);
        YIELD;

        // QSize outputSize = printer.pageRect().size();
        QSize outputSize(dc.device()->width(), dc.device()->height());

        QSize actualSize = calcAspect(img.size(), outputSize, true);
        // QPoint topLeft(printer.pageRect().topLeft());
        QPoint topLeft;

        topLeft.rx() += (outputSize.width() - actualSize.width()) / 2;
        topLeft.ry() += (outputSize.height() - actualSize.height()) / 2;

        dc.drawImage(QRect(topLeft, actualSize), img);

        /*img = img.scaled(outputSize, Qt::KeepAspectRatio,
        Qt::SmoothTransformation);

        QPoint topLeft(printer.pageRect().topLeft());

        topLeft.rx() += (outputSize.width() - img.width()) / 2;
        topLeft.ry() += (outputSize.height() - img.height()) / 2;

        dc.drawImage(QRect(topLeft, img.size()), img);*/

        if (progdlg) {
            progdlg->setValue(pageno + 1);
            if (progdlg->wasCanceled())
                return false;
        }
        if (pageno + 1 < dm_files.size())
            printer->newPage();
    }
    return true;
}

bool Project::exportToFiles(const QString &seedFilename,
                            QProgressDialog *progdlg) {
    FileNameSeries filenames(seedFilename);

    if (progdlg)
        progdlg->setRange(0, dm_files.size());

    for (int pageno = 0; pageno < dm_files.size(); ++pageno) {
        FileEntry &entry = dm_files[pageno];
        QString outfilename(filenames.fileNameAt(pageno));

        YIELD;
        QImage img = *fileCache().getImage(entry.fileName).get();
        YIELD;
        img = entry.transformOp.apply(img);
        YIELD;
        img = entry.clipOp.apply(img);
        YIELD;
        if (entry.usingLevel)
            img = entry.levelOp.apply(img);
        YIELD;

        img.save(outfilename);

        if (progdlg) {
            progdlg->setValue(pageno + 1);
            if (progdlg->wasCanceled())
                return false;
        }
    }
    return true;
}

bool Project::saveXML(const QString &filename) {
    QDomDocument doc;
    QFile f(filename);

    doc.appendChild(doc.createElement("pocketscan"));

    if (f.open(QIODevice::ReadOnly)) {
        QDomDocument readdoc;
        // read in the existing file, i guess
        if (readdoc.setContent(&f) &&
            readdoc.documentElement().tagName() == "pocketscan")
            doc = readdoc;
        f.close();
    }

    try {
        saveXML(NodePath(doc), QFileInfo(filename).absolutePath());
    } catch (const NodePath::error &) {
        return false;
    }

    if (!f.open(QIODevice::WriteOnly))
        return false;

    QTextStream ts(&f);
    doc.save(ts, 2);

    return true;
}

void Project::saveXML(hydra::NodePath p, const QString &projectdir) {
    p.erase("images");

    p["step"].setPropVal("current", dm_step);

    NodePath images = p["images"];

    for (int x = 0; x < dm_files.size(); ++x)
        dm_files[x].saveXML(images.append("image"), projectdir);
}

bool Project::loadXML(const QString &filename) {
    QFile f(filename);
    QDomDocument readdoc;

    if (!f.open(QIODevice::ReadOnly))
        return false;

    if (!readdoc.setContent(&f) ||
        readdoc.documentElement().tagName() != "pocketscan")
        return false;

    try {
        loadXML(NodePath(readdoc), QFileInfo(filename).absolutePath());

        return true;
    } catch (const NodePath::error &) {
        return false;
    }
}

void Project::loadXML(hydra::NodePath p, const QString &projectdir) {
    NodePath images = p("images");

    dm_step = 0;
    IGNORE_NODEPATH_EXCEPTIONS(dm_step = p("step").getPropAsLong("current");)

    // just incase the file has a step setting for a step we dont have
    // (such as loading an Ultimate made file in Standard
    if (MainWindow::instance()->stepList().indexOf(dm_step) == -1)
        dm_step = 0;

    if (images.hasChild("image")) {
        NodePath image(images("image"));

        do {
            FileEntry entry;

            if (entry.loadXML(image, projectdir))
                dm_files.push_back(entry);
        } while (image.loadNextSibling());
    }
}
