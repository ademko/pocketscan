
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#ifndef __INCLUDED_POCKETSCAN_IMAGEALG_H__
#define __INCLUDED_POCKETSCAN_IMAGEALG_H__

#include <hydra/TR1.h>

#include <QImage>
#include <QMutex>
#include <QColor>

/**
 * Base interface for all parallelizable image algorithms
 *
 * @author Aleksander Demko
 */ 
class ImageAlg
{
  public:
    virtual ~ImageAlg() { }

    /**
     * Runs the algoritm.
     *
     * 0 means "all cpus"
     *
     * @author Aleksander Demko
     */ 
    void run(int numcpu = 0);

  protected:
    virtual size_t height(void) const = 0;

    virtual void process(size_t y, size_t numrows) = 0;

  private:
    struct RunnableSharedArea;
    class ImageAlgRunnable;

    static void threadImageAlgRun(ImageAlg *alg, int numcpu);
};

/**
 * The texturemapping algorithm for ClipOp
 *
 * @author Aleksander Demko
 */ 
class ClipAlg : public ImageAlg
{
  public:
    typedef std::array<QPointF,4> PointFArray;
    typedef std::array<QPoint,4> PointArray;

  public:
    // the corners must be "sorted" via ClipOp::rearrange()
    ClipAlg(const QImage &src, const PointFArray &corners);

    void resizeOutputByMax(QSize maxsize);

    QImage &output(void) { return dm_output; }

  protected:
    virtual size_t height(void) const { return dm_output.height(); }

    virtual void process(size_t y, size_t numrows);

  protected:
    const QImage &dm_src;
    const PointFArray &dm_corners;

    PointArray dm_pix_corners;
    QImage dm_output;
};

/**
 * Same as ClipAlg, but using bilinear interpolation.
 *
 * @author Aleksander Demko
 */ 
class InterClipAlg : public ClipAlg
{
  public:
    // the corners must be "sorted" via ClipOp::rearrange()
    InterClipAlg(const QImage &src, const PointFArray &corners);
  protected:
    virtual void process(size_t y, size_t numrows);
};

/**
 * Computes histo grams.
 *
 * @author Aleksander Demko
 */ 
class HistoAlg : public ImageAlg
{
  public:
    static const int SIZE = 256;
    static const int HISTO_FACTOR = 256 / SIZE;

    typedef std::array<int,SIZE> HistoArray;

  public:
    HistoAlg(const QImage &src);

    HistoArray & countArray(void) { return dm_count; }
    const HistoArray & countArray(void) const { return dm_count; }

    int countMax(void) const { return dm_countmax; }

  protected:
    virtual size_t height(void) const { return dm_src.height(); }

    virtual void process(size_t y, size_t numrows);

  protected:
    const QImage &dm_src;

    HistoArray dm_count;
    int dm_countmax;

    QMutex dm_outputmutex;
};

/**
 * Applies levels to the image.
 * This is the old, original version that did (wrong) hsv-value based leveling.
 *
 * @author Aleksander Demko
 */
class OldLevelAlg : public ImageAlg
{
  public:
    //const static int SIZE = 3;
    const static int WHITE = 255;
    const static int MID_OUT = 127;

    typedef std::array<int,3> MarkArray;

  public:
    OldLevelAlg(const QImage &src, const MarkArray &marks);

    QImage &output(void) { return dm_output; }

  protected:
    virtual size_t height(void) const { return dm_output.height(); }

    virtual void process(size_t ystart, size_t numrows);

  protected:
    const QImage &dm_src;

    MarkArray dm_marks;

    QImage dm_output;
};

/**
 * RGB based leveling algorithm.
 *
 * @author Aleksander Demko
 */
class NewLevelAlg : public ImageAlg
{
  public:
    //const static int SIZE = 3;
    const static int WHITE = 255;
    const static int MID_OUT = 127;

    typedef std::array<int,3> MarkArray;
    typedef std::array<int,2> RangeArray;

  public:
    /**
     * marks are the 3 level marks, range is the output range
     *
     * @author Aleksander Demko
     */ 
    NewLevelAlg(const QImage &src, const MarkArray &marks, const RangeArray &range);

    QImage &output(void) { return dm_output; }

  protected:
    virtual size_t height(void) const { return dm_output.height(); }

    virtual void process(size_t ystart, size_t numrows);

    inline int capChannel(int col);
  protected:
    const QImage &dm_src;

    MarkArray dm_marks;
    RangeArray dm_range;

    QImage dm_output;
};

typedef NewLevelAlg LevelAlg;

/*class WhiteThreshAlg : public ImageAlg
{
  public:
    WhiteThreshAlg(const QImage &src);

    QImage &output(void) { return dm_output; }

  protected:
    virtual size_t height(void) const { return dm_output.height(); }

    virtual void process(size_t ystart, size_t numrows);

  protected:
    const QImage &dm_src;

    QImage dm_output;
};*/

/**
 * Base class for the thresholding algorithms.
 * These are algorithms that calcualte logical values for each pixel.
 *
 * @author Aleksander Demko
 */ 
class ThresholdAlg : public ImageAlg
{
  public:
    ThresholdAlg(const QImage &src);

    QImage &output(void) { return dm_output; }

    size_t trueCount(void) const { return dm_truecount; }
    size_t totalCount(void) const { return dm_output.width() * dm_output.height(); }

  protected:
    virtual size_t height(void) const { return dm_output.height(); }

  protected:
    const QImage &dm_src;

    QImage dm_output;

    QMutex dm_truemutex;
    size_t dm_truecount;
};

/**
 * A generic based implementation of ThresholdAlg
 * @author Aleksander Demko
 */ 
template <class FUNC> class GenericThresholdAlg : public ThresholdAlg
{
  public:
    GenericThresholdAlg(const QImage &src, const FUNC &f = FUNC() ) : ThresholdAlg(src), dm_func(f) { }

  protected:
    virtual void process(size_t ystart, size_t numrows)
    {
      int w = dm_output.width(), x, y;
      QColor c, hsv;
      QRgb W = QColor(Qt::white).rgb();
      QRgb B = QColor(Qt::black).rgb();
      size_t mycount = 0;
      bool b;

      for (y=ystart; y<ystart+numrows; ++y)
        for (x=0; x<w; ++x) {
          c = dm_src.pixel(x, y);

          b = dm_func(c);

          dm_output.setPixel(x, y, b ? W : B);

          if (b)
            ++mycount;
        }

      if (mycount>0) {
        QMutexLocker L(&dm_truemutex);

        dm_truecount += mycount;
      }
    }

  protected:
    FUNC dm_func;
};

#endif

