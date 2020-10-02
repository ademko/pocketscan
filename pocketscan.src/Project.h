
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#ifndef __INCLUDED_POCKETSCAN_PROJECT_H__
#define __INCLUDED_POCKETSCAN_PROJECT_H__

#include <list>
#include <vector>

#include <ImageAlg.h>

#include <QProgressDialog>
#include <QPrinter>
#include <QString>

#include <hydra/NodePath.h>

#include <ImageFileCache.h>

/**
 * Listens to changes to the Project object.
 *
 * @author Aleksander Demko
 */
class Listener {
  public:
    virtual ~Listener() {}

    // source may be null
    virtual void handleProjectChanged(Listener *source) = 0;
};

class TransformOp {
  public:
    TransformOp(void);

    void reset(void);

    QImage apply(QImage img);

    // similar to apply(), but applies the rotation to the size params instead
    // (ie width-height might be swapped)
    QSize applySize(QSize s);

    void rotateLeft(void);
    void rotateRight(void);

    // future TODO: flip operations (does this make sense for this app?

    void saveXML(hydra::NodePath p);

    void loadXML(hydra::NodePath p);

  private:
    int dm_rotatecode; // 0 = none, 1 = 90cw, 2 = 180cw, 3 == 270cw/90ccw
};

class ClipOp {
  public:
    const static int MAX_SIZE = 4;

  public:
    ClipOp(void);

    // returns true if the marks at the default, simple, reset case
    bool isReset(void) const;

    void reset(void);

    // make sure none of the points are out of 0..1
    void cap(void);

    // order the 4 points to topleft,topright,botleft,botright fasion
    void rearrange(void);

    QImage apply(QImage img, QSize maxsize = QSize());

    QPointF operator[](int index) const { return dm_corners[index]; }

    QPointF &operator[](int index) { return dm_corners[index]; }

    ClipAlg::PointFArray &corners(void) { return dm_corners; }
    const ClipAlg::PointFArray &corners(void) const { return dm_corners; }

    int &size(void) { return dm_size; }
    int size(void) const { return dm_size; }

    void saveXML(hydra::NodePath p);

    void loadXML(hydra::NodePath p);

  private:
    ClipAlg::PointFArray
        dm_corners; // topleft, topright, botright, botleft, respectivly
    // in the future, maybe replace this with a basic valid flag or something
    // ... unless we want >4 free-ploygon like clipping systems
    int dm_size;
};

/**
 * A statistical histogram.
 *
 * @author Aleksander Demko
 */
class Histogram {
  public:
    /// construct a null/empty histogram
    Histogram(void);
    /// build a histogram from the hsv-values from the image
    Histogram(const QImage &img);

    bool isNull(void) const { return dm_countmax == -1; }

    void computeHistogram(const QImage &img);

    int operator[](int index) const { return dm_count[index]; }

    int &operator[](int index) { return dm_count[index]; }

    int countMax(void) const { return dm_countmax; }

    // calcs based on the histogram... therefore relativly fast
    void meanAndStdDev(double &mean, double &stddev) const;

  private:
    HistoAlg::HistoArray dm_count;
    int dm_countmax;
};

/**
 * This is a core, color level operation that uses the LevelAlg.
 *
 * In addition to direct marks and range array settings, this class
 * lets you auto set these values via:
 *  - magic value
 *  - brightness and contrast
 * @author Aleksander Demko
 */
class LevelOp {
  public:
    /**
     * Inits the LevelOp object to setMagicValue(WHITE/2).
     * and NOT reset.
     *
     * This was deemed more logical and consistant with the UI.
     *
     * @author Aleksander Demko
     */
    LevelOp(void);

    void reset(void);

    // returns true if the marks at the default, simple, reset case
    bool isReset(void) const;

    QImage apply(QImage img);

    int operator[](int index) const { return dm_marks[index]; }

    int &operator[](int index) { return dm_marks[index]; }

    LevelAlg::MarkArray &marks(void) { return dm_marks; }
    const LevelAlg::MarkArray &marks(void) const { return dm_marks; }

    LevelAlg::RangeArray &range(void) { return dm_range; }
    const LevelAlg::RangeArray &range(void) const { return dm_range; }

    /**
     * Sets the mid (gray level) of the levels, and magically
     * sets the other two values accordingly.
     *
     * @author Aleksander Demko
     */
    void setMagicValue(int mid);

    /**
     * Gets the magic value, which is really just the gray value ([1]) for now.
     *
     * @author Aleksander Demko
     */
    int magicValue(void) const { return dm_marks[1]; }

    void setBCValue(int b, int c);

    int brightnessValue(void) const;

    int contrastValue(void) const;

    void saveXML(hydra::NodePath p);

    void loadXML(hydra::NodePath p);

  private:
    LevelAlg::MarkArray dm_marks;
    LevelAlg::RangeArray dm_range;
};

/**
 * Enumerates all the steps in the given edition.
 *
 * @author Aleksander Demko
 */
class StepList {
  public:
    // note that might not be the order of the steps (as newer steps may be
    // inserted between previous steps) use StepList to get that dont change the
    // numbers, as theyre wired into the file formats
    enum {
        ROTATE_STEP = 0,
        CROP_STEP = 1,
        LEVEL_STEP = 2,
        EXPORT_STEP = 3,
        // bonus, ultimate steps
        HISTO_STEP = 4,
        CONTRAST_STEP = 5,
    };

    typedef std::vector<int> StepVec;

  public:
    /// uses MainWindow::instance()->edition()
    // StepList(void);
    /// builds the step list for the given app edition type
    StepList(int edition);

    StepVec &steps(void) { return dm_steps; }
    const StepVec &steps(void) const { return dm_steps; }

    /// returns the index (within the steplist) of the given step, or -1 if not
    /// found
    int indexOf(int steptype) const;

    static bool isLevelStep(int step) {
        return step == LEVEL_STEP || step == EXPORT_STEP ||
               step == HISTO_STEP || step == CONTRAST_STEP;
    }

  private:
    void init(int edition);

  private:
    StepVec dm_steps;
};

/**
 * The core data type.
 *
 * @author Aleksander Demko
 */
class Project {
  public:
    class FileEntry {
      public:
        FileEntry(void);
        FileEntry(const QString &_fileName);

        // opens the file and returns an automatically deduced TransformOp
        TransformOp computeAutoTransformOp(void);

        static bool computeAutoClipOp(const QImage &shrunkimage,
                                      ClipOp &outputop, QImage *outimg = 0);

        // computes autoLevelOp
        // returns true if it is recommended to use it
        static bool computeAutoLevelOp(const Histogram &his, LevelOp &outputop);

        // throws NodePath errors options
        void saveXML(hydra::NodePath p, const QString &projectdir);

        // throws NodePath errors options
        // returns false though if the file doesn't exist
        // and should be remove
        bool loadXML(hydra::NodePath p, const QString &projectdir);

      public:
        QString fileName;

        bool didExifCheck;
        TransformOp transformOp;

        bool didClipCheck;
        bool usingClip;
        ClipOp clipOp;

        bool usingLevel;
        LevelOp levelOp;

        bool didlevelCheck;
        // LevelOp autoLevelOp;
    };

    typedef std::vector<FileEntry> FileList;

  public:
    Project(void);

    void setFileName(const QString &fileName);
    const QString &fileName(void) const { return dm_filename; }

    FileList &files(void) { return dm_files; }
    const FileList &files(void) const { return dm_files; }

    /**
     * Returns wether the given index'ed file in files() is a duplicate.
     * this will return -1 if it is not, otherwise it will return a >=0 number
     * indicating its place in the duplicate string.
     *
     * @author Aleksander Demko
     */
    int isDuplicate(int index);

    ImageFileCache &fileCache(void) { return dm_filecache; }

    void clear(void);
    void appendFiles(const QStringList &_filenames);

    /// caller should call notifyChange after
    void setStep(int newstep) { dm_step = newstep; }
    int step(void) const { return dm_step; }

    void addListener(Listener *l);
    void removeListener(Listener *l);

    // source may be null
    void notifyChange(Listener *source);

    /// returns true on success (user abort = failure)
    bool exportToPrinter(QPrinter *printer, QProgressDialog *progdlg = 0);
    /// returns true on success (user abort = failure)
    bool exportToFiles(const QString &seedFilename,
                       QProgressDialog *progdlg = 0);

    bool saveXML(const QString &filename);

    // throws on errors
    void saveXML(hydra::NodePath p, const QString &projectdir);

    bool loadXML(const QString &filename);

    // throws on errors
    void loadXML(hydra::NodePath p, const QString &projectdir);

  private:
    QString dm_filename;

    FileList dm_files;

    ImageFileCache dm_filecache;

    typedef std::list<Listener *> ListenerList;

    ListenerList dm_listeners;

    int dm_step;
};

#endif
