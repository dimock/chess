/*************************************************************
FastGenerator.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
*************************************************************/

#include "MovesGenerator.h"
#include "MovesTable.h"

//////////////////////////////////////////////////////////////////////////

FastGenerator::FastGenerator(Board & board, const Move & hmove, const Move & killer) :
  cg_(board), ug_(board), eg_(board), hmove_(hmove), killer_(killer), order_(oHash),
  board_(board), weakN_(0)
{
  fake_.clear();
  weak_[0].clear();

  if ( board_.underCheck() )
  {
    order_ = oEscapes;
    eg_.generate(hmove);
  }
}

Move & FastGenerator::move()
{
  if ( order_ == oHash )
  {
    order_ = oGenCaps;
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
    order_ = oCaps;
  }

  if ( order_ == oCaps )
  {
    for ( ;; )
    {
      Move * move = cg_.moves() + cg_.count();
      Move * mv = cg_.moves();
      for ( ; *mv; ++mv)
      {
        if ( mv->alreadyDone_ || mv->vsort_ <= move->vsort_ )
          continue;

        move = mv;
      }
      if ( !*move )
        break;


      if ( board_.see(*move) < 0 )
      {
        weak_[weakN_++] = *move;
        move->alreadyDone_ = 1;
        continue;
      }

      move->alreadyDone_ = 1;
      move->recapture_ = 1;
      return *move;
    }

    weak_[weakN_].clear();
    order_ = oKiller;
  }

  if ( order_ == oKiller )
  {
    order_ = oGenUsual;
    if ( killer_ )
      return killer_;
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

  if ( order_ == oWeak )
  {
    for ( ;; )
    {
      Move * move = weak_ + weakN_;
      Move * mv = weak_;
      for ( ; *mv; ++mv)
      {
        if ( mv->alreadyDone_ || mv->vsort_ <= move->vsort_ )
          continue;

        move = mv;
      }
      if ( !*move )
        break;

      move->alreadyDone_ = 1;
      return *move;
    }
  }

  return fake_;
}


//////////////////////////////////////////////////////////////////////////
QuiesGenerator::QuiesGenerator(const Move & hmove, Board & board, Figure::Type minimalType, int depth) :
  eg_(board), cg_(board), chg_(board), minimalType_(minimalType), hmove_(hmove), board_(board), order_(oHash), depth_(depth), fake_(0)
{
  if ( board_.underCheck() )
  {
    eg_.generate(hmove);
    order_ = oEscape;
  }
}

bool QuiesGenerator::singleReply() const
{
  return eg_.count() == 1;
}

Move & QuiesGenerator::next()
{
  if ( order_ == oHash )
  {
    order_ = oGenCaps;
    if ( hmove_ )
      return hmove_;
  }

  if ( order_ == oEscape )
  {
    return eg_.escape();
  }

  if ( order_ == oGenCaps )
  {
    cg_.generate(hmove_, Figure::TypePawn);
    order_ = oCaps;
  }

  if ( order_ == oCaps )
  {
    Move & cap = cg_.capture();
    if ( cap || depth_ < 0 )
      return cap;

    order_ = oGenChecks;
  }

  if ( order_ == oGenChecks )
  {
    chg_.generate(hmove_, Figure::TypePawn);
    order_ = oChecks;
  }

  if ( order_ == oChecks )
  {
    return chg_.check();
  }

  return fake_;
}
