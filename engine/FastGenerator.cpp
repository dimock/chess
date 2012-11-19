/*************************************************************
  FastGenerator.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "MovesGenerator.h"
#include "MovesTable.h"

//////////////////////////////////////////////////////////////////////////

FastGenerator::FastGenerator(Board & board, const Move & hmove, const Move & killer) :
  cg_(board), ug_(board), eg_(board), hmove_(hmove), killer_(killer), order_(oHash), board_(board), numCaps_(0), numWeak_(0)
{
  fake_.clear();

  if ( board_.underCheck() )
    eg_.generate(hmove);
}

Move & FastGenerator::move()
{
  if ( order_ == oHash )
  {
    order_ = board_.underCheck() ? oEscapes : oGenCaps;
    if ( hmove_ )
      return hmove_;
  }

  if ( order_ == oEscapes )
  {
    return eg_.escape();
  }

  if ( order_ == oGenCaps )
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

    order_ = numCaps_ > 0 ? oCaps : oGenUsual;
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

    order_ = oGenUsual;
  }

  if ( order_ == oGenUsual )
  {
    ug_.generate(hmove_, killer_);
    order_ = oUsual;
  }

  if ( order_ == oUsual )
  {
    Move & move = ug_.move();
    if ( move )
      return move;

    order_ = oWeak;
  }

  if ( order_ == oWeak && numWeak_ > 0)
    return weak_[--numWeak_];

  return fake_;
}
