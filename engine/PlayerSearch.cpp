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
          board_.unmakeMove();
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
  sres->board_   = board_;
  sdata_.board_ = board_;

  {
    FastGenerator mg(board_);
    for ( ;; )
    {
      const Move & move = mg.move();
      if ( !move )
        break;
      if ( board_.validateMove(move) )
        moves_[sdata_.numOfMoves_++] = move;
    }
    moves_[sdata_.numOfMoves_].clear();
  }


  const ScoreType alpha = -ScoreMax;
  const ScoreType betta = +ScoreMax;

  for (sdata_.depth_ = depth0_; !stopped() && sdata_.depth_ <= sparams_.depthMax_; ++sdata_.depth_)
  {
    plystack_[0].clearPV(sparams_.depthMax_);

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
        sres->pv_[i] = plystack_[0].pv_[i];
        if ( !sres->pv_[i] )
          break;
      }

      THROW_IF( sres->pv_[0] != sdata_.best_, "invalid PV found" );

      if ( callbacks_.sendOutput_ )
        (callbacks_.sendOutput_ )(sres);
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
int Player::nextDepth(int depth, const Move & move, bool pv) const
{
  if ( board_.underCheck() )
    return depth;

  depth--;

  if ( !move.see_good_ || !pv )
    return depth;

  if ( move.new_type_ == Figure::TypeQueen )
    return depth+1;

  if ( board_.halfmovesCount() > 1 )
  {
    const UndoInfo & prev = board_.undoInfoRev(-1);
    const UndoInfo & curr = board_.undoInfoRev(0);

    if ( move.capture_ && prev.to_ == curr.to_ || curr.en_passant_ == curr.to_ )
      return depth+1;
  }

  return depth;
}

//////////////////////////////////////////////////////////////////////////
ScoreType Player::alphaBetta0()
{
  if ( stopped() )
    return board_.evaluate();

  if ( sdata_.numOfMoves_ == 0 )
  {
    board_.setNoMoves();
    ScoreType score = board_.evaluate();
    return score;
  }

  ScoreType alpha = -std::numeric_limits<ScoreType>::max();
  ScoreType betta = +std::numeric_limits<ScoreType>::max();

  ScoreType scoreBest = -ScoreMax;
  
  bool check_escape = board_.underCheck();

  for (sdata_.counter_ = 0; sdata_.counter_ < sdata_.numOfMoves_; ++sdata_.counter_)
  {
    if ( checkForStop() )
      break;

    Move & move = moves_[sdata_.counter_];
    ScoreType score = -ScoreMax;

    if ( board_.isMoveThreat(move) )
      move.threat_ = 1;
    
    board_.makeMove(move);
    sdata_.inc_nc();

    {
      int depth1 = nextDepth(sdata_.depth_, move, true);

      if ( sdata_.depth_ == depth0_ || !sdata_.counter_ ) // 1st iteration
        score = -alphaBetta(depth1, 1, -betta, -alpha, true);
      else
      {
        int depth2 = nextDepth(sdata_.depth_, move, false);

        int R = 0;

#ifdef USE_LMR
        if ( !check_escape &&
             sdata_.depth_ > LMR_MinDepthLimit &&
             alpha > -Figure::MatScore-MaxPly && 
             board_.canBeReduced() )
        {
          R = 1;
        }
#endif

        score = -alphaBetta(depth2-R, 1, -alpha-1, -alpha, false);

        if ( !stopped() && score > alpha && R > 0 )
          score = -alphaBetta(depth2, 1, -alpha-1, -alpha, false);

        if ( !stopped() && score > alpha )
          score = -alphaBetta(depth1, 1, -betta, -alpha, true);
      }
    }

    board_.unmakeMove();

    move.vsort_ = score + ScoreMax;

    if ( !stopped() && score > scoreBest )
    {
      scoreBest = score;

      if ( score > alpha )
      {
        sdata_.best_ = move;
        alpha = score;
        assemblePV(move, board_.underCheck(), 0);

        // bring best move to front, shift other moves 1 position right
        if ( sdata_.depth_ > depth0_ )
        {
          Move mv = moves_[sdata_.counter_];
          for (int j = sdata_.counter_; j > 0; --j)
            moves_[j] = moves_[j-1];
          moves_[0] = mv;
        }
      }
    }
  }

  // don't need to continue
  if ( !stopped() && sdata_.counter_ == 1 && !sparams_.analyze_mode_ )
    pleaseStop();

  // sort only on 1st iteration
  if ( sdata_.depth_ == depth0_ )
    std::sort(moves_, moves_ + sdata_.numOfMoves_);

  return scoreBest;
}

//////////////////////////////////////////////////////////////////////////
ScoreType Player::alphaBetta(int depth, int ply, ScoreType alpha, ScoreType betta, bool pv)
{
  if ( alpha >= Figure::MatScore-ply )
    return alpha;

  if ( board_.drawState() || board_.countReps() > 1 )
  {
    // clear PV-string, because no move found
    if ( pv )
      plystack_[ply].pv_[ply].clear();

    return Figure::DrawScore;
  }

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
    return captures(depth, ply, alpha, betta, pv);

  if ( ply < MaxPly-1 )
    plystack_[ply].clear(ply);

  bool nm_threat = false, real_threat = false;

#ifdef USE_NULL_MOVE
  if ( !pv &&
       !board_.underCheck() &&
        board_.allowNullMove() &&
        depth >= board_.nullMoveDepthMin() &&
        betta < Figure::MatScore+MaxPly &&
        betta > -Figure::MatScore-MaxPly )
  {
    board_.makeNullMove();

    int null_depth = depth - board_.nullMoveReduce();

    ScoreType nullScore = -alphaBetta(null_depth, ply+1, -betta, -(betta-1), false);

    board_.unmakeNullMove();

    // verify null-move with shortened depth
    if ( nullScore >= betta )
    {
      depth -= board_.nullMoveReduce();
      if ( depth <= 0 )
        return captures(depth, ply, alpha, betta, pv);
    }
    else // may be we are in danger?
      nm_threat = true;
  }
#endif


#ifdef USE_FUTILITY_PRUNING
  if ( !pv &&
       !board_.underCheck() &&
       !board_.isWinnerLoser() &&
        alpha > -Figure::MatScore+MaxPly &&
        alpha < Figure::MatScore-MaxPly &&
        depth == 1 && ply > 1 )
  {
    ScoreType score0 = board_.evaluate();
    int delta = (int)alpha - (int)score0 - (int)Evaluator::positionGain_;
    if ( delta > 0 )
      return captures(depth, ply, alpha, betta, pv, score0);
  }
#endif

  int counter = 0;
  ScoreType scoreBest = -ScoreMax;
  Move best(0);

  Move killer(0);
  board_.extractKiller(plystack_[ply].killer_, hmove, killer);

  FastGenerator fg(board_, hmove, killer);

  if ( fg.singleReply() )
    depth++;

  UndoInfo & prev = board_.undoInfoRev(0);
  bool check_escape = board_.underCheck();

  for ( ; alpha < betta && !checkForStop(); )
  {
    Move & move = fg.move();
    if ( !move )
      break;

    if ( !board_.validateMove(move) )
      continue;

    ScoreType score = -ScoreMax;

    // detect some threat, like pawn's attack etc...
    // don't LMR such moves
    if ( board_.isMoveThreat(move) )
      move.threat_ = 1;

    board_.makeMove(move);
    sdata_.inc_nc();

    //findSequence(move, ply, depth, counter, alpha, betta);

    UndoInfo & curr = board_.undoInfoRev(0);

    {
      int depth1 = nextDepth(depth, move, pv);

      if ( !counter )
        score = -alphaBetta(depth1, ply+1, -betta, -alpha, pv);
      else
      {
        int depth2 = nextDepth(depth, move, false);
        int R = 0;

#ifdef USE_LMR
        if ( !check_escape &&
             sdata_.depth_ > LMR_MinDepthLimit &&
             depth > LMR_DepthLimit &&
             alpha > -Figure::MatScore-MaxPly &&             
             board_.canBeReduced() )
        {
          R = 1;
          curr.reduced_ = true;
        }
#endif

        score = -alphaBetta(depth2-R, ply+1, -alpha-1, -alpha, false);
        curr.reduced_ = false;

        if ( !stopped() && score > alpha && R > 0 )
          score = -alphaBetta(depth2, ply+1, -alpha-1, -alpha, false);

        if ( !stopped() && score > alpha && score < betta )
          score = -alphaBetta(depth1, ply+1, -betta, -alpha, true);
      }
    }

    board_.unmakeMove();

    plystack_[ply].setKiller(move);

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

#if ((defined USE_LMR) && (defined VERIFY_LMR))
    // have to recalculate with full depth, or indicate threat in hash
    if ( !stopped() && alpha >= betta && nm_threat && isRealThreat(best) )
    {
      if ( prev.reduced_ )
        return betta-1;
      else
        real_threat = true;
    }
#endif
  }

#ifdef USE_HASH
  putHash(best, alpha0, betta, scoreBest, depth, ply, real_threat);
#endif

  THROW_IF( scoreBest < -Figure::MatScore || scoreBest > +Figure::MatScore, "invalid score" );

  return scoreBest;
}

//////////////////////////////////////////////////////////////////////////
ScoreType Player::captures(int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, ScoreType score0)
{
  if ( alpha >= Figure::MatScore-ply )
    return alpha;

  if ( board_.drawState() || board_.countReps() > 1 )
    return Figure::DrawScore;

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
    delta = (int)alpha - (int)score0 - (int)Evaluator::positionGain_;
    if ( score0 > alpha )
      alpha = score0;

    scoreBest = score0;
  }

  Move best(0);
  Figure::Type thresholdType = board_.isWinnerLoser() ? Figure::TypePawn : delta2type(delta);

  TacticalGenerator tg(board_, thresholdType, depth);
  if ( tg.singleReply() )
    depth++;

  for ( ; alpha < betta && !checkForStop(); )
  {
    Move & move = tg.next();
    if ( !move )
      break;

    if ( !board_.validateMove(move) )
      continue;

    ScoreType score = -ScoreMax;
    
    board_.makeMove(move);
    sdata_.inc_nc();

    {
      int depth1 = nextDepth(depth, move, false);
      score = -captures(depth1, ply+1, -betta, -alpha, pv, -ScoreMax);
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
      scoreBest = -Figure::MatScore+ply;
    else
      scoreBest = score0;
  }

  THROW_IF( scoreBest < -Figure::MatScore || scoreBest > +Figure::MatScore, "invalid score" );

  return scoreBest;
}


/// extract data from hash table
#ifdef USE_HASH
GHashTable::Flag Player::getHash(int depth, int ply, ScoreType alpha, ScoreType betta, Move & hmove, ScoreType & hscore, bool pv)
{
  const HItem * hitem = hash_.find(board_.hashCode());
  if ( !hitem )
    return GHashTable::NoFlag;

  THROW_IF( hitem->hcode_ != board_.hashCode(), "invalid hash item found" );

  board_.unpack(hitem->move_, hmove);

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
      
      // clear PV-string
      if ( pv )
        plystack_[ply].pv_[ply].clear();

      return GHashTable::Alpha;
    }

    if ( hitem->flag_ > GHashTable::Alpha && hscore >= betta && hmove )
    {
      if ( board_.calculateReps(hmove) < 2 )
      {
        if ( pv )
          assemblePV(hmove, false, ply);

        /// danger move was reduced - recalculate it with full depth
        const UndoInfo & prev = board_.undoInfoRev(0);
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
void Player::putHash(const Move & move, ScoreType alpha, ScoreType betta, ScoreType score, int depth, int ply, bool threat)
{
  if ( board_.repsCount() >= 3 )
    return;

  PackedMove pm = board_.pack(move);
  GHashTable::Flag flag = GHashTable::NoFlag;
  if ( board_.repsCount() < 2 )
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
  hash_.push(board_.hashCode(), score, depth, flag, pm, threat);
}
#endif


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

  const UndoInfo & prev = board_.undoInfoRev(0);
  if ( !prev ) // null-move
    return false;

  Figure::Color  color = board_.getColor();
  Figure::Color ocolor = Figure::otherColor(color);

  const Field & pfield = board_.getField(prev.to_);
  THROW_IF( !pfield || pfield.color() != ocolor, "no figure of required color on the field it was move to while detecting threat" );

  // don't need forbid reduction of captures, checks, promotions and pawn's attack because we've already done it
  if ( prev.capture_ || prev.new_type_ > 0 || prev.checkingNum_ > 0 || prev.threat_ )
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
