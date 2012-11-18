/*************************************************************
  MovesGenerator.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "MovesGenerator.h"
#include "MovesTable.h"

//////////////////////////////////////////////////////////////////////////

FastGenerator::FastGenerator(Board & board, const Move & hmove, const Move & killer) :
  cg_(board), ug_(board), hmove_(hmove), killer_(killer), order_(oHash), board_(board), numCaps_(0), numWeak_(0)
{
  fake_.clear();
}

Move & FastGenerator::move()
{
  if ( order_ == oHash )
  {
    order_ = oStart;
    return hmove_;
  }

  if ( order_ == oStart )
  {
    cg_.generate(hmove_, Figure::TypePawn);

    for (int i = 0; i < cg_.count(); ++i)
    {
      const Move & move = cg_[i];
      if ( board_.see(move) >= 0 )
      {
        caps_[numCaps_] = move;
        caps_[numCaps_].recapture_ = 1;
        numCaps_++;
      }
      else
        weak_[numWeak_++] = move;
    }

    caps_[numCaps_].clear();
    weak_[numWeak_].clear();

    if ( numCaps_ > 0 )
      order_ = oCaps;
    else
      order_ = oUsual;
  }

  if ( order_ == oCaps )
  {
    for ( ;; )
    {
      Move * move = caps_ + numCaps_;
      Move * mv = caps_;
      for ( ; *mv; ++mv)
      {
        if ( mv->alreadyDone_ || mv->vsort_ < move->vsort_ )
          continue;

        move = mv;
      }
      if ( !*move )
        break;

      move->alreadyDone_ = 1;
      return *move;
    }

    ug_.generate(killer_);
    order_ = oUsual;
  }

  if ( order_ == oUsual )
  {
    Move & move = ug_.move();
    if ( move )
      return move;

    order_ = oWeak;
  }

  if ( order_ == oWeak )
  {
    if ( numWeak_ <= 0 )
      return fake_;

    return weak_[numWeak_--];
  }

  return weak_[0];
}
