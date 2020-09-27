
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#include <LevelEditor.h>

#include <QPaintEvent>
#include <QPainter>

//
// LevelEditor
//

LevelEditor::LevelEditor(void) {
    setWindowTitle("Level Editor");

    dm_selected = -1;

    dm_dirty = true;

    setMinimumSize(QSize(10, 2 * BORDER + TRI_HEIGHT + 20));
}

/*LevelEditor::LevelEditor(const Histogram &histo, const LevelAlg::MarkArray
&initialMarks) : dm_histogram(histo), dm_marks(initialMarks)
{
  assert(dm_histogram.countMax() > 0);

  setWindowTitle("Level Editor");

  dm_selected = -1;

  dm_dirty = true;
}*/

void LevelEditor::unsetHistoLevels(void) {
    dm_histogram = Histogram();
    dm_marks = LevelAlg::MarkArray();

    dm_selected = -1;

    dm_dirty = true;

    update();
}

void LevelEditor::setHisto(const Histogram &histo) {
    dm_histogram = histo;

    assert(dm_histogram.countMax() > 0);

    dm_dirty = true;

    update();
}

void LevelEditor::setLevels(const LevelAlg::MarkArray &initialMarks) {
    dm_marks = initialMarks;

    dm_selected = -1;

    opToPixels();

    update();
}

void LevelEditor::resizeEvent(QResizeEvent *event) { dm_dirty = true; }

void LevelEditor::paintEvent(QPaintEvent *event) {
    QPainter dc(this);

    dc.setBackground(Qt::white);

    dc.eraseRect(dc.window());

    if (dm_histogram.isNull())
        return;

    if (dm_dirty) {
        QSize s(dc.window().width() - 2 * BORDER,
                dc.device()->height() - 2 * BORDER - TRI_HEIGHT);

        if (s.width() <= 0 || s.height() <= 0)
            dm_pixmap = QPixmap();
        else {
            dm_pixmap = QPixmap(s);

            drawBars();
        }

        opToPixels();

        dm_dirty = false;
    }

    dc.drawPixmap(QRect(baseHisto(), dm_pixmap.size()), dm_pixmap);

    drawTri(dc);
}

void LevelEditor::mouseMoveEvent(QMouseEvent *event) {
    if ((event->buttons() & Qt::LeftButton) == 0 || dm_selected == -1)
        return;

    dm_points[dm_selected].rx() = event->pos().x();

    // commit
    pixelsToOp();
    // and get them back, incase theyve been changed
    opToPixels();
    // update the gui
    update();
    // fire the signal
    levelChanged();
}

void LevelEditor::mousePressEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton)
        return;

    if (dm_histogram.isNull())
        return;

    dm_selected = -1;
    int best = -1;
    int best_distance = 9999;
    // find the selected corner, if any
    for (int i = 0; i < std::tuple_size<LevelAlg::MarkArray>::value; ++i) {
        int this_distance = (event->pos() - dm_points[i]).manhattanLength();
        if (this_distance < 20 && this_distance < best_distance) {
            best_distance = this_distance;
            best = i;
        }
    }

    if (best != -1) {
        dm_selected = best;

        update();
    }
}

void LevelEditor::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton || dm_selected == -1)
        return;

    dm_selected = -1;

    // commit
    pixelsToOp();
    // and get them back, incase theyve been changed
    opToPixels();
    // update the gui
    update();
    // fire the signal
    levelChanged();
}

// we need this for the next two functions, otherwise
// they don't complement each other
static inline int roundedDiv(int num, int den) { return (num + den / 2) / den; }

void LevelEditor::opToPixels(void) {
    for (int t = 0; t < std::tuple_size<LevelAlg::MarkArray>::value; ++t) {
        dm_points[t] = QPoint(
            roundedDiv(dm_marks[t] * (sizeTri().width() - 1), LevelAlg::WHITE) +
                baseTri().x(),
            sizeTri().height() / 2 + baseTri().y());
    }
}

void LevelEditor::pixelsToOp(void) {
    LevelAlg::MarkArray newmarks;
    for (int t = 0; t < std::tuple_size<LevelAlg::MarkArray>::value; ++t) {
        int v = roundedDiv((dm_points[t].x() - baseTri().x()) * LevelAlg::WHITE,
                           (sizeTri().width() - 1));

        if (v < 0)
            v = 0;
        if (v > LevelAlg::WHITE)
            v = LevelAlg::WHITE;
        newmarks[t] = v;
    }
    if (newmarks[0] < newmarks[1] && newmarks[1] < newmarks[2])
        dm_marks = newmarks;
}

void LevelEditor::drawBars(void) {
    QPainter dc(&dm_pixmap);

    dc.setBackground(Qt::white);

    dc.eraseRect(dc.window());

    assert(!dm_histogram.isNull());
    assert(dm_histogram.countMax() > 0);

    QBrush b(Qt::black);

    int binwidth = dm_pixmap.width() / HistoAlg::SIZE + 1;
    if (binwidth < 1)
        binwidth = 1;

    for (int bin = 0; bin < HistoAlg::SIZE; ++bin) {
        int basex = bin * dm_pixmap.width() / HistoAlg::SIZE;
        dc.fillRect(basex,
                    (dm_histogram.countMax() - dm_histogram[bin]) *
                        (dm_pixmap.height() - 1) / dm_histogram.countMax(),
                    binwidth, dm_pixmap.height(), b);
    }
}

void LevelEditor::drawTri(QPainter &dc) {
    dc.setPen(Qt::black);

    for (int t = 0; t < std::tuple_size<LevelAlg::MarkArray>::value; ++t) {
        QBrush b(Qt::black);

        if (t == 1)
            b = QBrush(Qt::gray);
        if (t == 2)
            b = QBrush(Qt::white);
        QRect r(dm_points[t].x() - 4, dm_points[t].y() - 4, 9, 9);

        dc.fillRect(r, b);
        dc.drawRect(r);
    }
}

//
// RangeEditor
//

RangeEditor::RangeEditor(void) {
    setWindowTitle("Range Editor");

    dm_selected = -1;

    dm_range[0] = dm_range[1] = 0;

    dm_screenwidth = -1;

    dm_dirty = true;

    setMinimumSize(QSize(10, 2 * BORDER + TRI_HEIGHT));
}

void RangeEditor::unsetRange(void) {
    dm_range[0] = dm_range[1] = 0;

    dm_dirty = true;

    update();
}

void RangeEditor::setRange(const LevelAlg::RangeArray &r) {
    dm_range = r;

    dm_dirty = true;

    update();
}

void RangeEditor::resizeEvent(QResizeEvent *event) { dm_dirty = true; }

void RangeEditor::paintEvent(QPaintEvent *event) {
    QPainter dc(this);

    dc.setBackground(Qt::white);

    dc.eraseRect(dc.window());

    if (dm_range[0] == dm_range[1])
        return; // isNull, basically

    dm_screenwidth = dc.window().width();
    if (dm_screenwidth <= 0)
        return;

    if (dm_dirty) {
        opToPixels();
        dm_dirty = false;
    }

    drawTri(dc);
}

void RangeEditor::mouseMoveEvent(QMouseEvent *event) {
    if ((event->buttons() & Qt::LeftButton) == 0 || dm_selected == -1)
        return;

    dm_points[dm_selected].rx() = event->pos().x();

    // commit
    pixelsToOp();
    // and get them back, incase theyve been changed
    opToPixels();
    // update the gui
    update();
    // fire the signal
    levelChanged();
}

void RangeEditor::mousePressEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton)
        return;

    dm_selected = -1;
    int best = -1;
    int best_distance = 9999;
    // find the selected corner, if any
    for (int i = 0; i < std::tuple_size<LevelAlg::RangeArray>::value; ++i) {
        int this_distance = (event->pos() - dm_points[i]).manhattanLength();
        if (this_distance < 20 && this_distance < best_distance) {
            best_distance = this_distance;
            best = i;
        }
    }

    if (best != -1) {
        dm_selected = best;

        update();
    }
}

void RangeEditor::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton || dm_selected == -1)
        return;

    dm_selected = -1;

    // commit
    pixelsToOp();
    // and get them back, incase theyve been changed
    opToPixels();
    // update the gui
    update();
    // fire the signal
    levelChanged();
}

void RangeEditor::opToPixels(void) {
    for (int t = 0; t < std::tuple_size<LevelAlg::RangeArray>::value; ++t) {
        dm_points[t] = QPoint(
            roundedDiv(dm_range[t] * (sizeTri().width() - 1), LevelAlg::WHITE) +
                baseTri().x(),
            sizeTri().height() / 2 + baseTri().y());
    }
}

void RangeEditor::pixelsToOp(void) {
    LevelAlg::RangeArray newrange;
    for (int t = 0; t < std::tuple_size<LevelAlg::RangeArray>::value; ++t) {
        int v = roundedDiv((dm_points[t].x() - baseTri().x()) * LevelAlg::WHITE,
                           (sizeTri().width() - 1));

        if (v < 0)
            v = 0;
        if (v > LevelAlg::WHITE)
            v = LevelAlg::WHITE;
        newrange[t] = v;
    }
    if (newrange[0] < newrange[1])
        dm_range = newrange;
}

void RangeEditor::drawTri(QPainter &dc) {
    dc.setPen(Qt::black);

    for (int t = 0; t < std::tuple_size<LevelAlg::RangeArray>::value; ++t) {
        QBrush b(Qt::black);

        if (t == 1)
            b = QBrush(Qt::white);
        QRect r(dm_points[t].x() - 4, dm_points[t].y() - 4, 9, 9);

        dc.fillRect(r, b);
        dc.drawRect(r);
    }
}
