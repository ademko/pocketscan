
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#include <AutoClip.h>

#include <QDebug>

#include <Project.h>    // for ClipOp.corners()

class SatThresFunc
{
  public:
    inline bool operator()(const QColor &c) {
      return c.saturation() < 30 &&
        (c.red() + c.green() + c.blue()) / 3 > 120;
    }
};

class AvgThresFunc
{
  public:
    inline bool operator()(const QColor &c) {
      return (c.red() + c.green() + c.blue()) / 3 > 100;
    }
};

int AutoClip::operator()(const QImage &input, ClipAlg::PointFArray &outputpoints, QImage *outputimg)
{
  int found = 0;

  if (input.width() < 10 || input.height() < 10)
    return 0;

  // do a histogram check
  /*{
    LevelOp outlevel;

    if (!Project::FileEntry::computeAutoLevelOp(Histogram(input), outlevel))
      return 0;
  }*/

  /*{
    GenericThresholdAlg<SatThresFunc> alg(input);

    alg.run();
    int pertrue = 100 * alg.trueCount() / alg.totalCount();

    if (pertrue > 20 && pertrue<100 && (found = findCorners(outputpoints, alg.output()))) {
      if (outputimg)
        *outputimg = alg.output();
      return found;
    }
  }*/

  {
    GenericThresholdAlg<AvgThresFunc> alg(input);

    alg.run();
    int pertrue = 100 * alg.trueCount() / alg.totalCount();

    if (pertrue > 50 && pertrue<100 && (found = findCorners(outputpoints, alg.output()))) {
      if (outputimg)
        *outputimg = alg.output();
      return found;
    }
  }

  if (outputimg)
    *outputimg = input;

  return 0;
}

int AutoClip::findCorners(ClipAlg::PointFArray &outputpoints, const QImage &img)
{
  int maxdepth = img.width() / 4;
  QRgb black = qRgb(0, 0, 0);
  int foundcount = 0;
  int deltax, deltay, butteryflyx, butteryflyy, topx, topy;

  for (int corner = 0; corner<4; ++corner) {
    switch (corner) {
      case 0:
        deltax = 1;
        deltay = 1;
        topx = 0;
        topy = 0;
        outputpoints[corner].rx() = 0;
        outputpoints[corner].ry() = 0;
        break;
      case 1:
        deltax = -1;
        deltay = 1;
        topx = img.width()-1;
        topy = 0;
        outputpoints[corner].rx() = 1;
        outputpoints[corner].ry() = 0;
        break;
      case 2:
        deltax = -1;
        deltay = -1;
        topx = img.width()-1;
        topy = img.height()-1;
        outputpoints[corner].rx() = 1;
        outputpoints[corner].ry() = 1;
        break;
      case 3:
        deltax = 1;
        deltay = -1;
        topx = 0;
        topy = img.height()-1;
        outputpoints[corner].rx() = 0;
        outputpoints[corner].ry() = 1;
        break;
    }

    butteryflyx = deltax;
    butteryflyy = -deltay;

    int foundx = -1, foundy = -1;
    for (int d=0; d<maxdepth; ++d) {
      int basex = topx + deltax*d;
      int basey = topy + deltay*d;

      for (int sub=0; sub <= d; ++sub) {
        if (img.pixel(basex + sub*butteryflyx, basey + sub*butteryflyy) != black) {
          foundx = basex + sub*butteryflyx;
          foundy = basey + sub*butteryflyy;
          // kill the loops
          d = maxdepth;
          sub = d+1;
        } else if (img.pixel(basex - sub*butteryflyx, basey - sub*butteryflyy) != black) {
          foundx = basex - sub*butteryflyx;
          foundy = basey - sub*butteryflyy;
          // kill the loops
          d = maxdepth;
          sub = d+1;
        }
      }//for sub
    }//for d
    if (foundx != -1 && foundy != -1) {
      foundcount++;
      outputpoints[corner].rx() = static_cast<double>(foundx) / (img.width() - 1);
      outputpoints[corner].ry() = static_cast<double>(foundy) / (img.height() - 1);
    }
  }//for corner


  //return foundcount;

  // rather than find the ones we found (which could be the default,
  // return the number of non default
  int actualfound = 0;
  ClipAlg::PointFArray def = ClipOp().corners();
  for (int i=0; i<def.size(); ++i)
    if (outputpoints[i] != def[i])
      actualfound++;
  return actualfound;
}

