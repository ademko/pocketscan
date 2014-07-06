
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#ifndef __INCLUDED_FILENAMESERIES_H__
#define __INCLUDED_FILENAMESERIES_H__

#include <QString>

/**
 * Using a seed name, this can generate a list of files
 * names with numbers inserted into them.
 *
 * @author Aleksander Demko
 */ 
class FileNameSeries
{
  public:
    /// ctor
    FileNameSeries(const QString &seedFileName);

    /// returns the filename at the given index number (0 == first one)
    QString fileNameAt(int index);

  private:
    QString dm_seedFilename;

    QString dm_prefix, dm_suffix;
    int dm_width, dm_baseindex;
};

#endif

