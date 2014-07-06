
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#include <TileView.h>

#include <assert.h>

#include <QPainter>
#include <QFileInfo>
#include <QLabel>
#include <QToolBar>
#include <QApplication>
#include <QStyle>
#include <QAction>
#include <QMouseEvent>
#include <QSlider>
#include <QCheckBox>
#include <QDebug>

#include <DynamicSlot.h>

#include <LevelEditor.h>
#include <MathUtil.h>

static QImage imageShrink(const QImage &img)
{
  /*QSize s(img.size());
  QSize ideal(400,400);

  if (s.isEmpty())
    return img;

  while (s.width() > ideal.width() || s.height() > ideal.height()) {
    s.rwidth() /= 2;
    s.rheight() /= 2;
  }*/
  QSize s = calcAspectEven(img.size(), QSize(400,400), false);
  //qDebug() << img.size() << s;

  if (s != img.size())
    return img.scaled(s, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  else
    return img;
}

class TileView::Tile : public QWidget, public Listener
{
  public:
    Tile(Project *p);
    virtual ~Tile();

    virtual void handleProjectChanged(Listener *source);

    void setToolBar(ToolBar *tb) { dm_toolbar = tb; }

    // also acts as an update call
    void setCurrentIndex(int i);

    void transformChanged(void);
    void clipChanged(void);
    void levelChanged(void);

    virtual QSize sizeHint(void) const { return QSize(200,300); }

    QImage preLevelImage(void) const { return dm_prelevelimage; }

  protected:
    virtual void resizeEvent(QResizeEvent *event);
    virtual void paintEvent(QPaintEvent *event);

    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

  private:
    void initGui(void);
    void setDirty(void);
    void setJustLevelDirty(void);

    void cornersToClipOp(void);
    void clipOpToCorners(void);

    void calcLinePoints(void);
    void commitPoints(void);

    void drawClipBox(QPainter &dc);

  private:
    QPoint eventPointToImage(QPoint eventPt) {
      eventPt.rx() -= dm_drawbase.x();
      eventPt.ry() -= dm_drawbase.y();
      return eventPt;
    }

  private:
    Project *dm_project;
    ToolBar *dm_toolbar;

    bool dm_dirty;
    bool dm_justleveldirty;
    int dm_fileindex;

    int dm_mystep;

    QString dm_imgfilename;
    desktop::cache_ptr<QImage> dm_img;
    QPixmap dm_pixmap;

    QImage dm_prelevelimage;

    // box selection stuff

    // tied into dm_pixmap
    QPoint dm_drawbase;
    QPoint dm_corners[ClipOp::MAX_SIZE];
    QPoint dm_linepoints[ClipOp::MAX_SIZE];

    int dm_selected_corner;  // -1 if not selected
    int dm_selected_linepoint; // -1 if not selected
    bool dm_drawingrect;  // true if doing the initial rect drawing
};

class TileView::ToolBar : public QToolBar, public Listener
{
    const static int ACTION_EDIT_ROTATE_LEFT = 1;
    const static int ACTION_EDIT_ROTATE_RIGHT = 2;

    const static int ACTION_EDIT_MOVE_LEFT = 4;
    const static int ACTION_EDIT_MOVE_RIGHT = 5;
    const static int ACTION_EDIT_COPY = 6;
    const static int ACTION_EDIT_DELETE = 7;

    const static int ACTION_CLIP_CLEAR = 20;
    const static int ACTION_CLIP_AUTO = 21;

    const static int ACTION_LEVEL_AUTO = 31;

    const static int ACTION_CONTRAST_RESET = 40;

  public:
    ToolBar(Project *p, Tile *tile);
    virtual ~ToolBar();

    virtual void handleProjectChanged(Listener *source);

    void setCurrentIndex(int i);

    void clipChanged(void);
    void prelevelImageChanged(void);
    void levelChanged(void);

  private:
    void initGui(int step);

    void onButton(void);
    void onClipCheckBox(void);
    void onLevelCheckBox(void);
    void onLevelSlider(void);
    void onBCSlider(void);
    void onLevelEditor(void);

  private:
    Project *dm_project;
    Tile *dm_tile;

    int dm_mystep;

    int dm_fileindex;

    typedef std::vector<QAction*> ActionList;
    ActionList dm_actions;  // do we even need this... test for leaks

    QCheckBox *dm_clip_checkbox, *dm_level_checkbox;
    QSlider *dm_level_slider, *dm_b_slider, *dm_c_slider;
    LevelEditor *dm_level_editor;
    RangeEditor *dm_range_editor;

    DynamicSlot dm_buttonslot, dm_clip_checkboxslot, dm_level_checkboxslot,
      dm_level_sliderslot, dm_cb_sliderslot, dm_level_editor_slot;
};

class TileView::Widget : public QWidget
{
  public:
    Widget(Project *p);

    void setCurrentIndex(int i);

  public:
    QLabel *label;
    Tile *tile;
    ToolBar *bar;

  private:
    Project *dm_project;

    int dm_fileindex;
};

//
//
// TileView::Tile
//
//

TileView::Tile::Tile(Project *p)
  : dm_project(p), dm_toolbar(0)
{
  dm_dirty = true;
  dm_justleveldirty = false;
  dm_fileindex = -1;

  dm_mystep = dm_project->step();

  dm_selected_corner = -1;
  dm_selected_linepoint = -1;
  dm_drawingrect = false;

  dm_project->addListener(this);

  initGui();
}

TileView::Tile::~Tile()
{
  dm_project->removeListener(this);
}

void TileView::Tile::handleProjectChanged(Listener *source)
{
  //if (dm_project->step() == dm_mystep)
    //return;

  // yes, this is still being fleshed out
  bool is_dirty = StepList::isLevelStep(dm_project->step()) && !StepList::isLevelStep(dm_mystep);
  bool is_justleveldirty = StepList::isLevelStep(dm_project->step()) && StepList::isLevelStep(dm_mystep);

  dm_mystep = dm_project->step();

  if (is_dirty)
    setDirty();
  else if (is_justleveldirty)
    setJustLevelDirty();
  else
    update();
}

void TileView::Tile::setCurrentIndex(int i)
{
  assert(i>=0);

  dm_fileindex = i;

  if (dm_fileindex >= dm_project->files().size()) {
    dm_img = desktop::cache_ptr<QImage>();
    dm_imgfilename.clear();
  } else {
    const QString &fileName = dm_project->files()[dm_fileindex].fileName;

    if (fileName != dm_imgfilename) {
      dm_imgfilename = fileName;
      dm_img = dm_project->fileCache().getImage(dm_imgfilename);
    }
  }

  setDirty();
}

void TileView::Tile::transformChanged(void)
{
  setDirty();
}

void TileView::Tile::clipChanged(void)
{
  clipOpToCorners();
  update();
}

void TileView::Tile::levelChanged(void)
{
  setJustLevelDirty();
}

void TileView::Tile::resizeEvent(QResizeEvent *event)
{
  dm_dirty = true;
}

void TileView::Tile::paintEvent(QPaintEvent *event)
{
  QPainter dc(this);

  dc.setBackground(Qt::white);

  if (dm_dirty || dm_justleveldirty) {
    if (dm_img.get() == 0)
      dm_pixmap = QPixmap();
    else {
      Project::FileEntry &entry = dm_project->files()[dm_fileindex];

      QImage img;

      if (!dm_justleveldirty) {
        if (!entry.didExifCheck) {
          entry.didExifCheck = true;
          entry.transformOp = entry.computeAutoTransformOp();
        }
        img = entry.transformOp.apply(*dm_img);

        if (dm_mystep == StepList::CROP_STEP) {
          if (!entry.didClipCheck) {
            entry.didClipCheck = true;
            ClipOp newclip;
            if (entry.computeAutoClipOp(imageShrink(img), newclip)) {
              //img.save("/tmp/lastimg" + QString::number(dm_fileindex) + ".png");
              entry.usingClip = true;
              entry.clipOp = newclip;

              dm_toolbar->clipChanged();
            }
          }
        }

        bool didclip = false;
        if (dm_mystep >= StepList::LEVEL_STEP)
          if (entry.usingClip) {
            didclip = false;
            img = entry.clipOp.apply(img, QSize(dc.window().size()));
          }

        // prescale for the screen so the levelator doesnt have to work on the whole image
        // huge, optional performance boost
        if (!didclip) {
          //unsigned long c, r;
          //calcAspect(img.width(), img.height(), dc.window().width(), dc.window().height(), c, r, true);
          //img = img.scaled(QSize(c,r), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
          QSize s;
          s = calcAspect(img.size(), dc.window().size(), false);
          if (s != img.size())
            img = img.scaled(s, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }

        dm_prelevelimage = img;
      }

      // do leveling
      if (dm_mystep >= StepList::LEVEL_STEP) {
        // see if we need to do an automatic calc first
        if (!entry.didlevelCheck) {
          entry.didlevelCheck = true;
          Histogram h(dm_prelevelimage);
          LevelOp newop;
          if (entry.computeAutoLevelOp(h, newop)) {
            entry.usingLevel = true;
            entry.levelOp = newop;
          }

          dm_toolbar->levelChanged();
        }

        if (entry.usingLevel)
          img = entry.levelOp.apply(dm_prelevelimage);
        else
          img = dm_prelevelimage;
        dm_toolbar->prelevelImageChanged();
      }

      dm_pixmap = ImageFileCache::getPixmap(img, dc.window().width(), dc.window().height(), true);

      /*
      QSize scaled_size = entry.transformOp.applySize(dm_img->size());

      {
        unsigned long c, r;
        calcAspect(scaled_size.width(), scaled_size.height(), dc.window().width(), dc.window().height(), c, r, true);
        scaled_size = QSize(c, r);
      }
qDebug() << "scaled_size" << scaled_size;

      QImage img;

      // rotate the original image (can we optimize this?)
      img = entry.transformOp.apply(*dm_img);
      // scale it down for fast screen work
      img = img.scaled(scaled_size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
      if (dm_mystep >= StepList::LEVEL_STEP)
        img = entry.clipOp.apply(img);
      dm_prelevelimage = img;
      if (dm_mystep >= StepList::LEVEL_STEP)
        img = entry.levelOp.apply(img);

      dm_pixmap = ImageFileCache::getPixmap(img, dc.window().width(), dc.window().height(), true);*/

      clipOpToCorners();
    }

    dm_dirty = false;
    dm_justleveldirty = false;
  }

  dc.eraseRect(dc.window());

  if (dm_pixmap.isNull()) {
    if (dm_project->step() == StepList::ROTATE_STEP) {
      QString msg("Drag additional\nimage files here");

      int ptsize = 34;

      QFont font;
      QSize font_size;
      while (ptsize>8) {
        font.setPointSize(ptsize);
        font_size = QFontMetrics(font).size(Qt::TextSingleLine, msg);

        if (font_size.width() >= dc.window().width())
          ptsize -= 2;
        else
          break;
      }

      dc.setFont(font);

      //QPoint p((dc.device()->width() - s.width())/2, (dc.device()->height() - s.height())/2);

      QRect boundingRect = QFontMetrics(font).boundingRect(dc.window(), Qt::AlignCenter, msg);

      dc.fillRect(boundingRect, QColor(255,255,255,128));

      dc.setPen(QPen(QBrush(Qt::black), 2));
      //dc.drawText(boundingRect.topLeft(),  msg);
      dc.drawText(boundingRect, Qt::AlignCenter, msg);
    }
  } else {
    dm_drawbase.rx() = (dc.window().width()-dm_pixmap.width())/2;
    dm_drawbase.ry() = (dc.window().height()-dm_pixmap.height())/2;
    dc.drawPixmap(dm_drawbase.x(), dm_drawbase.y(), dm_pixmap.width(), dm_pixmap.height(), dm_pixmap);

    if (dm_mystep == StepList::CROP_STEP)
      drawClipBox(dc);
  }
}

void TileView::Tile::mouseMoveEvent(QMouseEvent *event)
{
  if (dm_fileindex >= dm_project->files().size())
    return;
  ClipOp &op = dm_project->files()[dm_fileindex].clipOp;

  if ((event->buttons() & Qt::LeftButton) == 0)
    return;

  if (dm_drawingrect) {
    dm_corners[2] = eventPointToImage(event->pos());
    dm_corners[1].rx() = dm_corners[2].x();
    dm_corners[3].ry() = dm_corners[2].y();

    calcLinePoints();

    update();
    return;
  }

  if (dm_selected_linepoint != -1) {
    QPoint delta = eventPointToImage(event->pos()) - dm_linepoints[dm_selected_linepoint];

    if (delta.isNull())
      return; // no need to do anything

    dm_corners[dm_selected_linepoint] += delta;
    dm_corners[(dm_selected_linepoint + 1) % ClipOp::MAX_SIZE] += delta;
    calcLinePoints();

    update();
    return;
  }

  if (dm_selected_corner == -1 || op.size() != ClipOp::MAX_SIZE)
    return;

  dm_corners[dm_selected_corner] = eventPointToImage(event->pos());
  calcLinePoints();

  update();
}

bool clickedPoint(const QPoint &p0, const QPoint &p1)
{
  return (p1 - p0).manhattanLength() < 40;
}

void TileView::Tile::mousePressEvent(QMouseEvent *event)
{
  if (dm_fileindex >= dm_project->files().size())
    return;
  ClipOp &op = dm_project->files()[dm_fileindex].clipOp;

  if (event->button() != Qt::LeftButton || dm_mystep != StepList::CROP_STEP)
    return;

  // if unchecked and they start drawing, reset the sqaure
  if (!dm_project->files()[dm_fileindex].usingClip)
    op.size() = 0;

  dm_project->files()[dm_fileindex].usingClip = true;
  dm_toolbar->clipChanged();

  if (op.size() < ClipOp::MAX_SIZE) {
    // add a point, possibly
    QPoint p = eventPointToImage(event->pos());

    dm_drawingrect = true;

    // draw the initial rect, all compressed
    dm_corners[0] = dm_corners[1] = dm_corners[2] = dm_corners[3] = p;
    op.size() = ClipOp::MAX_SIZE;
    commitPoints();

    update();
    return;
  }

  dm_selected_corner = -1;
  dm_selected_linepoint = -1;

  // find the selected corner, if any
  for (int i=0; i<op.size(); ++i)
    if (clickedPoint(eventPointToImage(event->pos()), dm_corners[i])) {
      dm_selected_corner = i;

      update();
      return;
    }

  // find the selected line point, if any
  for (int i=0; i<op.size(); ++i)
    if (clickedPoint(eventPointToImage(event->pos()), dm_linepoints[i])) {
      dm_selected_linepoint = i;

      update();
      return;
    }
}

void TileView::Tile::mouseReleaseEvent(QMouseEvent *event)
{
  if (dm_fileindex >= dm_project->files().size())
    return;
  if (event->button() != Qt::LeftButton || dm_mystep != StepList::CROP_STEP)
    return;

  //ClipOp &op = dm_project->files()[dm_fileindex].clipOp;

  if (dm_drawingrect) {
    dm_drawingrect = false;

    commitPoints();

    update();
    return;
  }

  // commit this new corner setting to the ClipOp
  if (dm_selected_corner != -1 || dm_selected_linepoint != -1) {
    dm_selected_corner = -1;
    dm_selected_linepoint = -1;

    commitPoints();

    update();
  }
}

void TileView::Tile::initGui(void)
{
  // nothing?
}

void TileView::Tile::setDirty(void)
{
  dm_dirty = true;
  dm_justleveldirty = false;
  update();
}

void TileView::Tile::setJustLevelDirty(void)
{
  if (!dm_dirty)
    dm_justleveldirty = true;
  update();
}

void TileView::Tile::cornersToClipOp(void)
{
  if (dm_fileindex >= dm_project->files().size())
    return;
  ClipOp &op = dm_project->files()[dm_fileindex].clipOp;

  for (int i=0; i<op.size(); ++i) {
    QPoint curp = dm_corners[i];

    QPointF fp(curp.x() / static_cast<float>(dm_pixmap.width()-1),
        curp.y() / static_cast<float>(dm_pixmap.height()-1));

    dm_project->files()[dm_fileindex].clipOp[i] = fp;

    dm_project->files()[dm_fileindex].clipOp.cap();
  }
}

void TileView::Tile::clipOpToCorners(void)
{
  if (dm_fileindex >= dm_project->files().size())
    return;
  ClipOp &op = dm_project->files()[dm_fileindex].clipOp;

  QSize s = dm_pixmap.size();

  for (int i=0; i<op.size(); ++i) {
    dm_corners[i].rx() = static_cast<int>( (s.width()-1) * dm_project->files()[dm_fileindex].clipOp[i].x() );
    dm_corners[i].ry() = static_cast<int>( (s.height()-1) * dm_project->files()[dm_fileindex].clipOp[i].y() );
  }

  calcLinePoints();
}

void TileView::Tile::calcLinePoints(void)
{
  if (dm_fileindex >= dm_project->files().size())
    return;
  ClipOp &op = dm_project->files()[dm_fileindex].clipOp;

  if (op.size() > 1)
    for (int i=0; i<op.size(); ++i)
      dm_linepoints[i] = lineFraction(1, 2, dm_corners[i], dm_corners[ (i+1) % ClipOp::MAX_SIZE ]);
}

void TileView::Tile::commitPoints(void)
{
  assert(dm_project->files()[dm_fileindex].clipOp.size() == ClipOp::MAX_SIZE);

  cornersToClipOp();
  dm_project->files()[dm_fileindex].clipOp.rearrange();

  clipOpToCorners();
}

void TileView::Tile::drawClipBox(QPainter &dc)
{
  if (dm_fileindex >= dm_project->files().size())
    return;
  ClipOp &op = dm_project->files()[dm_fileindex].clipOp;

  //if (!dm_project->files()[dm_fileindex].usingClip && op.size() > 0)
    //return;

  dc.save();

  if (!dm_project->files()[dm_fileindex].usingClip || op.size() == 0) {
    QString msg(
        "Click and drag to draw\n"
        "a clipping box\n"
        "(optional)");

    int ptsize = 34;

    QFont font;
    QSize font_size;
    while (ptsize>8) {
      font.setPointSize(ptsize);
      font_size = QFontMetrics(font).size(Qt::TextSingleLine, msg);

      if (font_size.width() >= dc.window().width())
        ptsize -= 2;
      else
        break;
    }

    dc.setFont(font);

    //QPoint p((dc.device()->width() - s.width())/2, (dc.device()->height() - s.height())/2);

    QRect boundingRect = QFontMetrics(font).boundingRect(dc.window(), Qt::AlignCenter, msg);

    dc.fillRect(boundingRect, QColor(255,255,255,128));

    dc.setPen(QPen(QBrush(Qt::black), 2));
    //dc.drawText(boundingRect.topLeft(),  msg);
    dc.drawText(boundingRect, Qt::AlignCenter, msg);
    dc.restore();
    return;
  }

  int cur;
  
  dc.setBrush(Qt::green);

  // draw lines
  if (op.size() > 1)
    for (cur=0; cur<op.size(); ++cur) {
      int next = (cur+1) % op.size();
      dc.setPen(QPen(QBrush(QColor(0, 170, 0)), 2));
      dc.drawLine(dm_drawbase.x() + dm_corners[cur].x(), dm_drawbase.y() + dm_corners[cur].y(),
          dm_drawbase.x() + dm_corners[next].x(), dm_drawbase.y() + dm_corners[next].y());

      dc.setPen(QPen(QBrush(Qt::green), 2));
      dc.drawEllipse(dm_drawbase.x() + dm_linepoints[cur].x() - 2, dm_drawbase.y() + dm_linepoints[cur].y() - 2, 5, 5);
    }

  // draw the points, overtop of the lines
  dc.setPen(QPen(QBrush(Qt::green), 2));
  for (cur=0; cur<op.size(); ++cur) {
    dc.drawEllipse(dm_drawbase.x() + dm_corners[cur].x() - 4, dm_drawbase.y() + dm_corners[cur].y() - 4, 7, 7);
  }

  dc.restore();
}

//
//
// TileView::ToolBar
//
//

TileView::ToolBar::ToolBar(Project *p, Tile *tile)
  : dm_project(p), dm_tile(tile),
    dm_buttonslot(this, &ToolBar::onButton),
    dm_clip_checkboxslot(this, &ToolBar::onClipCheckBox),
    dm_level_checkboxslot(this, &ToolBar::onLevelCheckBox),
    dm_level_sliderslot(this, &ToolBar::onLevelSlider),
    dm_cb_sliderslot(this, &ToolBar::onBCSlider),
    dm_level_editor_slot(this, &ToolBar::onLevelEditor)
{
  dm_mystep = dm_project->step();
  dm_fileindex = -1;
  dm_clip_checkbox = 0;
  dm_level_checkbox = 0;
  dm_level_slider = 0;

  setAutoFillBackground(true);

  // layout thing (in the future, make this smarter/deduced from runtime info
  setMinimumSize(QSize(0, 40));

  initGui(dm_mystep);

  dm_project->addListener(this);
}

TileView::ToolBar::~ToolBar()
{
  dm_project->removeListener(this);
}

void TileView::ToolBar::handleProjectChanged(Listener *source)
{
  levelChanged();

  if (dm_mystep == dm_project->step())
    return;

  dm_mystep = dm_project->step();
  initGui(dm_mystep);
}

void TileView::ToolBar::setCurrentIndex(int i)
{
  dm_fileindex = i;

  if (dm_project->step() == StepList::ROTATE_STEP && dm_actions.size() >= 4) {
    // this is such a hack, cuz im too lazy to make new data members
    dm_actions[2]->setEnabled(dm_fileindex>0);
    dm_actions[3]->setEnabled(dm_fileindex+1<dm_project->files().size());
  }

  clipChanged();
  levelChanged();
}

void TileView::ToolBar::clipChanged(void)
{
  if (dm_fileindex >= dm_project->files().size())
    return;

  if (dm_clip_checkbox)
    dm_clip_checkbox->setChecked(dm_project->files()[dm_fileindex].usingClip);
}

void TileView::ToolBar::prelevelImageChanged(void)
{
  if (!dm_level_editor)
    return;

  Histogram h(dm_tile->preLevelImage());
  if (h.countMax() > 0)
    dm_level_editor->setHisto(h);
}

void TileView::ToolBar::levelChanged(void)
{
  if (dm_fileindex >= dm_project->files().size()) {
    if (dm_level_editor)
      dm_level_editor->unsetHistoLevels();
    if (dm_range_editor)
      dm_range_editor->unsetRange();
    return;
  }

  assert(dm_tile);

  Project::FileEntry &entry = dm_project->files()[dm_fileindex];

  if (dm_level_slider && entry.levelOp.magicValue() != dm_level_slider->value()) {
    // this is to prevent setValue from triggered onLevelSlider()
    bool prev = dm_level_slider->blockSignals(true);
    dm_level_slider->setValue(entry.levelOp.magicValue());
    dm_level_slider->blockSignals(prev);
  }
  if (dm_b_slider && entry.levelOp.brightnessValue() != dm_b_slider->value()) {
    // this is to prevent setValue from triggered onLevelSlider()
    bool prev = dm_b_slider->blockSignals(true);
    dm_b_slider->setValue(entry.levelOp.brightnessValue());
    dm_b_slider->blockSignals(prev);
  }
  if (dm_c_slider && entry.levelOp.contrastValue() != dm_c_slider->value()) {
    // this is to prevent setValue from triggered onLevelSlider()
    bool prev = dm_c_slider->blockSignals(true);
    dm_c_slider->setValue(entry.levelOp.contrastValue());
    dm_c_slider->blockSignals(prev);
  }
  if (dm_level_checkbox && entry.usingLevel != dm_level_checkbox->isChecked())
    dm_level_checkbox->setChecked(entry.usingLevel);
  if (dm_level_editor)
    dm_level_editor->setLevels(dm_project->files()[dm_fileindex].levelOp.marks());
  if (dm_range_editor)
    dm_range_editor->setRange(dm_project->files()[dm_fileindex].levelOp.range());
}

void TileView::ToolBar::initGui(int step)
{
  clear();  // remove all actions from the toolbar
  dm_actions.clear();
  dm_clip_checkbox = 0;
  dm_level_checkbox = 0;
  dm_level_slider = 0;
  dm_b_slider = 0;
  dm_c_slider = 0;
  dm_level_editor = 0;
  dm_range_editor = 0;

  QAction *a;

  switch (step) {
    case StepList::ROTATE_STEP:
      // build the bar
      a = this->addAction(QIcon(":/rotate_left.png"), "Rotate image left");
      a->setProperty("action", ACTION_EDIT_ROTATE_LEFT);
      connect(a, SIGNAL(triggered(bool)), &dm_buttonslot, SLOT(trigger()));
      dm_actions.push_back(a);
      a = this->addAction(QIcon(":/rotate_right.png"), "Rotate image right");
      a->setProperty("action", ACTION_EDIT_ROTATE_RIGHT);
      connect(a, SIGNAL(triggered(bool)), &dm_buttonslot, SLOT(trigger()));
      dm_actions.push_back(a);
      a = this->addAction(QIcon(":/img_left.png"), "Move image left");
      a->setProperty("action", ACTION_EDIT_MOVE_LEFT);
      connect(a, SIGNAL(triggered(bool)), &dm_buttonslot, SLOT(trigger()));
      dm_actions.push_back(a);
      a = this->addAction(QIcon(":/img_right.png"), "Move image right");
      a->setProperty("action", ACTION_EDIT_MOVE_RIGHT);
      connect(a, SIGNAL(triggered(bool)), &dm_buttonslot, SLOT(trigger()));
      dm_actions.push_back(a);
      a = this->addAction(QIcon(":/img_dupe.png"), "Duplicate image");
      a->setProperty("action", ACTION_EDIT_COPY);
      connect(a, SIGNAL(triggered(bool)), &dm_buttonslot, SLOT(trigger()));
      dm_actions.push_back(a);
      a = this->addAction(QIcon(":/img_del.png"), "Remove image from book");
      a->setProperty("action", ACTION_EDIT_DELETE);
      connect(a, SIGNAL(triggered(bool)), &dm_buttonslot, SLOT(trigger()));
      dm_actions.push_back(a);
      break;
    case StepList::CROP_STEP:
      {
        dm_clip_checkbox = new QCheckBox("Do Clipping");
        connect(dm_clip_checkbox, SIGNAL(stateChanged(int)), &dm_clip_checkboxslot, SLOT(trigger()));
        dm_actions.push_back(addWidget(dm_clip_checkbox));

        clipChanged();
      }

      /* disabled for now... is this even needed?
      a = this->addAction(QIcon(":/crop.png"), "Clear Clipping Box");
      a->setProperty("action", ACTION_CLIP_CLEAR);
      connect(a, SIGNAL(triggered(bool)), &dm_buttonslot, SLOT(trigger()));
      dm_actions.push_back(a);*/

      //a = this->addAction(QIcon(":/levels.png"), "Auto Set Clipping Box");
      a = this->addAction(QIcon(":/crop.png"), "Auto Set Clipping Box");
      a->setProperty("action", ACTION_CLIP_AUTO);
      connect(a, SIGNAL(triggered(bool)), &dm_buttonslot, SLOT(trigger()));
      dm_actions.push_back(a);
      break;
    case StepList::LEVEL_STEP:
      {
        dm_level_checkbox = new QCheckBox("Use Levels:");
        connect(dm_level_checkbox, SIGNAL(stateChanged(int)), &dm_level_checkboxslot, SLOT(trigger()));
        dm_actions.push_back(addWidget(dm_level_checkbox));

        dm_level_slider = new QSlider(Qt::Horizontal);
        dm_level_slider->setRange(10, 245);
        dm_level_slider->setPageStep(30);
        //connect(dm_level_slider, SIGNAL(valueChanged(int)), &dm_level_sliderslot, SLOT(trigger()));
        connect(dm_level_slider, SIGNAL(valueChanged(int)), &dm_level_sliderslot, SLOT(trigger()));
        dm_actions.push_back(addWidget(dm_level_slider));

        levelChanged();
      }

      a = this->addAction(QIcon(":/levels.png"), "Auto Set Levels");
      a->setProperty("action", ACTION_LEVEL_AUTO);
      connect(a, SIGNAL(triggered(bool)), &dm_buttonslot, SLOT(trigger()));
      dm_actions.push_back(a);
      break;
    case StepList::CONTRAST_STEP:
      {
        dm_level_checkbox = new QCheckBox("Use B:");
        connect(dm_level_checkbox, SIGNAL(stateChanged(int)), &dm_level_checkboxslot, SLOT(trigger()));
        dm_actions.push_back(addWidget(dm_level_checkbox));

        //dm_actions.push_back(addWidget(new QLabel("Brightness:")));

        dm_b_slider = new QSlider(Qt::Horizontal);
        dm_b_slider->setRange(-127, 127);
        dm_b_slider->setPageStep(30);
        connect(dm_b_slider, SIGNAL(valueChanged(int)), &dm_cb_sliderslot, SLOT(trigger()));
        connect(dm_b_slider, SIGNAL(valueChanged(int)), &dm_cb_sliderslot, SLOT(trigger()));
        dm_actions.push_back(addWidget(dm_b_slider));

        //dm_actions.push_back(addWidget(new QLabel("Contrast:")));
        dm_actions.push_back(addWidget(new QLabel("C:")));

        dm_c_slider = new QSlider(Qt::Horizontal);
        dm_c_slider->setRange(-127, 127);
        dm_c_slider->setPageStep(30);
        connect(dm_c_slider, SIGNAL(valueChanged(int)), &dm_cb_sliderslot, SLOT(trigger()));
        connect(dm_c_slider, SIGNAL(valueChanged(int)), &dm_cb_sliderslot, SLOT(trigger()));
        dm_actions.push_back(addWidget(dm_c_slider));

        levelChanged();
      }

      a = this->addAction(QIcon(":/levels.png"), "Reset Brightness and Contrast");
      a->setProperty("action", ACTION_CONTRAST_RESET);
      connect(a, SIGNAL(triggered(bool)), &dm_buttonslot, SLOT(trigger()));
      dm_actions.push_back(a);
      break;
    case StepList::HISTO_STEP:
      {
        QSizePolicy pol;

        dm_level_editor = new LevelEditor;
        dm_range_editor = new RangeEditor;

        connect(dm_level_editor, SIGNAL(levelChanged()), &dm_level_editor_slot, SLOT(trigger()));
        connect(dm_range_editor, SIGNAL(levelChanged()), &dm_level_editor_slot, SLOT(trigger()));

        dm_level_editor->setMinimumSize(QSize(10,100));

        //pol = dm_level_editor->sizePolicy();
        //pol.setHorizontalStretch(1);
        //dm_level_editor->setSizePolicy(pol);

        //dm_range_editor->setMinimumSize(QSize(10,40));

        //pol = dm_range_editor->sizePolicy();
        //pol.setHorizontalStretch(1);
        //dm_range_editor->setSizePolicy(pol);

        QWidget *w = new QWidget;
        QVBoxLayout *vbox = new QVBoxLayout;

        vbox->addWidget(dm_level_editor);
        vbox->addWidget(dm_range_editor);
        vbox->setSpacing(0);
        w->setLayout(vbox);

        pol = w->sizePolicy();
        pol.setHorizontalStretch(1);
        w->setSizePolicy(pol);

        dm_actions.push_back(addWidget(w));

        levelChanged();
      }
      break;
  }//switch
}

void TileView::ToolBar::onButton(void)
{
  int cmd = dm_buttonslot.sender()->property("action").toInt();

  if (dm_fileindex >= dm_project->files().size())
    return;

  Project::FileEntry &entry = dm_project->files()[dm_fileindex];

  if (cmd == ACTION_EDIT_ROTATE_LEFT) {
    entry.transformOp.rotateLeft();
    dm_tile->transformChanged();
    return;
  }
  if (cmd == ACTION_EDIT_ROTATE_RIGHT) {
    entry.transformOp.rotateRight();
    dm_tile->transformChanged();
    return;
  }
  if (cmd == ACTION_EDIT_MOVE_LEFT && dm_fileindex>0) {
    Project::FileEntry tmp = entry;
    Project::FileEntry &other = dm_project->files()[dm_fileindex-1];

    entry = other;
    other = tmp;

    dm_project->notifyChange(0);
  }
  if (cmd == ACTION_EDIT_MOVE_RIGHT && dm_fileindex+1<dm_project->files().size()) {
    Project::FileEntry tmp = entry;
    Project::FileEntry &other = dm_project->files()[dm_fileindex+1];

    entry = other;
    other = tmp;

    dm_project->notifyChange(0);
  }
  if (cmd == ACTION_EDIT_COPY) {
    Project::FileEntry copy(entry);
    dm_project->files().insert(dm_project->files().begin() + dm_fileindex, copy);

    dm_project->notifyChange(0);
  }
  if (cmd == ACTION_EDIT_DELETE) {
    dm_project->files().erase(dm_project->files().begin() + dm_fileindex);

    dm_project->notifyChange(0);
  }
  if (cmd == ACTION_CLIP_CLEAR) {
    entry.usingClip = false;
    entry.clipOp.size() = 0;
    clipChanged();
    dm_tile->clipChanged();
  }
  if (cmd == ACTION_CLIP_AUTO) {
    ClipOp newclip;
    if (entry.computeAutoClipOp(dm_tile->preLevelImage(), newclip)) {
      entry.usingClip = true;
      entry.clipOp = newclip;

      clipChanged();
      dm_tile->clipChanged();
    }
  }
  if (cmd == ACTION_LEVEL_AUTO) {
    if (!entry.didlevelCheck)
      entry.didlevelCheck = true;

    LevelOp newop;
    Histogram h(dm_tile->preLevelImage());
    entry.computeAutoLevelOp(h, newop);

    entry.usingLevel = true;
    entry.levelOp = newop;

    levelChanged();
    dm_tile->levelChanged();
  }
  if (cmd == ACTION_CONTRAST_RESET) {
    entry.usingLevel = false;
    entry.levelOp.setBCValue(0, 0);

    levelChanged();
    dm_tile->levelChanged();
  }
}

void TileView::ToolBar::onClipCheckBox(void)
{
  if (dm_fileindex >= dm_project->files().size())
    return;
  dm_project->files()[dm_fileindex].usingClip = dm_clip_checkbox->isChecked();
  dm_tile->clipChanged();
}

void TileView::ToolBar::onLevelCheckBox(void)
{
  if (dm_fileindex >= dm_project->files().size())
    return;

  Project::FileEntry &entry = dm_project->files()[dm_fileindex];

  entry.usingLevel = dm_level_checkbox->isChecked();
  dm_tile->levelChanged();
}

void TileView::ToolBar::onLevelSlider(void)
{
  if (dm_fileindex >= dm_project->files().size())
    return;

  Project::FileEntry &entry = dm_project->files()[dm_fileindex];

  assert(dm_level_slider);

  entry.usingLevel = true;
  entry.levelOp.setMagicValue(dm_level_slider->value());

  dm_level_checkbox->setChecked(true);
  dm_tile->levelChanged();
}

void TileView::ToolBar::onBCSlider(void)
{
  if (dm_fileindex >= dm_project->files().size())
    return;

  Project::FileEntry &entry = dm_project->files()[dm_fileindex];

  assert(dm_b_slider && dm_c_slider);

  entry.usingLevel = true;
  entry.levelOp.setBCValue(dm_b_slider->value(), dm_c_slider->value());

  dm_level_checkbox->setChecked(true);
  dm_tile->levelChanged();
}

void TileView::ToolBar::onLevelEditor(void)
{
  if (dm_fileindex >= dm_project->files().size())
    return;

  Project::FileEntry &entry = dm_project->files()[dm_fileindex];

  entry.usingLevel = true;
  entry.levelOp.marks() = dm_level_editor->marks();
  entry.levelOp.range() = dm_range_editor->range();

  dm_tile->levelChanged();
}

//
//
// TileView::Widget
//
//

TileView::Widget::Widget(Project *p)
  : dm_project(p)
{
  dm_fileindex = -1;

  label = new QLabel("title");
  tile = new Tile(p);
  bar = new ToolBar(p, tile);
  tile->setToolBar(bar);

  QFont f = label->font();
  f.setPointSize(f.pointSize()*2);
  label->setFont(f);
  label->setAlignment(Qt::AlignHCenter);

  QVBoxLayout *h = new QVBoxLayout;

  h->addWidget(label);
  h->setStretchFactor(label, 0);
  h->addWidget(tile);
  h->setStretchFactor(tile, 1);
  h->addWidget(bar);
  h->setStretchFactor(bar, 0);

  setLayout(h);
}

void TileView::Widget::setCurrentIndex(int i)
{
  dm_fileindex = i;

  tile->setCurrentIndex(i);
  bar->setCurrentIndex(i);

  bool valid = i < dm_project->files().size();

  if (!valid) {
    label->setText("");
    return;
  }

  /*label->setText(QFileInfo(dm_project->files()[i].fileName).fileName()
      + " (" + QString::number(i+1) + "/"
      + QString::number(dm_project->files().size()) + ")");*/
  QString lab = "#" + QString::number(i+1) + " "
      + QFileInfo(dm_project->files()[i].fileName).fileName();

  int copyid = dm_project->isDuplicate(i);

  if (copyid>=0)
    lab += " (Copy " + QString::number(copyid+1) + ")";

  label->setText(lab);
}

//
//
// TileView
//
//

TileView::TileView(Project *p)
  : dm_project(p)
{
  assert(p);

  dm_baseindex = 0;
  dm_project->addListener(this);

  initGui();
}

TileView::~TileView()
{
  dm_project->removeListener(this);
}

void TileView::handleProjectChanged(Listener *source)
{
  for (int x=0; x<dm_widgets.size(); ++x)
    dm_widgets[x]->setCurrentIndex(dm_baseindex + x);
}

void TileView::setBaseIndex(int newbase)
{
  dm_baseindex = newbase;

  for (int x=0; x<dm_widgets.size(); ++x)
    dm_widgets[x]->setCurrentIndex(dm_baseindex + x);
}

void TileView::initGui(void)
{
  dm_hbox = new QHBoxLayout;

  setTileSize(3);

  setLayout(dm_hbox);
}

void TileView::setTileSize(int s)
{
  assert(s >= dm_widgets.size());

  if (s == dm_widgets.size())
    return;

  while (dm_widgets.size() < s) {
    Widget *w = new Widget(dm_project);
    int newindex = dm_widgets.size();

    dm_widgets.push_back(w);
    dm_hbox->addWidget(w, 1);

    w->setCurrentIndex(dm_baseindex + newindex);
  }
}

//
//
// TileScroller
//
//


TileScroller::TileScroller(Project *p, TileView *tv)
  : QScrollBar(Qt::Horizontal), dm_project(p), dm_tv(tv)
{
  dm_project->addListener(this);

  connect(this, SIGNAL(valueChanged(int)), this, SLOT(onValueChanged(int)));

  handleProjectChanged(0);
}

TileScroller::~TileScroller()
{
  dm_project->removeListener(this);
}

void TileScroller::handleProjectChanged(Listener *source)
{
  int pagesize = 3;
  int newmax = static_cast<int>(dm_project->files().size())-pagesize;

  if (newmax > 0)
    setRange(0, newmax);
  else
    setRange(0, 0);

  setValue(dm_tv->baseIndex());

  setSingleStep(1);
  setPageStep(pagesize);
}

void TileScroller::onValueChanged(int newvalue)
{
  dm_tv->setBaseIndex(newvalue);
}

