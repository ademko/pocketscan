
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#include <ImageAlg.h>

#include <assert.h>

#include <QColor>
#include <QDebug>
#include <QMutex>
#include <QRunnable>
#include <QThreadPool>
#include <QWaitCondition>

#include <MathUtil.h>

#include <ImageFileCache.h> // for calcAspect

//
// ThreadImageAlgRun
//

struct ImageAlg::RunnableSharedArea {
    ImageAlg *alg;

    int aliveCount;
    QMutex mutex;
    QWaitCondition cond;
};

class ImageAlg::ImageAlgRunnable : public QRunnable {
  public:
    ImageAlgRunnable(RunnableSharedArea *area, int y, int numrows);

    virtual void run(void);

  private:
    RunnableSharedArea *dm_area;
    int dm_y, dm_numrows;
};

ImageAlg::ImageAlgRunnable::ImageAlgRunnable(RunnableSharedArea *area, int y,
                                             int numrows)
    : dm_area(area), dm_y(y), dm_numrows(numrows) {}

void ImageAlg::ImageAlgRunnable::run(void) {
    dm_area->alg->process(dm_y, dm_numrows);

    {
        QMutexLocker locker(&dm_area->mutex);
        dm_area->aliveCount--;

        dm_area->cond.wakeOne();
    }
}

void ImageAlg::threadImageAlgRun(ImageAlg *alg, int numcpu) {
    assert(numcpu >= 0);

    QThreadPool *pool = QThreadPool::globalInstance();
    assert(pool);

    if (numcpu == 0)
        numcpu = pool->maxThreadCount();

    if (numcpu <= 1 || numcpu > alg->height()) {
        // cant seem to auto detect the number of cpu? just run one thread then
        // or we have more processors than lines
        alg->process(0, alg->height());
        return;
    }

    assert(numcpu > 1);

    RunnableSharedArea area;
    area.alg = alg;
    area.aliveCount = numcpu;

    int jobsize = alg->height() / numcpu;
    int joby = 0;

    assert(jobsize > 0);

    for (int i = 0; i < (numcpu - 1); ++i) {
        pool->start(new ImageAlgRunnable(&area, joby, jobsize));
        joby += jobsize;
    }
    // do the last, possibly odd sizes job
    pool->start(new ImageAlgRunnable(&area, joby, alg->height() - joby));

    {
        QMutexLocker l(&area.mutex);

        while (area.aliveCount > 0)
            area.cond.wait(&area.mutex);
    }
}

//
//
// ImageAlg
//
//

void ImageAlg::run(int numcpu) {
    assert(numcpu >= 0);

    if (numcpu == 1)
        process(0, height());
    else
        threadImageAlgRun(this, numcpu);
}

//
//
// ClipAlg
//
//

inline bool operator<(const QSize &l, const QSize &r) {
    return l.width() < r.width() && l.height() < r.height();
}

ClipAlg::ClipAlg(const QImage &src, const PointFArray &corners)
    : dm_src(src), dm_corners(corners) {
    int i;

    for (i = 0; i < dm_pix_corners.size(); ++i) {
        dm_pix_corners[i].rx() =
            static_cast<int>(dm_corners[i].x() * (dm_src.width() - 1));
        dm_pix_corners[i].ry() =
            static_cast<int>(dm_corners[i].y() * (dm_src.height() - 1));
    }

    QSize newsize;

    newsize.rwidth() =
        std::max(pointDistance(dm_pix_corners[0], dm_pix_corners[1]),
                 pointDistance(dm_pix_corners[2], dm_pix_corners[3]));
    newsize.rheight() =
        std::max(pointDistance(dm_pix_corners[0], dm_pix_corners[3]),
                 pointDistance(dm_pix_corners[1], dm_pix_corners[2]));

    dm_output = QImage(newsize, dm_src.format());
}

void ClipAlg::resizeOutputByMax(QSize maxsize) {
    assert(maxsize.isValid());

    if (dm_output.size() < maxsize)
        return;

    QSize newsize = calcAspect(dm_output.size(), maxsize, false);

    if (newsize != dm_output.size())
        dm_output = QImage(newsize, dm_src.format());
}

void ClipAlg::process(size_t y, size_t numrows) {
    QPoint d;

    for (d.ry() = y; d.y() < y + numrows; ++d.ry()) {
        QPoint leftp(lineFraction(d.y(), dm_output.size().height() - 1,
                                  dm_pix_corners[0], dm_pix_corners[3]));
        QPoint rightp(lineFraction(d.y(), dm_output.size().height() - 1,
                                   dm_pix_corners[1], dm_pix_corners[2]));
        for (d.rx() = 0; d.x() < dm_output.size().width(); ++d.rx()) {
            QPoint srcp = lineFraction(d.x(), dm_output.size().width() - 1,
                                       leftp, rightp);
            dm_output.setPixel(d, dm_src.pixel(srcp));
        }
    }
}

//
// InterClipAlg
//

InterClipAlg::InterClipAlg(const QImage &src, const PointFArray &corners)
    : ClipAlg(src, corners) {}

inline void addCol(double &r, double &g, double &b, double frac,
                   const QRgb &rgb) {
    r += qRed(rgb) * frac;
    g += qGreen(rgb) * frac;
    b += qBlue(rgb) * frac;
}

void InterClipAlg::process(size_t y, size_t numrows) {
    QPoint d;

    for (d.ry() = y; d.y() < y + numrows; ++d.ry()) {
        QPointF leftp(lineFractionF(d.y(), dm_output.size().height() - 1,
                                    dm_pix_corners[0], dm_pix_corners[3]));
        QPointF rightp(lineFractionF(d.y(), dm_output.size().height() - 1,
                                     dm_pix_corners[1], dm_pix_corners[2]));
        for (d.rx() = 0; d.x() < dm_output.size().width(); ++d.rx()) {
            QPointF srcp = lineFractionF(d.x(), dm_output.size().width() - 1,
                                         leftp, rightp);
            QPoint srcint =
                QPoint(static_cast<int>(srcp.x()), static_cast<int>(srcp.y()));

            bool hasextraX = srcint.x() + 1 < dm_src.width();
            bool hasextraY = srcint.y() + 1 < dm_src.height();

            double xfrac = srcp.x() - srcint.x();
            double yfrac = srcp.y() - srcint.y();

            double r = 0, g = 0, b = 0;

            addCol(r, g, b, (1 - xfrac) * (1 - yfrac),
                   dm_src.pixel(srcint.x(), srcint.y()));
            if (hasextraX)
                addCol(r, g, b, xfrac * (1 - yfrac),
                       dm_src.pixel(srcint.x() + 1, srcint.y()));
            if (hasextraY)
                addCol(r, g, b, (1 - xfrac) * yfrac,
                       dm_src.pixel(srcint.x(), srcint.y() + 1));
            if (hasextraX && hasextraY)
                addCol(r, g, b, xfrac * yfrac,
                       dm_src.pixel(srcint.x() + 1, srcint.y() + 1));

            dm_output.setPixel(d, qRgb(static_cast<int>(r), static_cast<int>(g),
                                       static_cast<int>(b)));
        } // for x
    }     // for y
}

//
//
// HistoAlg
//
//

HistoAlg::HistoAlg(const QImage &src) : dm_src(src) {
    int i;

    for (i = 0; i < SIZE; ++i)
        dm_count[i] = 0;
    dm_countmax = 0;
}

void HistoAlg::process(size_t y, size_t numrows) {
    int i;

    QPoint p;
    QColor c;

    HistoArray mycounts;
    for (i = 0; i < SIZE; ++i)
        mycounts[i] = 0;

    for (p.ry() = y; p.ry() < y + numrows; ++p.ry())
        for (p.rx() = 0; p.rx() < dm_src.width(); ++p.rx()) {
            c.setRgb(dm_src.pixel(p));
            int value = c.value() / HISTO_FACTOR;
            assert(value >= 0);
            assert(value < SIZE);
            mycounts[value]++;
        }

    // output time
    QMutexLocker l(&dm_outputmutex);

    for (i = 0; i < SIZE; ++i)
        dm_count[i] += mycounts[i];

    for (i = 0; i < SIZE; ++i)
        if (dm_count[i] > dm_countmax)
            dm_countmax = dm_count[i];
}

//
//
// OldLevelAlg
//
//

OldLevelAlg::OldLevelAlg(const QImage &src, const MarkArray &marks)
    : dm_src(src), dm_marks(marks) {
    dm_output = QImage(dm_src.width(), dm_src.height(), dm_src.format());
}

void OldLevelAlg::process(size_t ystart, size_t numrows) {
    int w = dm_output.width(), x, y;
    QColor c;

    for (y = ystart; y < ystart + numrows; ++y)
        for (x = 0; x < w; ++x) {
            c.setRgb(dm_src.pixel(x, y));
            int value = c.value();

            if (value == dm_marks[1])
                value = MID_OUT;
            else if (value <= dm_marks[0])
                value = 0;
            else if (value >= dm_marks[2])
                value = WHITE;
            else if (value < dm_marks[1])
                value = MID_OUT * (value - dm_marks[0]) /
                        (dm_marks[1] - dm_marks[0]);
            else if (value > dm_marks[1])
                value = (WHITE + 1 - MID_OUT) * (value - dm_marks[1]) /
                            (dm_marks[2] - dm_marks[1]) +
                        MID_OUT;
            assert(value >= 0);
            assert(value <= WHITE);

            // these 2-tests for explicit white/black seem to help
            // although getting a nice log based/curve leveler is probably the
            // 'right' solution
            if (value == 0)
                dm_output.setPixel(x, y, 0);
            else if (value == WHITE)
                dm_output.setPixel(x, y, 0xFFFFFF);
            else {
                c.setHsv(c.hue(), c.saturation(), value);
                dm_output.setPixel(x, y, c.rgb());
            }
        }
}

//
//
// NewLevelAlg
//
//

NewLevelAlg::NewLevelAlg(const QImage &src, const MarkArray &marks,
                         const RangeArray &range)
    : dm_src(src), dm_marks(marks), dm_range(range) {
    assert(dm_range[0] < dm_range[1]);
    dm_output = QImage(dm_src.width(), dm_src.height(), dm_src.format());
}

inline int NewLevelAlg::capChannel(int value) {
    if (value == dm_marks[1])
        value = MID_OUT;
    else if (value <= dm_marks[0])
        value = 0;
    else if (value >= dm_marks[2])
        value = WHITE;
    else if (value < dm_marks[1])
        value = MID_OUT * (value - dm_marks[0]) / (dm_marks[1] - dm_marks[0]);
    else if (value > dm_marks[1])
        value = (WHITE + 1 - MID_OUT) * (value - dm_marks[1]) /
                    (dm_marks[2] - dm_marks[1]) +
                MID_OUT;

    assert(value >= 0);
    assert(value <= WHITE);

    // new... scale the output value via the range array
    value = (dm_range[1] - dm_range[0]) * value / WHITE + dm_range[0];

    return value;
}

void NewLevelAlg::process(size_t ystart, size_t numrows) {
    int w = dm_output.width(), x, y;
    QColor c;

    for (y = ystart; y < ystart + numrows; ++y)
        for (x = 0; x < w; ++x) {
            c.setRgb(dm_src.pixel(x, y));

            c.setRed(capChannel(c.red()));
            c.setGreen(capChannel(c.green()));
            c.setBlue(capChannel(c.blue()));

            dm_output.setPixel(x, y, c.rgb());
        } // for x
}

//
// WhiteThreshAlg
//

/*WhiteThreshAlg::WhiteThreshAlg(const QImage &src)
  : dm_src(src)
{
  dm_output = QImage(dm_src.width(), dm_src.height(), dm_src.format());
}

void WhiteThreshAlg::process(size_t ystart, size_t numrows)
{
  int w = dm_output.width(), x, y;
  QColor c, hsv;
  QRgb W = QColor(Qt::red).rgb();
  QRgb B = QColor(Qt::blue).rgb();

  for (y=ystart; y<ystart+numrows; ++y)
    for (x=0; x<w; ++x) {
      c.setRgb(dm_src.pixel(x,y));
      hsv = c.toHsv();

      // 1st case: dual
      //bool iswhite = hsv.saturation() < 30;//(255-12);// && hsv.value() < 50;
      //bool iswhite = true;
      //bool iswhite2 = (c.red() + c.green() + c.blue()) / 3 > 80;
      //bool iswhite2 = true;

      // just value doesnt work too well
      //bool iswhite = hsv.value() > 60;

      // 2nd case
      // solo: really good, except for my complex case
      bool iswhite2 = (c.red() + c.green() + c.blue()) / 3 > 100;

      //dm_output.setPixel(x, y, iswhite&&iswhite2 ? W : B);
      dm_output.setPixel(x, y, iswhite2 ? W : B);

      //dm_output.setPixel(x, y, qRgb(hsv.saturation(), hsv.saturation(),
hsv.saturation()));
      //dm_output.setPixel(x, y, qRgb(hsv.value(), hsv.value(), hsv.value()));
    }
}*/

//
// ThresholdAlg
//

ThresholdAlg::ThresholdAlg(const QImage &src) : dm_src(src) {
    dm_output = QImage(dm_src.width(), dm_src.height(), dm_src.format());
    dm_truecount = 0;
}
