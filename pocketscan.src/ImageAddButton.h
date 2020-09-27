
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#ifndef __INCLUDED_POCKETSCAN_IMAGEADDBUTTON_H__
#define __INCLUDED_POCKETSCAN_IMAGEADDBUTTON_H__

#include <Project.h>

#include <QWidget>

class ImageAddButton : public QWidget {
    Q_OBJECT

  public:
    ImageAddButton(Project *p);

    virtual void handleProjectChanged(Listener *source) {}

  private slots:
    void onClick(void);

  private:
    Project *dm_project;
};

#endif
