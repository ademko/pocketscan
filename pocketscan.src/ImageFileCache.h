
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#ifndef __INCLUDED_IMAGEFILECACHE_H__
#define __INCLUDED_IMAGEFILECACHE_H__

#include <QImage>

#include <desktop/LoadCache.h>

QSize calcAspect(const QSize &current, const QSize &wantedFrame, bool growtofit);
void calcAspect(unsigned long C, unsigned long R, unsigned long WC, unsigned long WR, unsigned long &c, unsigned long &r, bool growtofit);

/// like calcAspect, but will only scale by integer ratios
/// is this even useful? perhaps power of 2 then?
QSize calcAspectEven(const QSize &current, const QSize &wantedFrame, bool growtofit);

class ImageFileCache;

/**
 * A image-reading cache, useful for the big image viewers. Also does rescaling
 * of the image as needed. The rescaled versions however are not cached.
 *
 * @author Aleksander Demko
 */ 
class ImageFileCache
{
  public:
    /// constructor
    ImageFileCache(int maxhold = 10);

    /**
     * Is the given fullfilename in the cache?
     *
     * @author Aleksander Demko
     */
    bool hasImage(const QString &fullfilename) const { return dm_cache.containsItem(fullfilename); }

    /**
     * Loads the given file as an image, from cache if possible.
     * This function never fails and never returns null - on error, an empty
     * image will be returned.
     *
     * @author Aleksander Demko
     */ 
    desktop::cache_ptr<QImage> getImage(const QString &fullfilename) { return dm_cache.getItem(fullfilename); }

    static QPixmap getPixmap(QImage &image, int windoww, int windowh, bool growtofit);

  private:
    class ImageLoader
    {
      public:
        // never returns null... failed loads will simply be empty images
        std::tr1::shared_ptr<QImage> operator()(const QString &fullfilename);
    };

    desktop::LoadCache<QImage, ImageLoader> dm_cache;
};

#endif

