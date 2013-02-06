/*************************************************************
  Player2.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/


#include "Player.h"
#include "MovesGenerator.h"
#include "Evaluator.h"
#include <algorithm>

//////////////////////////////////////////////////////////////////////////
inline Figure::Type delta2type(int delta)
{
  Figure::Type minimalType = Figure::TypePawn;

#ifdef USE_DELTA_PRUNING
  if ( delta > Figure::figureWeight_[Figure::TypeQueen] )
    minimalType = Figure::TypeKing;
  else if ( delta > Figure::figureWeight_[Figure::TypeRook] )
    minimalType = Figure::TypeQueen;
  else if ( delta > Figure::figureWeight_[Figure::TypeBishop] )
    minimalType = Figure::TypeRook;
  else if ( delta > Figure::figureWeight_[Figure::TypeKnight] )
    minimalType = Figure::TypeBishop;
  else if ( delta > Figure::figureWeight_[Figure::TypePawn] )
    minimalType = Figure::TypeKnight;
#endif

  return minimalType;
}


//////////////////////////////////////////////////////////////////////////
bool Player::findMove(SearchResult * sres)
{
  if ( !sres )
    return false;

  bool ok = false;
  bool leave = false;
  for ( ; !leave; )
  {
    ok = search(sres);
    leave = true;

    while ( !posted_.empty() )
    {
      PostedCommand & cmd = posted_.front();

      switch ( cmd.type_ )
      {
      case PostedCommand::ctUPDATE:
        {
          if ( callbacks_.sendStatus_ )
            (callbacks_.sendStatus_)(&sdata_);
          break;
        }

      case PostedCommand::ctHINT:
      case PostedCommand::ctNONE:
          break;

      case PostedCommand::ctNEW:
        {
          fromFEN(0);
          leave = !sparams_.analyze_mode_;
          break;
        }

      case PostedCommand::ctUNDO:
        {
          scontexts_[0].board_.unmakeMove();
          leave = !sparams_.analyze_mode_;
          break;
        }

      case PostedCommand::ctFEN:
        {
          fromFEN(cmd.fen_.c_str());
          leave = !sparams_.analyze_mode_;
          break;
        }
      }

      posted_.pop();
    }
  }

  return ok;
}

bool Player::search(SearchResult * sres)
{
  if ( !sres )
    return false;

#ifdef USE_HASH
  hash_.inc();
#endif

  reset();

  // stash board to correctly print status later
  sres->board_   = scontexts_[0].board_;
  sdata_.board_ = scontexts_[0].board_;

  {
    FastGenerator mg(scontexts_[0].board_);
    for ( ;; )
    {
      const Move & move = mg.move();
      if ( !move )
        break;
      if ( scontexts_[0].board_.validateMove(move) )
        scontexts_[0].moves_[sdata_.numOfMoves_++] = move;
    }
    scontexts_[0].moves_[sdata_.numOfMoves_].clear();
  }


  const ScoreType alpha = -ScoreMax;
  const ScoreType betta = +ScoreMax;

  for (sdata_.depth_ = depth0_; !stopped() && sdata_.depth_ <= sparams_.depthMax_; ++sdata_.depth_)
  {
    scontexts_[0].plystack_[0].clearPV(sparams_.depthMax_);

    sdata_.restart();

    ScoreType score = alphaBetta0();

    if ( sdata_.best_ )
    {
      if (  stop_ && sdata_.depth_ > 2 &&
            (abs(score-sres->score_) >= Figure::figureWeight_[Figure::TypePawn]/2 ||
            sdata_.best_ != sres->best_ && abs(score-sres->score_) >= 5) &&
            callbacks_.giveTime_ &&
            !sparams_.analyze_mode_ )
      {
        int t_add = (callbacks_.giveTime_)();
        if ( t_add > 0 )
        {
          stop_ = false;
          sparams_.timeLimitMS_ += t_add;
          if ( sdata_.counter_ < sdata_.numOfMoves_ )
            sdata_.depth_--;
          continue;
        }
      }

      clock_t t  = clock();
      clock_t dt = (t - sdata_.tstart_) / 10;
      sdata_.tprev_ = t;

      sres->score_ = score;
      sres->best_  = sdata_.best_;
      sres->depth_ = sdata_.depth_;
      sres->nodesCount_ = sdata_.nodesCount_;
      sres->totalNodes_ = sdata_.totalNodes_;
      sres->plyMax_ = sdata_.plyMax_;
      sres->dt_ = dt;

      for (int i = 0; i < sdata_.depth_; ++i)
      {
        sres->pv_[i] = scontexts_[0].plystack_[0].pv_[i];
        if ( !sres->pv_[i] )
          break;
      }

      THROW_IF( sres->pv_[0] != sdata_.best_, "invalid PV found" );

      if ( callbacks_.sendOutput_ )
        (callbacks_.sendOutput_ )(sres);
    }
    // we haven't found move and spend more time for search it than on prev. iteration
    else if ( stop_ && sdata_.depth_ > 2 && callbacks_.giveTime_ && !sparams_.analyze_mode_ )
    {
      clock_t t  = clock();
      if ( (t - sdata_.tprev_) >= (sdata_.tprev_ - sdata_.tstart_) )
      {
        int t_add = (callbacks_.giveTime_)();
        if ( t_add > 0 )
        {
          stop_ = false;
          sparams_.timeLimitMS_ += t_add;
          sdata_.depth_--;
          continue;
        }
      }
    }

    if ( !sdata_.best_ ||
         ( (score >= Figure::MatScore-MaxPly || score <= MaxPly-Figure::MatScore) &&
            !sparams_.analyze_mode_ ) )
    {
      break;
    }
  }

  sres->totalNodes_ = sdata_.totalNodes_;

  clock_t t  = clock();
  sres->dt_ = (t - sdata_.tstart_) / 10;

  return sres->best_;
}

//////////////////////////////////////////////////////////////////////////
int Player::nextDepth(int ictx, int depth, const Move & move, bool pv) const
{
  if ( scontexts_[ictx].board_.underCheck() )
    return depth;

  depth--;

  if ( !move.see_good_ || !pv )
    return depth;

  if ( move.new_type_ == Figure::TypeQueen )
    return depth+1;

  if ( scontexts_[ictx].board_.halfmovesCount() > 1 )
  {
    const UndoInfo & prev = scontexts_[ictx].board_.undoInfoRev(-1);
    const UndoInfo & curr = scontexts_[ictx].board_.undoInfoRev(0);

    if ( move.capture_ && prev.to_ == curr.to_ || curr.en_passant_ == curr.to_ )
      return depth+1;
  }

  return depth;
}

//////////////////////////////////////////////////////////////////////////
ScoreType Player::alphaBetta0()
{
  if ( stopped() )
    return scontexts_[0].eval_(-ScoreMax, +ScoreMax);

  if ( sdata_.numOfMoves_ == 0 )
  {
    scontexts_[0].board_.setNoMoves();
    ScoreType score = scontexts_[0].eval_(-ScoreMax, +ScoreMax);
    return score;
  }

  ScoreType alpha = -ScoreMax;
  ScoreType betta = +ScoreMax;
  
  bool check_escape = scontexts_[0].board_.underCheck();
  bool null_move = false;

  for (sdata_.counter_ = 0; sdata_.counter_ < sdata_.numOfMoves_; ++sdata_.counter_)
  {
    if ( checkForStop() )
      break;

    Move & move = scontexts_[0].moves_[sdata_.counter_];
    ScoreType score = -ScoreMax;

    if ( scontexts_[0].board_.isMoveThreat(move) )
      move.threat_ = 1;
    
    scontexts_[0].board_.makeMove(move);
    sdata_.inc_nc();

    {
      int depth1 = nextDepth(0, sdata_.depth_, move, true);

      if ( sdata_.depth_ == depth0_ || !sdata_.counter_ ) // 1st iteration
        score = -alphaBetta(0, depth1, 1, -betta, -alpha, true, null_move);
      else
      {
        int depth2 = nextDepth(0, sdata_.depth_, move, false);

        int R = 0;

        //int s_limit = alpha > 0 ? (((int)alpha) >> 1) : (((int)alpha) << 1);
        //int s = (int)move.vsort_ - ScoreMax;


#ifdef USE_LMR
        if ( !check_escape &&
             //(sdata_.counter_ > 2 || s < s_limit) &&
             sdata_.depth_ > LMR_MinDepthLimit &&
             alpha > -Figure::MatScore-MaxPly && 
             scontexts_[0].board_.canBeReduced() )
        {
          R = 1;
        }
#endif

        score = -alphaBetta(0, depth2-R, 1, -alpha-1, -alpha, false, null_move);

        if ( !stopped() && score > alpha && R > 0 )
          score = -alphaBetta(0, depth2, 1, -alpha-1, -alpha, false, null_move);

        if ( !stopped() && score > alpha )
          score = -alphaBetta(0, depth1, 1, -betta, -alpha, true, null_move);
      }
    }

    scontexts_[0].board_.unmakeMove();

    if ( !stopped() )
    {
      move.vsort_ = score + ScoreMax;

      if ( score > alpha )
      {
        sdata_.best_ = move;
        alpha = score;

        assemblePV(0, move, scontexts_[0].board_.underCheck(), 0);

        // bring best move to front, shift other moves 1 position right
        if ( sdata_.depth_ > depth0_ )
        {
          for (int j = sdata_.counter_; j > 0; --j)
            scontexts_[0].moves_[j] = scontexts_[0].moves_[j-1];
          scontexts_[0].moves_[0] = sdata_.best_;
        }
      }
    }
  }

  // don't need to continue
  if ( !stopped() && sdata_.counter_ == 1 && !sparams_.analyze_mode_ )
    pleaseStop();

  if ( !stopped() )
  {
    // full sort only on 1st iteration
    if ( sdata_.depth_ == depth0_ )
      std::sort(scontexts_[0].moves_, scontexts_[0].moves_ + sdata_.numOfMoves_);
    // then sort all moves but 1st
    else if ( sdata_.numOfMoves_ > 2 )
      std::sort(scontexts_[0].moves_+1, scontexts_[0].moves_ + sdata_.numOfMoves_);
  }

  return alpha;
}

//////////////////////////////////////////////////////////////////////////
ScoreType Player::alphaBetta(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, bool null_move)
{
  if ( alpha >= Figure::MatScore-ply )
    return alpha;

  if ( ply < MaxPly-1 )
    scontexts_[ictx].plystack_[ply].clear(ply);

  if ( scontexts_[ictx].board_.drawState() || scontexts_[ictx].board_.countReps() > 1 )
    return Figure::DrawScore;

  if ( stopped() || ply >= MaxPly )
    return scontexts_[ictx].eval_(alpha, betta);

  ScoreType alpha0 = alpha;

  Move hmove(0);

#ifdef USE_HASH
  ScoreType hscore = -ScoreMax;
  GHashTable::Flag flag = getHash(ictx, depth, ply, alpha, betta, hmove, hscore, pv);
  if ( flag == GHashTable::Alpha || flag == GHashTable::Betta )
    return hscore;
#endif

  if ( depth <= 0 )
    return captures(ictx, depth, ply, alpha, betta, pv);

  bool nm_threat = false, real_threat = false;

#ifdef USE_NULL_MOVE
  if ( !pv &&
       //!null_move &&
       !scontexts_[ictx].board_.underCheck() &&
        scontexts_[ictx].board_.allowNullMove() &&
        depth >= scontexts_[ictx].board_.nullMoveDepthMin() &&
        betta < Figure::MatScore+MaxPly &&
        betta > -Figure::MatScore-MaxPly )
  {
    int null_depth = depth - scontexts_[ictx].board_.nullMoveReduce();//scontexts_[ictx].board_.nullMoveDepth(depth);//
    if ( null_depth < 0 )
      null_depth = 0;

    scontexts_[ictx].board_.makeNullMove();

    ScoreType nullScore = -alphaBetta(ictx, null_depth, ply+1, -betta, -(betta-1), false, true);

    scontexts_[ictx].board_.unmakeNullMove();

    // verify null-move with shortened depth
    if ( nullScore >= betta )
    {
      depth = null_depth;
      //depth -= scontexts_[ictx].board_.nullMoveReduce();
      if ( depth <= 0 )
        return captures(ictx, depth, ply, alpha, betta, pv);
      null_move = true;
    }
    else // may be we are in danger?
      nm_threat = true;
  }
#endif


#ifdef USE_FUTILITY_PRUNING
  if ( !pv &&
       !scontexts_[ictx].board_.underCheck() &&
       !scontexts_[ictx].board_.isWinnerLoser() &&
        alpha > -Figure::MatScore+MaxPly &&
        alpha < Figure::MatScore-MaxPly &&
        depth == 1 && ply > 1 )
  {
    ScoreType score0 = scontexts_[ictx].eval_(alpha, betta);
    int delta = (int)alpha - (int)score0 - (int)Evaluator::positionGain_;
    if ( delta > 0 )
      return captures(ictx, depth, ply, alpha, betta, pv, score0);
  }
#endif

  int counter = 0;
  ScoreType scoreBest = -ScoreMax;
  Move best(0);

  Move killer(0);
  scontexts_[ictx].board_.extractKiller(scontexts_[ictx].plystack_[ply].killer_, hmove, killer);

  FastGenerator fg(scontexts_[ictx].board_, hmove, killer);

  if ( fg.singleReply() )
    depth++;

  UndoInfo & prev = scontexts_[ictx].board_.undoInfoRev(0);
  bool check_escape = scontexts_[ictx].board_.underCheck();

  for ( ; alpha < betta && !checkForStop(); )
  {
    Move & move = fg.move();
    if ( !move )
      break;

    if ( !scontexts_[ictx].board_.validateMove(move) )
      continue;

    ScoreType score = -ScoreMax;

    // detect some threat, like pawn's attack etc...
    // don't LMR such moves
    if ( scontexts_[ictx].board_.isMoveThreat(move) )
      move.threat_ = 1;

    scontexts_[ictx].board_.makeMove(move);
    sdata_.inc_nc();

    //findSequence(ictx, move, ply, depth, counter, alpha, betta);

    UndoInfo & curr = scontexts_[ictx].board_.undoInfoRev(0);

    {
      int depth1 = nextDepth(ictx, depth, move, pv);

      if ( !counter )
        score = -alphaBetta(ictx, depth1, ply+1, -betta, -alpha, pv, null_move);
      else
      {
        int depth2 = nextDepth(ictx, depth, move, false);
        int R = 0;

#ifdef USE_LMR
        if ( !check_escape &&
//             counter > 2 &&
             sdata_.depth_ > LMR_MinDepthLimit &&
             depth > LMR_DepthLimit &&
             alpha > -Figure::MatScore-MaxPly &&             
             scontexts_[ictx].board_.canBeReduced() )
        {
          R = 1;
          curr.reduced_ = true;
        }
#endif

        score = -alphaBetta(ictx, depth2-R, ply+1, -alpha-1, -alpha, false, null_move);
        curr.reduced_ = false;

        if ( !stopped() && score > alpha && R > 0 )
          score = -alphaBetta(ictx, depth2, ply+1, -alpha-1, -alpha, false, null_move);

        if ( !stopped() && score > alpha && score < betta )
          score = -alphaBetta(ictx, depth1, ply+1, -betta, -alpha, pv, null_move);
      }
    }

    scontexts_[ictx].board_.unmakeMove();

    scontexts_[ictx].plystack_[ply].setKiller(move);

    if ( !stopped() && score > scoreBest )
    {
      best = move;
      scoreBest = score;
      if ( score > alpha )
      {
        alpha = score;
        if ( pv )
          assemblePV(ictx, move, scontexts_[ictx].board_.underCheck(), ply);
      }
    }

    // should be increased here to consider invalid moves!!!
     ++counter;
  }

  if ( stopped() )
    return scoreBest;

  if ( !counter )
  {
    scontexts_[ictx].board_.setNoMoves();
    
    scoreBest = scontexts_[ictx].eval_(alpha, betta);

    if ( scontexts_[ictx].board_.matState() )
      scoreBest += ply;
  }


  if ( best )
  {
    History & hist = MovesGenerator::history(best.from_, best.to_);
    hist.inc_score(depth);

#if ((defined USE_LMR) && (defined VERIFY_LMR))
    // have to recalculate with full depth, or indicate threat in hash
    if ( !stopped() && alpha >= betta && nm_threat && isRealThreat(ictx, best) )
    {
      if ( prev.reduced_ )
        return betta-1;
      else
        real_threat = true;
    }
#endif
  }

#ifdef USE_HASH
  putHash(ictx, best, alpha0, betta, scoreBest, depth, ply, real_threat);
#endif

  THROW_IF( scoreBest < -Figure::MatScore || scoreBest > +Figure::MatScore, "invalid score" );

  return scoreBest;
}

//////////////////////////////////////////////////////////////////////////
ScoreType Player::captures(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, ScoreType score0)
{
  if ( alpha >= Figure::MatScore-ply )
    return alpha;

  if ( scontexts_[ictx].board_.drawState() || scontexts_[ictx].board_.countReps() > 1 )
    return Figure::DrawScore;

  // not initialized yet
  if ( score0 == -ScoreMax )
    score0 = scontexts_[ictx].eval_(alpha, betta);

  if ( stopped() || ply >= MaxPly )
    return score0;

  if ( !scontexts_[ictx].board_.underCheck() && score0 >= betta )
    return score0;

  int counter = 0;
  ScoreType scoreBest = -ScoreMax;
  int delta = 0;

  if ( !scontexts_[ictx].board_.underCheck() )
  {
    delta = (int)alpha - (int)score0 - (int)Evaluator::positionGain_;
    if ( score0 > alpha )
      alpha = score0;

    scoreBest = score0;
  }

  Move best(0);
  Figure::Type thresholdType = scontexts_[ictx].board_.isWinnerLoser() ? Figure::TypePawn : delta2type(delta);

  TacticalGenerator tg(scontexts_[ictx].board_, thresholdType, depth);
  if ( tg.singleReply() )
    depth++;

  for ( ; alpha < betta && !checkForStop(); )
  {
    Move & move = tg.next();
    if ( !move )
      break;

    if ( !scontexts_[ictx].board_.validateMove(move) )
      continue;

    ScoreType score = -ScoreMax;
    
    scontexts_[ictx].board_.makeMove(move);
    sdata_.inc_nc();

    {
      int depth1 = nextDepth(ictx, depth, move, false);
      score = -captures(ictx, depth1, ply+1, -betta, -alpha, pv, -ScoreMax);
    }

    scontexts_[ictx].board_.unmakeMove();

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
    if ( scontexts_[ictx].board_.underCheck() )
      scoreBest = -Figure::MatScore+ply;
    else
      scoreBest = score0;
  }

  THROW_IF( scoreBest < -Figure::MatScore || scoreBest > +Figure::MatScore, "invalid score" );

  return scoreBest;
}


/// extract data from hash table
#ifdef USE_HASH
GHashTable::Flag Player::getHash(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, Move & hmove, ScoreType & hscore, bool pv)
{
  const HItem * hitem = hash_.find(scontexts_[ictx].board_.hashCode());
  if ( !hitem )
    return GHashTable::NoFlag;

  THROW_IF( hitem->hcode_ != scontexts_[ictx].board_.hashCode(), "invalid hash item found" );

  scontexts_[ictx].board_.unpackMove(hitem->move_, hmove);

  if ( pv || hitem->mode_ != GHashTable::General )
    return GHashTable::AlphaBetta;

  hscore = hitem->score_;
  if ( hscore >= Figure::MatScore-MaxPly )
    hscore = hscore - ply;
  else if ( hscore <= MaxPly-Figure::MatScore )
    hscore = hscore + ply;

  THROW_IF(hscore > 32760 || hscore < -32760, "invalid value in hash");

  if ( (int)hitem->depth_ >= depth && ply > 0 )
  {
    if ( GHashTable::Alpha == hitem->flag_ && hscore <= alpha )
    {
      THROW_IF( !stop_ && alpha < -32760, "invalid hscore" );
      return GHashTable::Alpha;
    }

    if ( hitem->flag_ > GHashTable::Alpha && hscore >= betta && hmove )
    {
      if ( scontexts_[ictx].board_.calculateReps(hmove) < 2 )
      {
        if ( pv )
          assemblePV(ictx, hmove, false, ply);

        /// danger move was reduced - recalculate it with full depth
        const UndoInfo & prev = scontexts_[ictx].board_.undoInfoRev(0);
        if ( hitem->threat_ && prev.reduced_ )
        {
          hscore = betta-1;
          return GHashTable::Alpha;
        }

        return GHashTable::Betta;
      }
    }
  }

  return GHashTable::AlphaBetta;
}

/// insert data to hash table
void Player::putHash(int ictx, const Move & move, ScoreType alpha, ScoreType betta, ScoreType score, int depth, int ply, bool threat)
{
  if ( scontexts_[ictx].board_.repsCount() >= 3 )
    return;

  PackedMove pm = scontexts_[ictx].board_.packMove(move);
  GHashTable::Flag flag = GHashTable::NoFlag;
  if ( scontexts_[ictx].board_.repsCount() < 2 )
  {
    if ( score <= alpha || !move )
      flag = GHashTable::Alpha;
    else if ( score >= betta )
      flag = GHashTable::Betta;
    else
      flag = GHashTable::AlphaBetta;
  }
  if ( score >= +Figure::MatScore-MaxPly )
    score += ply;
  else if ( score <= -Figure::MatScore+MaxPly )
    score -= ply;
  hash_.push(scontexts_[ictx].board_.hashCode(), score, depth, flag, pm, threat);
}
#endif


//////////////////////////////////////////////////////////////////////////
// is given movement caused by previous? this mean that if we don't do this move we loose
// we actually test if moved figure was/will be attacked by previously moved one or from direction it was moved from
// or we should move our king even if we have a lot of other figures
//////////////////////////////////////////////////////////////////////////
bool Player::isRealThreat(int ictx, const Move & move)
{
  // don't need to forbid if our answer is capture or check ???
  if ( move.capture_ || move.checkFlag_ )
    return false;

  const UndoInfo & prev = scontexts_[ictx].board_.undoInfoRev(0);
  if ( !prev ) // null-move
    return false;

  Figure::Color  color = scontexts_[ictx].board_.getColor();
  Figure::Color ocolor = Figure::otherColor(color);

  const Field & pfield = scontexts_[ictx].board_.getField(prev.to_);
  THROW_IF( !pfield || pfield.color() != ocolor, "no figure of required color on the field it was move to while detecting threat" );

  // don't need forbid reduction of captures, checks, promotions and pawn's attack because we've already done it
  if ( prev.capture_ || prev.new_type_ > 0 || prev.checkingNum_ > 0 || prev.threat_ )
    return false;

  /// we have to move king even if there a lot of figures
  if ( scontexts_[ictx].board_.getField(move.from_).type() == Figure::TypeKing &&
       scontexts_[ictx].board_.fmgr().queens(color) > 0 &&
       scontexts_[ictx].board_.fmgr().rooks(color) > 0 &&
       scontexts_[ictx].board_.fmgr().knights(color) + scontexts_[ictx].board_.fmgr().bishops(color) > 0 )
  {
    return true;
  }

  const Field & cfield = scontexts_[ictx].board_.getField(move.from_);
  THROW_IF( !cfield || cfield.color() != color, "no figure of required color in while detecting threat" );

  // we have to put figure under attack
  if ( scontexts_[ictx].board_.ptAttackedBy(move.to_, prev.to_) )
    return true;

  // put our figure under attack, opened by prev movement
  if ( scontexts_[ictx].board_.getAttackedFrom(ocolor, move.to_, prev.from_) >= 0 )
    return true;

  // prev move was attack, and we should escape from it
  if ( scontexts_[ictx].board_.ptAttackedBy(move.from_, prev.to_) )
    return true;

  // our figure was attacked from direction, opened by prev movement
  if ( scontexts_[ictx].board_.getAttackedFrom(ocolor, move.from_, prev.from_) >= 0 )
    return true;

  return false;
}
