
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#ifndef __INCLUDED_POCKETSCAN_TABBAR_H__
#define __INCLUDED_POCKETSCAN_TABBAR_H__

#include <QLabel>
#include <QPushButton>
#include <QTabBar>

#include <Project.h>

class TabBar : public QWidget, public Listener
{
    Q_OBJECT

  public:
    TabBar(Project *p);
    virtual ~TabBar();

    virtual void handleProjectChanged(Listener *source);

  private slots:
    void onBut1(void);
    void onBut2(void);
    void onTabChange(int index);

  private:
    void initGui(void);
    void updateLabels(void);
    QString stepString(void);

  private:
    Project *dm_project;

    QTabBar *dm_tabber;

    QPushButton *dm_but1, *dm_but2;
    QLabel *dm_title, *dm_desc;
};


#endif

