/*************************************************************
  Player.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/


#include "Player.h"
#include "MovesGenerator.h"

//////////////////////////////////////////////////////////////////////////
SearchResult::SearchResult() :
  nodesCount_(0),
  totalNodes_(0),
  numOfMoves_(0),
  counter_(0),
  depth_(0),
  score_(0),
  dt_(0)
{
  best_.clear();
  for (int i = 0; i < MaxPly; ++i)
    pv_[i].clear();
}

//////////////////////////////////////////////////////////////////////////
SearchParams::SearchParams() :
  timeLimitMS_(0),
  depthMax_(0),
  analyze_mode_(false)
{
}

void SearchParams::reset()
{
  timeLimitMS_ = 0;
  depthMax_ = 0;
  analyze_mode_ = false;
}

//////////////////////////////////////////////////////////////////////////
SearchData::SearchData() :
  depth_(0),
  nodesCount_(0),
  totalNodes_(0),
  plyMax_(0),
  tstart_(0),
  tprev_(0),
  counter_(0),
  numOfMoves_(0),
  best_(0)
{
}

void SearchData::reset()
{
  depth_ = 0;
  nodesCount_ = 0;
  totalNodes_ = 0;
  tprev_ = tstart_ = clock();
  numOfMoves_ = 0;
  best_.clear();
  counter_ = 0;
  plyMax_ = 0;
}

void SearchData::restart()
{
  best_.clear();
  nodesCount_ = 0;
  plyMax_ = 0;
  counter_ = 0;
}

//////////////////////////////////////////////////////////////////////////
Player::SearchContext::SearchContext()
{
  undoStack_ = new UndoInfo[Board::GameLength];
  board_.set_undoStack(undoStack_);
}

Player::SearchContext::~SearchContext()
{
  board_.set_undoStack(0);
  delete [] undoStack_;
}

//////////////////////////////////////////////////////////////////////////
Player::Player() :
  stop_(false)
#ifdef USE_HASH
  ,hash_(19)
  ,ehash_(19)
#endif
{
  g_deltaPosCounter = new DeltaPosCounter;
  g_betweenMasks = new BetweenMask(g_deltaPosCounter);
  g_distanceCounter = new DistanceCounter;
  g_movesTable = new MovesTable;
  g_figureDir = new FigureDir;
  g_pawnMasks_ = new PawnMasks;

  for (int i = 0; i < sizeof(scontexts_)/sizeof(SearchContext); ++i)
  {
    scontexts_[i].eval_.initialize(&scontexts_[i].board_, &ehash_);
    scontexts_[i].board_.set_MovesTable(g_movesTable);
    scontexts_[i].board_.set_FigureDir(g_figureDir);
    scontexts_[i].board_.set_DeltaPosCounter(g_deltaPosCounter);
    scontexts_[i].board_.set_DistanceCounter(g_distanceCounter);
    scontexts_[i].board_.set_BetweenMask(g_betweenMasks);
    scontexts_[i].board_.set_PawnMasks(g_pawnMasks_);
  }
}

Player::~Player()
{
  for (int i = 0; i < sizeof(scontexts_)/sizeof(SearchContext); ++i)
  {
    scontexts_[i].board_.set_MovesTable(0);
    scontexts_[i].board_.set_FigureDir(0);
    scontexts_[i].board_.set_DeltaPosCounter(0);
    scontexts_[i].board_.set_DistanceCounter(0);
    scontexts_[i].board_.set_BetweenMask(0);
    scontexts_[i].board_.set_PawnMasks(0);
  }

  delete g_movesTable;
  delete g_figureDir;
  delete g_betweenMasks;
  delete g_deltaPosCounter;
  delete g_distanceCounter;
  delete g_pawnMasks_;
}

//////////////////////////////////////////////////////////////////////////
void Player::postCommand(const PostedCommand & cmd)
{
  posted_.push(cmd);
}

void Player::setMemory(int mb)
{
  if ( mb < 1 )
    return;

  int bytesN = mb*1024*1024;

#ifdef USE_HASH
  int hitemSize = sizeof(HItem);
  int hsize2 = log2(bytesN/hitemSize) - 3;
  if ( hsize2 >= 10 )
  {
    hash_.resize(hsize2);
    ehash_.resize(hsize2+1);
  }
#endif
}


bool Player::fromFEN(const char * fen)
{
  stop_ = false;

  Board tboard(scontexts_[0].board_);
  UndoInfo tundo[16];
  tboard.set_undoStack(tundo);

  // verify FEN first
  if ( !tboard.fromFEN(fen) )
    return false;

  MovesGenerator::clear_history();

  return scontexts_[0].board_.fromFEN(fen);
}

bool Player::toFEN(char * fen) const
{
  return scontexts_[0].board_.toFEN(fen);
}


void Player::reset()
{
  sdata_.reset();
  stop_ = false;
  MovesGenerator::clear_history();

  for (int i = 0; i < MaxPly; ++i)
    scontexts_[0].plystack_[i].clearKiller();
}

void Player::setCallbacks(CallbackStruct cs)
{
  callbacks_ = cs;
}

void Player::setAnalyzeMode(bool analyze)
{
  sparams_.analyze_mode_ = analyze;
}

void Player::setTimeLimit(int ms)
{
  sparams_.timeLimitMS_ = ms;
}

void Player::setMaxDepth(int d)
{
  if ( d >= 1 && d <= 32 )
    sparams_.depthMax_ = d;
}

void Player::pleaseStop()
{
  stop_ = true;
}

bool Player::checkForStop()
{
  if ( sdata_.totalNodes_ && !(sdata_.totalNodes_ & TIMING_FLAG) )
  {
    if ( sparams_.timeLimitMS_ > 0 )
      testTimer();
    else
      testInput();
  }
  return stop_;
}

void Player::testTimer()
{
  int t = clock();
  if ( (t - sdata_.tstart_) > sparams_.timeLimitMS_ )
    pleaseStop();

  testInput();
}

void Player::testInput()
{
  if ( callbacks_.queryInput_ )
  {
    (callbacks_.queryInput_)();
  }

  while ( !posted_.empty() )
  {
    PostedCommand & cmd = posted_.front();

    switch ( cmd.type_ )
    {
    case PostedCommand::ctUPDATE:
      {
        if ( callbacks_.sendStatus_ )
            (callbacks_.sendStatus_)(&sdata_);

        posted_.pop();
        break;
      }

    case PostedCommand::ctHINT:
    case PostedCommand::ctNONE:
      {
        posted_.pop();
        break;
      }

    case PostedCommand::ctNEW:
    case PostedCommand::ctUNDO:
    case PostedCommand::ctFEN:
      {
        pleaseStop();
        return;
      }
    }
  }
}

void Player::assemblePV(int ictx, const Move & move, bool checking, int ply)
{
  if ( ply > sdata_.depth_ || ply >= MaxPly )
    return;

  scontexts_[ictx].plystack_[ply].pv_[ply] = move;
  scontexts_[ictx].plystack_[ply].pv_[ply].checkFlag_ = checking;
  scontexts_[ictx].plystack_[ply].pv_[ply+1].clear();

  for (int i = ply+1; i < sdata_.depth_; ++i)
  {
    scontexts_[ictx].plystack_[ply].pv_[i] = scontexts_[ictx].plystack_[ply+1].pv_[i];
    if ( !scontexts_[ictx].plystack_[ply].pv_[i] )
      break;
  }
}
