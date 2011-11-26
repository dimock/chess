
#include "Player.h"

#undef NO_TIME_LIMIT

SearchResult::SearchResult() :
  nodesCount_(0),
  totalNodes_(0),
  forcedNodes_(0),
  additionalNodes_(0),
  nullMovesCount_(0),
  depth_(0),
  score_(0)
{
  best_.clear();
  for (int i = 0; i < MaxDepth; ++i)
    pv_[i].clear();
}

Player::Player() :
  stop_(false),
  timeLimitMS_(0),
  tstart_(0),
  depthMax_(4)
{
  g_moves = new MoveCmd[Board::GameLength];
  g_deltaPosCounter = new DeltaPosCounter;
  g_betweenMasks = new BetweenMask(g_deltaPosCounter);
  g_distanceCounter = new DistanceCounter;
  g_movesTable = new MovesTable;
  g_figureDir = new FigureDir;
  g_pawnMasks_ = new PawnMasks;

  board_.set_moves(g_moves);
  board_.set_MovesTable(g_movesTable);
  board_.set_FigureDir(g_figureDir);
  board_.set_DeltaPosCounter(g_deltaPosCounter);
  board_.set_DistanceCounter(g_distanceCounter);
  board_.set_BetweenMask(g_betweenMasks);
  board_.set_PawnMasks(g_pawnMasks_);
}

Player::~Player()
{
  board_.set_moves(0);
  board_.set_MovesTable(0);
  board_.set_FigureDir(0);
  board_.set_DeltaPosCounter(0);
  board_.set_DistanceCounter(0);
  board_.set_BetweenMask(0);
  board_.set_PawnMasks(0);

  delete [] g_moves;
  delete g_movesTable;
  delete g_figureDir;
  delete g_betweenMasks;
  delete g_deltaPosCounter;
  delete g_distanceCounter;
  delete g_pawnMasks_;
}

bool Player::fromFEN(const char * fen)
{
  stop_ = false;
  timeLimitMS_ = 0;
  return board_.fromFEN(fen);
}

bool Player::findMove(SearchResult & sres)
{
  sres = SearchResult();

  stop_ = false;
  nodesCounter_ = 0;
  tstart_ = clock();

  Move before;
  before.clear();
  for (int depth = 2; !stop_ && depth <= depthMax_; ++depth)
  {
    Move best;
    bool found = false;

    ScoreType score = alphaBetta(depth, 0, -std::numeric_limits<ScoreType>::max(), std::numeric_limits<ScoreType>::max(), before, best, found);

    if ( !stop_ || (stop_ && found) )
    {
      sres.score_ = score;
      sres.best_  = best;
      sres.depth_ = depth;
      before = best;
    }

    if ( score >= Figure::WeightMat-MaxDepth || score <= MaxDepth-Figure::WeightMat )
      break;
  }

  sres.nodesCount_ = nodesCounter_;

  return sres.best_.to_ >= 0;
}

void Player::testTimer()
{
#ifndef NO_TIME_LIMIT
  int t = clock();
  stop_ = stop_ || ( (t - tstart_) > timeLimitMS_);
#endif
}

//////////////////////////////////////////////////////////////////////////
ScoreType Player::alphaBetta(int depth, int ply, ScoreType alpha, ScoreType betta, const Move & before, Move & move, bool & found)
{
  if ( stop_ )
    return alpha;

  int counter = 0;
  Move b;
  b.clear();

  if ( before && board_.validMove(before) )
  {
    nodesCounter_++;
    movement(depth, ply, alpha, betta, before, b, before, move, found, counter);
  }

  Move moves[Board::MovesMax];
  int num = board_.generateMoves(moves);
  for (int i = 0; !stop_ && alpha < betta && i < num; ++i)
  {
    const Move & mv = moves[i];
    if ( nodesCounter_ && !(nodesCounter_ & 0xffff) )
      testTimer();

    if ( stop_ )
      break;
    
    nodesCounter_++;

    THROW_IF( !board_.validMove(mv), "move validation failed" );

    movement(depth, ply, alpha, betta, before, b, mv, move, found, counter);
  }

  if ( stop_ )
    return alpha;

  if ( 0 == counter )
  {
    board_.setNoMoves();
    ScoreType s = board_.evaluate();
    if ( board_.getState() == Board::ChessMat )
      s += ply;
    return s;
  }

  return alpha;
}