
#include "Player.h"

#define NO_TIME_LIMIT

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
}

bool Player::initialize(const char * fen)
{
  stop_ = false;
  timeLimitMS_ = 0;
  return board_.initialize(fen);
}

bool Player::findMove(SearchResult & sres)
{
  static FPosIndexer s_fposindexer;
  static BetweenMask s_betweenMasks;
  static BitsCounter s_bitsCounter;
  static DeltaPosCounter s_deltaPosCounter;
  static DistanceCounter s_distanceCounter;

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
    else
    {
      int ttt = 0;
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

//  if ( before && board_.validMove(before) )
//  {
//    nodesCounter_++;
//    if ( board_.makeMove(before) )
//    {
//      counter++;
//      ScoreType s = alpha;
//      if ( board_.drawState() )
//        s = 0;
//      else if ( depth <= 1 )
//      {
//        s = -board_.evaluate();
//      }
//      else
//      {
//        Move m;
//        bool f = false;
//
//        s = -alphaBetta(depth-1, ply+1, -betta, -alpha, b, m, f);
//      }
//
//      if ( s > alpha )
//      {
//        alpha = s;
//        move = before;
//        found = true;
//      }
//    }
//
//#ifndef NDEBUG
//    board_.verifyMasks();
//#endif
//
//    board_.unmakeMove();
//
//#ifndef NDEBUG
//    board_.verifyMasks();
//#endif
//  }

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
    
    if ( board_.makeMove(mv) )
    {
      counter++;
      ScoreType s = alpha;
      if ( board_.drawState() )
        s = 0;
      else if ( depth <= 1 )
      {
        s = -board_.evaluate();
      }
      else
      {
        Move m;
        bool f = false;

        s = -alphaBetta(depth-1, ply+1, -betta, -alpha, b, m, f);
      }

      if ( s > alpha )
      {
        alpha = s;
        move = mv;
        if ( before == move )
          found = true;
      }
    }

#ifndef NDEBUG
    board_.verifyMasks();
#endif

    board_.unmakeMove();

#ifndef NDEBUG
    board_.verifyMasks();
#endif
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