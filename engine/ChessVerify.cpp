#include "ChessBoard.h"
#include "MovesPrecounter.h"

//////////////////////////////////////////////////////////////////////////
bool Board::hasSteps()
{
  int istart = 0, istop = 0, di = 1;
  if ( UnderCheck == state_ )
    istart = KingIndex, istop = -1, di = -1;
  else
    istart = 0, istop = NumOfFigures, di = 1;

  for (int i = istart; i != istop; i += di)
  {
    if ( 2 == chkNum_ && i < KingIndex )
    {
      THROW_IF( UnderCheck != state_ || KingIndex != istart, "wrong lookup direction while verifying steps" );
      break;
    }

    const Figure & fig = getFigure(color_, i);
    if ( !fig )
      continue;

    if ( Figure::TypePawn == fig.getType() )
    {
      int * indices = MovesTable::inds(fig.where(), color_, fig.getType());

      bool go = true;
      for (int dir = 0; go && dir < s_numDirs_[fig.getType()]; ++dir)
      {
        int idx = *(indices + (dir<<3));
        if ( idx < 0 )
          continue;

        const Field & f = getField(idx);

        int rindex = -1;
        bool can_go = false;

        if ( !f )
        {
          Figure fake;
          if ( getFake(fake) && fake.where() == idx && fake.getColor() != color_ )
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

        Step step;
        step.clear();

        int y = (idx>>3) & 7;
        if ( (Figure::ColorWhite == fig.getColor() && 7 == y) || (Figure::ColorBlack == fig.getColor() && 0 == y) ) // going to reach the last line
        {
          THROW_IF( 3 == dir, "pawn is going to reach the last line on the 1st step");
          step.newType_ = Figure::TypeKnight;
        }

        step.index_ = fig.getIndex();
        step.rindex_ = rindex;
        step.to_ = idx;
        step.from_ = fig.where();
        step.ftype_ = fig.getType();

        bool valid = false;

#ifndef NDEBUG
        uint64 hcode0 = fmgr_.hashCode();
        WeightType w00 = fmgr_.eval(0);
        WeightType w01 = fmgr_.eval(1);
#endif

        if ( doStep(step) )
        {
          valid = wasStepValid(step);
          undoStep(step);

        }

#ifndef NDEBUG
        uint64 hcode1 = fmgr_.hashCode();
        WeightType w10 = fmgr_.eval(0);
        WeightType w11 = fmgr_.eval(1);
#endif

        THROW_IF(hcode0 != hcode1, "hash code was restored incorrectly");

        THROW_IF(w00 != w10 || w01 != w11, "position evaluation changed during do/undo move");

        if ( valid )
          return true;
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

          Step step;
          step.clear();

          step.index_ = fig.getIndex();
          step.rindex_ = rindex;
          step.to_ = *inds;
          step.from_ = fig.where();
          step.ftype_ = fig.getType();

          bool valid = false;
          if ( doStep(step) )
          {
            valid = wasStepValid(step);
            undoStep(step);
          }

          if ( valid )
            return true;

          if ( f )
            break;
        }
      }
    }
  }

  return false;
}
