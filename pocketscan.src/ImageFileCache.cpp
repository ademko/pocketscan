
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#include <ImageFileCache.h>

#include <QDebug>
#include <QPixmap>

QSize calcAspect(const QSize &current, const QSize &wantedFrame,
                 bool growtofit) {
    unsigned long C, R, WC, WR, c, r;

    C = current.width();
    R = current.height();
    WC = wantedFrame.width();
    WR = wantedFrame.height();

    calcAspect(C, R, WC, WR, c, r, growtofit);

    return QSize(c, r);
}

void calcAspect(unsigned long C, unsigned long R, unsigned long WC,
                unsigned long WR, unsigned long &c, unsigned long &r,
                bool growtofit) {
    if (!growtofit && R <= WR && C <= WC) {
        r = R;
        c = C;
        return;
    }
    double scr, scc;

    scr = static_cast<double>(WR) / R;
    scc = static_cast<double>(WC) / C;
    // assume scc is the smaller
    if (scr < scc)
        scc = scr; // its not?

    r = static_cast<unsigned long>(scc * R);
    c = static_cast<unsigned long>(scc * C);
}

QSize calcAspectEven(const QSize &current, const QSize &wantedFrame,
                     bool growtofit) {
    QSize ret(current);

    if (!growtofit && ret.width() < wantedFrame.width() &&
        ret.height() < wantedFrame.height())
        return ret;

    double scr, scc;

    scr = static_cast<double>(wantedFrame.height()) / ret.height();
    scc = static_cast<double>(wantedFrame.width()) / ret.width();
    // assume scc is the smaller
    if (scr < scc)
        scc = scr; // its not?

    if (growtofit && scc > 1) {
        assert(false); // this code needs to be tested for off-by-1 etc
        int mult = static_cast<int>(scc);

        if (scc - mult > 0)
            mult += 1; // add 1 for the remainder

        if (mult > 1) {
            ret.rwidth() *= mult;
            ret.rheight() *= mult;
        }
        return ret;
    }
    if (scc > 0) {
        int divi = static_cast<int>(1 / scc);

        if (1 / scc - divi > 0)
            divi++;

        // qDebug() << "divi" << divi;
        if (divi > 1) {
            ret.rwidth() /= divi;
            ret.rheight() /= divi;
        }
        return ret;
    }

    return ret;
}

//
//
// ImageFileCache
//
//

ImageFileCache::ImageFileCache(int maxhold) : dm_cache(maxhold) {}

QPixmap ImageFileCache::getPixmap(QImage &image, int windoww, int windowh,
                                  bool growtofit) {
    unsigned long final_w, final_h;

    calcAspect(image.width(), image.height(), windoww, windowh, final_w,
               final_h, growtofit);

    QImage scaled_image =
        image.scaled(QSize(final_w, final_h), Qt::IgnoreAspectRatio,
                     Qt::SmoothTransformation);
    return QPixmap::fromImage(scaled_image);
}

std::shared_ptr<QImage>
ImageFileCache::ImageLoader::operator()(const QString &fullfilename) {
    std::shared_ptr<QImage> i(new QImage);
    bool ok = i->load(fullfilename);

    /*
    if (ok)
      *i = hydra::rotateImageExif(hydra::detectExifRotate(fullfilename), *i);
    else
      *i = QImage();
    */

    if (!ok)
        *i = QImage();

    return i;
}
