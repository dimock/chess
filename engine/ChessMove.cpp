#include "ChessBoard.h"
#include "MovesPrecounter.h"

// None, Pawn, Knight, Bishop, Rook, Queen, King
const int Board::s_numDirs_[8] = { 0, 4, 8, 4, 4, 8, 8, 0 };

#ifndef USE_EXTRA_QUIS_
int Board::getSteps(Step (&steps)[NumOfSteps], bool capturesOnly, const Step & prev, const StepId & killerId)
{
  if ( drawState() || ChessMat == state_ )
    return -1;

  StepSorter ssorter(steps, prev, killerId);

  for (int i = KingIndex; i >= 0; --i)
  {
    const Figure & fig = getFigure(color_, i);
    if ( !fig )
      continue;

    if ( Figure::TypePawn == fig.getType() )
    {
      int * indices = MovesTable::inds(fig.where(), color_, fig.getType());

      bool go = true;
      for (int dir = 0; go && dir < s_numDirs_[fig.getType()]; ++dir)
      {
        int to_idx = *(indices + (dir<<3));
        if ( to_idx < 0 )
          continue;

        const Field & f = getField(to_idx);

        int rindex = -1;
        bool can_go = false;

        if ( !f )
        {
          Figure fake;
          if ( getFake(fake) && fake.where() == to_idx && fake.getColor() != color_ )
          {
            THROW_IF( (dir != 0 && dir != 1) || fig.isFirstStep() || Figure::TypePawn != fake.getType(), "attempt to eat fake pawn by wrong way");

            rindex = fakeIndex_;
            can_go = true;
          }
          else
            can_go = (2 == dir|| 3 == dir);
        }
        else if ( (0 == dir || 1 == dir) && f.color() != color_ )
        {
          rindex = f.index();
          can_go = true;
        }
        else if ( 2 == dir )
          go = false;

        if ( !can_go || (rindex < 0 && capturesOnly) )
          continue;

        int y = (to_idx>>3) & 7;
        if ( (Figure::ColorWhite == fig.getColor() && 7 == y) || (Figure::ColorBlack == fig.getColor() && 0 == y) ) // going to reach the last line
        {
          THROW_IF( 3 == dir, "pawn is going to reach the last line on the 1st step");

          for (int newType = Figure::TypeKnight; newType < Figure::TypeKing; ++newType)
          {
            setPawnStep(ssorter, rindex, newType, to_idx, fig);
          }
        }
        else
        {
          setPawnStep(ssorter, rindex, 0, to_idx, fig);
        }
      }
    }
    else
    {
      int * indices = MovesTable::inds(fig.where(), 0, fig.getType());

      for (int dir = 0; dir < s_numDirs_[fig.getType()]; ++dir)
      {
        for (int * inds = indices + (dir<<3); *inds >= 0; ++inds)
        {
          const Field & f = getField(*inds);

          if ( Figure::TypeKing == fig.getType() && Figure::TypeKing == f.type() )
            continue;

          int rindex = f.index();
          if ( rindex >= 0 && f.color() == color_ )
            break;

          if ( capturesOnly && rindex < 0 )
            continue;

          Step & step = ssorter.back();
          step.clear();

          step.index_ = fig.getIndex();
          step.to_ = *inds;
          step.from_ = fig.where();
          step.rindex_ = rindex;
          step.ftype_ = fig.getType();

          ssorter.push(step, *this);

          if ( f )
            break;
        }
      }
    }
  }

  if ( UnderCheck == state_ )
    return ssorter.first();

  // castling
  const Figure & king = getFigure(color_, KingIndex);
  if ( !capturesOnly && king.isFirstStep() )
  {
    for (int i = 0; i < 2; ++i)
    {
      Step & step = ssorter.back();
      step.clear();

      doCastling(king, 8+i, step);
      if ( !step )
        continue;

      ssorter.push(step, *this);
    }
  }

  return ssorter.first();
}
#else // USE_EXTRA_QUIS_
int Board::getSteps(Step (&steps)[NumOfSteps], bool capturesOnly, const Step & prev, const StepId & killerId)
{
  if ( drawState() || ChessMat == state_ )
    return -1;

  StepSorter ssorter(steps, prev, killerId);

  for (int i = KingIndex; i >= 0; --i)
  {
    const Figure & fig = getFigure(color_, i);
    if ( !fig )
      continue;

    if ( Figure::TypePawn == fig.getType() )
    {
      int * indices = MovesTable::inds(fig.where(), color_, fig.getType());

      bool go = true;
      for (int dir = 0; go && dir < s_numDirs_[fig.getType()]; ++dir)
      {
        int to_idx = *(indices + (dir<<3));
        if ( to_idx < 0 )
          continue;

        const Field & f = getField(to_idx);

        int rindex = -1;
        bool can_go = false;

        if ( !f )
        {
          Figure fake;
          if ( getFake(fake) && fake.where() == to_idx && fake.getColor() != color_ )
          {
            THROW_IF( (dir != 0 && dir != 1) || fig.isFirstStep() || Figure::TypePawn != fake.getType(), "attempt to eat fake pawn by wrong way");

            rindex = fakeIndex_;
            can_go = true;
          }
          else
            can_go = (2 == dir|| 3 == dir);
        }
        else if ( (0 == dir || 1 == dir) && f.color() != color_ )
        {
          rindex = f.index();
          can_go = true;
        }
        else if ( 2 == dir )
          go = false;

        if ( !can_go )
          continue;

        bool verifyCheck = capturesOnly;
        if ( capturesOnly )
        {
          if ( rindex >= 0 )
            verifyCheck = false;
        }

        int y = (to_idx>>3) & 7;
        if ( (Figure::ColorWhite == fig.getColor() && 7 == y) || (Figure::ColorBlack == fig.getColor() && 0 == y) ) // going to reach the last line
        {
          THROW_IF( 3 == dir, "pawn is going to reach the last line on the 1st step");

          if ( capturesOnly )
          {
            setPawnStep(ssorter, rindex, Figure::TypeKnight, to_idx, fig, true);
            setPawnStep(ssorter, rindex, Figure::TypeQueen, to_idx, fig, false);
          }
          else
          for (int newType = Figure::TypeKnight; newType < Figure::TypeKing; ++newType)
          {
            setPawnStep(ssorter, rindex, newType, to_idx, fig);
          }
        }
        else
        {
          setPawnStep(ssorter, rindex, 0, to_idx, fig, verifyCheck);
        }
      }
    }
    else
    {
      int * indices = MovesTable::inds(fig.where(), 0, fig.getType());

      for (int dir = 0; dir < s_numDirs_[fig.getType()]; ++dir)
      {
        for (int * inds = indices + (dir<<3); *inds >= 0; ++inds)
        {
          const Field & f = getField(*inds);

          if ( Figure::TypeKing == fig.getType() && Figure::TypeKing == f.type() )
            continue;

          int rindex = f.index();
          if ( rindex >= 0 && f.color() == color_ )
            break;

          Step & step = ssorter.back();
          step.clear();

          step.index_ = fig.getIndex();
          step.to_ = *inds;
          step.from_ = fig.where();
          step.rindex_ = rindex;
          step.ftype_ = fig.getType();

          if ( capturesOnly && rindex < 0 )
          {
            bool ok = false;
            if ( doStep(step) )
            {
              ok = /*wasStepValid(step) && */isChecking(step);
              undoStep(step);
            }
            if ( !ok )
              continue;
          }

          ssorter.push(step, *this);

          if ( f )
            break;
        }
      }
    }
  }

  if ( UnderCheck == state_ )
    return ssorter.first();

  // castling
  const Figure & king = getFigure(color_, KingIndex);
  if ( !capturesOnly && king.isFirstStep() )
  {
    for (int i = 0; i < 2; ++i)
    {
      Step & step = ssorter.back();
      step.clear();

      doCastling(king, 8+i, step);
      if ( !step )
        continue;

      ssorter.push(step, *this);
    }
  }

  return ssorter.first();
}
#endif // USE_EXTRA_QUIS_