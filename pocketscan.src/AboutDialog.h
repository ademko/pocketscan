
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#ifndef __INCLUDED_POCKETSCAN_ABOUTDIALOG_H__
#define __INCLUDED_POCKETSCAN_ABOUTDIALOG_H__

#include <QDialog>
#include <QVBoxLayout>
#include <QTabWidget>

class AboutDialog;

/**
 * A handy about box builder
 *
 * @author Aleksander Demko
 */ 
class AboutDialog : public QDialog
{
  public:
    /// constructor
    AboutDialog(QWidget *parent, const QString &appname);

    void addPixmap(const QPixmap &px);

    /**
     * If title starts with :, it will be loaded as a resource image
     * instead.
     *
     * Either/or tagline and copyyear may be "".
     *
     * @author Aleksander Demko
     */ 
    void addTitle(const QString &title,
        const QString &tagline = "",
        const QString &copyyear = "", const QString &copywho = "");

    //addTitle(QString &

    void addContact(const QString &contact, const QString &email = "", const QString &web = "");
    void addVersion(const QString &version, const QString &flags = "", bool showqtversion = true, bool showstockflags = true, bool showbuildtime = true);
    void addParagraph(const QString &title, const QString &value);

    void addDisclaimer(void);
    void addCopyright(const QString &year, const QString &bywhom);

    void addTab(const QString &tabname);

  private:
    void appendWidget(QWidget *w, Qt::Alignment a = 0);

  private:
    QString dm_appname;

    QVBoxLayout *dm_mainlay, *dm_appendlay;
    QTabWidget *dm_tabber;
    int dm_nexty;
};

#endif

