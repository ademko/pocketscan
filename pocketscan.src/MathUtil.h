
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#ifndef __INCLUDED_POCKETSCAN_MATHUTIL_H__
#define __INCLUDED_POCKETSCAN_MATHUTIL_H__

#include <math.h>

#include <QPoint>

inline int sqr(int v) { return v*v; }

inline int pointDistance(const QPoint &p0, const QPoint &p1)
{
  return static_cast<int>(sqrt( static_cast<double>(sqr(p0.x() - p1.x()) + sqr(p0.y() - p1.y()) )));
}

/*inline double pointDistanceF(const QPointF &p0, const QPointF &p1)
{
  return sqrt( static_cast<double>(sqr(p0.x() - p1.x()) + sqr(p0.y() - p1.y()) ));
}*/

inline QPoint lineFraction(int num, int den, const QPoint &p0, const QPoint &p1)
{
  return QPoint(
      num * (p1.x() - p0.x()) / den + p0.x(),
      num * (p1.y() - p0.y()) / den + p0.y()
      );
}

inline QPointF lineFractionF(int num, int den, const QPointF &p0, const QPointF &p1)
{
  return QPointF(
      num * (p1.x() - p0.x()) / den + p0.x(),
      num * (p1.y() - p0.y()) / den + p0.y()
      );
}

#endif

