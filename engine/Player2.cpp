/*************************************************************
  Player2.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/


#include "Player.h"
#include "MovesGenerator.h"
#include <algorithm>


//////////////////////////////////////////////////////////////////////////
ScoreType Player::alphaBetta0(ScoreType alpha, ScoreType betta)
{
  if ( stop_ )
    return board_.evaluate();

  if ( numOfMoves_ == 0 )
  {
    board_.setNoMoves();
    ScoreType score = board_.evaluate();
    return score;
  }

  ScoreType scoreBest = -ScoreMax;

  for (int i = 0; i < numOfMoves_; ++i)
  {
    if ( checkForStop() )
      break;

    Move & move = moves0_[i];
    ScoreType score = -ScoreMax;
    
    board_.makeMove(move);
    inc_nc();

    int depth1 = nextDepth(depth_);

    if ( depth_ == depth0_ || !i ) // 1st iteration
      score = -alphaBetta2(depth1, 1, -betta, -alpha, true);
    else
    {
      score = -alphaBetta2(depth1, 1, -alpha-1, -alpha, false);
      if ( score > alpha )
        score = -alphaBetta2(depth1, 1, -betta, -alpha, true);
    }

    board_.unmakeMove();

    if ( score > scoreBest )
    {
      scoreBest = score;
      if ( score > alpha )
      {
        best_ = move;
        if ( best_ == before_ )
          beforeFound_ = true;
        alpha = score;
        assemblePV(move, board_.underCheck(), 0);
      }
    }

    move.vsort_ = score + ScoreMax;

    counter_ = i;
  }

  if ( counter_ == 1 && !analyze_mode_ )
  {
    beforeFound_ = true;
    pleaseStop();
  }

  std::sort(moves0_, moves0_ + numOfMoves_);

  return scoreBest;
}

ScoreType Player::alphaBetta2(int depth, int ply, ScoreType alpha, ScoreType betta, bool pv)
{
  if ( alpha >= Figure::WeightMat-ply )
    return alpha;

  if ( board_.drawState() )
    return Figure::WeightDraw;

  if ( stop_ || ply >= MaxPly )
    return board_.evaluate();

  if ( depth <= 0 )
    return captures2(depth, ply, alpha, betta);

  if ( ply < MaxPly-1 )
    contexts_[ply].clear(ply);

  int counter = 0;
  ScoreType scoreBest = -ScoreMax;

  Move killer(0), hmove(0);
  board_.extractKiller(contexts_[ply].killer_, hmove, killer);

  FastGenerator fg(board_, hmove, killer);
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

    int depth1 = nextDepth(depth);

    if ( !counter || !pv )
      score = -alphaBetta2(depth1, ply+1, -betta, -alpha, pv);
    else if ( pv )
    {
      score = -alphaBetta2(depth1, ply+1, -alpha-1, -alpha, false);
      if ( score > alpha && score < betta )
        score = -alphaBetta2(depth1, ply+1, -betta, -alpha, true);
    }

    board_.unmakeMove();

    History & hist = MovesGenerator::history(move.from_, move.to_);
    hist.inc_gb(score > alpha);

    contexts_[ply].setKiller(move, score);

    if ( score > scoreBest )
    {
      scoreBest = score;
      if ( score > alpha )
      {
        alpha = score;
        assemblePV(move, board_.underCheck(), ply);
        hist.inc_score(depth);
      }
    }

    counter++;
  }

  if ( !counter )
  {
    board_.setNoMoves();
    ScoreType score = board_.evaluate();
    if ( board_.matState() )
      score += ply;
    return score;
  }

  THROW_IF( scoreBest < -Figure::WeightMat || scoreBest > +Figure::WeightMat, "invalid score" );

  return scoreBest;
}

ScoreType Player::captures2(int depth, int ply, ScoreType alpha, ScoreType betta)
{
  if ( alpha >= Figure::WeightMat-ply )
    return alpha;

  if ( board_.drawState() )
    return Figure::WeightDraw;

  ScoreType score0 = board_.evaluate();

  if ( stop_ || ply >= MaxPly )
    return score0;

  if ( !board_.underCheck() && score0 >= betta )
    return score0;

  int counter = 0;
  ScoreType scoreBest = -ScoreMax;

  if ( !board_.underCheck() )
  {
    if ( score0 > alpha )
      alpha = score0;
    scoreBest = score0;
  }

  Move hmove(0);
  Figure::Type minimalType = Figure::TypePawn;

  QuiesGenerator qg(hmove, board_, minimalType, depth);

  if ( qg.singleReply() )
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

    int depth1 = nextDepth(depth);

    score = -captures2(depth1, ply+1, -betta, -alpha);

    board_.unmakeMove();

    if ( score > scoreBest )
    {
      scoreBest = score;
      if ( score > alpha )
        alpha = score;
    }

    counter++;
  }

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
