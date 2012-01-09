#include "setparamsdlg.h"
#include <QBoxLayout>
#include <QSettings>

SetParamsDlg::SetParamsDlg(QWidget * parent) :
  QDialog(parent)
{
  depthLabel_ = new QLabel(tr("Max depth (PLY)"), this);
  timeLabel_ = new QLabel(tr("Time per step (s)"), this);
  depthCB_ = new QComboBox(this);
  timeCB_ = new QComboBox(this);
  buttonCancel_ = new QPushButton(tr("Cancel"), this);
  buttonOK_ = new QPushButton(tr("OK"), this);

  QHBoxLayout * mainLayout = new QHBoxLayout();
  QVBoxLayout * leftLayout = new QVBoxLayout();
  QVBoxLayout * rightLayout = new QVBoxLayout();
  
  QHBoxLayout * depthLayout = new QHBoxLayout();
  QHBoxLayout * timeLayout = new QHBoxLayout();

  depthLayout->addWidget(depthLabel_);
  depthLayout->addWidget(depthCB_);

  timeLayout->addWidget(timeLabel_);
  timeLayout->addWidget(timeCB_);

  leftLayout->addLayout(depthLayout);
  leftLayout->addLayout(timeLayout);

  rightLayout->addWidget(buttonOK_);
  rightLayout->addWidget(buttonCancel_);
  rightLayout->addStretch();

  mainLayout->addLayout(leftLayout);
  mainLayout->addLayout(rightLayout);

  setLayout(mainLayout);

  setWindowTitle(tr("Settings"));
  setFixedSize(sizeHint());

  connect(buttonCancel_, SIGNAL(clicked()), this, SLOT(reject()));
  connect(buttonOK_, SIGNAL(clicked()), this, SLOT(accept()));

  int maxPly = 16;

  static int ttime_ [] = { 0, 1, 2, 3, 4, 5, 10, 15, 20, 30, 45, 60 };

  for (int ply = 2; ply <= maxPly; ++ply)
    depthCB_->insertItem(ply, QString("%1").arg(ply), ply);

  for (int i = 0; i < sizeof(ttime_)/sizeof(*ttime_); ++i)
  {
    int t = ttime_[i];
    QString tstr;
    if ( t > 0 )
      tstr = QString("%1").arg(t);
    else
      tstr = tr("-");
    timeCB_->insertItem(i, tstr, t);
  }

  QSettings settings(tr("Dimock"), tr("qchess"));
  int tm = settings.value(tr("step_time"), 2).toInt();
  int de = settings.value(tr("max_depth"), 16).toInt();

  int it = timeCB_->findData(tm);
  int id = depthCB_->findData(de);

  if ( it < 0 || it >= timeCB_->count() )
    it = 0;

  if ( id < 0 || id >= depthCB_->count() )
    id = 0;

  timeCB_->setCurrentIndex(it);
  depthCB_->setCurrentIndex(id);
}

int SetParamsDlg::getMaxDepth() const
{
  int i = depthCB_->currentIndex();
  QVariant q = depthCB_->itemData(i);
  if ( q != QVariant::Invalid )
    return q.toInt();
  return 16;
}

int SetParamsDlg::getStepTime() const
{
  int i = timeCB_->currentIndex();
  QVariant q = timeCB_->itemData(i);
  if ( q != QVariant::Invalid )
      return q.toInt();
  return 0;
}
