#pragma once

#include "ui_selfiguredlg.h"
#include <QDialog>

class SelectFigureDlg : public QDialog, public Ui::FigureDialog
{
  Q_OBJECT

public:

  SelectFigureDlg(QWidget *parent = 0) : QDialog(parent)
  {
    setupUi(this);
    rbBishop_->setChecked(true);
  }

};