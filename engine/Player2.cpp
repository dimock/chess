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
        score = -alphaBetta2(depth1, 1, -betta, -alpha, true, false);
      else
      {
        score = -alphaBetta2(depth1, 1, -alpha-1, -alpha, false, false);
        if ( score > alpha )
          score = -alphaBetta2(depth1, 1, -betta, -alpha, true, false);
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

ScoreType Player::alphaBetta2(int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, bool null_move)
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

  bool nm_threat = false, real_threat = false;

#ifdef USE_NULL_MOVE
  if ( !pv && !board_.underCheck() && board_.allowNullMove() && depth >= 2 &&
        betta < Figure::WeightMat-MaxPly &&
        betta > -Figure::WeightMat+MaxPly)
  {
    board_.makeNullMove();

    int null_depth = depth-4;

    ScoreType nullScore = -alphaBetta2(null_depth, ply+1, -betta, -(betta-1), false, true);

    board_.unmakeNullMove();

    // verify null-move
    if ( nullScore >= betta )
    {
      null_move = true;
      depth -= 5;
      if ( depth < 1 )
        return captures2(depth, ply, alpha, betta, pv);
    }
    else // may be we are in danger? verify it later
      nm_threat = true;
  }
#endif


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

  if ( fg.singleReply() )
    depth++;

  MoveCmd & prev = board_.getMoveRev(0);

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
    
    MoveCmd & curr = board_.getMoveRev(0);

    {
      int depth1 = nextDepth(depth, move, pv);

      if ( !counter || !pv )
        score = -alphaBetta2(depth1, ply+1, -betta, -alpha, pv, null_move);
      else if ( pv )
      {
        int R = 1 - board_.underCheck();

#ifdef USE_LMR
        if ( !null_move &&
             depth_ > LMR_MinDepthLimit &&
             depth > LMR_DepthLimit &&
             alpha > -Figure::WeightMat+MaxPly &&             
             board_.canBeReduced() )
        {
          R = LMR_PlyReduce;
          curr.reduced_ = true;
        }
#endif

        score = -alphaBetta2(depth-R, ply+1, -alpha-1, -alpha, false, null_move);
        curr.reduced_ = false;

#ifdef USE_LMR
        if ( !stopped() && score > alpha && R > 1 )
        {
          score = -alphaBetta2(depth-1, ply+1, -alpha-1, -alpha, false, null_move);
        }
#endif

        if ( !stopped() && score > alpha && score < betta )
          score = -alphaBetta2(depth1, ply+1, -betta, -alpha, true, null_move);
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

    // have to recalculate with full depth
    if ( !stopped() && alpha >= betta && nm_threat && isRealThreat(move) )
    {
      if ( prev.reduced_ )
        return betta-1;
      else
        real_threat = true;
    }
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
  putHash(best, alpha0, betta, scoreBest, depth, ply, real_threat);
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

  ScoreType alpha0 = alpha;
  Move best(0), hmove(0);

#ifdef USE_HASH_CAPS
  ScoreType hscore = -ScoreMax;
  GHashTable::Flag flag = getCap(depth, ply, alpha, betta, hmove, hscore, pv);
  if ( flag == GHashTable::Alpha || flag == GHashTable::Betta )
    return hscore;
#endif

  Figure::Type thresholdType = board_.isWinnerLoser() ? Figure::TypePawn : delta2type(delta);

  QuiesGenerator qg(hmove, board_, thresholdType, depth);

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
    inc_nc();

    {
      int depth1 = nextDepth(depth, move, false);
      score = -captures2(depth1, ply+1, -betta, -alpha, pv);
    }

    board_.unmakeMove();

    if ( !stopped() && score > scoreBest )
    {
      best = move;
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
      scoreBest = -Figure::WeightMat+ply;
    else
      scoreBest = score0;
  }

  THROW_IF( scoreBest < -Figure::WeightMat || scoreBest > +Figure::WeightMat, "invalid score" );

#ifdef USE_HASH_CAPS
  putCap(best, alpha0, betta, scoreBest, ply);
#endif

  return scoreBest;
}

//////////////////////////////////////////////////////////////////////////
// is given movement caused by previous? this mean that if we don't do this move we loose
// we actually test if moved figure was/will be attacked by previously moved one or from direction it was moved from
// or we should move our king even if we have a lot of other figures
//////////////////////////////////////////////////////////////////////////
bool Player::isRealThreat(const Move & move)
{
  // don't need to forbid if our answer is capture or check ???
  if ( move.capture_ || move.checkFlag_ )
    return false;

  const MoveCmd & prev = board_.getMoveRev(0);
  if ( !prev ) // null-move
    return false;

  Figure::Color  color = board_.getColor();
  Figure::Color ocolor = Figure::otherColor(color);

  const Field & pfield = board_.getField(prev.to_);
  THROW_IF( !pfield || pfield.color() != ocolor, "no figure of required color on the field it was move to while detecting threat" );

  // don't need forbid reduction of captures, checks, promotions and pawn's attack because we've already done it
  if ( prev.capture_ || prev.new_type_ > 0 || prev.checkingNum_ > 0 || board_.isDangerPawn(prev) )
    return false;

  /// we have to move king even if there a lot of figures
  if ( board_.getField(move.from_).type() == Figure::TypeKing &&
       board_.fmgr().queens(color) > 0 &&
       board_.fmgr().rooks(color) > 0 &&
       board_.fmgr().knights(color)+board_.fmgr().bishops(color) > 0 )
  {
    return true;
  }

  const Field & cfield = board_.getField(move.from_);
  THROW_IF( !cfield || cfield.color() != color, "no figure of required color in while detecting threat" );

  // we have to put figure under attack
  if ( board_.ptAttackedBy(move.to_, prev.to_) )
    return true;

  // put our figure under attack, opened by prev movement
  if ( board_.getAttackedFrom(ocolor, move.to_, prev.from_) >= 0 )
    return true;

  // prev move was attack, and we should escape from it
  if ( board_.ptAttackedBy(move.from_, prev.to_) )
    return true;

  // our figure was attacked from direction, opened by prev movement
  if ( board_.getAttackedFrom(ocolor, move.from_, prev.from_) >= 0 )
    return true;

  return false;
}
