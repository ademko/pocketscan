
/*
 * Copyright (c) 2010    Aleksander B. Demko
 * This source code is distributed under the MIT license.
 * See the accompanying file LICENSE.MIT.txt for details.
 */

#include <ImageAddButton.h>

#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QDebug>

ImageAddButton::ImageAddButton(Project *p)
  : dm_project(p)
{
  QVBoxLayout *v = new QVBoxLayout;
  QPushButton *but = new QPushButton(QIcon(":/file_add.png"), "Add More Images...");

  connect(but, SIGNAL(clicked(bool)), this, SLOT(onClick()));

  v->addStretch();
  v->addWidget(but);
  v->addStretch();

  setLayout(v);
}

void ImageAddButton::onClick(void)
{
  QFileDialog dlg;

  QStringList filters;
  filters << "JPEG Image files (*.jpg *.jpeg)"
    << "Any files (*)";


  dlg.setWindowTitle("Add Images");
  dlg.setFileMode(QFileDialog::ExistingFiles);
  dlg.setAcceptMode(QFileDialog::AcceptOpen);
  dlg.setNameFilters(filters);

  if (dlg.exec() != QDialog::Accepted)
    return;

  dm_project->appendFiles(dlg.selectedFiles());
  dm_project->notifyChange(0);
}

