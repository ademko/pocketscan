
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#include <FileNameSeries.h>

#include <QDebug>

FileNameSeries::FileNameSeries(const QString &seedFileName)
    : dm_seedFilename(seedFileName) {
    // try to find where, if any, the numeric mask is in the file
    int state = 0;
    int i = dm_seedFilename.size() - 1;
    int extdot = -1;
    int numdigits = 0;
    int firstdigit = -1;

    for (; i >= 0; --i) {
        const QChar c = dm_seedFilename[i];

        switch (state) {
        case 0: // looking for the first . past the extension
            if (c == '.') {
                extdot = i;
                state = 1;
            }
            break;
        case 1: // find the first non digit
            if (c.isDigit())
                numdigits++;
            else {
                if (c.isLetter())
                    state = -1; // i dont want a stream of digits prefixed by a
                                // letter, as thats probably part of the
                                // intended filename
                else {
                    state = 2; // done, found a good stream
                    firstdigit = i + 1;
                }
            }
            break;
        }
    }
    if (state == 2 && numdigits > 0) {
        dm_width = numdigits;
        dm_baseindex = dm_seedFilename.mid(firstdigit, numdigits).toInt();
        dm_prefix = dm_seedFilename.left(firstdigit);
        dm_suffix = dm_seedFilename.mid(extdot);
    } else if (state == 0) { // we never even found the file extension!
        dm_width = 4;
        dm_baseindex = 1;
        dm_prefix = dm_seedFilename + '.';
        dm_suffix = "";
    } else { // file extension, but no number given
        dm_width = 4;
        dm_baseindex = 1;
        dm_prefix = dm_seedFilename.left(extdot) + '.';
        dm_suffix = dm_seedFilename.mid(extdot);
    }

    // qDebug() << dm_prefix << dm_suffix << dm_width << dm_baseindex;
}

QString FileNameSeries::fileNameAt(int index) {
    if (index == 0)
        return dm_seedFilename;
    return dm_prefix +
           QString::number(index + dm_baseindex).rightJustified(dm_width, '0') +
           dm_suffix;
}

/*
  {FileNameSeries s1("test.jpg");
  qDebug() << s1.fileNameAt(0) << s1.fileNameAt(1) << s1.fileNameAt(2);}
  {FileNameSeries s1("noext");
  qDebug() << s1.fileNameAt(0) << s1.fileNameAt(1) << s1.fileNameAt(2);}
  {FileNameSeries s1("withnumbers000.jpg");
  qDebug() << s1.fileNameAt(0) << s1.fileNameAt(1) << s1.fileNameAt(2);}
  {FileNameSeries s1("withnumbers.00.jpg");
  qDebug() << s1.fileNameAt(0) << s1.fileNameAt(1) << s1.fileNameAt(2);}
*/
