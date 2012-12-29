
/*************************************************************
  ChessWidget.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

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
#include <QTextStream>
#include <QFile>
#include "setparamsdlg.h"

static ChessWidget * g_chesswidget;

void updateCallback(SearchResult * sres)
{
  if ( g_chesswidget )
    g_chesswidget->updatePV(sres);
}

//////////////////////////////////////////////////////////////////////////
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
  QMainWindow(parent), upleft_(20, 50), depth_(0), bs_count_(0), moves_avg_base_(0), depth_avg_(0), movesCount_(0),
  moves_base_(0),
  thread_(this), goingToClose_(false), changed_(false), autoPlay_(false), useTimer_(true), timelimit_(1000),
  depthMax_(2), infoHeight_(60),
  onNewAction_(0),
  onLoadAction_(0),
  onSaveAction_(0),
  onNextAction_(0),
  onPrevAction_(0),
  onGoAction_(0),
  onTurnBoardAction_(0),
  onHumanVsHumanAction_(0),
  onOpenBookAction_(0),
  onSettingsAction_(0)
{
  g_chesswidget = this;

  QSettings settings(tr("Dimock"), tr("qchess"));
  timelimit_ = settings.value(tr("step_time"), 1).toInt()*1000;
  depthMax_ = settings.value(tr("max_depth"), 16).toInt();

  //setFixedSize(450, 600);
  pv_str_[0] = 0;
  setAttribute(Qt::WA_DeleteOnClose);

  setWindowIcon(QIcon(":/images/chess.png"));

  upleft_.setY(cpos_.getDiffHeight() + 50);
  cpos_.setUpLeft(upleft_);

  setFixedSize(cpos_.getBoardWidth() + upleft_.x()*2, cpos_.getBoardHeight() + infoHeight_ + upleft_.y());

  createMenu();

  onNew();

  obook_.load( "debut.tbl", cpos_.getBoard() );

  cpos_.setUpdateCallback(&updateCallback);

  connect(&thread_, SIGNAL(finished()), this, SLOT(onMoveFound()));
  connect(this, SIGNAL(pvUpdated()), this, SLOT(onPvUpdated()));
}

ChessWidget::~ChessWidget()
{
  g_chesswidget = 0;
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

  onHumanVsHumanAction_ = new QAction(tr("&Human vs. Human"), this);
  onHumanVsHumanAction_->setStatusTip(tr("Switch to Human with Human mode"));
  onHumanVsHumanAction_->setCheckable(true);
  onHumanVsHumanAction_->setChecked(false);

  QSettings settings(tr("Dimock"), tr("qchess"));
  onOpenBookAction_ = new QAction(tr("&Open book"), this);
  onOpenBookAction_->setStatusTip(tr("Use open book"));
  onOpenBookAction_->setCheckable(true);
  onOpenBookAction_->setChecked( settings.value(tr("open_book"), true).toBool() );

  onSettingsAction_ = new QAction(tr("Settin&gs"), this);
  onSettingsAction_->setStatusTip(tr("Change game settings"));

  gameMenu->addAction(onNewAction_);
  gameMenu->addAction(onLoadAction_);
  gameMenu->addAction(onSaveAction_);
  gameMenu->addAction(onPrevAction_);
  gameMenu->addAction(onNextAction_);
  gameMenu->addAction(onGoAction_);
  gameMenu->addAction(onTurnBoardAction_);
  gameMenu->addAction(onOpenBookAction_);
  gameMenu->addSeparator();
  gameMenu->addAction(onHumanVsHumanAction_);
  gameMenu->addSeparator();
  gameMenu->addAction(onSettingsAction_);

  connect(onNewAction_, SIGNAL(triggered()), this, SLOT(onNew()));
  connect(onLoadAction_, SIGNAL(triggered()), this, SLOT(onLoad()));
  connect(onSaveAction_, SIGNAL(triggered()), this, SLOT(onSave()));
  connect(onPrevAction_, SIGNAL(triggered()), this, SLOT(onPrev()));
  connect(onNextAction_, SIGNAL(triggered()), this, SLOT(onNext()));
  connect(onGoAction_, SIGNAL(triggered()), this, SLOT(onGo()));
  connect(onTurnBoardAction_, SIGNAL(toggled(bool)), this, SLOT(onTurnBoard(bool)));
  connect(onHumanVsHumanAction_, SIGNAL(toggled(bool)), this, SLOT(onHumanWithHumanMode(bool)));
  connect(onOpenBookAction_, SIGNAL(toggled(bool)), this, SLOT(onUseOpenBook(bool)));
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

  if ( !cpos_.initialize(depthMax_) )
    return;

  cpos_.setTimeLimit(timelimit_);

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

  if ( computerAnswers() )
  {
    if ( useTimer_ )
    {
      enableActions(false);
      thread_.start();
    }
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

    onGo();
  }
}

void ChessWidget::onTurnBoard(bool t)
{
  cpos_.setTurned(t);
  update();
}

void ChessWidget::onHumanWithHumanMode(bool)
{
  update();
}

void ChessWidget::onUseOpenBook(bool o)
{
  QSettings settings(tr("Dimock"), tr("qchess"));
  settings.setValue(tr("open_book"), o);
}

void ChessWidget::onPvUpdated()
{
  formatPV();
  update();
}

//////////////////////////////////////////////////////////////////////////

bool ChessWidget::computerAnswers() const
{
  return onHumanVsHumanAction_ && !onHumanVsHumanAction_->isChecked();
}

bool ChessWidget::useOpenBook() const
{
  return onOpenBookAction_ && onOpenBookAction_->isChecked();
}

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

  QSettings settings(tr("Dimock"), tr("qchess"));
  settings.setValue(tr("open_book"), useOpenBook());

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
  uint8 state = board.getState();

  QString stateText = color == Figure::ColorBlack ? tr("Black") : tr("White");
  stateText += tr(" have ");
  if ( board.matState() )
    stateText += tr("Mat");
  else if ( board.underCheck() )
    stateText += tr("Check");
  else if ( state & Board::DrawInsuf )
    stateText = tr("Material insufficient");
  else if ( state & Board::DrawReps )
    stateText = tr("Threefold repetition");
  else if ( state & Board::Draw50Moves )
    stateText = tr("50 moves rule");
  else if ( state & Board::Stalemat )
    stateText = tr("Stalemat");
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

  int nps = sres_.dt_ > 0 ? sres_.totalNodes_*100.0/sres_.dt_ : 0;
  //int ticksN = Board::ticks_;
  //int hscore = Board::tcounter_;
  //infoText.sprintf("[%d] depth = %d, nodes count = %d, time = %d (ms), %d nps\nscore = %d, LMR-errors = %d, hist. score(avg) = %d\n{ %s }",
  //  cpos_.movesCount(), sres_.depth_, sres_.totalNodes_, dt_, nps, sres_.score_, ticksN, hscore, pv_str_);

  if ( computerAnswers() )
    infoText.sprintf("[%d] (%d ply) { %s }\nscore = %4.2f, %d nodes, %d nps", cpos_.movesCount(), sres_.depth_, pv_str_, sres_.score_/100.f, sres_.totalNodes_, nps);
  else
    infoText.sprintf("[%d]", cpos_.movesCount());

  painter.drawText(QRect(upleft_.x(), upleft_.y() + cpos_.getBoardHeight(), cpos_.getBoardWidth(), infoHeight_), Qt::AlignCenter, infoText);
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
    if ( cpos_.makeMovement(curPt_) )
    {
      changed_ = true;
      movesCount_ = cpos_.movesCount();
      onGo();
    }
  }

  update();
}

bool ChessWidget::findInBook()
{
  Board board = cpos_.getBoard();
  Move move = obook_.nextMove(board);
  if ( !move )
    return false;

  if ( !cpos_.applyMove(move) )
    return false;

  return true;
}

void ChessWidget::findMove()
{
  if ( useOpenBook() && findInBook() )
    return;

  Board::ticks_ = 0;
  Board::tcounter_ = 0;

  Board pv_board = cpos_.getBoard();

  SearchResult sres;
  depth_ = cpos_.findMove(&sres);
  if ( 0 == depth_ || !sres.best_ )
    return;

  bs_count_++;

  if ( depth_ > 0 )
  {
    moves_base_ = exp(log((double)sres.nodesCount_)/depth_);
    moves_avg_base_ += moves_base_;
  }
  depth_avg_ += depth_;
  movesCount_ = cpos_.movesCount();
  if ( Board::ticks_ )
    Board::tcounter_ /= Board::ticks_;

  sres_ = sres;
  updatePV(&sres);
}

void ChessWidget::updatePV(SearchResult * sres)
{
  sres_ = *sres;
  emit pvUpdated();
}

void ChessWidget::formatPV()
{
  pv_str_[0] = 0;  
  Board board = sres_.board_;
  board.set_undoStack(pvundoStack_);
  for (int i = 0; i < sres_.depth_ && sres_.pv_[i]; ++i)
  {
    if ( !board.possibleMove(sres_.pv_[i]) || !board.validateMove(sres_.pv_[i]) )
      break;

    char str[32];
    if ( !printSAN(board, sres_.pv_[i], str) )
      break;

    board.makeMove(sres_.pv_[i]);

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

//////////////////////////////////////////////////////////////////////////
bool OpenBook::load(const char * fname, const Board & i_board)
{
  QFile qf(fname);
  if ( !qf.open(QIODevice::ReadOnly) )
    return false;

  QTextStream sbook(&qf);

  if ( sbook.status() != QTextStream::Ok )
    return false;

  UndoInfo tundo[Board::GameLength];

  for ( ; !sbook.atEnd(); )
  {
    QString sline = sbook.readLine();
    if ( sline.isEmpty() )
      break;

    Board board = i_board;
    board.set_undoStack(tundo);
    board.fromFEN(0);

    MovesLine moves;
    QStringList slist = sline.split( QObject::tr(" "), QString::SkipEmptyParts);
    for (QStringList::iterator it = slist.begin(); it != slist.end(); ++it)
    {
      Move move;
      if ( !strToMove(it->toAscii().data(), board, move) || !board.validateMove(move) )
        break;

      board.makeMove(move);
      moves.push_back(move);
    }

    mtable_.push_back(moves);
  }

  return true;
}

unsigned long xorshf96()
{
  static unsigned long x= (unsigned long)time(0) ^ clock(), y=362436069, z=521288629;

  unsigned long t;
  x ^= x << 16;
  x ^= x >> 5;
  x ^= x << 1;

  t = x;
  x = y;
  y = z;
  z = t ^ x ^ y;

  return z;
}

Move OpenBook::nextMove(const Board & board)
{
  MovesLine valid_moves;

  for (size_t i = 0; i < mtable_.size(); ++i)
  {
    MovesLine & mline = mtable_[i];
    size_t j = 0;
    for ( ; j < (size_t)board.halfmovesCount() && j < mline.size(); ++j)
    {
      Move mv = board.undoInfo((int)j);
      if ( mv != mline[j] )
        j = mline.size();
    }

    if ( j < mline.size() )
    {
      const Move & m = mline[j];
      valid_moves.push_back(m);
    }
  }

  Move mres;
  mres.clear();

  if ( valid_moves.empty() )
    return mres;

  size_t n = xorshf96() % valid_moves.size();
  mres = valid_moves[n];

  return mres;
}
