
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#ifndef __INCLUDED_POCKETSCAN_TILEVIEW_H__
#define __INCLUDED_POCKETSCAN_TILEVIEW_H__

#include <vector>

#include <QHBoxLayout>
#include <QScrollBar>
#include <QWidget>

#include <Project.h>

/**
 * The collection of tile widgets.
 * Doesn't include the add button or the scroll bar.
 *
 * @author Aleksander Demko
 */
class TileView : public QWidget, public Listener {
    Q_OBJECT

  public:
    TileView(Project *p);
    virtual ~TileView();

    virtual void handleProjectChanged(Listener *source);

    int baseIndex(void) const { return dm_baseindex; }

    void setBaseIndex(int newbase);

  private:
    void initGui(void);

    void setTileSize(int s);

  private:
    class Tile;
    class ToolBar; // still short on names :)
    class Widget;  // yeah, im short on names here :)

    typedef std::vector<Widget *> WidgetList;

    QHBoxLayout *dm_hbox;
    WidgetList dm_widgets;

    int dm_baseindex;

    Project *dm_project;
};

/**
 * Presents a scroll bar that the user can use to change the current image.
 *
 * @author Aleksander Demko
 */
class TileScroller : public QScrollBar, public Listener {
    Q_OBJECT

  public:
    TileScroller(Project *p, TileView *tv);
    virtual ~TileScroller();

    virtual void handleProjectChanged(Listener *source);

  private slots:
    void onValueChanged(int newvalue);

  private:
    Project *dm_project;
    TileView *dm_tv;
};

#endif
