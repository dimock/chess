#include "ChessWidget.h"
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QTime>
#include <QApplication>
#include <QMenuBar>
#include <QStatusBar>
#include <QTimer>
#include <QMessageBox>
#include <QSettings>
#include "setparamsdlg.h"


ChessAlgThread::ChessAlgThread(ChessWidget * widget) :
  widget_(widget)
{
}

void ChessAlgThread::run()
{
  if ( !widget_ )
    return;

  widget_->findMove();
}

//////////////////////////////////////////////////////////////////////////
ChessWidget::ChessWidget(QWidget * parent) :
  QMainWindow(parent), upleft_(30, 50), full_t_(0), depth_(0), bs_count_(0), moves_avg_base_(0), depth_avg_(0), movesCount_(0),
  moves_base_(0), dt_(0),
  thread_(this), goingToClose_(false), changed_(false), autoPlay_(false), useTimer_(true), computerAnswers_(true), timelimit_(1000),
  depthMax_(2), enableBook_(true)
{
  QSettings settings(tr("Dimock"), tr("qchess"));
  timelimit_ = settings.value(tr("step_time"), 1).toInt()*1000;
  depthMax_ = settings.value(tr("max_depth"), 16).toInt();

  setFixedSize(450, 500);
  pv_str_[0] = 0;
  setAttribute(Qt::WA_DeleteOnClose);
  cpos_.setUpLeft(upleft_);

  createMenu();

  onNew();

  connect(&thread_, SIGNAL(finished()), this, SLOT(onMoveFound()));
}

ChessWidget::~ChessWidget()
{
}

void ChessWidget::createMenu()
{
  QMenu * gameMenu = menuBar()->addMenu(tr("&Game"));

  onNewAction_ = new QAction(tr("&New"), this);
  onNewAction_->setStatusTip(tr("Start new game"));

  onLoadAction_ = new QAction(tr("&Load"), this);
  onLoadAction_->setStatusTip(tr("Load previously saved game"));

  onSaveAction_ = new QAction(tr("&Save"), this);
  onSaveAction_->setStatusTip(tr("Save current game"));

  onPrevAction_ = new QAction(tr("&Undo move"), this);
  onPrevAction_->setStatusTip(tr("Undo last move. (only step of one color will be undone)"));

  onNextAction_ = new QAction(tr("&Redo move"), this);
  onNextAction_->setStatusTip(tr("Restore undone move"));

  onGoAction_ = new QAction(tr("&Go"), this);
  onGoAction_->setStatusTip(tr("Lets program make move"));

  onTurnBoardAction_ = new QAction(tr("&Turn board"), this);
  onTurnBoardAction_->setStatusTip(tr("Turn board to play another color"));
  onTurnBoardAction_->setCheckable(true);
  onTurnBoardAction_->setChecked(false);

  onSettingsAction_ = new QAction(tr("Settin&gs"), this);
  onSettingsAction_->setStatusTip(tr("Change game settings"));

  gameMenu->addAction(onNewAction_);
  gameMenu->addAction(onLoadAction_);
  gameMenu->addAction(onSaveAction_);
  gameMenu->addAction(onPrevAction_);
  gameMenu->addAction(onNextAction_);
  gameMenu->addAction(onGoAction_);
  gameMenu->addAction(onTurnBoardAction_);
  gameMenu->addSeparator();
  gameMenu->addAction(onSettingsAction_);

  connect(onNewAction_, SIGNAL(triggered()), this, SLOT(onNew()));
  connect(onLoadAction_, SIGNAL(triggered()), this, SLOT(onLoad()));
  connect(onSaveAction_, SIGNAL(triggered()), this, SLOT(onSave()));
  connect(onPrevAction_, SIGNAL(triggered()), this, SLOT(onPrev()));
  connect(onNextAction_, SIGNAL(triggered()), this, SLOT(onNext()));
  connect(onGoAction_, SIGNAL(triggered()), this, SLOT(onGo()));
  connect(onTurnBoardAction_, SIGNAL(toggled(bool)), this, SLOT(onTurnBoard(bool)));
  connect(onSettingsAction_, SIGNAL(triggered()), this, SLOT(onSettings()));
}

void ChessWidget::enableActions(bool on)
{
  onNewAction_->setEnabled(on);
  onLoadAction_->setEnabled(on);
  onSaveAction_->setEnabled(on);
  onPrevAction_->setEnabled(on);
  onNextAction_->setEnabled(on);
  onGoAction_->setEnabled(on);
  onTurnBoardAction_->setEnabled(on);
}

void ChessWidget::onSettings()
{
  SetParamsDlg dlg(this);
  if ( !dlg.exec() )
    return;

  QSettings settings(tr("Dimock"), tr("qchess"));
  settings.setValue(tr("step_time"), dlg.getStepTime());
  settings.setValue(tr("max_depth"), dlg.getMaxDepth());

  depthMax_ = dlg.getMaxDepth();
  timelimit_ = dlg.getStepTime()*1000;

  cpos_.setMaxDepth(depthMax_);
  cpos_.setTimeLimit(timelimit_);
}

void ChessWidget::onNew()
{
  if ( !okToReset() )
    return;

  if ( !cpos_.initialize(enableBook_, depthMax_) )
    return;

  cpos_.setTimeLimit(timelimit_);

  dt_ = 0;
  full_t_ = 0;
  depth_ = 0;
  bs_count_ = 0;
  moves_avg_base_ = 0;
  depth_avg_ = 0;
  movesCount_ = 0;
  changed_ = false;
  moves_base_ = 0;
  pv_str_[0] = 0;

  update();
}

void ChessWidget::onLoad()
{
  if ( !okToReset() )
    return;

  cpos_.load();
  movesCount_ = cpos_.movesCount();
  update();
}

bool ChessWidget::onSave()
{
  if ( !changed_ )
    return true;

  bool r = cpos_.save();
  if ( r )
    changed_ = false;
  update();

  return r;
}

void ChessWidget::onNext()
{
  cpos_.redo();
  update();
}

void ChessWidget::onPrev()
{
  cpos_.undo();
  update();
}

void ChessWidget::onGo()
{
  if ( thread_.isRunning() )
    return;

  if ( computerAnswers_ )
  {
    if ( useTimer_ )
    {
      thread_.start();
//      QTimer::singleShot(timelimit_, this, SLOT(onTimeoutStop()));
      enableActions(false);
    }
    //else
    //  doBestStep();
  }

  update();
  changed_ = true;
}

void ChessWidget::onMoveFound()
{
  enableActions(true);
  update();

  if ( autoPlay_ )
  {
    if ( goingToClose_ )
      return;

    //std::vector<Step> steps;
    //cpos_.calculateSteps(steps);
    //if ( steps.empty() )
    //  return;

    onGo();
  }
}

//void ChessWidget::onTimeoutStop()
//{
//  cpos_.stop();
//}

void ChessWidget::onTurnBoard(bool t)
{
  cpos_.setTurned(t);
  update();
}

//////////////////////////////////////////////////////////////////////////

bool ChessWidget::okToReset()
{
  if ( !changed_ )
    return true;

  int r = QMessageBox::warning(this, tr("QChess"), tr("Do you want to save current game?"),
    QMessageBox::Yes | QMessageBox::Default, QMessageBox::No, QMessageBox::Cancel | QMessageBox::Escape);

  if ( QMessageBox::Yes == r )
    return onSave();
  else if ( QMessageBox::Cancel == r )
    return false;

  return true;
}

void ChessWidget::closeEvent(QCloseEvent *event)
{
  goingToClose_ = true;
  cpos_.pleaseStop();
  thread_.wait();

  if ( !okToReset() )
  {
    event->ignore();
    return;
  }

  event->accept();
}

void ChessWidget::paintEvent(QPaintEvent * )
{
  cpos_.draw(this, curPt_);

  drawState();
  drawInfo();
}

void ChessWidget::drawState()
{
  const Board & board = cpos_.getBoard();
  Figure::Color color = board.getColor();
  Board::State state = board.getState();

  QString stateText = color == Figure::ColorBlack ? tr("Black") : tr("White");
  stateText += tr(" have ");
  if ( state == Board::UnderCheck )
    stateText += tr("Check");
  else if ( state == Board::ChessMat )
    stateText += tr("Mat");
  else if ( Board::isDraw(state) )
    stateText = tr("There is ChessDraw");
  else
    return;

  QPainter painter(this);
  QFont font(QObject::tr("Time New Roman"), 12, QFont::Bold);
  QPen pen(Qt::red);
  painter.setPen(pen);
  painter.setFont(font);
  painter.drawText(QRect(40, 20, 200, 30), Qt::AlignCenter, stateText);
}

void ChessWidget::drawInfo()
{
  QPainter painter(this);
  QString infoText;

  //double nps = 0, nps_curr = 0;
  //if ( full_t_ > 0 )
  //{
  //  nps = 0;//movesCount_/(double)full_t_;
  //  nps_curr = sres_.totalNodes_/(double)dt_;
  //}

  //double mavg = 0;
  //double davg = 0;
  //double step_dt = 0;

  //if ( bs_count_ > 0 )
  //{
  //  mavg = moves_avg_base_/bs_count_;
  //  davg = depth_avg_/bs_count_;
  //  step_dt = (((double)full_t_)/(1000.0*bs_count_));
  //}

  //double base_real = depth_ > 0 ? exp(log((double)(movesCurr_-forcesCount_-additionalCount_))/depth_) : 0;
  //infoText.sprintf("Moves base (avg) = %5.2f Step time = %5.2f (s) Steps = %d\n%5.2f kNodes/s Depth = %d Depth (avg) = %4.1f Eval = %4.2f\nCurrent step %5.2f kNodes/s  Moves base (current) = %5.2f\ntotal = %d moves = %d forced = %d additional = %d base = %5.2f\nnull moves = %d\n%s",
  //  mavg, dt_/1000.0, stepsCount_, nps, depth_, davg, w_/100.0f,
  //  nps_curr, moves_base_,
  //  totalCurr_, movesCurr_, forcesCount_, additionalCount_, base_real, nullMovesCount_, pv_str_);

  //infoText.sprintf("%d: [%d] %s (%0.2f) (%d)", cpos_.movesCount(), depth_, pv_str_, w_/100.0, (int)cpos_.getTicks());
  long long ticks = cpos_.getTicks();
  int tickPerMove = 0;
  if ( cpos_.numOfMoves() )
	  tickPerMove = ticks/cpos_.numOfMoves();
  //infoText.sprintf("%d ticks total, %d ticks per move, %d moves", (int)ticks, tickPerMove, cpos_.numOfMoves());
  int nps = dt_ > 0 ? sres_.nodesCount_*1000.0/dt_ : 0;
  infoText.sprintf("[%d] depth = %d, nodes count = %d, time = %d (ms), %d nps, score = %d", cpos_.movesCount(), sres_.depth_, sres_.nodesCount_, dt_, nps, sres_.score_);

  painter.drawText(QRect(00, 450, 450, 50), Qt::AlignCenter, infoText);
}

void ChessWidget::mouseDoubleClickEvent(QMouseEvent * e)
{
  if ( e->button() != Qt::LeftButton )
    return;

  curPt_ = e->pos();
}

void ChessWidget::mousePressEvent(QMouseEvent * e)
{
  if ( e->button() != Qt::LeftButton )
    return;

  curPt_ = e->pos();

  cpos_.selectFigure(curPt_);

  update();
}

void ChessWidget::mouseReleaseEvent(QMouseEvent * e)
{
  curPt_ = e->pos();

  if ( e->button() == Qt::LeftButton )
  {
//    cpos_.clearSelected();
    if ( cpos_.makeMovement(curPt_) )
    {
      changed_ = true;
      movesCount_ = cpos_.movesCount();
      onGo();
    }
  }
  //else if ( e->button() == Qt::RightButton )
  //{
  //  onGo();
  //}

  update();
}

void ChessWidget::findMove()
{
  QTime tm;
  tm.start();

  depth_ = cpos_.findMove(sres_);
  if ( 0 == depth_ )
    return;

  dt_ = tm.elapsed();
  full_t_ += dt_;
  bs_count_++;

  if ( depth_ > 0 )
  {
    moves_base_ = exp(log((double)sres_.nodesCount_)/depth_);
    moves_avg_base_ += moves_base_;
  }
  depth_avg_ += depth_;
  movesCount_ = cpos_.movesCount();

  pv_str_[0] = 0;
  //for (int i = 0; i < sres.depth_; ++i)
  //{
  //  char str[32];
  //  if ( !formatMove(cres.steps_[i], str) )
  //    break;
  //  strcat(pv_str_, str);
  //  strcat(pv_str_, " ");
  //}
}

void ChessWidget::mouseMoveEvent(QMouseEvent * e)
{
  if ( !(e->buttons() & Qt::LeftButton) )
    return;

  curPt_ = e->pos();
  update();
}

void ChessWidget::keyReleaseEvent(QKeyEvent * e)
{
  if ( !e )
    return;

  if  ( e->key() == Qt::Key_F2 )
    onSave();
  else if ( e->key() == Qt::Key_F3 )
    onLoad();
  else if ( e->key() == Qt::Key_PageUp )
    onNext();
  else if ( e->key() == Qt::Key_PageDown )
    onPrev();
}