
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#ifndef __INCLUDED_LEVELEDITOR_H__
#define __INCLUDED_LEVELEDITOR_H__

#include <QWidget>

#include <Project.h>

class LevelEditor : public QWidget
{
    Q_OBJECT

  public:
    /**
     * Null constuctor
     *
     * @author Aleksander Demko
     */
    LevelEditor(void);

    /**
     * Opens a level editor on the given image.
     *
     * @author Aleksander Demko
     */ 
    //LevelEditor(const Histogram &histo, const LevelAlg::MarkArray &initialMarks);

    void unsetHistoLevels(void);
    void setHisto(const Histogram &histo);
    void setLevels(const LevelAlg::MarkArray &initialMarks);

    const LevelAlg::MarkArray & marks(void) const { return dm_marks; }

  signals:
    void levelChanged(void);

  protected:
    virtual void resizeEvent(QResizeEvent *event);
    virtual void paintEvent(QPaintEvent *event);

    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

  private:
    static QPoint baseHisto(void) { return QPoint(BORDER,BORDER); }
    QSize sizeHisto(void) { return dm_pixmap.size(); }
    QPoint baseTri(void) { return QPoint(BORDER, BORDER + dm_pixmap.size().height()); }
    QSize sizeTri(void) { return QSize(dm_pixmap.size().width(), TRI_HEIGHT); }

    void opToPixels(void);
    void pixelsToOp(void);

    void drawBars(void);
    void drawTri(QPainter &dc);

  private:
    const static int BORDER = 6;
    const static int TRI_HEIGHT = 10;

    LevelAlg::MarkArray dm_marks;

    bool dm_dirty;
    QPixmap dm_pixmap;

    Histogram dm_histogram;

    QPoint dm_points[std::tuple_size<LevelAlg::MarkArray>::value];
    int dm_selected;
};

class RangeEditor : public QWidget
{
    Q_OBJECT

  public:
    /**
     * Null constuctor
     *
     * @author Aleksander Demko
     */
    RangeEditor(void);

    void unsetRange(void);
    void setRange(const LevelAlg::RangeArray &r);

    const LevelAlg::RangeArray & range(void) const { return dm_range; }

  signals:
    void levelChanged(void);

  protected:
    virtual void resizeEvent(QResizeEvent *event);
    virtual void paintEvent(QPaintEvent *event);

    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

  private:
    QPoint baseTri(void) { return QPoint(BORDER, BORDER); }
    QSize sizeTri(void) { assert(dm_screenwidth>0); return QSize(dm_screenwidth-2*BORDER, TRI_HEIGHT); }

    void opToPixels(void);
    void pixelsToOp(void);

    void drawTri(QPainter &dc);

  private:
    const static int BORDER = 6;
    const static int TRI_HEIGHT = 10;

    LevelAlg::RangeArray dm_range;

    bool dm_dirty;
    int dm_screenwidth;

    QPoint dm_points[std::tuple_size<LevelAlg::RangeArray>::value];
    int dm_selected;
};

#endif

