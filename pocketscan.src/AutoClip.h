
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#ifndef __INCLUDED_AUTOCLIP_H__
#define __INCLUDED_AUTOCLIP_H__

#include <QImage>

#include <ImageAlg.h>

class AutoClip {
  public:
    /**
     * Will return clipping params?
     *
     * Returns the number of corners found.
     *
     * @author Aleksander Demko
     */
    int operator()(const QImage &input, ClipAlg::PointFArray &outputpoints,
                   QImage *outputimg = 0);

  private:
    static int findCorners(ClipAlg::PointFArray &outputpoints,
                           const QImage &img);
};

#endif
