/*************************************************************
  Player.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/


#include "Player.h"
#include "MovesGenerator.h"

//////////////////////////////////////////////////////////////////////////
SearchResult::SearchResult() :
  nodesCount_(0),
  totalNodes_(0),
  forcedNodes_(0),
  additionalNodes_(0),
  nullMovesCount_(0),
  depth_(0),
  score_(0),
  dt_(0),
  out_(0)
{
  best_.clear();
  for (int i = 0; i < MaxPly; ++i)
    pv_[i].clear();
}

//////////////////////////////////////////////////////////////////////////
Player::Player() :
  analyze_mode_(false),
  counter_(0),
  numOfMoves_(0),
  sres_(0),
  callback_(0),
  givetime_(0),
  posted_command_(Posted_NONE),
  stop_(false),
  timeLimitMS_(0),
  tstart_(0),
  nodesCount_(0),
  totalNodes_(0),
  depthMax_(2),
  depth_(0),
  plyMax_(0)
#ifdef USE_HASH
  ,hash_(20)
#endif
{
  posted_fen_[0] = 0;

  g_undoStack = new UndoInfo[Board::GameLength];
  pvundoStack_ = new UndoInfo[Board::GameLength];
  g_deltaPosCounter = new DeltaPosCounter;
  g_betweenMasks = new BetweenMask(g_deltaPosCounter);
  g_distanceCounter = new DistanceCounter;
  g_movesTable = new MovesTable;
  g_figureDir = new FigureDir;
  g_pawnMasks_ = new PawnMasks;

  board_.set_undoStack(g_undoStack);
  board_.set_MovesTable(g_movesTable);
  board_.set_FigureDir(g_figureDir);
  board_.set_DeltaPosCounter(g_deltaPosCounter);
  board_.set_DistanceCounter(g_distanceCounter);
  board_.set_BetweenMask(g_betweenMasks);
  board_.set_PawnMasks(g_pawnMasks_);
}

Player::~Player()
{
  board_.set_undoStack(0);
  board_.set_MovesTable(0);
  board_.set_FigureDir(0);
  board_.set_DeltaPosCounter(0);
  board_.set_DistanceCounter(0);
  board_.set_BetweenMask(0);
  board_.set_PawnMasks(0);

  delete [] pvundoStack_;
  delete [] g_undoStack;
  delete g_movesTable;
  delete g_figureDir;
  delete g_betweenMasks;
  delete g_deltaPosCounter;
  delete g_distanceCounter;
  delete g_pawnMasks_;
}

//////////////////////////////////////////////////////////////////////////
void Player::postUndo()
{
  if ( posted_command_ )
    return;

  posted_command_ = Posted_UNDO;
}

void Player::postNew()
{
  if ( posted_command_ )
    return;

  posted_command_ = Posted_NEW;
}

void Player::postFEN(const char * fen)
{
  if ( fen )
    strncpy(posted_fen_, fen, sizeof(posted_fen_));
}

void Player::postStatus()
{
  if ( posted_command_ )
    return;

  posted_command_ = Posted_UPDATE;
}

void Player::postHint()
{
  if ( posted_command_ )
    return;

  posted_command_ = Posted_HINT;
}

void Player::setMemory(int mb)
{
  if ( mb < 1 )
    return;

  int bytesN = mb*1024*1024;

#if ((defined USE_HASH_TABLE_GENERAL) || (defined USE_HASH_TABLE_CAPTURE))
  int ghitemSize = sizeof(GeneralHItem);
  int chitemSize = sizeof(CaptureHItem);

  int hsize = log2(bytesN/ghitemSize) - 1;
  if ( hsize < 10 )
    return;
#endif

#ifdef USE_HASH
  int hitemSize = sizeof(HItem);
  int hsize2 = log2(bytesN/hitemSize) - 2;
  if ( hsize2 >= 10 )
    hash_.resize(hsize2);
#endif

}


bool Player::fromFEN(const char * fen)
{
  stop_ = false;

  Board tboard(board_);
  UndoInfo tundo[16];
  tboard.set_undoStack(tundo);

  // verify FEN first
  if ( !tboard.fromFEN(fen) )
    return false;

  MovesGenerator::clear_history();

  return board_.fromFEN(fen);
}

bool Player::toFEN(char * fen) const
{
  return board_.toFEN(fen);
}

void Player::printPV()
{
  if ( !sres_ || !sres_->out_ || !sres_->best_ )
    return;

  *sres_->out_ << sres_->depth_ << " " << sres_->score_ << " " << (int)sres_->dt_ << " " << sres_->totalNodes_;
  for (int i = 0; i < sres_->depth_ && sres_->pv_[i]; ++i)
  {
    *sres_->out_ << " ";

    Move pv = sres_->pv_[i];
    uint8 captured = pv.capture_;
    pv.clearFlags();
    pv.capture_ = captured;

    if ( !pv_board_.possibleMove(pv) )
      break;

    char str[64];
    if ( !printSAN(pv_board_, pv, str) )
      break;

    THROW_IF( !pv_board_.validateMove(pv), "move is invalid but it is not detected by printSAN()");

    pv_board_.makeMove(pv);

    *sres_->out_ << str;
  }
  *sres_->out_ << std::endl;
}

void Player::reset()
{
  stop_ = false;
  totalNodes_ = 0;
  tprev_ = tstart_ = clock();
  numOfMoves_ = 0;

  MovesGenerator::clear_history();

  for (int i = 0; i < MaxPly; ++i)
    contexts_[i].clearKiller();
}

void Player::setCallback(PLAYER_CALLBACK cbk)
{
  callback_ = cbk;
}

void Player::setGiveTimeCbk(GIVE_MORE_TIME gvt)
{
  givetime_ = gvt;
}

void Player::setAnalyzeMode(bool analyze)
{
  analyze_mode_ = analyze;
}

void Player::testTimer()
{
  int t = clock();
  if ( (t - tstart_) > timeLimitMS_ )
    pleaseStop();

  if ( callback_ )
    (callback_)();

  if ( !posted_command_ )
    return;

  if ( posted_command_ == Posted_NEW || posted_command_ == Posted_UNDO || posted_command_ == Posted_FEN )
  {
    pleaseStop();
    return;
  }

  processPosted(t - tstart_);
}

void Player::testInput()
{
  if ( callback_ )
    (callback_)();

  if ( !posted_command_ )
    return;

  if ( posted_command_ == Posted_NEW || posted_command_ == Posted_UNDO || posted_command_ == Posted_FEN )
  {
    stop_ = true;
    return;
  }

  processPosted( clock() - tstart_ );
}

void Player::processPosted(int t)
{
  if ( !posted_command_ )
    return;

  if ( posted_command_ == Posted_UPDATE )
  {
    if ( sres_ && sres_->out_ && depth_ > 0 )
    {
      posted_command_ = Posted_NONE;

      char outstr[1024];
      sprintf(outstr, "stat01: %d %d %d %d %d", t/10, totalNodes_, depth_-1, numOfMoves_-counter_, numOfMoves_);

      Move mv = best_;
      if ( !mv )
        mv = sres_->best_;
      uint8 captured = mv.capture_;
      mv.clearFlags();
      mv.capture_ = captured;

      char str[64];
      if ( mv && pv_board_.validateMove(mv) && printSAN(pv_board_, mv, str) )
      {
        strcat(outstr, " ");
        strcat(outstr, str);
      }

      *sres_->out_ << outstr << std::endl;
    }
  }
  else if ( posted_command_ == Posted_HINT )
  {
    posted_command_ = Posted_NONE;
  }
}
