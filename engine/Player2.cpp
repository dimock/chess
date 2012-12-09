/*************************************************************
  Player2.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/


#include "Player.h"
#include "MovesGenerator.h"
#include <algorithm>


//////////////////////////////////////////////////////////////////////////
ScoreType Player::alphaBetta0(ScoreType alpha, ScoreType betta)
{
  if ( stopped() )
    return board_.evaluate();

  if ( numOfMoves_ == 0 )
  {
    board_.setNoMoves();
    ScoreType score = board_.evaluate();
    return score;
  }

  ScoreType scoreBest = -ScoreMax;

  for (counter_ = 0; counter_ < numOfMoves_; ++counter_)
  {
    if ( checkForStop() )
      break;

    Move & move = moves_[counter_];
    ScoreType score = -ScoreMax;
    
    board_.makeMove(move);
    inc_nc();

    {
      int depth1 = nextDepth(depth_, move, false);

      if ( depth_ == depth0_ || !counter_ ) // 1st iteration
        score = -alphaBetta2(depth1, 1, -betta, -alpha, true);
      else
      {
        score = -alphaBetta2(depth1, 1, -alpha-1, -alpha, false);
        if ( score > alpha )
          score = -alphaBetta2(depth1, 1, -betta, -alpha, true);
      }
    }

    board_.unmakeMove();

    move.vsort_ = score + ScoreMax;

    if ( !stopped() && score > scoreBest )
    {
      scoreBest = score;

      if ( score > alpha )
      {
        best_ = move;
        if ( best_ == before_ )
          beforeFound_ = true;

        alpha = score;
        assemblePV(move, board_.underCheck(), 0);

        // bring best move to front, shift other moves 1 position right
        if ( depth_ > depth0_ )
        {
          Move mv = moves_[counter_];
          for (int j = counter_; j > 0; --j)
            moves_[j] = moves_[j-1];
          moves_[0] = mv;
        }
      }
    }
  }

  // don't need to continue
  if ( !stopped() && counter_ == 1 && !analyze_mode_ )
  {
    beforeFound_ = true;
    pleaseStop();
  }

  // sort only on 1st iteration
  if ( depth_ == depth0_ )
    std::sort(moves_, moves_ + numOfMoves_);

  return scoreBest;
}

ScoreType Player::alphaBetta2(int depth, int ply, ScoreType alpha, ScoreType betta, bool pv)
{
  if ( alpha >= Figure::WeightMat-ply )
    return alpha;

  if ( board_.drawState() )
    return Figure::WeightDraw;

  if ( stopped() || ply >= MaxPly )
    return board_.evaluate();

  ScoreType alpha0 = alpha;

  Move hmove(0);

#ifdef USE_HASH
  ScoreType hscore = -ScoreMax;
  GHashTable::Flag flag = getHash(depth, ply, alpha, betta, hmove, hscore, pv);
  if ( flag == GHashTable::Alpha || flag == GHashTable::Betta )
    return hscore;
#endif

  if ( depth <= 0 )
    return captures2(depth, ply, alpha, betta, pv);

  if ( ply < MaxPly-1 )
    contexts_[ply].clear(ply);

#ifdef USE_FUTILITY_PRUNING
  if ( !board_.underCheck() && !board_.isWinnerLoser() &&
        alpha > -Figure::WeightMat+MaxPly &&
        alpha < Figure::WeightMat-MaxPly &&
        depth == 1 && ply > 1 )
  {
    ScoreType score0 = board_.evaluate();
    int delta = (int)alpha - (int)score0 - (int)Figure::positionGain_;
    if ( delta > 0 )
      return captures2(depth, ply, alpha, betta, pv, score0);
  }
#endif

  int counter = 0;
  ScoreType scoreBest = -ScoreMax;
  Move best(0);

  Move killer(0);
  board_.extractKiller(contexts_[ply].killer_, hmove, killer);

  FastGenerator fg(board_, hmove, killer);

  if ( /*pv && */fg.singleReply() )
    depth++;

  for ( ; alpha < betta && !checkForStop(); )
  {
    Move & move = fg.move();
    if ( !move )
      break;

    if ( !board_.validateMove(move) )
      continue;

    ScoreType score = -ScoreMax;

    board_.makeMove(move);
    inc_nc();
    
    {
      int depth1 = nextDepth(depth, move, pv);

      if ( !counter || !pv )
        score = -alphaBetta2(depth1, ply+1, -betta, -alpha, pv);
      else if ( pv )
      {
        int depth2 = nextDepth(depth, move, false);
        score = -alphaBetta2(depth2, ply+1, -alpha-1, -alpha, false);
        if ( score > alpha && score < betta )
          score = -alphaBetta2(depth1, ply+1, -betta, -alpha, true);
      }
    }

    board_.unmakeMove();

    contexts_[ply].setKiller(move, score);

    if ( !stopped() && score > scoreBest )
    {
      best = move;
      scoreBest = score;
      if ( score > alpha )
      {
        alpha = score;
        if ( pv )
          assemblePV(move, board_.underCheck(), ply);
      }
    }

    counter++;
  }

  if ( stopped() )
    return scoreBest;

  if ( !counter )
  {
    board_.setNoMoves();
    scoreBest = board_.evaluate();
    if ( board_.matState() )
      scoreBest += ply;
  }


  if ( best )
  {
    History & hist = MovesGenerator::history(best.from_, best.to_);
    hist.inc_score(depth);
  }

#ifdef USE_HASH
  putHash(best, alpha0, betta, scoreBest, depth, ply);
#endif

  THROW_IF( scoreBest < -Figure::WeightMat || scoreBest > +Figure::WeightMat, "invalid score" );

  return scoreBest;
}

ScoreType Player::captures2(int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, ScoreType score0)
{
  if ( alpha >= Figure::WeightMat-ply )
    return alpha;

  if ( board_.drawState() )
    return Figure::WeightDraw;

  // not initialized yet
  if ( score0 == -ScoreMax )
    score0 = board_.evaluate();

  if ( stopped() || ply >= MaxPly )
    return score0;

  if ( !board_.underCheck() && score0 >= betta )
    return score0;

  int counter = 0;
  ScoreType scoreBest = -ScoreMax;
  int delta = 0;

  if ( !board_.underCheck() )
  {
    delta = (int)alpha - (int)score0 - (int)Figure::positionGain_;
    if ( score0 > alpha )
      alpha = score0;

    scoreBest = score0;
  }

  Move hmove(0);
  Figure::Type thresholdType = board_.isWinnerLoser() ? Figure::TypePawn : delta2type(delta);

  QuiesGenerator qg(hmove, board_, thresholdType, depth);

  if ( /*pv && */qg.singleReply() )
    depth++;

  for ( ; alpha < betta && !checkForStop(); )
  {
    Move & move = qg.next();
    if ( !move )
      break;

    if ( !board_.validateMove(move) )
      continue;

    ScoreType score = -ScoreMax;
    
    board_.makeMove(move);
    inc_nc();

    {
      int depth1 = nextDepth(depth, move, false);
      score = -captures2(depth1, ply+1, -betta, -alpha, pv);
    }

    board_.unmakeMove();

    if ( !stopped() && score > scoreBest )
    {
      scoreBest = score;
      if ( score > alpha )
        alpha = score;
    }

    counter++;
  }

  if ( stopped() )
    return scoreBest;

  if ( !counter )
  {
    if ( board_.underCheck() )
      return -Figure::WeightMat+ply;
    else
      return score0;
  }

  THROW_IF( scoreBest < -Figure::WeightMat || scoreBest > +Figure::WeightMat, "invalid score" );

  return scoreBest;
}
