/*************************************************************
FastGenerator.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
*************************************************************/

#include "MovesGenerator.h"
#include "MovesTable.h"

//////////////////////////////////////////////////////////////////////////
FastGenerator::FastGenerator(Board & board) :
  cg_(board), ug_(board), eg_(board), hmove_(0), killer_(0), order_(oHash),
  board_(board), weakN_(0)
{
  fake_.clear();
  weak_[0].clear();

  if ( board_.underCheck() )
  {
    order_ = oEscapes;
    eg_.generate(hmove_);
  }
}

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

void FastGenerator::restart()
{
  if ( board_.underCheck() )
  {
    eg_.restart();
    order_ = oEscapes;
    return;
  }

  order_ = oHash;
  weak_[0].clear();
  weakN_ = 0;

  cg_.restart();
  ug_.restart();
}

Move & FastGenerator::move()
{
  if ( order_ == oHash )
  {
    order_ = oGenCaps;
    if ( hmove_ )
    {
      hmove_.see_good_ = (board_.see(hmove_) >= 0);
      return hmove_;
    }
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


      if ( !move->seen_ )
      {
        move->see_good_ = board_.see(*move) >= 0;
        move->seen_ = 1;
      }

      move->alreadyDone_ = 1;

      if ( !move->see_good_ )
      {
        weak_[weakN_++] = *move;
        continue;
      }

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
TacticalGenerator::TacticalGenerator(Board & board, Figure::Type thresholdType, int depth) :
  eg_(board), cg_(board), chg_(board), thresholdType_(thresholdType), board_(board),
  order_(oGenCaps), depth_(depth), fake_(0)
{
  if ( board_.underCheck() )
  {
    Move hmove(0);
    eg_.generate(hmove);
    order_ = oEscape;
  }
}

Move & TacticalGenerator::next()
{
  if ( order_ == oEscape )
  {
    return eg_.escape();
  }

  if ( order_ == oGenCaps )
  {
    Move hmove(0);
    cg_.generate(hmove, thresholdType_);
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
    chg_.generate();
    order_ = oChecks;
  }

  if ( order_ == oChecks )
  {
    return chg_.check();
  }

  return fake_;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
FutilityGenerator::FutilityGenerator(Board & board, const Move & hmove, Figure::Type thresholdType) :
  eg_(board), cg_(board), chg_(board), ug_(board), thresholdType_(thresholdType), board_(board),
  order_(oHash), fake_(0), counter_(0), hmove_(0)
{
  if ( board_.underCheck() )
  {
    eg_.generate(hmove);
    order_ = oEscape;
  }
}

Move & FutilityGenerator::next()
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
    cg_.generate(hmove_, thresholdType_);
    order_ = oCaps;
  }

  if ( order_ == oCaps )
  {
    Move & cap = cg_.capture();
    if ( cap )
      return cap;

    order_ = oGenChecks;
  }

  if ( order_ == oGenChecks )
  {
    chg_.generate(hmove_);
    order_ = oChecks;
  }

  if ( order_ == oChecks )
  {
    Move & move = chg_.check();
    if ( move )
      return move;

    order_ = oGenUsual;
  }

  if ( order_ == oGenUsual && counter_ == 0 )
  {
    Move killer(0);
    ug_.generate(hmove_, killer);
    order_ = oUsual;
  }

  if ( order_ == oUsual && counter_ == 0 )
  {
    Move & move = ug_.move();
    return move;
  }

  return fake_;
}
