#pragma once

#include <QDialog>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>

class SetParamsDlg : public QDialog
{
  Q_OBJECT

public:

  SetParamsDlg(QWidget * parent);

  int getMaxDepth() const;
  int getStepTime() const;

private:

  QLabel * depthLabel_, * timeLabel_;
  QComboBox * depthCB_, * timeCB_;
  QPushButton * buttonOK_, * buttonCancel_;
};
