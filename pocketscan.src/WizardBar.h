
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#ifndef __INCLUDED_POCKETSCAN_WIZARDBAR_H__
#define __INCLUDED_POCKETSCAN_WIZARDBAR_H__

#include <QPushButton>
#include <QLabel>

#include <Project.h>

/**
 * The wizard/druid bard at the bottom of the main display.
 * Used for advancing between the steps of the whole process.
 *
 * @author Aleksander Demko
 */ 
class WizardBar : public QWidget, public Listener
{
    Q_OBJECT

  public:
    WizardBar(Project *p);
    virtual ~WizardBar();

    virtual void handleProjectChanged(Listener *source) { updateLabels(); }

  private slots:
    void onPrevBut(void);
    void onNextBut(void);
    void onBut1(void);
    void onBut2(void);

  private:
    void initGui(void);
    void updateLabels(void);
    QString stepString(void);

  private:
    Project *dm_project;

    QPushButton *dm_prevbut, *dm_nextbut;
    QPushButton *dm_but1, *dm_but2;
    QLabel *dm_title, *dm_desc;
};

#endif

