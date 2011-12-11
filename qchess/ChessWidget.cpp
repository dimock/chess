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
#include <QClipboard>
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
  depthMax_(2), enableBook_(true), ticksAll_(0)
{
  QSettings settings(tr("Dimock"), tr("qchess"));
  timelimit_ = settings.value(tr("step_time"), 1).toInt()*1000;
  depthMax_ = settings.value(tr("max_depth"), 16).toInt();

  setFixedSize(450, 525);
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

void ChessWidget::onGetFEN()
{
  QString qfen = QApplication::clipboard()->text();
  if ( cpos_.fromFEN(qfen.toAscii().data()) )
  {
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
    repaint();
  }
}

void ChessWidget::onPutFEN() const
{
  char fen[Board::FENsize];
  cpos_.toFEN(fen);
  QString qstr(fen);
  QApplication::clipboard()->setText(qstr);
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

  int nps = dt_ > 0 ? sres_.totalNodes_*1000.0/dt_ : 0;
  int ticksN = Board::ticks_/1000;
  infoText.sprintf("[%d] depth = %d, nodes count = %d, time = %d (ms), %d nps\nscore = %d, ticks = %d\n{ %s }",
    cpos_.movesCount(), sres_.depth_, sres_.totalNodes_, dt_, nps, sres_.score_, ticksN, pv_str_);

  painter.drawText(QRect(00, 450, 450, 75), Qt::AlignCenter, infoText);
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

  QpfTimer qpt;

  Board::ticks_ = 0;
  Board::tcounter_ = 0;

  Board pv_board = cpos_.getBoard();

  depth_ = cpos_.findMove(sres_);
  if ( 0 == depth_ || !sres_.best_ )
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
  if ( Board::tcounter_ )
    Board::ticks_ /= Board::tcounter_;

  pv_str_[0] = 0;

  ticksAll_ = qpt.ticks();
  
  pv_board.set_moves(pv_moves_);

  for (int i = 0; i < sres_.depth_ && sres_.pv_[i]; ++i)
  {
    if ( !pv_board.validMove(sres_.pv_[i]) || !pv_board.makeMove(sres_.pv_[i]) )
      break;

    pv_board.unmakeMove();

    char str[32];
    if ( !printSAN(pv_board, sres_.pv_[i], str) )
      break;

    pv_board.makeMove(sres_.pv_[i]);

    strcat_s(pv_str_, str);
    strcat_s(pv_str_, " ");
  }
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

  switch ( e->key() )
  {
  case Qt::Key_F2:
    onSave();
    break;

  case Qt::Key_F3:
    onLoad();
    break;

  case Qt::Key_PageUp:
    onNext();
    break;

  case Qt::Key_PageDown:
    onPrev();
    break;

  case Qt::Key_C:
    if ( e->modifiers() & Qt::ControlModifier )
    {
      onPutFEN();
    }
    break;

  case Qt::Key_V:
    if ( e->modifiers() & Qt::ControlModifier )
    {
      onGetFEN();
    }
    break;
  }
}